/*
 *  C64_WIN32.h - Put the pieces together, WIN32 specific stuff
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

#include <process.h>
#include "main.h"

#define FRAME_INTERVAL		(1000/SCREEN_FREQ)	// in milliseconds
#ifdef FRODO_SC
#define SPEEDOMETER_INTERVAL	4000			// in milliseconds
#else
#define SPEEDOMETER_INTERVAL	1000			// in milliseconds
#endif
#define JOYSTICK_SENSITIVITY	40			// % of live range
#define JOYSTICK_MIN		0x0000			// min value of range
#define JOYSTICK_MAX		0xffff			// max value of range
#define JOYSTICK_RANGE		(JOYSTICK_MAX - JOYSTICK_MIN)

static BOOL high_resolution_timer = FALSE;

/*
 *  Constructor, system-dependent things
 */

void C64::c64_ctor1()
{
	Debug("C64::c64_ctor1\n");

	// Initialize joystick variables.
	joy_state = 0xff;

	// No need to check for state change.
	state_change = FALSE;

	// Start the synchronization timer.
	timer_semaphore = NULL;
	timer_id = NULL;
	StartTimer();
}

void C64::c64_ctor2()
{
	Debug("C64::c64_ctor2\n");
}


/*
 *  Destructor, system-dependent things
 */

void C64::c64_dtor()
{
	Debug("C64::c64_dtor\n");

	StopTimer();
}


/*
 *  Start emulation
 */

void C64::Run()
{
	// Reset chips
	TheCPU->Reset();
	TheSID->Reset();
	TheCIA1->Reset();
	TheCIA2->Reset();
	TheCPU1541->Reset();

	// Patch kernal IEC routines
	orig_kernal_1d84 = Kernal[0x1d84];
	orig_kernal_1d85 = Kernal[0x1d85];
	patch_kernal(ThePrefs.FastReset, ThePrefs.Emul1541Proc);

	// Start the CPU thread
	thread_func();
}


/*
 *  Stop emulation
 */

void C64::Quit()
{
	// Ask the thread to quit itself if it is running
	quit_thyself = TRUE;
	state_change = TRUE;
}


/*
 *  Pause emulation
 */

void C64::Pause()
{
	StopTimer();
	TheSID->PauseSound();
	have_a_break = TRUE;
	state_change = TRUE;
}


/*
 *  Resume emulation
 */

void C64::Resume()
{
	StartTimer();
	TheSID->ResumeSound();
	have_a_break = FALSE;
}


/*
 *  Vertical blank: Poll keyboard and joysticks, update window
 */

void C64::VBlank(bool draw_frame)
{
	//Debug("C64::VBlank\n");

	// Poll the keyboard.
	TheDisplay->PollKeyboard(TheCIA1->KeyMatrix, TheCIA1->RevMatrix, &joykey);
	// Poll the joysticks.
	TheCIA1->Joystick1 = poll_joystick(0);
	TheCIA1->Joystick2 = poll_joystick(1);

	if (ThePrefs.JoystickSwap) {
		uint8 tmp = TheCIA1->Joystick1;
		TheCIA1->Joystick1 = TheCIA1->Joystick2;
		TheCIA1->Joystick2 = tmp;
	}

	// Joystick keyboard emulation.
	if (TheDisplay->NumLock())
		TheCIA1->Joystick1 &= joykey;
	else
		TheCIA1->Joystick2 &= joykey;

	// Count TOD clocks.
	TheCIA1->CountTOD();
	TheCIA2->CountTOD();

#if 1
	// Output a frag.
	TheSID->VBlank();
#endif
	
	if (have_a_break)
		return;

	// Update the window if needed.
	frame++;
	if (draw_frame) {

		// Synchronize to the timer if limiting the speed.
		if (ThePrefs.LimitSpeed) {
			if (skipped_frames == 0) {
				// There is a tiny race condtion here that
				// could cause a full extra delay cycle.
				WaitForSingleObject(timer_semaphore, INFINITE);
			}
			else {
				Debug("*** Skipped a frame! ***\n");
				skipped_frames = 0;
			}
		}

		// Perform the actual screen update exactly at the
		// beginning of an interval for the smoothest video.
		TheDisplay->Update();

		// Compute the speed index and show it in the speedometer.
		DWORD now = timeGetTime();
		int elapsed_time = now - ref_time;
		if (now - ref_time >= SPEEDOMETER_INTERVAL) {
			double speed_index = double(frame * FRAME_INTERVAL * 100 + elapsed_time/2) / elapsed_time;
			TheDisplay->Speedometer((int)speed_index);
			ref_time = now;
			frame = 0;
		}

		// Make sure our timer is set correctly.
		CheckTimerChange();
	}
}


void C64::CheckTimerChange()
{
	// Make sure the timer interval matches the preferences.
	if (!ThePrefs.LimitSpeed && timer_every == 0)
		return;
	if (ThePrefs.LimitSpeed && ThePrefs.SkipFrames == timer_every)
		return;
	StopTimer();
	StartTimer();
}

/*
 *  Open/close joystick drivers given old and new state of
 *  joystick preferences
 */

BOOL joystick_open[2];

void C64::open_close_joysticks(int oldjoy1, int oldjoy2, int newjoy1, int newjoy2)
{
	if (oldjoy1 != newjoy1) {
		joystick_open[0] = FALSE;
		if (newjoy1) {
			JOYINFO joyinfo;
			if (joyGetPos(0, &joyinfo) == JOYERR_NOERROR)
				joystick_open[0] = TRUE;
		}
	}

	if (oldjoy2 != newjoy2) {
		joystick_open[1] = FALSE;
		if (newjoy1) {
			JOYINFO joyinfo;
			if (joyGetPos(1, &joyinfo) == JOYERR_NOERROR)
				joystick_open[1] = TRUE;
		}
	}

	// XXX: Should have our own new prefs!
	state_change = TRUE;
}


/*
 *  Poll joystick port, return CIA mask
 */

uint8 C64::poll_joystick(int port)
{
	uint8 j = 0xff;

	if (joystick_open[port]) {
		JOYINFO joyinfo;
		if (joyGetPos(port, &joyinfo) == JOYERR_NOERROR) {
			int x = joyinfo.wXpos;
			int y = joyinfo.wYpos;
			int buttons = joyinfo.wButtons;
			int s1 = JOYSTICK_SENSITIVITY;
			int s2 = 100 - JOYSTICK_SENSITIVITY;
			if (x < JOYSTICK_MIN + s1*JOYSTICK_RANGE/100)
				j &= 0xfb; // Left
			else if (x > JOYSTICK_MIN + s2*JOYSTICK_RANGE/100)
				j &= 0xf7; // Right
			if (y < JOYSTICK_MIN + s1*JOYSTICK_RANGE/100)
				j &= 0xfe; // Up
			else if (y > JOYSTICK_MIN + s2*JOYSTICK_RANGE/100)
				j &= 0xfd; // Down
			if (buttons & 1)
				j &= 0xef; // Button
			if (buttons & 2) {
				Pause();
				while (joyGetPos(port, &joyinfo) == JOYERR_NOERROR && (joyinfo.wButtons & 2))
					Sleep(100);
				Resume();
			}
		}
	}

	return j;
}

void C64::StartTimer()
{
	ref_time = timeGetTime();
	skipped_frames = 0;
	frame = 0;

	if (!ThePrefs.LimitSpeed) {
		timer_every = 0;
		StopTimer();
		return;
	}
	timer_every = ThePrefs.SkipFrames;

	if (!timer_semaphore) {
		timer_semaphore = CreateSemaphore(NULL, 0, 1, NULL);
		if (!timer_semaphore)
			Debug("CreateSemaphore failed\n");
	}

	if (!timer_id) {

		// Turn on high-resolution times and delays.
		int resolution = FRAME_INTERVAL;
		if (high_resolution_timer) {
			timeBeginPeriod(1);
			resolution = 0;
		}

		timer_id = timeSetEvent(timer_every*FRAME_INTERVAL, resolution, StaticTimeProc, (DWORD) this, TIME_PERIODIC);
		if (!timer_id)
			Debug("timeSetEvent failed\n");
	}
}

void C64::StopTimer()
{
	if (timer_semaphore) {
		CloseHandle(timer_semaphore);
		timer_semaphore = NULL;
	}
	if (timer_id) {
		timeKillEvent(timer_id);
		timer_id = NULL;

		// Turn off high-resolution delays.
		if (high_resolution_timer)
			timeEndPeriod(1);
	}

}

void CALLBACK C64::StaticTimeProc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	C64* TheC64 = (C64 *) dwUser;
	TheC64->TimeProc(uID);
}

void C64::TimeProc(UINT id)
{
	if (id != timer_id) {
		Debug("TimeProc called for wrong timer id!\n");
		timeKillEvent(id);
		return;
	}

	if (!ReleaseSemaphore(timer_semaphore, 1, NULL))
		skipped_frames++;
}


/*
 * The emulation's main loop
 */

void C64::thread_func()
{
	Debug("C64::thread_func\n");

	thread_running = TRUE;

	while (!quit_thyself) {

		if (have_a_break)
			TheDisplay->WaitUntilActive();

#ifdef FRODO_SC
		if (ThePrefs.Emul1541Proc)
			EmulateCyclesWith1541();
		else
			EmulateCyclesWithout1541();
		state_change = FALSE;
#else
		// The order of calls is important here
		int cycles = TheVIC->EmulateLine();
		TheSID->EmulateLine();
#if !PRECISE_CIA_CYCLES
		TheCIA1->EmulateLine(ThePrefs.CIACycles);
		TheCIA2->EmulateLine(ThePrefs.CIACycles);
#endif
		if (ThePrefs.Emul1541Proc) {
			int cycles_1541 = ThePrefs.FloppyCycles;
			TheCPU1541->CountVIATimers(cycles_1541);

			if (!TheCPU1541->Idle) {
				// 1541 processor active, alternately execute
				//  6502 and 6510 instructions until both have
				//  used up their cycles
				while (cycles >= 0 || cycles_1541 >= 0)
					if (cycles > cycles_1541)
						cycles -= TheCPU->EmulateLine(1);
					else
						cycles_1541 -= TheCPU1541->EmulateLine(1);
			} else
				TheCPU->EmulateLine(cycles);
		} else
			// 1541 processor disabled, only emulate 6510
			TheCPU->EmulateLine(cycles);
#endif
	}

	thread_running = FALSE;

}

#ifdef FRODO_SC

void C64::EmulateCyclesWith1541()
{
	thread_running = TRUE;
	while (!state_change) {
		// The order of calls is important here
		if (TheVIC->EmulateCycle())
			TheSID->EmulateLine();
#ifndef BATCH_CIA_CYCLES
		TheCIA1->EmulateCycle();
		TheCIA2->EmulateCycle();
#endif
		TheCPU->EmulateCycle();
		TheCPU1541->CountVIATimers(1);
		if (!TheCPU1541->Idle)
			TheCPU1541->EmulateCycle();
		CycleCounter++;
	}
}

void C64::EmulateCyclesWithout1541()
{
	thread_running = TRUE;
	while (!state_change) {
		// The order of calls is important here
		if (TheVIC->EmulateCycle())
			TheSID->EmulateLine();
#ifndef BATCH_CIA_CYCLES
		TheCIA1->EmulateCycle();
		TheCIA2->EmulateCycle();
#endif
		TheCPU->EmulateCycle();
		CycleCounter++;
	}
}

#endif
