/*
 *  Display_Acorn.h - C64 graphics display, emulator window handling,
 *                    RISC OS specific stuff
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


#include "C64.h"
#include "ROlib.h"
#include "AcornGUI.h"
#include "SAM.h"
#include "VIC.h"



// (from Display_x.i)
/*
  C64 keyboard matrix:

    Bit 7   6   5   4   3   2   1   0
  0    CUD  F5  F3  F1  F7 CLR RET DEL
  1    SHL  E   S   Z   4   A   W   3
  2     X   T   F   C   6   D   R   5
  3     V   U   H   B   8   G   Y   7
  4     N   O   K   M   0   J   I   9
  5     ,   @   :   .   -   L   P   +
  6     /   ^   =  SHR HOM  ;   *   £
  7    R/S  Q   C= SPC  2  CTL  <-  1
*/


#define IntKey_MinCode   3	// Scan from ShiftLeft (leave out Shift, Ctrl, Alt)
#define IntKey_MaxCode   124
#define IntKey_Copy	105

// Maps internal keyboard numbers (Acorn) to C64 keyboard-matrix.
// Format: top nibble - row#, bottom nibble - column (bit#).
// Entry == 0xff <==> don't map
char KeysAcornToCBM[] = {
  0x17, 0x72, 0x75, 0x17,	// 0  -  3: SHL, CTRL, ALT(C=), SHL
  0x72, 0x75, 0x64, 0x72,	// 4  -  7: CTRL, ALT, SHR, CTRL
  0x75, 0xff, 0xff, 0xff,	// 8  - 11: ALT, MouseSlct, MouseMen, MouseAdj
  0xff, 0xff, 0xff, 0xff,	// 12 - 15: dummies
  0x76, 0x10, 0x13, 0x20,	// 16 - 19: q, 3,4,5
  0x03, 0x33, 0xff, 0x53,	// 20 - 23: F4(F7), 8, F7, -
  0x23, 0x02, 0xff, 0xff,	// 24 - 27: 6, crsrL, num6, num7
  0xff, 0xff, 0xff, 0xff,	// 28 - 31: F11, F12, F10, ScrLock
  0xff, 0x11, 0x16, 0x26,	// 32 - 35: Print, w, e, t
  0x30, 0x41, 0x40, 0x43,	// 36 - 39: 7, i, 9, 0
  0x53, 0x07, 0xff, 0xff,	// 40 - 43: -, crsrD, num8, num9
  0x77, 0x71, 0x60, 0x00,	// 44 - 47: break, `, £, DEL
  0x70, 0x73, 0x22, 0x21,	// 48 - 51: 1, 2, d, r
  0x23, 0x36, 0x46, 0x51,	// 52 - 55: 6, u, o, p
  0x56, 0x07, 0x50, 0x53,	// 56 - 59: [(@), crsrU, num+(+), num-(-)
  0xff, 0x00, 0x63, 0xff,	// 60 - 63: numENTER, insert, home, pgUp
  0x17, 0x12, 0x27, 0x25,	// 64 - 67: capsLCK, a, x, f
  0x31, 0x42, 0x45, 0x73,	// 68 - 71: y, j, k, 2
  0x55, 0x01, 0xff, 0xff,	// 72 - 75: ;(:), RET, num/, dummy
  0xff, 0xff, 0xff, 0x62,	// 76 - 79: num., numLCK, pgDown, '(;)
  0xff, 0x15, 0x24, 0x32,	// 80 - 83: dummy, s, c, g
  0x35, 0x47, 0x52, 0x55,	// 84 - 87: h, n, l, ;(:)
  0x61, 0x00, 0xff, 0xff,	// 88 - 91: ](*), Delete, num#, num*
  0xff, 0x65, 0xff, 0xff,	// 92 - 95: dummy, =, dummies
  0x72, 0x14, 0x74, 0x37,	// 96 - 99: TAB(CTRL), z, SPACE, v
  0x34, 0x44, 0x57, 0x54,	// 100-103: b, m, ',', .
  0x67, 0xff, 0xff, 0xff,	// 104-107: /, Copy, num0, num1
  0xff, 0xff, 0xff, 0xff,	// 108-111: num3, dummies
  0x77, 0x04, 0x05, 0x06,	// 112-115: ESC, F1(F1), F2(F3), F3(F5)
  0xff, 0xff, 0xff, 0xff,	// 116-119: F5, F6, F8, F9
  0x66, 0x02, 0xff, 0xff,	// 120-123: \(^), crsrR, num4, num5
  0xff, 0xff, 0xff, 0xff	// 124-127: num2, dummies
};


// Special keycodes that have to be processed seperately:
#define IntKey_CrsrL	25
#define IntKey_CrsrR	121
#define IntKey_CrsrU	57
#define IntKey_CrsrD	41
#define IntKey_Insert	61
#define IntKey_NumLock	77
#define IntKey_F5	116
#define IntKey_F6	117
#define IntKey_F7	22
#define IntKey_F8	118
#define IntKey_PageUp	63
#define IntKey_PageDown	78
#define IntKey_NumSlash	74
#define IntKey_NumStar	91
#define IntKey_NumCross	90

#define KeyJoy1_Up	108	// num3
#define KeyJoy1_Down	76	// num.
#define KeyJoy1_Left	107	// num1
#define KeyJoy1_Right	124	// num2
#define KeyJoy1_Fire	60	// numReturn
#define KeyJoy2_Up	67	// "f"
#define KeyJoy2_Down	82	// "c"
#define KeyJoy2_Left	97	// "z"
#define KeyJoy2_Right	66	// "x"
#define KeyJoy2_Fire	83	// "g"




C64Display::C64Display(C64 *the_c64) : TheC64(the_c64)
{
  int i;

  bitmap = new uint8[DISPLAY_X * DISPLAY_Y];
  screen = new ROScreen();
  ModeChange();
  for (i=0; i<8; i++) {lastkeys[i] = 0;}
  // First joystick: mapped to port 2 if numLOCK is on, else port 2
  JoystickKeys[0].up   = KeyJoy1_Up;   JoystickKeys[0].down  = KeyJoy1_Down;
  JoystickKeys[0].left = KeyJoy1_Left; JoystickKeys[0].right = KeyJoy1_Right;
  JoystickKeys[0].fire = KeyJoy1_Fire;
  // Second joystick: only active if numLOCK is off! Mapped to port 2 then.
  JoystickKeys[1].up   = KeyJoy2_Up;   JoystickKeys[1].down  = KeyJoy2_Down;
  JoystickKeys[1].left = KeyJoy2_Left; JoystickKeys[1].right = KeyJoy2_Right;
  JoystickKeys[1].fire = KeyJoy2_Fire;
}


C64Display::~C64Display(void)
{
  delete bitmap; delete screen;
}


void C64Display::ModeChange(void)
{
  register int i;

  screen->ReadMode();
  // find best matching colours in current mode.
  switch (screen->ldbpp)
  {
    case 0:
    case 1:
    case 2:
    case 3: for (i=0; i<16; i++)	// for 1,2,4 and 8bpp
            {
              mode_cols[i] = ModeColourNumber((palette_blue[i] << 24) + (palette_green[i] << 16) + (palette_red[i] << 8));
            }
            break;
    case 4: for (i=0; i<16; i++)	// for 16bpp
            {
              int r,g,b;

              r = (palette_red[i]   + 4) & 0x1f8; if (r > 0xff) {r = 0xf8;}
              g = (palette_green[i] + 4) & 0x1f8; if (g > 0xff) {g = 0xf8;}
              b = (palette_blue[i]  + 4) & 0x1f8; if (b > 0xff) {b = 0xf8;}
              mode_cols[i] = (r >> 3) | (g << 2) | (b << 7);
            }
            break;
    case 5: for (i=0; i<16; i++)	// for 32bpp
            {
              mode_cols[i] = palette_red[i] | (palette_green[i] << 8) | (palette_blue[i] << 16);
            }
            break;
  }
}


uint8 *C64Display::BitmapBase(void)
{
  return bitmap;
}


void C64Display::InitColors(uint8 *colors)
{
  register int i;

  // write index mapping C64colours -> ROcolours
  if (screen->ldbpp <= 3) // at most 8bpp ==> use actual colour
  {
    for (i=0; i<256; i++) {colors[i] = mode_cols[i&15];}
  }
  else // else use index (takes time but can't be changed...
  {
    for (i=0; i<256; i++) {colors[i] = i&15;}
  }
}


int C64Display::BitmapXMod(void)
{
  return DISPLAY_X;
}


// This routine reads the raw keyboard data from the host machine. Not entirely
// conformant with Acorn's rules but the only way to detect multiple simultaneous
// keypresses.
void C64Display::PollKeyboard(uint8 *key_matrix, uint8 *rev_matrix, uint8 *joystick, uint8 *joystick2)
{
  register int scan_from=IntKey_MinCode, code, row, col;
  int status;
  uint8 kjoy, kjoy2;
  uint32 newkeys[8];
  UBYTE kjoy, kjoy2;

  // Clear keyboard
  for (code=0; code<8; code++) {key_matrix[code] = 0xff; rev_matrix[code] = 0xff; newkeys[code] = 0;}
  kjoy = kjoy2 = 0xff;
  status = ReadKeyboardStatus();
  if ((status & 16) == 0) {key_matrix[1] &= 0x7f; rev_matrix[7] &= 0xfd;} // Caps lock

  while (scan_from <= IntKey_MaxCode)
  {
    if ((code = ScanKeys(scan_from)) != 0xff)
    {
      newkeys[code >> 5] |= (1 << (code & 0x1f));	// update keys pressed
      row = KeysAcornToCBM[code];
      if ((status & 4) != 0)	// numLOCK off? ==> check for 2nd keyboard joystick too
      {
        if (code == JoystickKeys[1].up) {kjoy2 &= 0xfe; row = 0xff;}
        else if (code == JoystickKeys[1].down)  {kjoy2 &= 0xfd; row = 0xff;}
        else if (code == JoystickKeys[1].left)  {kjoy2 &= 0xfb; row = 0xff;}
        else if (code == JoystickKeys[1].right) {kjoy2 &= 0xf7; row = 0xff;}
        else if (code == JoystickKeys[1].fire)  {kjoy2 &= 0xef; row = 0xff;}
      }
      // check 1st keyboard joystick
      if (code == JoystickKeys[0].up) {kjoy &= 0xfe; row = 0xff;}
      else if (code == JoystickKeys[0].down)  {kjoy &= 0xfd; row = 0xff;}
      else if (code == JoystickKeys[0].left)  {kjoy &= 0xfb; row = 0xff;}
      else if (code == JoystickKeys[0].right) {kjoy &= 0xf7; row = 0xff;}
      else if (code == JoystickKeys[0].fire)  {kjoy &= 0xef; row = 0xff;}

      // If key not mapped to joystick: try mapping to keyboard
      if (row != 0xff)
      {
      	col = row & 7; row >>= 4;
      	key_matrix[row] &= ~(1<<col); rev_matrix[col] &= ~(1<<row);
      }

      // None of the keys listed below should be used for
      // joystick definitions since they're always used here.
      switch(code)
      {
        // For either of these: additionally set SHIFT key.
        case IntKey_CrsrL:  // already mapped to CrsrL
        case IntKey_CrsrU:  // already mapped to CrsrD
        case IntKey_Insert: // already mapped to DEL
             key_matrix[6] &= (0xff - (1<<4)); rev_matrix[4] &= (0xff - (1<<6));
             break;
        case IntKey_F6:
             if ((status & 2) == 0)	// call SAM only in multitasking mode!
             {
               TheC64->Pause(); SAM(TheC64); TheC64->Resume();
             }
             break;
        case IntKey_F7: TheC64->NMI(); break;
        case IntKey_F8: TheC64->Reset(); break;
        default: break;
      }

      // These shouldn't auto-repeat, therefore I check them seperately.
      if ((lastkeys[code >> 5] & (1 << (code & 0x1f))) == 0)
      {
        // Icons should be updated, not force-redrawed (--> single tasking)
        switch (code)
        {
          // decrease framerate
          case IntKey_PageUp:
               TheC64->TheWIMP->PrefsWindow->
                 WriteIconNumberU(Icon_Prefs_SkipFText,++ThePrefs.SkipFrames);
               break;
          // increase framerate
          case IntKey_PageDown: if (ThePrefs.SkipFrames > 0)
               {
                 TheC64->TheWIMP->PrefsWindow->
                   WriteIconNumberU(Icon_Prefs_SkipFText,--ThePrefs.SkipFrames);
               }
               break;
          // toggle floppy emulation status
          case IntKey_NumSlash:
               {
                 register int eor, i;
                 Prefs *prefs = new Prefs(ThePrefs);

                 // If Emulation active then ungrey icons now, else grey them
                 prefs->Emul1541Proc = !prefs->Emul1541Proc;
                 TheC64->TheWIMP->SetLEDIcons(prefs->Emul1541Proc);
                 TheC64->NewPrefs(prefs);
                 ThePrefs = *prefs;
                 // Show change in prefs window too
                 TheC64->TheWIMP->PrefsWindow->
                   SetIconState(Icon_Prefs_Emul1541,(prefs->Emul1541Proc)?IFlg_Slct:0,IFlg_Slct);
                 delete prefs;
               }
               break;
          // toggle speed limiter
          case IntKey_NumStar:
               ThePrefs.LimitSpeed = !ThePrefs.LimitSpeed;
               TheC64->TheWIMP->SetSpeedLimiter(ThePrefs.LimitSpeed);
               break;
          // toggle sound emulation
          case IntKey_F5:
               {
                 Window *pw = TheC64->TheWIMP->PrefsWindow;
                 int i, j;
                 Prefs *prefs = new Prefs(ThePrefs);

                 if (prefs->SIDType == SIDTYPE_NONE) {prefs->SIDType = SIDTYPE_DIGITAL; i = 1;}
                 else {prefs->SIDType = SIDTYPE_NONE; i = 0;}
                 for (j=0; j<3; j++)
                 {
                   pw->SetIconState(SIDtoIcon[j], (j==i) ? IFlg_Slct : 0, IFlg_Slct);
                 }
                 TheC64->TheWIMP->SoundWindow->
                   SetIconState(Icon_Sound_Notes, (i==0) ? IFlg_Grey : 0, IFlg_Grey);
                 TheC64->NewPrefs(prefs);
                 ThePrefs = *prefs;
                 delete prefs;
               }
               break;
          case IntKey_Copy: TheC64->Pause();
               TheC64->TheWIMP->EmuPane->WriteIconTextU(Icon_Pane_Pause,PANE_TEXT_RESUME); break;
          default: break;
        }
      }
    }
    scan_from = code+1;
  }
  for (code=0; code<8; code++) {lastkeys[code] = newkeys[code];}
  *joystick = kjoy; *joystick2 = kjoy2;
}


bool C64Display::NumLock(void)
{
  return(((ReadKeyboardStatus() & 4) == 0) ? true : false);
}


/*
 *  Prefs may have changed
 */

void C64Display::NewPrefs(Prefs *prefs)
{
}


void C64Display::Update(void)
{
  int i, state;
  int *ic;

  // Do a redraw of the emulator window
  TheC64->TheWIMP->UpdateEmuWindow();

  // Update the LEDs if necessary
  for (i=0; i<4; i++)
  {
    if ((state = led_state[i]) != old_led_state[i])
    {
      ic = (int*)TheC64->TheWIMP->EmuPane->GetIcon(LEDtoIcon[i]);
      switch(state)
      {
        case LED_OFF:
        case LED_ERROR_OFF:
             sprintf((char*)ic[5],"led_off"); break;
        case LED_ON:
             sprintf((char*)ic[5],"led_on"); break;
        case LED_ERROR_ON:
             sprintf((char*)ic[5],"led_error"); break;
      }
      TheC64->TheWIMP->EmuPane->UpdateIcon(LEDtoIcon[i]);	// update, not force-redraw!
      old_led_state[i] = state;
    }
  }
}


unsigned int *C64Display::GetColourTable(void)
{
  return (mode_cols);
}


// Check whether unpause-key (copy) is pressed
bool C64Display::CheckForUnpause(bool CheckLastState)
{
  int scan_from = IntKey_MinCode, code;
  uint32 newkeys[8];
  uint32 lastpause;

  for (code=0; code<8; code++) {newkeys[code] = 0;}

  while (scan_from <= IntKey_MaxCode)
  {
    if ((code = ScanKeys(scan_from)) != 0xff)
    {
      newkeys[code >> 5] |= (1 << (code & 0x1f));
    }
    scan_from = code+1;
  }
  lastpause = lastkeys[IntKey_Copy >> 5] & (1 << (IntKey_Copy & 0x1f));
  for (code=0; code<8; code++) {lastkeys[code] = newkeys[code];}
  // unpause-key pressed?
  if ((newkeys[IntKey_Copy >> 5] & (1 << (IntKey_Copy & 0x1f))) != 0)
  {
    if ((lastpause == 0) || !CheckLastState)
    {
      TheC64->Resume();
      TheC64->TheWIMP->EmuPane->WriteIconTextU(Icon_Pane_Pause,PANE_TEXT_PAUSE);
      return(true);
    }
  }
  return(false);
}


// Requester dialogue box
long ShowRequester(char *str, char *button1, char *button2)
{
  _kernel_oserror myerr;

  myerr.errnum = 0x0; strcpy(myerr.errmess,str);
  Wimp_ReportError(&myerr,1,TASKNAME); // always provide an OK box
  return(1);
}
