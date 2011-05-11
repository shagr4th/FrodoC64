/*
 *  REU.cpp - 17xx REU emulation
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
 * Incompatibilities:
 * ------------------
 *
 *  - REU interrupts are not emulated
 *  - Transfer time is not accounted for, all transfers
 *    are done in 0 cycles
 */

#include "sysdeps.h"

#include "REU.h"
#include "CPUC64.h"
#include "Prefs.h"


/*
 *  Constructor
 */

REU::REU(MOS6510 *CPU) : the_cpu(CPU)
{
	int i;

	// Init registers
	regs[0] = 0x40;
	for (i=1; i<11; i++)
		regs[i] = 0;
	for (i=11; i<16; i++)
		regs[i] = 0xff;

	ex_ram = NULL;
	ram_size = ram_mask = 0;

	// Allocate RAM
	open_close_reu(REU_NONE, ThePrefs.REUSize);
}


/*
 *  Destructor
 */

REU::~REU()
{
	// Free RAM
	open_close_reu(ThePrefs.REUSize, REU_NONE);
}


/*
 *  Prefs may have changed, reallocate expansion RAM
 */

void REU::NewPrefs(Prefs *prefs)
{
	open_close_reu(ThePrefs.REUSize, prefs->REUSize);
}


/*
 *  Allocate/free expansion RAM
 */

void REU::open_close_reu(int old_size, int new_size)
{
	if (old_size == new_size)
		return;

	// Free old RAM
	if (old_size != REU_NONE) {
		delete[] ex_ram;
		ex_ram = NULL;
	}

	// Allocate new RAM
	if (new_size != REU_NONE) {
		switch (new_size) {
			case REU_128K:
				ram_size = 0x20000;
				break;
			case REU_256K:
				ram_size = 0x40000;
				break;
			case REU_512K:
				ram_size = 0x80000;
				break;
		}
		ram_mask = ram_size - 1;
		ex_ram = new uint8[ram_size];

		// Set size bit in status register
		if (ram_size > 0x20000)
			regs[0] |= 0x10;
		else
			regs[0] &= 0xef;
	}
}


/*
 *  Reset the REU
 */

void REU::Reset(void)
{
	int i;

	for (i=1; i<11; i++)
		regs[i] = 0;
	for (i=11; i<16; i++)
		regs[i] = 0xff;

	if (ram_size > 0x20000)
		regs[0] = 0x50;
	else
		regs[0] = 0x40;
}


/*
 *  Read from REU register
 */

uint8 REU::ReadRegister(uint16 adr)
{
	if (ex_ram == NULL)
		return rand();

	switch (adr) {
		case 0:{
			uint8 ret = regs[0];
			regs[0] &= 0x1f;
			return ret;
		}
		case 6:
			return regs[6] | 0xf8;
		case 9:
			return regs[9] | 0x1f;
		case 10:
			return regs[10] | 0x3f;
		default:
			return regs[adr];
	}
}


/*
 *  Write to REU register
 */

void REU::WriteRegister(uint16 adr, uint8 byte)
{
	if (ex_ram == NULL)
		return;

	switch (adr) {
		case 0:		// Status register is read-only
		case 11:	// Unconnected registers
		case 12:
		case 13:
		case 14:
		case 15:
			break;
		case 1:		// Command register
			regs[1] = byte;
			if ((byte & 0x90) == 0x90)
				execute_dma();
			break;
		default:
			regs[adr] = byte;
			break;
	}
}


/*
 *  CPU triggered REU by writing to $ff00
 */

void REU::FF00Trigger(void)
{
	if (ex_ram == NULL)
		return;

	if ((regs[1] & 0x90) == 0x80)
		execute_dma();
}


/*
 *  Execute REU DMA transfer
 */

void REU::execute_dma(void)
{
	// Get C64 and REU transfer base addresses
	uint16 c64_adr = regs[2] | (regs[3] << 8);
	uint32 reu_adr = regs[4] | (regs[5] << 8) | (regs[6] << 16);

	// Calculate transfer length
	int length = regs[7] | (regs[8] << 8);
	if (!length)
		length = 0x10000;

	// Calculate address increments
	uint32 c64_inc = (regs[10] & 0x80) ? 0 : 1;
	uint32 reu_inc = (regs[10] & 0x40) ? 0 : 1;

	// Do transfer
	switch (regs[1] & 3) {

		case 0:		// C64 -> REU
			for (; length--; c64_adr+=c64_inc, reu_adr+=reu_inc)
				ex_ram[reu_adr & ram_mask] = the_cpu->REUReadByte(c64_adr);
			break;

		case 1:		// C64 <- REU
			for (; length--; c64_adr+=c64_inc, reu_adr+=reu_inc)
				the_cpu->REUWriteByte(c64_adr, ex_ram[reu_adr & ram_mask]);
			break;

		case 2:		// C64 <-> REU
			for (; length--; c64_adr+=c64_inc, reu_adr+=reu_inc) {
				uint8 tmp = the_cpu->REUReadByte(c64_adr);
				the_cpu->REUWriteByte(c64_adr, ex_ram[reu_adr & ram_mask]);
				ex_ram[reu_adr & ram_mask] = tmp;
			}
			break;

		case 3:		// Compare
			for (; length--; c64_adr+=c64_inc, reu_adr+=reu_inc)
				if (ex_ram[reu_adr & ram_mask] != the_cpu->REUReadByte(c64_adr)) {
					regs[0] |= 0x20;
					break;
				}
			break;
	}

	// Update address and length registers if autoload is off
	if (!(regs[1] & 0x20)) {
		regs[2] = c64_adr;
		regs[3] = c64_adr >> 8;
		regs[4] = reu_adr;
		regs[5] = reu_adr >> 8;
		regs[6] = reu_adr >> 16;
		regs[7] = length + 1;
		regs[8] = (length + 1) >> 8;
	}

	// Set complete bit in status register
	regs[0] |= 0x40;

	// Clear execute bit in command register
	regs[1] &= 0x7f;
}
