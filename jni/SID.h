/*
 *  SID.h - 6581 emulation
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

#ifndef _SID_H
#define _SID_H

#include <stdlib.h>


// Define this if you want an emulation of an 8580
// (affects combined waveforms)
#undef EMUL_MOS8580


class Prefs;
class C64;
class SIDRenderer;
struct MOS6581State;

// Class for administrative functions
class MOS6581 {
public:
	MOS6581(C64 *c64);
	~MOS6581();

	void Reset(void);
	uint8 ReadRegister(uint16 adr);
	void WriteRegister(uint16 adr, uint8 byte);
	void NewPrefs(Prefs *prefs);
	void PauseSound(void);
	void ResumeSound(void);
	void GetState(MOS6581State *ss);
	void SetState(MOS6581State *ss);
	void EmulateLine(void);

private:
	void open_close_renderer(int old_type, int new_type);

	C64 *the_c64;				// Pointer to C64 object
	SIDRenderer *the_renderer;	// Pointer to current renderer
	uint8 regs[32];				// Copies of the 25 write-only SID registers
	uint8 last_sid_byte;		// Last value written to SID
};


// Renderers do the actual audio data processing
class SIDRenderer {
public:
	virtual ~SIDRenderer() {}
	virtual void Reset(void)=0;
	virtual void EmulateLine(void)=0;
	virtual void WriteRegister(uint16 adr, uint8 byte)=0;
	virtual void NewPrefs(Prefs *prefs)=0;
	virtual void Pause(void)=0;
	virtual void Resume(void)=0;
};


// SID state
struct MOS6581State {
	uint8 freq_lo_1;
	uint8 freq_hi_1;
	uint8 pw_lo_1;
	uint8 pw_hi_1;
	uint8 ctrl_1;
	uint8 AD_1;
	uint8 SR_1;

	uint8 freq_lo_2;
	uint8 freq_hi_2;
	uint8 pw_lo_2;
	uint8 pw_hi_2;
	uint8 ctrl_2;
	uint8 AD_2;
	uint8 SR_2;

	uint8 freq_lo_3;
	uint8 freq_hi_3;
	uint8 pw_lo_3;
	uint8 pw_hi_3;
	uint8 ctrl_3;
	uint8 AD_3;
	uint8 SR_3;

	uint8 fc_lo;
	uint8 fc_hi;
	uint8 res_filt;
	uint8 mode_vol;

	uint8 pot_x;
	uint8 pot_y;
	uint8 osc_3;
	uint8 env_3;
};


/*
 * Fill buffer (for Unix sound routines), sample volume (for sampled voice)
 */

inline void MOS6581::EmulateLine(void)
{
	if (the_renderer != NULL)
		the_renderer->EmulateLine();
}


/*
 *  Read from register
 */

inline uint8 MOS6581::ReadRegister(uint16 adr)
{
	// A/D converters
	if (adr == 0x19 || adr == 0x1a) {
		last_sid_byte = 0;
		return 0xff;
	}

	// Voice 3 oscillator/EG readout
	if (adr == 0x1b || adr == 0x1c) {
		last_sid_byte = 0;
		return rand();
	}

	// Write-only register: Return last value written to SID
	return last_sid_byte;
}


/*
 *  Write to register
 */

inline void MOS6581::WriteRegister(uint16 adr, uint8 byte)
{
	// Keep a local copy of the register values
	last_sid_byte = regs[adr] = byte;

	if (the_renderer != NULL)
		the_renderer->WriteRegister(adr, byte);
}

#endif
