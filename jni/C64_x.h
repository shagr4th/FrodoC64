/*
 *  C64_x.h - Put the pieces together, X specific stuff
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

#include "main.h"


//static struct timeval tv_start;
static struct timespec tv_start;

#ifndef HAVE_USLEEP
/*
 *  NAME:
 *      usleep     -- This is the precision timer for Test Set
 *                    Automation. It uses the select(2) system
 *                    call to delay for the desired number of
 *                    micro-seconds. This call returns ZERO
 *                    (which is usually ignored) on successful
 *                    completion, -1 otherwise.
 *
 *  ALGORITHM:
 *      1) We range check the passed in microseconds and log a
 *         warning message if appropriate. We then return without
 *         delay, flagging an error.
 *      2) Load the Seconds and micro-seconds portion of the
 *         interval timer structure.
 *      3) Call select(2) with no file descriptors set, just the
 *         timer, this results in either delaying the proper
 *         ammount of time or being interupted early by a signal.
 *
 *  HISTORY:
 *      Added when the need for a subsecond timer was evident.
 *
 *  AUTHOR:
 *      Michael J. Dyer                   Telephone:   AT&T 414.647.4044
 *      General Electric Medical Systems        GE DialComm  8 *767.4044
 *      P.O. Box 414  Mail Stop 12-27         Sect'y   AT&T 414.647.4584
 *      Milwaukee, Wisconsin  USA 53201                      8 *767.4584
 *      internet:  mike@sherlock.med.ge.com     GEMS WIZARD e-mail: DYER
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/types.h>



int usleep(unsigned long int microSeconds)
{
        unsigned int            Seconds, uSec;
        int                     nfds, readfds, writefds, exceptfds;
        struct  timeval         Timer;

        nfds = readfds = writefds = exceptfds = 0;

        if( (microSeconds == (unsigned long) 0)
                || microSeconds > (unsigned long) 4000000 )
        {
                errno = ERANGE;         /* value out of range */
                perror( "usleep time out of range ( 0 -> 4000000 ) " );
                return -1;
        }

        Seconds = microSeconds / (unsigned long) 1000000;
        uSec    = microSeconds % (unsigned long) 1000000;

        Timer.tv_sec            = Seconds;
        Timer.tv_usec           = uSec;

        if( select( nfds, &readfds, &writefds, &exceptfds, &Timer ) < 0 )
        {
                perror( "usleep (select) failed" );
                return -1;
        }

        return 0;
}
#endif

#ifdef __linux__ || ANDROID
// select() timing is much more accurate under Linux
static void Delay_usec(unsigned long usec)
{
	int was_error;
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = usec;
	do {
		errno = 0;
		was_error = select(0, NULL, NULL, NULL, &tv);
	} while (was_error && (errno == EINTR));
}
#else
static void Delay_usec(unsigned long usec)
{
	usleep(usec);
}
#endif


/*
 *  Constructor, system-dependent things
 */

void C64::c64_ctor1(void)
{
	joy_minx[0] = joy_miny[0] = 32767;
	joy_maxx[0] = joy_maxy[0] = -32768;
	joy_minx[1] = joy_miny[1] = 32767;
	joy_maxx[1] = joy_maxy[1] = -32768;

	#ifdef ANDROID
		gui = 0;
		return;
	#else
	// Initialize joystick variables
	
	// we need to create a potential GUI subprocess here, because we don't want
	// it to inherit file-descriptors (such as for the audio-device and alike..)
#if defined(__svgalib__) || defined(__sdlui__)
	gui = 0;
#else
	// try to start up Tk gui.
	gui = new CmdPipe("wish", BINDIR "Frodo_GUI.tcl");
	if (gui) {
		if (gui->fail) {
			delete gui; gui = 0;
		}
	}
	// wait until the GUI process responds (if it does...)
	if (gui) {
		if (5 != gui->ewrite("ping\n",5)) {
			delete gui; gui = 0;
		} else {
			char c;
			fd_set set;
			FD_ZERO(&set);
			FD_SET(gui->get_read_fd(), &set);
 			struct timeval tv;
			tv.tv_usec = 0;
			tv.tv_sec = 5;
// Use the following commented line for HP-UX < 10.20
//			if (select(FD_SETSIZE, (int *)&set, (int *)NULL, (int *)NULL, &tv) <= 0) {
			if (select(FD_SETSIZE, &set, NULL, NULL, &tv) <= 0) {
				delete gui; gui = 0;
			} else {
				if (1 != gui->eread(&c, 1)) {
					delete gui; gui = 0;
				} else {
					if (c != 'o') {
						delete gui; gui = 0;
					}
				}
			}
		}
	}
#endif // __svgalib__

#endif
}

void C64::c64_ctor2(void)
{
#if  !defined(__svgalib__) && !defined(__sdlui__) && !defined(ANDROID)
   if (!gui) {
   	LOGE("Alas, master, no preferences window will be available.\n"
   	               "If you wish to see one, make sure the 'wish' interpreter\n"
   	               "(Tk version >= 4.1) is installed in your path.\n"
   	               "You can still use Frodo, though. Use F10 to quit, \n"
   	               "F11 to cause an NMI and F12 to reset the C64.\n"
   	               "You can change the preferences by editing ~/.frodorc\n");
   }
#endif // SVGAlib
  
	//gettimeofday(&tv_start, NULL);
   clock_gettime(CLOCK_MONOTONIC, &tv_start);
}


/*
 *  Destructor, system-dependent things
 */

void C64::c64_dtor(void)
{
}


/*
 *  Start main emulation thread
 */

void C64::Run(void)
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
	PatchKernal(ThePrefs.FastReset, ThePrefs.Emul1541Proc);

	quit_thyself = false;
	LOGI("Run thread_func!");
	(android_env)->CallVoidMethod(android_callback, emulatorReady);
	thread_func();
	LOGI("Stop thread_func!");
}

void C64::Quit(void)
{
quit_thyself = true;
}

double elapsed_time;

/*
 *  Vertical blank: Poll keyboard and joysticks, update window
 */

void C64::VBlank(bool draw_frame)
{

	//(android_env)->CallVoidMethod(android_callback, manageEventInMainThread);

	// Poll keyboard
	TheDisplay->PollKeyboard(TheCIA1->KeyMatrix, TheCIA1->RevMatrix, &joykey);
	TheDisplay->PollKeyboard(TheCIA1->KeyMatrix, TheCIA1->RevMatrix, &joykey);
	if (TheDisplay->quit_requested)
		quit_thyself = true;

	// Poll joysticks
	TheCIA1->Joystick1 = poll_joystick(0);
	TheCIA1->Joystick2 = poll_joystick(1);

	if (ThePrefs.JoystickSwap) {
		uint8 tmp = TheCIA1->Joystick1;
		TheCIA1->Joystick1 = TheCIA1->Joystick2;
		TheCIA1->Joystick2 = tmp;
	}

	// Joystick keyboard emulation
	if (TheDisplay->NumLock())
		TheCIA1->Joystick1 &= joykey;
	else
		TheCIA1->Joystick2 &= joykey;
       
	// Count TOD clocks
	TheCIA1->CountTOD();
	TheCIA2->CountTOD();

	if (have_a_break >= 1)
	{
		have_a_break = 2;
		return;
	}

	
	// Update window if needed
	if (draw_frame) {



		TheDisplay->Update();
		
		//return;
/*
		// Calculate time between VBlanks, display speedometer
		struct timeval tv;
		gettimeofday(&tv, NULL);
		if ((tv.tv_usec -= tv_start.tv_usec) < 0) {
			tv.tv_usec += 1000000;
			tv.tv_sec -= 1;
		}
		tv.tv_sec -= tv_start.tv_sec;
		double elapsed_time = (double)tv.tv_sec * 1000000 + tv.tv_usec;
		speed_index = 20000 / (elapsed_time + 1) * ThePrefs.SkipFrames * 100;

		// Limit speed to 100% if desired
		if ((speed_index > 100) && ThePrefs.LimitSpeed) {
			Delay_usec((unsigned long)(ThePrefs.SkipFrames * 20000 - elapsed_time));
			speed_index = 100;
		}

		gettimeofday(&tv_start, NULL);
*/

		struct timespec tv;
		clock_gettime(CLOCK_MONOTONIC, &tv);
		if ((tv.tv_nsec -= tv_start.tv_nsec) < 0) {
			tv.tv_nsec += 1000000000;
			tv.tv_sec -= 1;
		}
		tv.tv_sec -= tv_start.tv_sec;
		double elapsed_time = (double)tv.tv_sec * 1000000 + tv.tv_nsec / 1000;
		speed_index = 20000 / (elapsed_time + 1) * ThePrefs.SkipFrames * 100;

		// Limit speed to 100% if desired
		if ((speed_index > 100) && ThePrefs.LimitSpeed) {
			Delay_usec((unsigned long)(ThePrefs.SkipFrames * 20000 - elapsed_time));
			speed_index = 100;
		}

		clock_gettime(CLOCK_MONOTONIC, &tv_start);

		//TheDisplay->Speedometer((int)speed_index);
	}
/*
	struct timeval tv;
	gettimeofday(&tv, NULL);

	double current_time = (double)tv.tv_sec * 1000000 + tv.tv_usec;
		
	if (elapsed_time == 0)
	{
		TheDisplay->Update();
		elapsed_time = current_time;
	} else {
		if (current_time - elapsed_time > 20000)
		{
			// skip this one
			elapsed_time = current_time;
		} else {
			TheDisplay->Update();
			elapsed_time = current_time;
		}
	}
*/

}

//int yyy;

/*
 * The emulation's main loop
 */

void C64::thread_func(void)
{
	int linecnt = 0;

#ifdef FRODO_SC
	while (!quit_thyself) {

		// The order of calls is important here
		if (TheVIC->EmulateCycle())
			TheSID->EmulateLine();
		TheCIA1->CheckIRQs();
		TheCIA2->CheckIRQs();
		TheCIA1->EmulateCycle();
		TheCIA2->EmulateCycle();
		TheCPU->EmulateCycle();

		if (ThePrefs.Emul1541Proc) {
			TheCPU1541->CountVIATimers(1);
			if (!TheCPU1541->Idle)
				TheCPU1541->EmulateCycle();
		}
		CycleCounter++;
#else
	while (!quit_thyself) {

#ifdef ANDROID
		while(have_a_break == 2) {
			//TheC64->poll_joystick(0);
			//VBlank(false);
			//TheDisplay->Update();
			//usleep(1000000/50);
			usleep(100000);
		}
#endif
		/*if (yyy == 1)
			{
				LOGI("exit loop");
				yyy = 0;
			}*/

		// The order of calls is important here
		int cycles = TheVIC->EmulateLine();
		TheSID->EmulateLine();
		//if (yyy) LOGI(">>3");
#if !PRECISE_CIA_CYCLES
		TheCIA1->EmulateLine(ThePrefs.CIACycles);
//if (yyy) LOGI(">>4");
		TheCIA2->EmulateLine(ThePrefs.CIACycles);
#endif
	//if (yyy) LOGI(">>5");

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
	
		linecnt++;

	}

	
}

//#ifdef __sdlui__
void C64::Pause() {
	LOGI("pauseC64");
	have_a_break=1;
	if (TheSID)
		TheSID->PauseSound();
}

void C64::Resume() {
	LOGI("resumeC64");
	have_a_break=0;
	if (TheSID)
		TheSID->ResumeSound();
}
//#endif

