/*
 *  Display_svga.h - C64 graphics display, emulator window handling,
 *                   SVGAlib specific stuff
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

#include <vga.h>
#include <vgamouse.h>
#include <vgakeyboard.h>

#include "C64.h"


#define SCODE_CURSORBLOCKUP 103		/* Cursor key block. */
#define SCODE_CURSORBLOCKLEFT 105
#define SCODE_CURSORBLOCKRIGHT 106
#define SCODE_CURSORBLOCKDOWN 108

#define SCODE_INSERT 110
#define SCODE_HOME 102
#define SCODE_PGUP 104
#define SCODE_DELETE 111
#define SCODE_END 107
#define SCODE_PGDN 109

#define SCODE_NUMLOCK 69

#define SCODE_KEYPAD0  82
#define SCODE_KEYPAD1  79
#define SCODE_KEYPAD2  80
#define SCODE_KEYPAD3  81
#define SCODE_KEYPAD4  75
#define SCODE_KEYPAD5  76
#define SCODE_KEYPAD6  77
#define SCODE_KEYPAD7  71
#define SCODE_KEYPAD8  72
#define SCODE_KEYPAD9  73
#define SCODE_KEYPADENTER      96
#define SCODE_KEYPADPLUS       78
#define SCODE_KEYPADMINUS      74
#define SCODE_KEYPADMULTIPLY   55
#define SCODE_KEYPADDIVIDE     98

#define SCODE_Q                16
#define SCODE_W                17
#define SCODE_E                18
#define SCODE_R                19
#define SCODE_T                20
#define SCODE_Y                21
#define SCODE_U                22
#define SCODE_I                23
#define SCODE_O                24
#define SCODE_P                25

#define SCODE_A                30
#define SCODE_S                31
#define SCODE_D                32
#define SCODE_F                33
#define SCODE_G                34
#define SCODE_H                35
#define SCODE_J                36
#define SCODE_K                37
#define SCODE_L                38

#define SCODE_Z                44
#define SCODE_X                45
#define SCODE_C                46
#define SCODE_V                47
#define SCODE_B                48
#define SCODE_N                49
#define SCODE_M                50

#define SCODE_ESCAPE           1
#define SCODE_ENTER            28
#define SCODE_RIGHTCONTROL     97
#define SCODE_CONTROL  97
#define SCODE_RIGHTALT 100
#define SCODE_LEFTCONTROL      29
#define SCODE_LEFTALT  56
#define SCODE_SPACE            57

#define SCODE_F1               59
#define SCODE_F2               60
#define SCODE_F3               61
#define SCODE_F4               62
#define SCODE_F5               63
#define SCODE_F6               64
#define SCODE_F7               65
#define SCODE_F8               66
#define SCODE_F9               67
#define SCODE_F10              68

#define SCODE_0 11
#define SCODE_1 2
#define SCODE_2 3
#define SCODE_3 4
#define SCODE_4 5 
#define SCODE_5 6
#define SCODE_6 7 
#define SCODE_7 8
#define SCODE_8 9
#define SCODE_9 10

#define SCODE_LEFTSHIFT 42
#define SCODE_RIGHTSHIFT 54
#define SCODE_TAB 15

#define SCODE_F11 87
#define SCODE_F12 88
#define SCODE_NEXT 81
#define SCODE_PRIOR 73
#define SCODE_BS 14

#define SCODE_asciicircum 41
#define SCODE_bracketleft 26
#define SCODE_bracketright 27
#define SCODE_comma 51
#define SCODE_period 52
#define SCODE_slash 53
#define SCODE_semicolon 39
#define SCODE_grave 40
#define SCODE_minus 12
#define SCODE_equal 13
#define SCODE_numbersign 43
#define SCODE_ltgt 86
#define SCODE_scrolllock 70

static int bitdepth;
static char *bufmem;
static int hsize;
static vga_modeinfo modeinfo;
static char *linear_mem;

static int keystate[256];
static int f11pressed = 0, f12pressed = 0, quit = 0;
static int joystate = 0xFF;
static int numlock = 0;
static uint8 rev_matrix[8], key_matrix[8];

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
#define MATRIX(a,b) (((a) << 3) | (b))
#define KEY_F10 512
#define KEY_F11 513
#define KEY_F12 514

#define KEY_FIRE 515
#define KEY_JUP 516
#define KEY_JDN 517
#define KEY_JLF 518
#define KEY_JRT 519

#define KEY_NUMLOCK 520

#define KEY_KPPLUS 521
#define KEY_KPMINUS 522
#define KEY_KPMULT 523
#define KEY_KPDIV 524

static int scode2c64(int scancode)
{
	switch (scancode) {
	case SCODE_asciicircum: return MATRIX(7,1);
	case SCODE_KEYPAD0: return KEY_FIRE;
	case SCODE_KEYPAD1: return -1;
	case SCODE_KEYPAD2: return KEY_JDN;
	case SCODE_KEYPAD3: return -1;
	case SCODE_KEYPAD4: return KEY_JLF;
	case SCODE_KEYPAD5: return -1;
	case SCODE_KEYPAD6: return KEY_JRT;
	case SCODE_KEYPAD7: return -1;
	case SCODE_KEYPAD8: return KEY_JUP;
	case SCODE_KEYPAD9: return -1;
		
	case SCODE_NUMLOCK: return KEY_NUMLOCK;
	case SCODE_KEYPADMULTIPLY: return KEY_KPMULT;
	case SCODE_KEYPADDIVIDE: return KEY_KPDIV;
	case SCODE_KEYPADMINUS: return KEY_KPMINUS;
	case SCODE_KEYPADPLUS: return KEY_KPPLUS;
	case SCODE_KEYPADENTER: return MATRIX(0,1);
               
	case SCODE_F10: return KEY_F10;
	case SCODE_F11: return KEY_F11;
	case SCODE_F12: return KEY_F12;
		
	case SCODE_comma: return MATRIX(5,7);
	case SCODE_period: return MATRIX(5,4);
		
	case SCODE_A: return MATRIX(1,2);
	case SCODE_B: return MATRIX(3,4);
	case SCODE_C: return MATRIX(2,4);
	case SCODE_D: return MATRIX(2,2);
	case SCODE_E: return MATRIX(1,6);
	case SCODE_F: return MATRIX(2,5);
	case SCODE_G: return MATRIX(3,2);
	case SCODE_H: return MATRIX(3,5);
	case SCODE_I: return MATRIX(4,1);
	case SCODE_J: return MATRIX(4,2);
	case SCODE_K: return MATRIX(4,5);
	case SCODE_L: return MATRIX(5,2);
	case SCODE_M: return MATRIX(4,4);
	case SCODE_N: return MATRIX(4,7);
	case SCODE_O: return MATRIX(4,6);
	case SCODE_P: return MATRIX(5,1);
	case SCODE_Q: return MATRIX(7,6);
	case SCODE_R: return MATRIX(2,1);
	case SCODE_S: return MATRIX(1,5);
	case SCODE_T: return MATRIX(2,6);
	case SCODE_U: return MATRIX(3,6);
	case SCODE_V: return MATRIX(3,7);
	case SCODE_W: return MATRIX(1,1);
	case SCODE_X: return MATRIX(2,7);
	case SCODE_Y: return MATRIX(3,1);
	case SCODE_Z: return MATRIX(1,4);
		
	case SCODE_BS: return MATRIX(0,0);
	case SCODE_DELETE: return MATRIX(0,0);
	case SCODE_LEFTCONTROL: return MATRIX(7,2);
	case SCODE_TAB: return MATRIX(7,1);
	case SCODE_ENTER: return MATRIX(0,1);
	case SCODE_SPACE: return MATRIX(7,4);
	case SCODE_LEFTSHIFT: return MATRIX(1,7);
	case SCODE_RIGHTSHIFT: return MATRIX(6,4);
	case SCODE_ESCAPE: return MATRIX(7,7);
	case SCODE_RIGHTCONTROL:
	case SCODE_LEFTALT:
	case SCODE_RIGHTALT: return MATRIX(7,5);

	case SCODE_INSERT: return MATRIX(0,0) | 0x80;
	case SCODE_HOME: return MATRIX(6,3);
	case SCODE_END: return MATRIX(6,0);
	case SCODE_PGUP: return MATRIX(6,6);
	case SCODE_PGDN: return MATRIX(6,5);
		
	case SCODE_CURSORBLOCKUP: return MATRIX(0,7)| 0x80;
	case SCODE_CURSORBLOCKDOWN: return MATRIX(0,7);
	case SCODE_CURSORBLOCKLEFT: return MATRIX(0,2) | 0x80;
	case SCODE_CURSORBLOCKRIGHT: return MATRIX(0,2);
		
	case SCODE_F1: return MATRIX(0,4);
	case SCODE_F2: return MATRIX(0,4) | 0x80;
	case SCODE_F3: return MATRIX(0,5);
	case SCODE_F4: return MATRIX(0,5) | 0x80;
	case SCODE_F5: return MATRIX(0,6);
	case SCODE_F6: return MATRIX(0,6) | 0x80;
	case SCODE_F7: return MATRIX(0,3);
	case SCODE_F8: return MATRIX(0,3) | 0x80;
		
	case SCODE_0: return MATRIX(4,3);
	case SCODE_1: return MATRIX(7,0);
	case SCODE_2: return MATRIX(7,3);
	case SCODE_3: return MATRIX(1,0);
	case SCODE_4: return MATRIX(1,3);
	case SCODE_5: return MATRIX(2,0);
	case SCODE_6: return MATRIX(2,3);
	case SCODE_7: return MATRIX(3,0);
	case SCODE_8: return MATRIX(3,3);
	case SCODE_9: return MATRIX(4,0);

	case SCODE_bracketleft: return MATRIX(5,6);
	case SCODE_bracketright: return MATRIX(6,1);
	case SCODE_slash: return MATRIX(6,7);
	case SCODE_semicolon: return MATRIX(5,5);
	case SCODE_grave: return MATRIX(6,2);
	case SCODE_numbersign: return MATRIX(6,5);
	case SCODE_ltgt: return MATRIX(6,6);
	case SCODE_minus: return MATRIX(5,0);
	case SCODE_equal: return MATRIX(5,3);
	}
}

static void my_kbd_handler(int scancode, int newstate)
{
	int kc = scode2c64(scancode);
#if 0
	if (kc == -1) {
		printf("%d\n",kc);
		return;
	}
#endif
	if (newstate == KEY_EVENTPRESS) {
		switch (kc) {
		case KEY_KPPLUS:
			if (ThePrefs.SkipFrames < 10)
				ThePrefs.SkipFrames++;
			break;

		case KEY_KPMINUS:
			if (ThePrefs.SkipFrames > 1)
				ThePrefs.SkipFrames--;
			break;

		case KEY_KPMULT:
			ThePrefs.LimitSpeed = !ThePrefs.LimitSpeed;
			break;

		case KEY_KPDIV:
			ThePrefs.JoystickSwap = !ThePrefs.JoystickSwap;
			break;

		case KEY_NUMLOCK:
			numlock = !numlock;
			break;
			
		case KEY_F10:
			quit = 1;
			break;
                               
		case KEY_F11:
			f11pressed = 1;
			break;
			
		case KEY_F12:
			f12pressed = 1;
			break;
			
		case KEY_FIRE:
			joystate &= ~0x10;
			break;
			
		case KEY_JDN:
			joystate &= ~0x2;
			break;
			
		case KEY_JUP:
			joystate &= ~0x1;
			break;
			
		case KEY_JLF:
			joystate &= ~0x4;
			break;
			
		case KEY_JRT:
			joystate &= ~0x8;
			break;
			
		default:
			if (keystate[kc])
				break;
			keystate[kc] = 1;
			int c64_byte, c64_bit, shifted;
			c64_byte = kc >> 3;
			c64_bit = kc & 7;
			shifted = kc & 128;
			c64_byte &= 7;
			if (shifted) {
				key_matrix[6] &= 0xef;
				rev_matrix[4] &= 0xbf;
			}
			key_matrix[c64_byte] &= ~(1 << c64_bit);
			rev_matrix[c64_bit] &= ~(1 << c64_byte);
			break;
		}
	} else {
		switch (kc) {			
		case KEY_FIRE:
			joystate |= 0x10;
			break;
			
		case KEY_JDN:
			joystate |= 0x2;
			break;
			
		case KEY_JUP:
			joystate |= 0x1;
			break;
			
		case KEY_JLF:
			joystate |= 0x4;
			break;
			
		case KEY_JRT:
			joystate |= 0x8;
			break;
			
		default:
			if (!keystate[kc])
				break;
			keystate[kc] = 0;
			int c64_byte, c64_bit, shifted;
			c64_byte = kc >> 3;
			c64_bit = kc & 7;
			shifted = kc & 128;
			c64_byte &= 7;
			if (shifted) {
				key_matrix[6] |= 0x10;
				rev_matrix[4] |= 0x40;
			}
			key_matrix[c64_byte] |= (1 << c64_bit);
			rev_matrix[c64_bit] |= (1 << c64_byte);
			break;
		}
		
	}
}


C64Display::C64Display(C64 *the_c64) : TheC64(the_c64)
{
    quit_requested = false;
}


C64Display::~C64Display()
{
    sleep(1);
    vga_setmode(TEXT);
}


/*
 *  Prefs may have changed
 */

void C64Display::NewPrefs(Prefs *prefs)
{
}


void C64Display::Speedometer(int speed)
{
}


int init_graphics(void)
{      
    int vgamode = G640x480x256;
    modeinfo = *vga_getmodeinfo (vgamode);

    if (vga_setmode(vgamode) < 0) {
	sleep(1);
	vga_setmode(TEXT);
	LOGE( "SVGAlib doesn't like my video mode. Giving up.\n");
	return 0;
    }

    hsize = modeinfo.linewidth;
    if (hsize < DISPLAY_X)
	hsize = DISPLAY_X;

	bufmem = NULL;
    if ((modeinfo.flags & CAPABLE_LINEAR) && modeinfo.linewidth >= DISPLAY_X) {
	if (vga_setlinearaddressing() != -1) {
	    linear_mem = (char *)vga_getgraphmem();
	    printf("Using linear addressing: %p.\n", linear_mem);
	    bufmem = linear_mem;
	}
    }
    if (bufmem == NULL)
	bufmem = (char *)malloc(hsize * DISPLAY_Y);

    if (keyboard_init() != 0)
	abort();
    keyboard_seteventhandler(my_kbd_handler);
    /*     keyboard_translatekeys(DONT_CATCH_CTRLC);*/

    memset(keystate, 0, sizeof(keystate));
    memset(key_matrix, 0xFF, 8);
    memset(rev_matrix, 0xFF, 8);
    return 1;
}


void C64Display::Update(void)
{
    int y;

    if (linear_mem)
	return;
    
    for (y = 0; y < DISPLAY_Y; y++) {
	vga_drawscanline(y, (uint8 *)bufmem + hsize * y);
    }
}


uint8 *C64Display::BitmapBase(void)
{
       return (uint8 *)bufmem;
}


int C64Display::BitmapXMod(void)
{
       return hsize;
}


void C64Display::PollKeyboard(uint8 *CIA_key_matrix, uint8 *CIA_rev_matrix, uint8 *joystick)
{
    keyboard_update();
    *joystick = joystate;
    memcpy(CIA_key_matrix, key_matrix, 8);
    memcpy(CIA_rev_matrix, rev_matrix, 8);
    if (f11pressed)
	TheC64->NMI();
    if (f12pressed)
	TheC64->Reset();
    if (quit)
	quit_requested = true;
    f11pressed = f12pressed = 0;
}


/*
 *  Check if NumLock is down (for switching the joystick keyboard emulation)
 */

bool C64Display::NumLock(void)
{
       return numlock;
}


/*
 *  Allocate C64 colors
 */

static int colorval(int v)
{
       return ((v & 255)*0x01010101) >> 26;
}

void C64Display::InitColors(uint8 *colors)
{
       int i;

       for (i=0; i< 256; i++) {
               vga_setpalette(i, colorval(palette_red[i & 0x0f]), colorval(palette_green[i & 0x0f]), colorval(palette_blue[i & 0x0f]));
               colors[i] = i;
       }
}


/*
 *  Show a requester (error message)
 */

long int ShowRequester(char *a,char *b,char *)
{
       printf("%s: %s\n", a, b);
       return 1;
}
