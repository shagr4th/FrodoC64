/*
 *  SID_Amiga.h - 6581 emulation, Amiga specific stuff
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

#include <dos/dostags.h>
#include <hardware/cia.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/ahi.h>
#include <proto/graphics.h>


// Library bases
struct Library *AHIBase;

// CIA-A base
extern struct CIA ciaa;


/*
 *  Initialization, create sub-process
 */

void DigitalRenderer::init_sound(void)
{
	// Find our (main) task
	main_task = FindTask(NULL);

	// Create signal for communication
	main_sig = AllocSignal(-1);

	// Create sub-process and wait until it is ready
	if ((sound_process = CreateNewProcTags(
		NP_Entry, (ULONG)&sub_invoc,
		NP_Name, (ULONG)"Frodo Sound Process",
		NP_Priority, 1,
		NP_ExitData, (ULONG)this,	// Easiest way to supply sub_invoc with this pointer
		TAG_DONE)) != NULL)
		Wait(1 << main_sig);
}


/*
 *  Destructor, delete sub-process
 */

DigitalRenderer::~DigitalRenderer()
{
	// Tell sub-process to quit and wait for completion
	if (sound_process != NULL) {
		Signal(&(sound_process->pr_Task), 1 << quit_sig);
		Wait(1 << main_sig);
	}

	// Free signal
	FreeSignal(main_sig);
}


/*
 *  Sample volume (for sampled voice)
 */

void DigitalRenderer::EmulateLine(void)
{
	sample_buf[sample_in_ptr] = volume;
	sample_in_ptr = (sample_in_ptr + 1) % SAMPLE_BUF_SIZE;
}


/*
 *  Pause sound output
 */

void DigitalRenderer::Pause(void)
{
	if (sound_process != NULL)
		Signal(&(sound_process->pr_Task), 1 << pause_sig);
}


/*
 *  Resume sound output
 */

void DigitalRenderer::Resume(void)
{
	if (sound_process != NULL)
		Signal(&(sound_process->pr_Task), 1 << resume_sig);
}


/*
 *  Sound sub-process
 */

void DigitalRenderer::sub_invoc(void)
{
	// Get pointer to the DigitalRenderer object and call sub_func()
	DigitalRenderer *r = (DigitalRenderer *)((struct Process *)FindTask(NULL))->pr_ExitData;
	r->sub_func();
}

void DigitalRenderer::sub_func(void)
{
	ahi_port = NULL;
	ahi_io = NULL;
	ahi_ctrl = NULL;
	sample[0].ahisi_Address = sample[1].ahisi_Address = NULL;
	ready = FALSE;

	// Create signals for communication
	quit_sig = AllocSignal(-1);
	pause_sig = AllocSignal(-1);
	resume_sig = AllocSignal(-1);
	ahi_sig = AllocSignal(-1);

	// Open AHI
	if ((ahi_port = CreateMsgPort()) == NULL)
		goto wait_for_quit;
	if ((ahi_io = (struct AHIRequest *)CreateIORequest(ahi_port, sizeof(struct AHIRequest))) == NULL)
		goto wait_for_quit;
	ahi_io->ahir_Version = 2;
	if (OpenDevice(AHINAME, AHI_NO_UNIT, (struct IORequest *)ahi_io, NULL))
		goto wait_for_quit;
	AHIBase = (struct Library *)ahi_io->ahir_Std.io_Device;

	// Initialize callback hook
	sf_hook.h_Entry = sound_func;

	// Open audio control structure
	if ((ahi_ctrl = AHI_AllocAudio(
		AHIA_AudioID, 0x0002000b,
		AHIA_MixFreq, SAMPLE_FREQ,
		AHIA_Channels, 1,
		AHIA_Sounds, 2,
		AHIA_SoundFunc, (ULONG)&sf_hook,
		AHIA_UserData, (ULONG)this,
		TAG_DONE)) == NULL)
		goto wait_for_quit;

	// Prepare SampleInfos and load sounds (two sounds for double buffering)
	sample[0].ahisi_Type = AHIST_M16S;
	sample[0].ahisi_Length = SAMPLE_FREQ / CALC_FREQ;
	sample[0].ahisi_Address = AllocVec(SAMPLE_FREQ / CALC_FREQ * 2, MEMF_PUBLIC | MEMF_CLEAR);
	sample[1].ahisi_Type = AHIST_M16S;
	sample[1].ahisi_Length = SAMPLE_FREQ / CALC_FREQ;
	sample[1].ahisi_Address = AllocVec(SAMPLE_FREQ / CALC_FREQ * 2, MEMF_PUBLIC | MEMF_CLEAR);
	if (sample[0].ahisi_Address == NULL || sample[1].ahisi_Address == NULL)
		goto wait_for_quit;
	AHI_LoadSound(0, AHIST_DYNAMICSAMPLE, &sample[0], ahi_ctrl);
	AHI_LoadSound(1, AHIST_DYNAMICSAMPLE, &sample[1], ahi_ctrl);

	// Set parameters
	play_buf = 0;
	AHI_SetVol(0, 0x10000, 0x8000, ahi_ctrl, AHISF_IMM);
	AHI_SetFreq(0, SAMPLE_FREQ, ahi_ctrl, AHISF_IMM);
	AHI_SetSound(0, play_buf, 0, 0, ahi_ctrl, AHISF_IMM);

	// Start audio output
	AHI_ControlAudio(ahi_ctrl, AHIC_Play, TRUE, TAG_DONE);

	// We are now ready for commands
	ready = TRUE;
	Signal(main_task, 1 << main_sig);

	// Accept and execute commands
	for (;;) {
		ULONG sigs = Wait((1 << quit_sig) | (1 << pause_sig) | (1 << resume_sig) | (1 << ahi_sig));

		// Quit sub-process
		if (sigs & (1 << quit_sig))
			goto quit;

		// Pause sound output
		if (sigs & (1 << pause_sig))
			AHI_ControlAudio(ahi_ctrl, AHIC_Play, FALSE, TAG_DONE);

		// Resume sound output
		if (sigs & (1 << resume_sig))
			AHI_ControlAudio(ahi_ctrl, AHIC_Play, TRUE, TAG_DONE);

		// Calculate next buffer
		if (sigs & (1 << ahi_sig))
			calc_buffer((int16 *)(sample[play_buf].ahisi_Address), sample[play_buf].ahisi_Length * 2);
	}

wait_for_quit:
	// Initialization failed, wait for quit signal
	Wait(1 << quit_sig);

quit:
	// Free everything
	if (ahi_ctrl != NULL) {
		AHI_ControlAudio(ahi_ctrl, AHIC_Play, FALSE, TAG_DONE);
		AHI_FreeAudio(ahi_ctrl);
		CloseDevice((struct IORequest *)ahi_io);
	}

	FreeVec(sample[0].ahisi_Address);
	FreeVec(sample[1].ahisi_Address);

	if (ahi_io != NULL)
		DeleteIORequest((struct IORequest *)ahi_io);

	if (ahi_port != NULL)
		DeleteMsgPort(ahi_port);

	FreeSignal(quit_sig);
	FreeSignal(pause_sig);
	FreeSignal(resume_sig);
	FreeSignal(ahi_sig);

	// Quit (synchronized with main task)
	Forbid();
	Signal(main_task, 1 << main_sig);
}


/*
 *  AHI sound callback, play next buffer and signal sub-process
 */

ULONG DigitalRenderer::sound_func(void)
{
	register struct AHIAudioCtrl *ahi_ctrl asm ("a2");
	DigitalRenderer *r = (DigitalRenderer *)ahi_ctrl->ahiac_UserData;
	r->play_buf ^= 1;
	AHI_SetSound(0, r->play_buf, 0, 0, ahi_ctrl, 0);
	Signal(&(r->sound_process->pr_Task), 1 << (r->ahi_sig));
	return 0;
}


/*
 *  Renderer for SID card
 */

// Renderer class
class SIDCardRenderer : public SIDRenderer {
public:
	SIDCardRenderer();
	virtual ~SIDCardRenderer();

	virtual void Reset(void);
	virtual void EmulateLine(void) {}
	virtual void WriteRegister(uint16 adr, uint8 byte);
	virtual void NewPrefs(Prefs *prefs) {}
	virtual void Pause(void) {}
	virtual void Resume(void) {}

private:
	UBYTE *sid_base;	// SID card base pointer
};

// Constructor: Reset SID
SIDCardRenderer::SIDCardRenderer()
{
	sid_base = (UBYTE *)0xa00001;
	Reset();
}

// Destructor: Reset SID
SIDCardRenderer::~SIDCardRenderer()
{
	Reset();
}

// Reset SID
void SIDCardRenderer::Reset(void)
{
	WaitTOF();
	ciaa.ciapra |= CIAF_LED;
	WaitTOF();
	ciaa.ciapra &= ~CIAF_LED;
}

// Write to register
void SIDCardRenderer::WriteRegister(uint16 adr, uint8 byte)
{
	sid_base[adr << 1] = byte;
}
