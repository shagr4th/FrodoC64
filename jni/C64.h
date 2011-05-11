/*
 *  C64.h - Put the pieces together
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

#ifndef _C64_H
#define _C64_H

#ifdef __BEOS__
#include <KernelKit.h>
#endif

#ifdef AMIGA
#include <devices/timer.h>
#include <devices/gameport.h>
#include <devices/inputevent.h>
#endif

#ifdef __riscos__
#include "ROlib.h"
#endif
#include <android/log.h> 
#include <jni.h>

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "frodoc64",  __VA_ARGS__) 
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , "frodoc64",  __VA_ARGS__) 
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , "frodoc64",  __VA_ARGS__) 
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , "frodoc64",  __VA_ARGS__) 
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , "frodoc64",  __VA_ARGS__) 

extern JNIEnv *android_env;
extern jobject android_callback;
/*extern jmethodID refreshMainScreen;*/
extern jmethodID getEvent;
extern jmethodID initAudio;
extern jmethodID pauseAudio;
extern jmethodID playAudio;
extern jmethodID sendAudio;
extern jmethodID emulatorReady;
extern jmethodID drawled;
extern jmethodID requestRender;
// Sizes of memory areas
const int C64_RAM_SIZE = 0x10000;
const int COLOR_RAM_SIZE = 0x400;
const int BASIC_ROM_SIZE = 0x2000;
const int KERNAL_ROM_SIZE = 0x2000;
const int CHAR_ROM_SIZE = 0x1000;
const int DRIVE_RAM_SIZE = 0x800;
const int DRIVE_ROM_SIZE = 0x4000;


// false: Frodo, true: FrodoSC
extern bool IsFrodoSC;


class Prefs;
class C64Display;
class MOS6510;
class MOS6569;
class MOS6581;
class MOS6526_1;
class MOS6526_2;
class IEC;
class REU;
class MOS6502_1541;
class Job1541;
class CmdPipe;

class C64 {
public:
	C64();
	~C64();

	void Run(void);
	void Quit(void);
	void Pause(void);
	void Resume(void);
	void Reset(void);
	void NMI(void);
	void VBlank(bool draw_frame);
	void NewPrefs(Prefs *prefs);
	void PatchKernal(bool fast_reset, bool emul_1541_proc);
	void SaveRAM(char *filename);
	void SaveSnapshot(char *filename);
	bool LoadSnapshot(char *filename);
	int SaveCPUState(FILE *f);
	int Save1541State(FILE *f);
	bool Save1541JobState(FILE *f);
	bool SaveVICState(FILE *f);
	bool SaveSIDState(FILE *f);
	bool SaveCIAState(FILE *f);
	bool LoadCPUState(FILE *f);
	bool Load1541State(FILE *f);
	bool Load1541JobState(FILE *f);
	bool LoadVICState(FILE *f);
	bool LoadSIDState(FILE *f);
	bool LoadCIAState(FILE *f);

	uint8 *RAM, *Basic, *Kernal,
		  *Char, *Color;		// C64
	uint8 *RAM1541, *ROM1541;	// 1541

	C64Display *TheDisplay;

	MOS6510 *TheCPU;			// C64
	MOS6569 *TheVIC;
	MOS6581 *TheSID;
	MOS6526_1 *TheCIA1;
	MOS6526_2 *TheCIA2;
	IEC *TheIEC;
	REU *TheREU;

	MOS6502_1541 *TheCPU1541;	// 1541
	Job1541 *TheJob1541;

#ifdef FRODO_SC
	uint32 CycleCounter;
#endif

private:
	void c64_ctor1(void);
	void c64_ctor2(void);
	void c64_dtor(void);
	void open_close_joysticks(int oldjoy1, int oldjoy2, int newjoy1, int newjoy2);
	uint8 poll_joystick(int port);
	void thread_func(void);

	bool thread_running;	// Emulation thread is running
	bool quit_thyself;		// Emulation thread shall quit
	int have_a_break;		// Emulation thread shall pause

	int joy_minx[2], joy_maxx[2], joy_miny[2], joy_maxy[2]; // For dynamic joystick calibration
	uint8 joykey;			// Joystick keyboard emulation mask value

	uint8 orig_kernal_1d84,	// Original contents of kernal locations $1d84 and $1d85
		  orig_kernal_1d85;	// (for undoing the Fast Reset patch)

#ifdef __BEOS__
public:
	void SoundSync(void);

private:
	static long thread_invoc(void *obj);
	void open_close_joystick(int port, int oldjoy, int newjoy);

	void *joy[2];			// Joystick objects (BJoystick or BDigitalPort)
	bool joy_geek_port[2];	// Flag: joystick on GeekPort?
	thread_id the_thread;
	sem_id pause_sem;
	sem_id sound_sync_sem;
	bigtime_t start_time;
#endif

#ifdef AMIGA
	struct MsgPort *timer_port;		// For speed limiter
	struct timerequest *timer_io;
	struct timeval start_time;
	struct MsgPort *game_port;		// For joystick
	struct IOStdReq *game_io;
	struct GamePortTrigger game_trigger;
	struct InputEvent game_event;
	UBYTE joy_state;				// Current state of joystick
	bool game_open, port_allocated;	// Flags: gameport.device opened, game port allocated
#endif

#ifdef __unix
	void open_close_joystick(int port, int oldjoy, int newjoy);
	double speed_index;
public:
	CmdPipe *gui;
#endif


#ifdef WIN32
private:
	void CheckTimerChange();
	void StartTimer();
	void StopTimer();
	static void CALLBACK StaticTimeProc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);
	void TimeProc(UINT id);
#ifdef FRODO_SC
	void EmulateCyclesWith1541();
	void EmulateCyclesWithout1541();
#endif

	DWORD ref_time;				// when frame count was reset
	int skipped_frames;			// number of skipped frames
	int timer_every;			// frequency of timer in frames
	HANDLE timer_semaphore;		// Timer semaphore for synch
	MMRESULT timer_id;			// Timer identifier
	int frame;					// current frame number
	uint8 joy_state;			// Current state of joystick
	bool state_change;
#endif

#ifdef __riscos__
public:
	void RequestSnapshot(void);
	bool LoadOldSnapshot(FILE *f);
	void LoadSystemConfig(const char *filename);	// loads timing vals and keyboard joys
	void SaveSystemConfig(const char *filename);	// saves timing vals and keyboard joys
	void ReadTimings(int *poll_after, int *speed_after, int *sound_after);
	void WriteTimings(int poll_after, int speed_after, int sound_after);

	WIMP *TheWIMP;
	int PollAfter;		// centiseconds before polling
	int SpeedAfter;		// centiseconds before updating speedometer
	int PollSoundAfter;	// *rasterlines* after which DigitalRenderer is polled
	int HostVolume;		// sound volume of host machine

private:
	bool make_a_snapshot;

	uint8 joykey2;			// two keyboard joysticks possible here

	uint8 joystate[2];		// Joystick state
	bool Poll;			// TRUE if polling should take place
	int LastPoll, LastFrame, LastSpeed;	// time of last poll / last frame / speedom (cs)
	int FramesSince;
	int laststate;			// last keyboard state (-> scroll lock)
	int lastptr;			// last mouse pointer shape
	bool SingleTasking;
#endif

};



#endif
