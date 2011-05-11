/*
 *  C64_Acorn.h - Put the pieces together, RISC OS specific stuff
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

#include "Prefs.h"
#include "ROlib.h"
#include "AcornGUI.h"


void C64::LoadSystemConfig(const char *filename)
{
  FILE *fp;

  if ((fp = fopen(filename, "r")) != NULL)
  {
    int i;
    Joy_Keys *jk;
    int args[10];
    char line[256];

    while (fgets(line, 255, fp) != 0)
    {
      char *b = line;
      register char c;

      do {c = *b++;} while (c > 32);
      if (c == 32)	// keyword mustn't contain spaces
      {
        *(b-1) = '\0';
        do {c = *b++;} while ((c >= 32) && (c != '='));
        if (c == '=')	// read in keyword's arguments
        {
          int i=0;

          while ((*b != '\0') && (i < 10))
          {
            args[i++] = strtol(b, &b, 10);
          }
          if (strcmp(line, "PollAfter") == 0) {PollAfter = args[0];}
          else if (strcmp(line, "SpeedAfter") == 0) {SpeedAfter = args[0];}
          else if (strcmp(line, "PollSoundAfter") == 0) {PollSoundAfter = args[0];}
          else if (strcmp(line, "JoystickKeys1") == 0)
          {
            jk = &(TheDisplay->JoystickKeys[0]);
            jk->up = args[0]; jk->down = args[1]; jk->left = args[2]; jk->right = args[3];
            jk->fire = args[4];
          }
          else if (strcmp(line, "JoystickKeys2") == 0)
          {
            jk = &(TheDisplay->JoystickKeys[1]);
            jk->up = args[0]; jk->down = args[1]; jk->left = args[2]; jk->right = args[3];
            jk->fire = args[4];
          }
          else
          {
            _kernel_oserror err;

            err.errnum = 0;
            sprintf(err.errmess,"Bad keyword <%s> in system configure file!",line);
            Wimp_ReportError(&err,1,TASKNAME);
          }
        }
      }
    }
    fclose(fp);
  }
}


void C64::SaveSystemConfig(const char *filename)
{
  FILE *fp;

  if ((fp = fopen(filename, "w")) != NULL)
  {
    int i;
    Joy_Keys *jk;

    fprintf(fp,"PollAfter = %d\n", PollAfter);
    fprintf(fp,"SpeedAfter = %d\n", SpeedAfter);
    fprintf(fp,"PollSoundAfter = %d\n", PollSoundAfter);
    for (i=0; i<2; i++)
    {
      jk = &(TheDisplay->JoystickKeys[i]);
      fprintf(fp,"JoystickKeys%d",i+1);
      fprintf(fp," = %d %d %d %d %d\n", jk->up, jk->down, jk->left, jk->right, jk->fire);
    }
    fclose(fp);
  }
}


void C64::ReadTimings(int *poll_after, int *speed_after, int *sound_after)
{
  *poll_after = PollAfter; *speed_after = SpeedAfter; *sound_after = PollSoundAfter;
}


void C64::WriteTimings(int poll_after, int speed_after, int sound_after)
{
  PollAfter = poll_after; SpeedAfter = speed_after; PollSoundAfter = sound_after;
}


void C64::RequestSnapshot(void)
{
  // Snapshots are only possible if the emulation progresses to the next vsync
  if (have_a_break) Resume();
  make_a_snapshot = true;
}


void C64::c64_ctor1(void)
{
  TheWIMP = new WIMP(this);
  PollAfter = 20;	// poll every 20 centiseconds
  SpeedAfter = 200;	// update speedometer every 2 seconds
  PollSoundAfter = 50;	// poll DigitalRenderer every 50 lines
  HostVolume = Sound_Volume(0);
  // Just a precaution
  if (HostVolume < 0) {HostVolume = 0;}
  if (HostVolume > MaximumVolume) {HostVolume = MaximumVolume;}
  Poll = false;
  make_a_snapshot = false;
}


void C64::c64_ctor2(void)
{
  LoadSystemConfig(DEFAULT_SYSCONF);
  // was started from multitasking so pretend ScrollLock OFF no matter what
  laststate = (ReadKeyboardStatus() & ~2); SingleTasking = false;
  lastptr = 1;
}


void C64::c64_dtor(void)
{
  delete TheWIMP;
}


void C64::open_close_joysticks(int oldjoy1, int oldjoy2, int newjoy1, int newjoy2)
{
  // Check if the Joystick module is loaded. If not then write an illegal value to
  // the joystick state.
  if (Joystick_Read(0) == -2) {joystate[0] = 0;} else {joystate[0] = 0xff;}
  if (Joystick_Read(1) == -2) {joystate[1] = 0;} else {joystate[1] = 0xff;}
}


uint8 C64::poll_joystick(int port)
{
  register int state;
  uint8 joy;

  if ((state = Joystick_Read(port)) != -2) // module present
  {
    if (state == -1) {joy = joystate[port];}	// use old value
    else
    {
      joy = 0xff;
      if ((state & (JoyButton1 + JoyButton2)) != 0) {joy &= 0xef;} // fire
      if ((state & 0x80) == 0) // positive direction #1
      {
        if ((state & 0xff) >= JoyDir_Thresh) {joy &= 0xfe;}		// up
      }
      else
      {
        if ((256 - (state & 0xff)) >= JoyDir_Thresh) {joy &= 0xfd;}	// down
      }
      if ((state & 0x8000) == 0) // positive direction #2
      {
        if ((state & 0xff00) >= JoyDir_Thresh<<8) {joy &= 0xf7;}	// right
      }
      else
      {
        if ((0x10000 - (state & 0xff00)) >= JoyDir_Thresh<<8) {joy &= 0xfb;} // left
      }
    }
    joystate[port] = joy; return(joy);
  }
  else
  {
    joystate[port] = 0; return(0xff);
  }
}


void C64::VBlank(bool draw_frame)
{
  int Now, KeyState;
  bool InputFocus;

  // Poll keyboard if the window has the input focus.
  InputFocus = TheWIMP->EmuWindow->HaveInput();
  if (InputFocus)
  {
    TheDisplay->PollKeyboard(TheCIA1->KeyMatrix, TheCIA1->RevMatrix, &joykey, &joykey2);
  }

  // Poll Joysticks
  TheCIA1->Joystick1 = (ThePrefs.Joystick1On) ? poll_joystick(0) : 0xff;
  TheCIA1->Joystick2 = (ThePrefs.Joystick2On) ? poll_joystick(1) : 0xff;

  // Swap joysticks?
  if (ThePrefs.JoystickSwap)
  {
    register uint8 h;

    h = TheCIA1->Joystick1; TheCIA1->Joystick1 = TheCIA1->Joystick2; TheCIA1->Joystick2 = h;
  }

  // Read keyboard state directly since we'll also need ScrollLock later!
  KeyState = ReadKeyboardStatus();
  if (InputFocus)
  {
    // Keyboard emulates which joystick? (NumLock ==> Port 2, else Port 1)
    if ((KeyState & 4) == 0)
    {
      TheCIA1->Joystick2 &= joykey;
    }
    else	// joykey2 only mapped if numLOCK is off.
    {
      TheCIA1->Joystick1 &= joykey; TheCIA1->Joystick2 &= joykey2;
    }
  }

  if (draw_frame)
  {
    TheDisplay->Update();
  }

  // Make snapshot?
  if (make_a_snapshot)
  {
    SaveSnapshot((TheWIMP->SnapFile)+44);
    make_a_snapshot = false;
  }

  Now = OS_ReadMonotonicTime();

  // Limit speed? (hahaha.... ah well...)
  if (ThePrefs.LimitSpeed)
  {
    int Now;

    while ((Now - LastFrame) < 2) // 2cs per frame = 50fps (original speed)
    {
      Now = OS_ReadMonotonicTime();
    }
    LastFrame = Now;
  }
  FramesSince++;

  // Update speedometer (update, not force redraw!)?
  if ((Now - LastSpeed) >= SpeedAfter)
  {
    char b[16];

    if ((Now - LastSpeed) <= 0) {Now = LastSpeed+1;}
    // Speed: 100% equals 50fps (round result)
    sprintf(b,"%d%%\0",((400*FramesSince)/(Now - LastSpeed) + 1) >> 1);
    TheWIMP->EmuPane->WriteIconTextU(Icon_Pane_Speed,b);
    LastSpeed = Now; FramesSince = 0;
  }

  if (InputFocus)
  {
    // Scroll lock state changed?
    if (((KeyState ^ laststate) & 2) != 0)
    {
      // change to single tasking: turn off mouse, else restore previous pointer
      if ((KeyState & 2) != 0) {lastptr = SetMousePointer(0); SingleTasking = true;}
      else {SetMousePointer(lastptr); OS_FlushBuffer(9); SingleTasking = false;}
    }
    if ((KeyState & 2) != 0) {lastptr = SetMousePointer(0);}
    else {SetMousePointer(lastptr); OS_FlushBuffer(9);}
  }

  // Poll? ScrollLock forces single tasking, i.e. overrides timings.
  if (!SingleTasking)
  {
    if ((Now - LastPoll) >= PollAfter)
    {
      Poll = true;
    }
  }
  laststate = KeyState;
}


void C64::Run(void)
{
  // Resetting chips
  TheCPU->Reset();
  TheSID->Reset();
  TheCIA1->Reset();
  TheCIA2->Reset();
  TheCPU1541->Reset();

  // Patch kernel IEC routines (copied from C64_Amiga.i
  orig_kernal_1d84 = Kernal[0x1d84];
  orig_kernal_1d85 = Kernal[0x1d85];
  PatchKernal(ThePrefs.FastReset, ThePrefs.Emul1541Proc);

  // Start the emulation
  thread_running = true; quit_thyself = false; have_a_break = false;
  thread_func();
}


void C64::Quit(void)
{
  if (thread_running)
  {
    quit_thyself = true; thread_running = false;
  }
}


void C64::Pause(void)
{
  have_a_break = true; TheSID->PauseSound();
}


void C64::Resume(void)
{
  have_a_break = false; TheSID->ResumeSound();
}


void C64::thread_func(void)
{
  LastPoll = LastFrame = LastSpeed = OS_ReadMonotonicTime(); FramesSince = 0;

  while (!quit_thyself)
  {
#ifdef FRODO_SC
    if (TheVIC->EmulateCycle()) {TheSID->EmulateLine();}
    TheCIA1->EmulateCycle();
    TheCIA2->EmulateCycle();
    TheCPU->EmulateCycle();

    if (ThePrefs.Emul1541Proc)
    {
      TheCPU1541->CountVIATimers(1);
      if (!TheCPU1541->Idle) {TheCPU1541->EmulateCycle();}
    }
    CycleCounter++;

#else
    // Emulate each device one rasterline. Order is important!
    int cycles = TheVIC->EmulateLine();
    TheSID->EmulateLine();
#if !PRECISE_CIA_CYCLES
	TheCIA1->EmulateLine(ThePrefs.CIACycles);
	TheCIA2->EmulateLine(ThePrefs.CIACycles);
#endif

    if (ThePrefs.Emul1541Proc)
    {
      int cycles_1541 = ThePrefs.FloppyCycles;
      TheCPU1541->CountVIATimers(cycles_1541);
      if (!TheCPU1541->Idle)
      {
        while ((cycles >= 0) || (cycles_1541 >= 0))
        {
          if (cycles > cycles_1541) {cycles -= TheCPU->EmulateLine(1);}
          else {cycles_1541 -= TheCPU1541->EmulateLine(1);}
        }
      }
      else {TheCPU->EmulateLine(cycles);}
    }
    else
    {
      TheCPU->EmulateLine(cycles);
    }
#endif

    // Single-tasking: busy-wait 'til unpause
    while (SingleTasking && have_a_break)
    {
      int KeyState;

      TheDisplay->CheckForUnpause(true);	// unpause?
      KeyState = ReadKeyboardStatus();
      if ((KeyState & 2) == 0)		// leave single tasking?
      {
        SetMousePointer(lastptr); OS_FlushBuffer(9); SingleTasking = false;
      }
      laststate = KeyState;
    }
    if (!SingleTasking)
    {
      // The system-specific part of this function
      if (Poll || have_a_break)
      {
        TheWIMP->Poll(have_a_break);
        LastPoll = LastFrame = OS_ReadMonotonicTime(); Poll = false;
      }
    }
  }
}
