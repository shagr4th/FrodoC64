/*
 *  REU.h - 17xx REU emulation
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

#ifndef _REU_H
#define _REU_H


class MOS6510;
class Prefs;

class REU {
public:
	REU(MOS6510 *CPU);
	~REU();

	void NewPrefs(Prefs *prefs);
	void Reset(void);
	uint8 ReadRegister(uint16 adr);
	void WriteRegister(uint16 adr, uint8 byte);
	void FF00Trigger(void);

private:
	void open_close_reu(int old_size, int new_size);
	void execute_dma(void);

	MOS6510 *the_cpu;	// Pointer to 6510

	uint8 *ex_ram;		// REU expansion RAM

	uint32 ram_size;		// Size of expansion RAM
	uint32 ram_mask;		// Expansion RAM address bit mask

	uint8 regs[16];		// REU registers
};

#endif
