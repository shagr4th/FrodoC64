/*
 *  C64_epoc32.i - EPOC32 specific stuff
 *
 *  Frodo (C) 1994-1997 Christian Bauer
 *
 *  -> ported to Epoc32 by Alfred E. Heggestad & Jal Panvel (c) 1999-2000
 */

#include <process.h>
#include "main.h"
#include "sys/time.h"

#ifndef BOOL
#define BOOL bool
#endif

#define DWORD long

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

#ifdef USE_STDLIB
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#endif

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
#ifdef USE_STDLIB
                errno = ERANGE;         /* value out of range */
#else
#endif
                //!!perror( "usleep time out of range ( 0 -> 4000000 ) " );
                return -1;
        }

        Seconds = microSeconds / (unsigned long) 1000000;
        uSec    = microSeconds % (unsigned long) 1000000;

        Timer.tv_sec            = Seconds;
        Timer.tv_usec           = uSec;

        return 0;
}
#endif



void C64::c64_ctor1()
/**
 *  Constructor, system-dependent things
 */
	{
	autoskip_value = 3; // Default value
	cumulative_timer_counter = 0;
	autoskip_recalibration_counter = 0;
	timer_start_ticks = 0;

#if _DEBUG
	vic_time = 0;
#endif
	}


void C64::c64_ctor2()
	{
	}


void C64::c64_dtor()
/*
 *  Destructor, system-dependent things
 */
	{
	}


void C64::Run()
/**
 *  Start emulation
 */
	{
	TheCPU->Reset();
	TheSID->Reset(); /// @todo enable SID
	TheCIA1->Reset();
	TheCIA2->Reset();
	TheCPU1541->Reset();

	// Patch kernal IEC routines
	orig_kernal_1d84 = Kernal[0x1d84];
	orig_kernal_1d85 = Kernal[0x1d85];
    PatchKernal(ThePrefs.FastReset, ThePrefs.Emul1541Proc);

	// Start the CPU thread
	}


void C64::Quit()
/**
 *  Stop emulation
 */
	{
	quit_thyself = TRUE;
	}


//#define ABS(a) ((a)<0 ? -(a) : (a))

void C64::VBlank(bool draw_frame)
/**
 *  Vertical blank: Poll keyboard and joysticks, update window
 */
	{
	// Poll keyboard
	TheDisplay->PollKeyboard(TheCIA1->KeyMatrix, TheCIA1->RevMatrix, &joykey);
	if (TheDisplay->quit_requested)
		quit_thyself = true;

	// Poll joysticks
	if (ThePrefs.Joystick1On)
		TheCIA1->Joystick1 = poll_joystick(0);
	if (ThePrefs.Joystick2On)
		TheCIA1->Joystick2 = poll_joystick(1);

//	TheCIA1->Joystick1 = 0xff;
//	TheCIA1->Joystick2 = 0xff;

#ifndef __SYMBIAN32__
	if (ThePrefs.JoystickSwap) {
		uint8 tmp = TheCIA1->Joystick1;
		TheCIA1->Joystick1 = TheCIA1->Joystick2;
		TheCIA1->Joystick2 = tmp;
	}
#endif

       
	// Count TOD clocks
	TheCIA1->CountTOD();
	TheCIA2->CountTOD();

	// Update window if needed
	while (draw_frame) { // do only once!

    	TheDisplay->Update();

#ifdef BACKGND_UPDATE
		if(TheVIC->ec_changed > 0)	// reset flag
			--TheVIC->ec_changed;
#endif

		// Calculate time between VBlanks, display speedometer
/*
		struct timeval tv;
#ifdef USE_STDLIB
		gettimeofday(&tv, NULL);
#else
		tv.tv_usec = User::TickCount() * 15625;
		tv.tv_sec = tv.tv_usec / 1000000;
		tv.tv_usec %= 1000000;
#endif		
		if ((tv.tv_usec -= tv_start.tv_usec) < 0) {
			tv.tv_usec += 1000000;
			tv.tv_sec -= 1;
		}
		tv.tv_sec -= tv_start.tv_sec;
		double elapsed_time = (double)tv.tv_sec * 1000000 + tv.tv_usec;
		speed_index = 20000 / (elapsed_time + 1) * ThePrefs.SkipFrames * 100;
*/

		int frameskip = (ThePrefs.AutoSkipFramesOn ? autoskip_value : ThePrefs.SkipFrames);
#ifdef __WINS__
		if ((cumulative_timer_counter++)%(60 - frameskip))
			break;
		int speed_of_100_percent_ms = frameskip * 20 * 60;
#else
		if ((cumulative_timer_counter++)%(16 - frameskip))
			break;
		int speed_of_100_percent_ms = frameskip * 20 * 16;
#endif

		// Elapsed time can be quite much if e.g. pause has been selected
		// That will not disturb autoframskipping (only will decrement autoskip_value for 1)
		TUint current_ticks = User::TickCount();	
		TUint elapsed_ticks = current_ticks - timer_start_ticks;	
		if (elapsed_ticks < 0) { // out of scope?
			timer_start_ticks = current_ticks;
			break;	
		}

		TTimeIntervalMicroSeconds32 period;
		UserHal::TickPeriod(period);
		TInt period_ms = period.Int() / 1000;
		TUint elapsed_time_ms = elapsed_ticks * period_ms;
		TUint prev_speed_index = speed_index;
		speed_index = ( speed_of_100_percent_ms * 100 / (elapsed_time_ms + 1)); // as percentage 		

		//
		// Autoframeskip handling
		//

		if (ThePrefs.AutoSkipFramesOn) {

			
			// Sanity checks! Try to spot incorrect values and skip autoframeskipping.
			if ((abs(prev_speed_index - speed_index) < 10) // Do not accept over 10% speed difference per round!
			     && (speed_index > 40) && (speed_index < 140)) 
				 { 
		                           
			// Sanity checks! Try to spot incorrect valuen and reset history.  
			if ((abs(stored_speed_index - speed_index) > 50) // Too big difference!
			    || ((stored_autoskip_value > autoskip_value) && (stored_speed_index < speed_index)) // not possible! 
			    || ((stored_autoskip_value < autoskip_value) && (stored_speed_index > speed_index))) // not possible!
				
				stored_autoskip_value = 0; // Reset. Try new skip value!

			// First suppose that we always change the skip value
			int new_autoskip_value = 0;
			if (speed_index < 100)
				new_autoskip_value = autoskip_value + 1;
			else
				new_autoskip_value = autoskip_value - 1;
			if (new_autoskip_value > 15)
				new_autoskip_value = 15;
			if (new_autoskip_value < 1)
				new_autoskip_value = 1;

			// Check if we can utilize history information
			if (stored_autoskip_value == new_autoskip_value) {
				TUint speed_diff = abs(100-speed_index) - abs(100-stored_speed_index); 
			    if (abs(speed_diff) < 10) {
					// stored skip value is better
					stored_autoskip_value = autoskip_value;
					stored_speed_index = speed_index;
					autoskip_value = new_autoskip_value; // = old stored value!
				}
			}
			else {
				// Try new skip value 
				stored_autoskip_value = autoskip_value;
				stored_speed_index = speed_index;
				autoskip_value = new_autoskip_value;
			}
		}
		}

			// Limit speed to 100% if desired
		if (ThePrefs.LimitSpeed && speed_index > 100) {
			usleep(1000*(unsigned long)(speed_of_100_percent_ms - (long)elapsed_time_ms));	
			elapsed_time_ms = (User::TickCount() - timer_start_ticks) * period_ms;
			speed_index = ( speed_of_100_percent_ms * 100 / (elapsed_time_ms + 1)); // as percentage 		
		}

/*!!
#ifdef USE_STDLIB
		gettimeofday(&tv_start, NULL);
#else
		tv_start.tv_usec = User::TickCount() * 15625;
		tv_start.tv_sec = tv_start.tv_usec / 1000000;
		tv_start.tv_usec %= 1000000;
#endif
*/
		timer_start_ticks = current_ticks;
		TheDisplay->Speedometer((int)speed_index);
		
		break;
		}

#ifdef __SYMBIAN32__
	
	//
	// handle asyncronous snapshots
	// the dialogs are now done in the AppUi class
	//
	CE32FrodoAppUi* appui = iFrodoPtr->iDocPtr->iAppUiPtr;
	__CHECK_NULL(appui);

	switch(appui->iSnapState)
		{
	case ESnapShotSave:
		{
		/**
		 * @todo saving snapshots on 9210 takes 9 seconds (!)
		 * this is too slow. Considering using Store here instead
		 * of FS directly.
		 */
		if(appui->iCurrentFile.Length())
			SaveSnapshot((char*)appui->iCurrentFile.PtrZ());
		appui->iSnapState = ESnapShotIdle;
		}
		break;

	case ESnapShotLoad:
		{
		if(appui->iCurrentFile.Length())
			LoadSnapshot((char*)appui->iCurrentFile.PtrZ());
		appui->iSnapState = ESnapShotIdle;
		}
		break;

	case ESnapShotIdle:
		default:
		break;
		}

#endif // __SYMBIAN32__
	
	}



/*
 *  Open/close joystick drivers given old and new state of
 *  joystick preferences
 */


/*
 *  Poll joystick port, return CIA mask
 */
uint8 C64::poll_joystick(int port)
{
	return joykey;

	//TODO: insert Joystick input types here:

//	uint8 j=0xff;
//	if (port==1) j=iJoyPadObserver->GetJoyPadState();
//	return j;

}


bool C64::TogglePause(void)
/**
 * The emulation's main loop
 */
	{
	have_a_break = !have_a_break;
	if (have_a_break) 
		{
		iSoundHaveBeenPaused = ETrue;
		TheSID->PauseSound();
		}
	else
		{
		iFrodoPtr->StartC64();
		TheSID->ResumeSound();
		}
	return have_a_break;
	}

bool C64::Paused()
/**
 * Return ETrue if the emulator is paused
 */
{
	return have_a_break;
}

void C64::thread_func()
{
	for(TInt i=0;i<TOTAL_RASTERS;i++)
		{
		//!!DEBUG
		if (debug_audio_ready_scanline_num == -1) // Audio is beeing fed
			debug_audio_ready_scanline_num = i; 

		int cycles = TheVIC->EmulateLine();

		TheSID->EmulateLine();

		TheCIA1->EmulateLine(ThePrefs.CIACycles);
		TheCIA2->EmulateLine(ThePrefs.CIACycles);

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

	}

	frameDone=true;
} // end, C64::thread_func



// EOF - C64_epoc32.i
