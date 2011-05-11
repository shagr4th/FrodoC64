/*
 *  CPUC64_SC.cpp - Single-cycle 6510 (C64) emulation
 *
 *  Frodo (C) 1994-1997,2002-2005 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Notes:
 * ------
 *
 * Opcode execution:
 *  - All opcodes are resolved into single clock cycles. There is one
 *    switch case for each cycle.
 *  - The "state" variable specifies the routine to be executed in the
 *    next cycle. Its upper 8 bits contain the current opcode, its lower
 *    8 bits contain the cycle number (0..7) within the opcode.
 *  - Opcodes are fetched in cycle 0 (state = 0)
 *  - The states 0x0010..0x0027 are used for interrupts
 *  - There is exactly one memory access in each clock cycle
 *
 * Memory configurations:
 *
 * $01  $a000-$bfff  $d000-$dfff  $e000-$ffff
 * -----------------------------------------------
 *  0       RAM          RAM          RAM
 *  1       RAM       Char ROM        RAM
 *  2       RAM       Char ROM    Kernal ROM
 *  3    Basic ROM    Char ROM    Kernal ROM
 *  4       RAM          RAM          RAM
 *  5       RAM          I/O          RAM
 *  6       RAM          I/O      Kernal ROM
 *  7    Basic ROM       I/O      Kernal ROM
 *
 *  - All memory accesses are done with the read_byte() and
 *    write_byte() functions which also do the memory address
 *    decoding.
 *  - If a write occurs to addresses 0 or 1, new_config is
 *    called to check whether the memory configuration has
 *    changed
 *  - The possible interrupt sources are:
 *      INT_VICIRQ: I flag is checked, jump to ($fffe)
 *      INT_CIAIRQ: I flag is checked, jump to ($fffe)
 *      INT_NMI: Jump to ($fffa)
 *      INT_RESET: Jump to ($fffc)
 *  - The z_flag variable has the inverse meaning of the
 *    6510 Z flag
 *  - Only the highest bit of the n_flag variable is used
 *  - The $f2 opcode that would normally crash the 6510 is
 *    used to implement emulator-specific functions, mainly
 *    those for the IEC routines
 *
 * Incompatibilities:
 * ------------------
 *
 *  - If BA is low and AEC is high, read accesses should occur
 */

#include "sysdeps.h"

#include "CPUC64.h"
#include "CPU_common.h"
#include "C64.h"
#include "VIC.h"
#include "SID.h"
#include "CIA.h"
#include "REU.h"
#include "IEC.h"
#include "Display.h"
#include "Version.h"


enum {
	INT_RESET = 3
};


/*
 *  6510 constructor: Initialize registers
 */

MOS6510::MOS6510(C64 *c64, uint8 *Ram, uint8 *Basic, uint8 *Kernal, uint8 *Char, uint8 *Color)
 : the_c64(c64), ram(Ram), basic_rom(Basic), kernal_rom(Kernal), char_rom(Char), color_ram(Color)
{
	a = x = y = 0;
	sp = 0xff;
	n_flag = z_flag = 0;
	v_flag = d_flag = c_flag = false;
	i_flag = true;
	dfff_byte = 0x55;
	BALow = false;
	first_irq_cycle = first_nmi_cycle = 0;
}


/*
 *  Reset CPU asynchronously
 */

void MOS6510::AsyncReset(void)
{
	interrupt.intr[INT_RESET] = true;
}


/*
 *  Raise NMI asynchronously (Restore key)
 */

void MOS6510::AsyncNMI(void)
{
	if (!nmi_state)
		interrupt.intr[INT_NMI] = true;
}


/*
 *  Get 6510 register state
 */

void MOS6510::GetState(MOS6510State *s)
{
	s->a = a;
	s->x = x;
	s->y = y;

	s->p = 0x20 | (n_flag & 0x80);
	if (v_flag) s->p |= 0x40;
	if (d_flag) s->p |= 0x08;
	if (i_flag) s->p |= 0x04;
	if (!z_flag) s->p |= 0x02;
	if (c_flag) s->p |= 0x01;
	
	s->ddr = ddr;
	s->pr = pr;

	s->pc = pc;
	s->sp = sp | 0x0100;

	s->intr[INT_VICIRQ] = interrupt.intr[INT_VICIRQ];
	s->intr[INT_CIAIRQ] = interrupt.intr[INT_CIAIRQ];
	s->intr[INT_NMI] = interrupt.intr[INT_NMI];
	s->intr[INT_RESET] = interrupt.intr[INT_RESET];
	s->nmi_state = nmi_state;
	s->dfff_byte = dfff_byte;
	s->instruction_complete = (state == 0);
}


/*
 *  Restore 6510 state
 */

void MOS6510::SetState(MOS6510State *s)
{
	a = s->a;
	x = s->x;
	y = s->y;

	n_flag = s->p;
	v_flag = s->p & 0x40;
	d_flag = s->p & 0x08;
	i_flag = s->p & 0x04;
	z_flag = !(s->p & 0x02);
	c_flag = s->p & 0x01;

	ddr = s->ddr;
	pr = s->pr;
	new_config();

	pc = s->pc;
	sp = s->sp & 0xff;

	interrupt.intr[INT_VICIRQ] = s->intr[INT_VICIRQ];
	interrupt.intr[INT_CIAIRQ] = s->intr[INT_CIAIRQ];
	interrupt.intr[INT_NMI] = s->intr[INT_NMI];
	interrupt.intr[INT_RESET] = s->intr[INT_RESET];
	nmi_state = s->nmi_state;
	dfff_byte = s->dfff_byte;
	if (s->instruction_complete)
		state = 0;
}


/*
 *  Memory configuration has probably changed
 */

void MOS6510::new_config(void)
{
	uint8 port = ~ddr | pr;

	basic_in = (port & 3) == 3;
	kernal_in = port & 2;
	char_in = (port & 3) && !(port & 4);
	io_in = (port & 3) && (port & 4);
}


/*
 *  Read a byte from I/O / ROM space
 */

inline uint8 MOS6510::read_byte_io(uint16 adr)
{
	switch (adr >> 12) {
		case 0xa:
		case 0xb:
			if (basic_in)
				return basic_rom[adr & 0x1fff];
			else
				return ram[adr];
		case 0xc:
			return ram[adr];
		case 0xd:
			if (io_in)
				switch ((adr >> 8) & 0x0f) {
					case 0x0:	// VIC
					case 0x1:
					case 0x2:
					case 0x3:
						return TheVIC->ReadRegister(adr & 0x3f);
					case 0x4:	// SID
					case 0x5:
					case 0x6:
					case 0x7:
						return TheSID->ReadRegister(adr & 0x1f);
					case 0x8:	// Color RAM
					case 0x9:
					case 0xa:
					case 0xb:
						return color_ram[adr & 0x03ff] & 0x0f | TheVIC->LastVICByte & 0xf0;
					case 0xc:	// CIA 1
						return TheCIA1->ReadRegister(adr & 0x0f);
					case 0xd:	// CIA 2
						return TheCIA2->ReadRegister(adr & 0x0f);
					case 0xe:	// REU/Open I/O
					case 0xf:
						if ((adr & 0xfff0) == 0xdf00)
							return TheREU->ReadRegister(adr & 0x0f);
						else if (adr < 0xdfa0)
							return TheVIC->LastVICByte;
						else
							return read_emulator_id(adr & 0x7f);
				}
			else if (char_in)
				return char_rom[adr & 0x0fff];
			else
				return ram[adr];
		case 0xe:
		case 0xf:
			if (kernal_in)
				return kernal_rom[adr & 0x1fff];
			else
				return ram[adr];
		default:	// Can't happen
			return 0;
	}
}


/*
 *  Read a byte from the CPU's address space
 */

uint8 MOS6510::read_byte(uint16 adr)
{
	if (adr < 0xa000) {
		if (adr >= 2)
			return ram[adr];
		else if (adr == 0)
			return ddr;
		else
			return (ddr & pr) | (~ddr & 0x17);
	} else
		return read_byte_io(adr);
}


/*
 *  $dfa0-$dfff: Emulator identification
 */

const char frodo_id[0x5c] = "FRODO\r(C) 1994-1997 CHRISTIAN BAUER";

uint8 MOS6510::read_emulator_id(uint16 adr)
{
	switch (adr) {
		case 0x7c:	// $dffc: revision
			return FRODO_REVISION << 4;
		case 0x7d:	// $dffd: version
			return FRODO_VERSION;
		case 0x7e:	// $dffe returns 'F' (Frodo ID)
			return 'F';
		case 0x7f:	// $dfff alternates between $55 and $aa
			dfff_byte = ~dfff_byte;
			return dfff_byte;
		default:
			return frodo_id[adr - 0x20];
	}
}


/*
 *  Read a word (little-endian) from the CPU's address space
 */

inline uint16 MOS6510::read_word(uint16 adr)
{
	return read_byte(adr) | (read_byte(adr+1) << 8);
}


/*
 *  Write a byte to I/O space
 */

inline void MOS6510::write_byte_io(uint16 adr, uint8 byte)
{
	if (adr >= 0xe000) {
		ram[adr] = byte;
		if (adr == 0xff00)
			TheREU->FF00Trigger();
	} else if (io_in)
		switch ((adr >> 8) & 0x0f) {
			case 0x0:	// VIC
			case 0x1:
			case 0x2:
			case 0x3:
				TheVIC->WriteRegister(adr & 0x3f, byte);
				return;
			case 0x4:	// SID
			case 0x5:
			case 0x6:
			case 0x7:
				TheSID->WriteRegister(adr & 0x1f, byte);
				return;
			case 0x8:	// Color RAM
			case 0x9:
			case 0xa:
			case 0xb:
				color_ram[adr & 0x03ff] = byte & 0x0f;
				return;
			case 0xc:	// CIA 1
				TheCIA1->WriteRegister(adr & 0x0f, byte);
				return;
			case 0xd:	// CIA 2
				TheCIA2->WriteRegister(adr & 0x0f, byte);
				return;
			case 0xe:	// REU/Open I/O
			case 0xf:
				if ((adr & 0xfff0) == 0xdf00)
					TheREU->WriteRegister(adr & 0x0f, byte);
				return;
		}
	else
		ram[adr] = byte;	
}


/*
 *  Write a byte to the CPU's address space
 */

void MOS6510::write_byte(uint16 adr, uint8 byte)
{
	if (adr < 0xd000) {
		if (adr >= 2)
			ram[adr] = byte;
		else if (adr == 0) {
			ddr = byte;
			ram[0] = TheVIC->LastVICByte;
			new_config();
		} else {
			pr = byte;
			ram[1] = TheVIC->LastVICByte;
			new_config();
		}
	} else
		write_byte_io(adr, byte);
}


/*
 *  Read byte from 6510 address space with special memory config (used by SAM)
 */

uint8 MOS6510::ExtReadByte(uint16 adr)
{
	// Save old memory configuration
	bool bi = basic_in, ki = kernal_in, ci = char_in, ii = io_in;

	// Set new configuration
	basic_in = (ExtConfig & 3) == 3;
	kernal_in = ExtConfig & 2;
	char_in = (ExtConfig & 3) && ~(ExtConfig & 4);
	io_in = (ExtConfig & 3) && (ExtConfig & 4);

	// Read byte
	uint8 byte = read_byte(adr);

	// Restore old configuration
	basic_in = bi; kernal_in = ki; char_in = ci; io_in = ii;

	return byte;
}


/*
 *  Write byte to 6510 address space with special memory config (used by SAM)
 */

void MOS6510::ExtWriteByte(uint16 adr, uint8 byte)
{
	// Save old memory configuration
	bool bi = basic_in, ki = kernal_in, ci = char_in, ii = io_in;

	// Set new configuration
	basic_in = (ExtConfig & 3) == 3;
	kernal_in = ExtConfig & 2;
	char_in = (ExtConfig & 3) && ~(ExtConfig & 4);
	io_in = (ExtConfig & 3) && (ExtConfig & 4);

	// Write byte
	write_byte(adr, byte);

	// Restore old configuration
	basic_in = bi; kernal_in = ki; char_in = ci; io_in = ii;
}


/*
 *  Read byte from 6510 address space with current memory config (used by REU)
 */

uint8 MOS6510::REUReadByte(uint16 adr)
{
	return read_byte(adr);
}


/*
 *  Write byte to 6510 address space with current memory config (used by REU)
 */

void MOS6510::REUWriteByte(uint16 adr, uint8 byte)
{
	write_byte(adr, byte);
}


/*
 *  Adc instruction
 */

inline void MOS6510::do_adc(uint8 byte)
{
	if (!d_flag) {
		uint16 tmp;

		// Binary mode
		tmp = a + byte + (c_flag ? 1 : 0);
		c_flag = tmp > 0xff;
		v_flag = !((a ^ byte) & 0x80) && ((a ^ tmp) & 0x80);
		z_flag = n_flag = a = tmp;

	} else {
		uint16 al, ah;

		// Decimal mode
		al = (a & 0x0f) + (byte & 0x0f) + (c_flag ? 1 : 0);		// Calculate lower nybble
		if (al > 9) al += 6;									// BCD fixup for lower nybble

		ah = (a >> 4) + (byte >> 4);							// Calculate upper nybble
		if (al > 0x0f) ah++;

		z_flag = a + byte + (c_flag ? 1 : 0);					// Set flags
		n_flag = ah << 4;	// Only highest bit used
		v_flag = (((ah << 4) ^ a) & 0x80) && !((a ^ byte) & 0x80);

		if (ah > 9) ah += 6;									// BCD fixup for upper nybble
		c_flag = ah > 0x0f;										// Set carry flag
		a = (ah << 4) | (al & 0x0f);							// Compose result
	}
}


/*
 * Sbc instruction
 */

inline void MOS6510::do_sbc(uint8 byte)
{
	uint16 tmp = a - byte - (c_flag ? 0 : 1);

	if (!d_flag) {

		// Binary mode
		c_flag = tmp < 0x100;
		v_flag = ((a ^ tmp) & 0x80) && ((a ^ byte) & 0x80);
		z_flag = n_flag = a = tmp;

	} else {
		uint16 al, ah;

		// Decimal mode
		al = (a & 0x0f) - (byte & 0x0f) - (c_flag ? 0 : 1);	// Calculate lower nybble
		ah = (a >> 4) - (byte >> 4);							// Calculate upper nybble
		if (al & 0x10) {
			al -= 6;											// BCD fixup for lower nybble
			ah--;
		}
		if (ah & 0x10) ah -= 6;									// BCD fixup for upper nybble

		c_flag = tmp < 0x100;									// Set flags
		v_flag = ((a ^ tmp) & 0x80) && ((a ^ byte) & 0x80);
		z_flag = n_flag = tmp;

		a = (ah << 4) | (al & 0x0f);							// Compose result
	}
}


/*
 *  Reset CPU
 */

void MOS6510::Reset(void)
{
	// Delete 'CBM80' if present
	if (ram[0x8004] == 0xc3 && ram[0x8005] == 0xc2 && ram[0x8006] == 0xcd
	 && ram[0x8007] == 0x38 && ram[0x8008] == 0x30)
		ram[0x8004] = 0;

	// Initialize extra 6510 registers and memory configuration
	ddr = pr = 0;
	new_config();

	// Clear all interrupt lines
	interrupt.intr_any = 0;
	nmi_state = false;

	// Read reset vector
	pc = read_word(0xfffc);
	state = 0;
}


/*
 *  Illegal opcode encountered
 */

void MOS6510::illegal_op(uint8 op, uint16 at)
{
	char illop_msg[80];

	sprintf(illop_msg, "Illegal opcode %02x at %04x.", op, at);
	ShowRequester(illop_msg, "Reset");
	the_c64->Reset();
	Reset();
}


/*
 *  Emulate one 6510 clock cycle
 */

// Read byte from memory
#define read_to(adr, to) \
	if (BALow) \
		return; \
	to = read_byte(adr);

// Read byte from memory, throw away result
#define read_idle(adr) \
	if (BALow) \
		return; \
	read_byte(adr);

void MOS6510::EmulateCycle(void)
{
	uint8 data, tmp;

	// Any pending interrupts in state 0 (opcode fetch)?
	if (!state && interrupt.intr_any) {
		if (interrupt.intr[INT_RESET])
			Reset();
		else if (interrupt.intr[INT_NMI] && (the_c64->CycleCounter-first_nmi_cycle >= 2)) {
			interrupt.intr[INT_NMI] = false;	// Simulate an edge-triggered input
			state = 0x0010;
		} else if ((interrupt.intr[INT_VICIRQ] || interrupt.intr[INT_CIAIRQ]) && (the_c64->CycleCounter-first_irq_cycle >= 2) && !i_flag)
			state = 0x0008;
	}

#include "CPU_emulcycle.h"

		// Extension opcode
		case O_EXT:
			if (pc < 0xe000) {
				illegal_op(0xf2, pc-1);
				break;
			}
			switch (read_byte(pc++)) {
				case 0x00:
					ram[0x90] |= TheIEC->Out(ram[0x95], ram[0xa3] & 0x80);
					c_flag = false;
					pc = 0xedac;
					Last;
				case 0x01:
					ram[0x90] |= TheIEC->OutATN(ram[0x95]);
					c_flag = false;
					pc = 0xedac;
					Last;
				case 0x02:
					ram[0x90] |= TheIEC->OutSec(ram[0x95]);
					c_flag = false;
					pc = 0xedac;
					Last;
				case 0x03:
					ram[0x90] |= TheIEC->In(a);
					set_nz(a);
					c_flag = false;
					pc = 0xedac;
					Last;
				case 0x04:
					TheIEC->SetATN();
					pc = 0xedfb;
					Last;
				case 0x05:
					TheIEC->RelATN();
					pc = 0xedac;
					Last;
				case 0x06:
					TheIEC->Turnaround();
					pc = 0xedac;
					Last;
				case 0x07:
					TheIEC->Release();
					pc = 0xedac;
					Last;
				default:
					illegal_op(0xf2, pc-1);
					break;
			}
			break;

		default:
			illegal_op(op, pc-1);
			break;
	}
}
