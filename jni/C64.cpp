/*
 *  C64.cpp - Put the pieces together
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

#include "sysdeps.h"

#include "C64.h"
#include "CPUC64.h"
#include "CPU1541.h"
#include "VIC.h"
#include "SID.h"
#include "CIA.h"
#include "REU.h"
#include "IEC.h"
#include "1541job.h"
#include "Display.h"
#include "Prefs.h"

#if defined(__unix) && !defined(__svgalib__) && !defined(__sdlui__)
#include "CmdPipe.h"
#endif

JNIEnv *android_env;
jobject android_callback;
/*jmethodID refreshMainScreen;*/
jmethodID getEvent;
jmethodID initAudio;
jmethodID sendAudio;
jmethodID pauseAudio;
jmethodID playAudio;
jmethodID emulatorReady;
jmethodID drawled;
jmethodID requestRender;
#ifdef FRODO_SC
bool IsFrodoSC = true;
#else
bool IsFrodoSC = false;
#endif


/*
 *  Constructor: Allocate objects and memory
 */

C64::C64()
{
	uint8 *p;

	// The thread is not yet running
	thread_running = false;
	quit_thyself = false;
	have_a_break = 0;

	// System-dependent things
	c64_ctor1();

	// Open display
	TheDisplay = new C64Display(this);

	// Allocate RAM/ROM memory
	RAM = new uint8[C64_RAM_SIZE];
	Basic = new uint8[BASIC_ROM_SIZE];
	Kernal = new uint8[KERNAL_ROM_SIZE];
	Char = new uint8[CHAR_ROM_SIZE];
	Color = new uint8[COLOR_RAM_SIZE];
	RAM1541 = new uint8[DRIVE_RAM_SIZE];
	ROM1541 = new uint8[DRIVE_ROM_SIZE];

	// Create the chips
	TheCPU = new MOS6510(this, RAM, Basic, Kernal, Char, Color);

	TheJob1541 = new Job1541(RAM1541);
	TheCPU1541 = new MOS6502_1541(this, TheJob1541, TheDisplay, RAM1541, ROM1541);

	TheVIC = TheCPU->TheVIC = new MOS6569(this, TheDisplay, TheCPU, RAM, Char, Color);
	TheSID = TheCPU->TheSID = new MOS6581(this);
	TheCIA1 = TheCPU->TheCIA1 = new MOS6526_1(TheCPU, TheVIC);
	TheCIA2 = TheCPU->TheCIA2 = TheCPU1541->TheCIA2 = new MOS6526_2(TheCPU, TheVIC, TheCPU1541);
	TheIEC = TheCPU->TheIEC = new IEC(TheDisplay);
	TheREU = TheCPU->TheREU = new REU(TheCPU);

	// Initialize RAM with powerup pattern
	p = RAM;
	for (unsigned i=0; i<512; i++) {
		for (unsigned j=0; j<64; j++)
			*p++ = 0;
		for (unsigned j=0; j<64; j++)
			*p++ = 0xff;
	}

	// Initialize color RAM with random values
	p = Color;
	for (unsigned i=0; i<COLOR_RAM_SIZE; i++)
		*p++ = rand() & 0x0f;

	// Clear 1541 RAM
	memset(RAM1541, 0, DRIVE_RAM_SIZE);

	// Open joystick drivers if required
	open_close_joysticks(0, 0, ThePrefs.Joystick1Port, ThePrefs.Joystick2Port);
	joykey = 0xff;

#ifdef FRODO_SC
	CycleCounter = 0;
#endif

	// System-dependent things
	c64_ctor2();
}


/*
 *  Destructor: Delete all objects
 */

C64::~C64()
{
	open_close_joysticks(ThePrefs.Joystick1Port, ThePrefs.Joystick2Port, 0, 0);

	delete TheJob1541;
	delete TheREU;
	delete TheIEC;
	delete TheCIA2;
	delete TheCIA1;
	delete TheSID;
	delete TheVIC;
	delete TheCPU1541;
	delete TheCPU;
	delete TheDisplay;

	delete[] RAM;
	delete[] Basic;
	delete[] Kernal;
	delete[] Char;
	delete[] Color;
	delete[] RAM1541;
	delete[] ROM1541;

	c64_dtor();
}


/*
 *  Reset C64
 */

void C64::Reset(void)
{
	TheCPU->AsyncReset();
	TheCPU1541->AsyncReset();
	TheSID->Reset();
	TheCIA1->Reset();
	TheCIA2->Reset();
	TheIEC->Reset();
}


/*
 *  NMI C64
 */

void C64::NMI(void)
{
	TheCPU->AsyncNMI();
}


/*
 *  The preferences have changed. prefs is a pointer to the new
 *   preferences, ThePrefs still holds the previous ones.
 *   The emulation must be in the paused state!
 */

void C64::NewPrefs(Prefs *prefs)
{
	open_close_joysticks(ThePrefs.Joystick1Port, ThePrefs.Joystick2Port, prefs->Joystick1Port, prefs->Joystick2Port);
	PatchKernal(prefs->FastReset, prefs->Emul1541Proc);

	TheDisplay->NewPrefs(prefs);

#ifdef __riscos__
	// Changed order of calls. If 1541 mode hasn't changed the order is insignificant.
	if (prefs->Emul1541Proc) {
		// New prefs have 1541 enabled ==> if old prefs had disabled free drives FIRST
		TheIEC->NewPrefs(prefs);
		TheJob1541->NewPrefs(prefs);
	} else {
		// New prefs has 1541 disabled ==> if old prefs had enabled free job FIRST
		TheJob1541->NewPrefs(prefs);
		TheIEC->NewPrefs(prefs);
	}
#else
	TheIEC->NewPrefs(prefs);
	TheJob1541->NewPrefs(prefs);
#endif

	TheREU->NewPrefs(prefs);
	TheSID->NewPrefs(prefs);

	// Reset 1541 processor if turned on
	if (!ThePrefs.Emul1541Proc && prefs->Emul1541Proc)
		TheCPU1541->AsyncReset();
}


/*
 *  Patch kernal IEC routines
 */

void C64::PatchKernal(bool fast_reset, bool emul_1541_proc)
{
	if (fast_reset) {
		Kernal[0x1d84] = 0xa0;
		Kernal[0x1d85] = 0x00;
	} else {
		Kernal[0x1d84] = orig_kernal_1d84;
		Kernal[0x1d85] = orig_kernal_1d85;
	}

	if (emul_1541_proc) {
		Kernal[0x0d40] = 0x78;
		Kernal[0x0d41] = 0x20;
		Kernal[0x0d23] = 0x78;
		Kernal[0x0d24] = 0x20;
		Kernal[0x0d36] = 0x78;
		Kernal[0x0d37] = 0x20;
		Kernal[0x0e13] = 0x78;
		Kernal[0x0e14] = 0xa9;
		Kernal[0x0def] = 0x78;
		Kernal[0x0df0] = 0x20;
		Kernal[0x0dbe] = 0xad;
		Kernal[0x0dbf] = 0x00;
		Kernal[0x0dcc] = 0x78;
		Kernal[0x0dcd] = 0x20;
		Kernal[0x0e03] = 0x20;
		Kernal[0x0e04] = 0xbe;
	} else {
		Kernal[0x0d40] = 0xf2;	// IECOut
		Kernal[0x0d41] = 0x00;
		Kernal[0x0d23] = 0xf2;	// IECOutATN
		Kernal[0x0d24] = 0x01;
		Kernal[0x0d36] = 0xf2;	// IECOutSec
		Kernal[0x0d37] = 0x02;
		Kernal[0x0e13] = 0xf2;	// IECIn
		Kernal[0x0e14] = 0x03;
		Kernal[0x0def] = 0xf2;	// IECSetATN
		Kernal[0x0df0] = 0x04;
		Kernal[0x0dbe] = 0xf2;	// IECRelATN
		Kernal[0x0dbf] = 0x05;
		Kernal[0x0dcc] = 0xf2;	// IECTurnaround
		Kernal[0x0dcd] = 0x06;
		Kernal[0x0e03] = 0xf2;	// IECRelease
		Kernal[0x0e04] = 0x07;
	}

	// 1541
	ROM1541[0x2ae4] = 0xea;		// Don't check ROM checksum
	ROM1541[0x2ae5] = 0xea;
	ROM1541[0x2ae8] = 0xea;
	ROM1541[0x2ae9] = 0xea;
	ROM1541[0x2c9b] = 0xf2;		// DOS idle loop
	ROM1541[0x2c9c] = 0x00;
	ROM1541[0x3594] = 0x20;		// Write sector
	ROM1541[0x3595] = 0xf2;
	ROM1541[0x3596] = 0xf5;
	ROM1541[0x3597] = 0xf2;
	ROM1541[0x3598] = 0x01;
	ROM1541[0x3b0c] = 0xf2;		// Format track
	ROM1541[0x3b0d] = 0x02;
}


/*
 *  Save RAM contents
 */

void C64::SaveRAM(char *filename)
{
	FILE *f;

	if ((f = fopen(filename, "wb")) == NULL)
		ShowRequester("RAM save failed.", "OK", NULL);
	else {
		fwrite((void*)RAM, 1, 0x10000, f);
		fwrite((void*)Color, 1, 0x400, f);
		if (ThePrefs.Emul1541Proc)
			fwrite((void*)RAM1541, 1, 0x800, f);
		fclose(f);
	}
}


/*
 *  Save CPU state to snapshot
 *
 *  0: Error
 *  1: OK
 *  -1: Instruction not completed
 */

int C64::SaveCPUState(FILE *f)
{
	MOS6510State state;
	TheCPU->GetState(&state);

	if (!state.instruction_complete)
		return -1;

	int i = fwrite(RAM, 0x10000, 1, f);
	i += fwrite(Color, 0x400, 1, f);
	i += fwrite((void*)&state, sizeof(state), 1, f);

	return i == 3;
}


/*
 *  Load CPU state from snapshot
 */

bool C64::LoadCPUState(FILE *f)
{
	MOS6510State state;

	int i = fread(RAM, 0x10000, 1, f);
	i += fread(Color, 0x400, 1, f);
	i += fread((void*)&state, sizeof(state), 1, f);

	if (i == 3) {
		TheCPU->SetState(&state);
		return true;
	} else
		return false;
}


/*
 *  Save 1541 state to snapshot
 *
 *  0: Error
 *  1: OK
 *  -1: Instruction not completed
 */

int C64::Save1541State(FILE *f)
{
	MOS6502State state;
	TheCPU1541->GetState(&state);

	if (!state.idle && !state.instruction_complete)
		return -1;

	int i = fwrite(RAM1541, 0x800, 1, f);
	i += fwrite((void*)&state, sizeof(state), 1, f);

	return i == 2;
}


/*
 *  Load 1541 state from snapshot
 */

bool C64::Load1541State(FILE *f)
{
	MOS6502State state;

	int i = fread(RAM1541, 0x800, 1, f);
	i += fread((void*)&state, sizeof(state), 1, f);

	if (i == 2) {
		TheCPU1541->SetState(&state);
		return true;
	} else
		return false;
}


/*
 *  Save VIC state to snapshot
 */

bool C64::SaveVICState(FILE *f)
{
	MOS6569State state;
	TheVIC->GetState(&state);
	return fwrite((void*)&state, sizeof(state), 1, f) == 1;
}


/*
 *  Load VIC state from snapshot
 */

bool C64::LoadVICState(FILE *f)
{
	MOS6569State state;

	if (fread((void*)&state, sizeof(state), 1, f) == 1) {
		TheVIC->SetState(&state);
		return true;
	} else
		return false;
}


/*
 *  Save SID state to snapshot
 */

bool C64::SaveSIDState(FILE *f)
{
	MOS6581State state;
	TheSID->GetState(&state);
	return fwrite((void*)&state, sizeof(state), 1, f) == 1;
}


/*
 *  Load SID state from snapshot
 */

bool C64::LoadSIDState(FILE *f)
{
	MOS6581State state;

	if (fread((void*)&state, sizeof(state), 1, f) == 1) {
		TheSID->SetState(&state);
		return true;
	} else
		return false;
}


/*
 *  Save CIA states to snapshot
 */

bool C64::SaveCIAState(FILE *f)
{
	MOS6526State state;
	TheCIA1->GetState(&state);

	if (fwrite((void*)&state, sizeof(state), 1, f) == 1) {
		TheCIA2->GetState(&state);
		return fwrite((void*)&state, sizeof(state), 1, f) == 1;
	} else
		return false;
}


/*
 *  Load CIA states from snapshot
 */

bool C64::LoadCIAState(FILE *f)
{
	MOS6526State state;

	if (fread((void*)&state, sizeof(state), 1, f) == 1) {
		TheCIA1->SetState(&state);
		if (fread((void*)&state, sizeof(state), 1, f) == 1) {
			TheCIA2->SetState(&state);
			return true;
		} else
			return false;
	} else
		return false;
}


/*
 *  Save 1541 GCR state to snapshot
 */

bool C64::Save1541JobState(FILE *f)
{
	Job1541State state;
	TheJob1541->GetState(&state);
	return fwrite((void*)&state, sizeof(state), 1, f) == 1;
}


/*
 *  Load 1541 GCR state from snapshot
 */

bool C64::Load1541JobState(FILE *f)
{
	Job1541State state;

	if (fread((void*)&state, sizeof(state), 1, f) == 1) {
		TheJob1541->SetState(&state);
		return true;
	} else
		return false;
}


#define SNAPSHOT_HEADER "FrodoSnapshot"
#define SNAPSHOT_1541 1

#define ADVANCE_CYCLES	\
	TheVIC->EmulateCycle(); \
	TheCIA1->EmulateCycle(); \
	TheCIA2->EmulateCycle(); \
	TheCPU->EmulateCycle(); \
	if (ThePrefs.Emul1541Proc) { \
		TheCPU1541->CountVIATimers(1); \
		if (!TheCPU1541->Idle) \
			TheCPU1541->EmulateCycle(); \
	}


/*
 *  Save snapshot (emulation must be paused and in VBlank)
 *
 *  To be able to use SC snapshots with SL, SC snapshots are made thus that no
 *  partially dealt with instructions are saved. Instead all devices are advanced
 *  cycle by cycle until the current instruction has been finished. The number of
 *  cycles this takes is saved in the snapshot and will be reconstructed if the
 *  snapshot is loaded into FrodoSC again.
 */

void C64::SaveSnapshot(char *filename)
{
	FILE *f;
	uint8 flags;
	uint8 delay;
	int stat;

	if ((f = fopen(filename, "wb")) == NULL) {
		ShowRequester("Unable to open snapshot file", "OK", NULL);
		return;
	}

	fprintf(f, "%s%c", SNAPSHOT_HEADER, 10);
	fputc(0, f);	// Version number 0
	flags = 0;
	if (ThePrefs.Emul1541Proc)
		flags |= SNAPSHOT_1541;
	fputc(flags, f);
	SaveVICState(f);
	SaveSIDState(f);
	SaveCIAState(f);

#ifdef FRODO_SC
	delay = 0;
	do {
		if ((stat = SaveCPUState(f)) == -1) {	// -1 -> Instruction not finished yet
			ADVANCE_CYCLES;	// Advance everything by one cycle
			delay++;
		}
	} while (stat == -1);
	fputc(delay, f);	// Number of cycles the saved CPUC64 lags behind the previous chips
#else
	SaveCPUState(f);
	fputc(0, f);		// No delay
#endif

	if (ThePrefs.Emul1541Proc) {
		fwrite(ThePrefs.DrivePath[0], 256, 1, f);
#ifdef FRODO_SC
		delay = 0;
		do {
			if ((stat = Save1541State(f)) == -1) {
				ADVANCE_CYCLES;
				delay++;
			}
		} while (stat == -1);
		fputc(delay, f);
#else
		Save1541State(f);
		fputc(0, f);	// No delay
#endif
		Save1541JobState(f);
		
	}
	fclose(f);

	LOGI("Snapshot saved");
#ifdef __riscos__
	TheWIMP->SnapshotSaved(true);
#endif
}


/*
 *  Load snapshot (emulation must be paused and in VBlank)
 */

bool C64::LoadSnapshot(char *filename)
{
	FILE *f;

	if ((f = fopen(filename, "rb")) != NULL) {
		char Header[] = SNAPSHOT_HEADER;
		char *b = Header, c = 0;
		uint8 delay, i;

		// For some reason memcmp()/strcmp() and so forth utterly fail here.
		while (*b > 32) {
			if ((c = fgetc(f)) != *b++) {
				b = NULL;
				break;
			}
		}
		if (b != NULL) {
			uint8 flags;
			bool error = false;
#ifndef FRODO_SC
			long vicptr;	// File offset of VIC data
#endif

			while (c != 10)
				c = fgetc(f);	// Shouldn't be necessary
			if (fgetc(f) != 0) {
				ShowRequester("Unknown snapshot format", "OK", NULL);
				fclose(f);
				return false;
			}
			flags = fgetc(f);
#ifndef FRODO_SC
			vicptr = ftell(f);
#endif

			error |= !LoadVICState(f);
			error |= !LoadSIDState(f);
			error |= !LoadCIAState(f);
			error |= !LoadCPUState(f);

			delay = fgetc(f);	// Number of cycles the 6510 is ahead of the previous chips
#ifdef FRODO_SC
			// Make the other chips "catch up" with the 6510
			for (i=0; i<delay; i++) {
				TheVIC->EmulateCycle();
				TheCIA1->EmulateCycle();
				TheCIA2->EmulateCycle();
			}
#endif
			if ((flags & SNAPSHOT_1541) != 0) {
				Prefs *prefs = new Prefs(ThePrefs);
	
				// First switch on emulation
				error |= (fread(prefs->DrivePath[0], 256, 1, f) != 1);
				prefs->Emul1541Proc = true;
				NewPrefs(prefs);
				ThePrefs = *prefs;
				delete prefs;
	
				// Then read the context
				error |= !Load1541State(f);
	
				delay = fgetc(f);	// Number of cycles the 6502 is ahead of the previous chips
#ifdef FRODO_SC
				// Make the other chips "catch up" with the 6502
				for (i=0; i<delay; i++) {
					TheVIC->EmulateCycle();
					TheCIA1->EmulateCycle();
					TheCIA2->EmulateCycle();
					TheCPU->EmulateCycle();
				}
#endif
				Load1541JobState(f);
#ifdef __riscos__
				TheWIMP->ThePrefsToWindow();
#endif
			} else if (ThePrefs.Emul1541Proc) {	// No emulation in snapshot, but currently active?
				Prefs *prefs = new Prefs(ThePrefs);
				prefs->Emul1541Proc = false;
				NewPrefs(prefs);
				ThePrefs = *prefs;
				delete prefs;
#ifdef __riscos__
				TheWIMP->ThePrefsToWindow();
#endif
			}

#ifndef FRODO_SC
			fseek(f, vicptr, SEEK_SET);
			LoadVICState(f);	// Load VIC data twice in SL (is REALLY necessary sometimes!)
#endif
			fclose(f);
	
			if (error) {
				ShowRequester("Error reading snapshot file", "OK", NULL);
				Reset();
				return false;
			} else {
				LOGI("Snapshot loaded");
				return true;
			}
		} else {
			fclose(f);
			ShowRequester("Not a Frodo snapshot file", "OK", NULL);
			return false;
		}
	} else {
		ShowRequester("Can't open snapshot file", "OK", NULL);
		return false;
	}

	LOGI("Snapshot loaded");
}


#ifdef __BEOS__
#include "C64_Be.h"
#endif

#ifdef AMIGA
#include "C64_Amiga.h"
#endif

#ifdef __unix
#include "C64_x.h"
#endif

#ifdef __mac__
#include "C64_mac.h"
#endif

#ifdef WIN32
#include "C64_WIN32.h"
#endif

#ifdef __riscos__
#include "C64_Acorn.h"
#endif
