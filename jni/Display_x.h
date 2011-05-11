/*
 *  Display_x.h - C64 graphics display, emulator window handling,
 *                X specific stuff
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

#include "SAM.h"
#include "C64.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#if defined(X_USE_SHM)
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
static XShmSegmentInfo shminfo;
#endif

static Display *display;
static int screen;
static Window rootwin, mywin;

static GC black_gc, led_gc;
static XColor black, fill_gray, shine_gray, shadow_gray, red, green;
static Colormap cmap;
static Font led_font;

static XImage *img;
static Visual *vis;
static XVisualInfo visualInfo;
static int bitdepth;
static char *bufmem;
static int hsize;

// For LED error blinking
static C64Display *c64_disp;
static struct sigaction pulse_sa;
static itimerval pulse_tv;

// Keyboard and joystick
static int keystate[256];
static int joystate = 0xFF;
static bool num_locked = false;

static const long int eventmask = (KeyPressMask|KeyReleaseMask|FocusChangeMask|ExposureMask);


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

#define KEY_F9 512
#define KEY_F10 513
#define KEY_F11 514
#define KEY_F12 515

#ifdef SUN
#define KEY_FIRE 58
#define KEY_JU 135
#define KEY_JD 7
#define KEY_JL 130
#define KEY_JR 2
#else
#define KEY_FIRE 516
#define KEY_JU 517
#define KEY_JD 518
#define KEY_JL 519
#define KEY_JR 520
#endif

#define KEY_JUL 521
#define KEY_JUR 522
#define KEY_JDL 523
#define KEY_JDR 524

#define KEY_KP_PLUS 525
#define KEY_KP_MINUS 526
#define KEY_KP_MULT 527
#define KEY_NUM_LOCK 528


/*
 *  Decode KeySyms. This function knows about all keys that
 *  are common between different keyboard languages.
 */

static int kc_decode(KeySym ks)
{
	switch (ks) {      
		case XK_A: case XK_a: return MATRIX(1,2);
		case XK_B: case XK_b: return MATRIX(3,4);
		case XK_C: case XK_c: return MATRIX(2,4);
		case XK_D: case XK_d: return MATRIX(2,2);
		case XK_E: case XK_e: return MATRIX(1,6);
		case XK_F: case XK_f: return MATRIX(2,5);
		case XK_G: case XK_g: return MATRIX(3,2);
		case XK_H: case XK_h: return MATRIX(3,5);
		case XK_I: case XK_i: return MATRIX(4,1);
		case XK_J: case XK_j: return MATRIX(4,2);
		case XK_K: case XK_k: return MATRIX(4,5);
		case XK_L: case XK_l: return MATRIX(5,2);
		case XK_M: case XK_m: return MATRIX(4,4);
		case XK_N: case XK_n: return MATRIX(4,7);
		case XK_O: case XK_o: return MATRIX(4,6);
		case XK_P: case XK_p: return MATRIX(5,1);
		case XK_Q: case XK_q: return MATRIX(7,6);
		case XK_R: case XK_r: return MATRIX(2,1);
		case XK_S: case XK_s: return MATRIX(1,5);
		case XK_T: case XK_t: return MATRIX(2,6);
		case XK_U: case XK_u: return MATRIX(3,6);
		case XK_V: case XK_v: return MATRIX(3,7);
		case XK_W: case XK_w: return MATRIX(1,1);
		case XK_X: case XK_x: return MATRIX(2,7);
		case XK_Y: case XK_y: return MATRIX(3,1);
		case XK_Z: case XK_z: return MATRIX(1,4);

		case XK_0: return MATRIX(4,3);
		case XK_1: return MATRIX(7,0);
		case XK_2: return MATRIX(7,3);
		case XK_3: return MATRIX(1,0);
		case XK_4: return MATRIX(1,3);
		case XK_5: return MATRIX(2,0);
		case XK_6: return MATRIX(2,3);
		case XK_7: return MATRIX(3,0);
		case XK_8: return MATRIX(3,3);
		case XK_9: return MATRIX(4,0);

		case XK_space: return MATRIX(7,4);
		case XK_grave: return MATRIX(7,1);
		case XK_backslash: return MATRIX(6,6);
		case XK_comma: return MATRIX(5,7);
		case XK_period: return MATRIX(5,4);

		case XK_Escape: return MATRIX(7,7);
		case XK_Return: return MATRIX(0,1);
		case XK_BackSpace: case XK_Delete: return MATRIX(0,0);
		case XK_Insert: return MATRIX(6,3);
		case XK_Home: case XK_Help: return MATRIX(6,3);
		case XK_End: return MATRIX(6,0);
#ifdef __hpux
		case XK_Prior: return MATRIX(6,0);
		case XK_Next: return MATRIX(6,5);
#else
		case XK_Page_Up: return MATRIX(6,0);
		case XK_Page_Down: return MATRIX(6,5);
#endif
		case XK_Control_L: return MATRIX(7,2);
		case XK_Control_R: return MATRIX(7,5);
		case XK_Shift_L: return MATRIX(1,7);
		case XK_Shift_R: return MATRIX(6,4);
		case XK_Alt_L: return MATRIX(7,5);
		case XK_Alt_R: return MATRIX(7,5);

		case XK_Up: return MATRIX(0,7)| 0x80;
		case XK_Down: return MATRIX(0,7);
		case XK_Left: return MATRIX(0,2) | 0x80;
		case XK_Right: return MATRIX(0,2);

		case XK_F1: return MATRIX(0,4);
		case XK_F2: return MATRIX(0,4) | 0x80;
		case XK_F3: return MATRIX(0,5);
		case XK_F4: return MATRIX(0,5) | 0x80;
		case XK_F5: return MATRIX(0,6);
		case XK_F6: return MATRIX(0,6) | 0x80;
		case XK_F7: return MATRIX(0,3);
		case XK_F8: return MATRIX(0,3) | 0x80;

		case XK_F9: return KEY_F9;
		case XK_F10: return KEY_F10;
		case XK_F11: return KEY_F11;
		case XK_F12: return KEY_F12;

		/* You never know which Keysyms might be missing on some workstation
		 * This #ifdef should be enough. */
#if defined(XK_KP_Prior) && defined(XK_KP_Left) && defined(XK_KP_Insert) && defined (XK_KP_End)
		case XK_KP_0: case XK_KP_Insert: return KEY_FIRE;
		case XK_KP_1: case XK_KP_End: return KEY_JDL;
		case XK_KP_2: case XK_KP_Down: return KEY_JD;
		case XK_KP_3: case XK_KP_Next: return KEY_JDR;
		case XK_KP_4: case XK_KP_Left: return KEY_JL;
		case XK_KP_5: case XK_KP_Begin: return KEY_FIRE;
		case XK_KP_6: case XK_KP_Right: return KEY_JR;
		case XK_KP_7: case XK_KP_Home: return KEY_JUL;
		case XK_KP_8: case XK_KP_Up: return KEY_JU;
		case XK_KP_9: case XK_KP_Prior: return KEY_JUR;
#else
		case XK_KP_0: return KEY_FIRE;
		case XK_KP_1: return KEY_JDL;
		case XK_KP_2: return KEY_JD;
		case XK_KP_3: return KEY_JDR;
		case XK_KP_4: return KEY_JL;
		case XK_KP_5: return KEY_FIRE;
		case XK_KP_6: return KEY_JR;
		case XK_KP_7: return KEY_JUL;
		case XK_KP_8: return KEY_JU;
		case XK_KP_9: return KEY_JUR;
#endif

		case XK_KP_Add: return KEY_KP_PLUS;
		case XK_KP_Subtract: return KEY_KP_MINUS;
		case XK_KP_Multiply: return KEY_KP_MULT;
		case XK_KP_Divide: return MATRIX(6,7);
		case XK_KP_Enter: return MATRIX(0,1);

#ifdef SUN
		case XK_Num_Lock: return KEY_NUM_LOCK;
#endif
	}
	return -1;
}

static int decode_us(KeySym ks)
{
	switch(ks) {	/* US specific */       
		case XK_minus: return MATRIX(5,0);
		case XK_equal: return MATRIX(5,3);
		case XK_bracketleft: return MATRIX(5,6);
		case XK_bracketright: return MATRIX(6,1);
		case XK_semicolon: return MATRIX(5,5);
		case XK_apostrophe: return MATRIX(6,2);
		case XK_slash: return MATRIX(6,7);
	}

	return -1;
}

static int decode_de(KeySym ks)
{
	switch(ks) {	/* DE specific */
		case XK_ssharp: return MATRIX(5,0);
		case XK_apostrophe: return MATRIX(5,3);
		case XK_Udiaeresis: case XK_udiaeresis: return MATRIX(5,6);
		case XK_plus: return MATRIX(6,1);
		case XK_Odiaeresis: case XK_odiaeresis: return MATRIX(5,5);
		case XK_Adiaeresis: case XK_adiaeresis: return MATRIX(6,2);
		case XK_numbersign: return MATRIX(6,5);
		case XK_less: case XK_greater: return MATRIX(6,0);
		case XK_minus: return MATRIX(6,7);
	}

	return -1;
}

static int keycode2c64(XKeyEvent *event)
{
	KeySym ks;
	int as;
	int index = 0;

	do {
		ks = XLookupKeysym(event, index);
		as = kc_decode(ks);
       
		if (as == -1)
			as = KBD_LANG == 0 ? decode_us(ks) : decode_de(ks);
		if (as != -1)
			return as;
		index++;
	} while (ks != NoSymbol);

	return -1;
}


/*
 *  Display constructor: Draw Speedometer/LEDs in window
 */

C64Display::C64Display(C64 *the_c64) : TheC64(the_c64)
{
	int i;
	char str[16];

	quit_requested = false;

	// LEDs off
	for (i=0; i<4; i++)
		led_state[i] = old_led_state[i] = LED_OFF;

	// Draw speedometer/LEDs
	led_gc = XCreateGC(display, mywin, 0, 0);
	XSetFont(display, led_gc, led_font);

	XSetForeground(display, led_gc, fill_gray.pixel);
	XFillRectangle(display, mywin, led_gc, 0, DISPLAY_Y, DISPLAY_X-1, 16);

	XSetForeground(display, led_gc, shine_gray.pixel);
	XDrawLine(display, mywin, led_gc, 0, DISPLAY_Y, DISPLAY_X-1, DISPLAY_Y);
	for (i=0; i<5; i++)
		XDrawLine(display, mywin, led_gc, DISPLAY_X*i/5, DISPLAY_Y, DISPLAY_X*i/5, DISPLAY_Y+14);
	for (i=2; i<6; i++) {
		XDrawLine(display, mywin, led_gc, DISPLAY_X*i/5-23, DISPLAY_Y+11, DISPLAY_X*i/5-9, DISPLAY_Y+11);
		XDrawLine(display, mywin, led_gc, DISPLAY_X*i/5-9, DISPLAY_Y+11, DISPLAY_X*i/5-9, DISPLAY_Y+5);
	}

	XSetForeground(display, led_gc, shadow_gray.pixel);
	XDrawLine(display, mywin, led_gc, 0, DISPLAY_Y+15, DISPLAY_X-1, DISPLAY_Y+15);
	for (i=1; i<6; i++)
		XDrawLine(display, mywin, led_gc, DISPLAY_X*i/5-1, DISPLAY_Y+1, DISPLAY_X*i/5-1, DISPLAY_Y+15);
	for (i=2; i<6; i++) {
		XDrawLine(display, mywin, led_gc, DISPLAY_X*i/5-24, DISPLAY_Y+11, DISPLAY_X*i/5-24, DISPLAY_Y+4);
		XDrawLine(display, mywin, led_gc, DISPLAY_X*i/5-24, DISPLAY_Y+4, DISPLAY_X*i/5-9, DISPLAY_Y+4);
	}

	for (i=0; i<4; i++) {
		sprintf(str, "Drive %d", i+8);
		XSetForeground(display, led_gc, black.pixel);
		XDrawString(display, mywin, led_gc, DISPLAY_X*(i+1)/5+8, DISPLAY_Y+12, str, strlen(str));
		draw_led(i, LED_OFF);
	}

	// Start timer for LED error blinking
	c64_disp = this;
	pulse_sa.sa_handler = (void (*)(int))pulse_handler;
	pulse_sa.sa_flags = SA_RESTART;
	sigemptyset(&pulse_sa.sa_mask);
	sigaction(SIGALRM, &pulse_sa, NULL);
	pulse_tv.it_interval.tv_sec = 0;
	pulse_tv.it_interval.tv_usec = 400000;
	pulse_tv.it_value.tv_sec = 0;
	pulse_tv.it_value.tv_usec = 400000;
	setitimer(ITIMER_REAL, &pulse_tv, NULL);
}


/*
 *  Display destructor
 */

C64Display::~C64Display()
{
	XAutoRepeatOn(display);
	XSync(display, 0);
}


/*
 *  Prefs may have changed
 */

void C64Display::NewPrefs(Prefs *prefs)
{
}


/*
 *  Connect to X server and open window
 */

int init_graphics(void)
{
	int i;
	char *display_name = 0;
	XSetWindowAttributes wattr;
	XSizeHints *hints;
	XColor exact_color;
	int pixbytes;

	display = XOpenDisplay(display_name);
	if (display == 0)  {
		LOGE( "Can't connect to X server %s\n", XDisplayName(display_name));
		return 0;
	}

	screen = XDefaultScreen(display);
	rootwin = XRootWindow(display, screen);
	if (XMatchVisualInfo(display, screen, 8, PseudoColor, &visualInfo)) {
		/* for our HP boxes */
	} else if (XMatchVisualInfo(display, screen, 8, GrayScale, &visualInfo)) {
	} else {
		LOGE( "Can't obtain appropriate X visual\n");
		return 0;
	}

	vis = visualInfo.visual;
	bitdepth = visualInfo.depth;
	pixbytes = (bitdepth == 24 || bitdepth == 32 ? 4 : bitdepth == 12 || bitdepth == 16 ? 2 : 1);
	LOGE( "Using %d bit visual\n", bitdepth);

	hsize = (DISPLAY_X + 3) & ~3;

#if defined(X_USE_SHM)
        img = XShmCreateImage(display, vis, bitdepth, ZPixmap, 0, &shminfo,
			      hsize, DISPLAY_Y);
        
        shminfo.shmid = shmget(IPC_PRIVATE, DISPLAY_Y * img->bytes_per_line,
                               IPC_CREAT | 0777);
        shminfo.shmaddr = img->data = bufmem = (char *)shmat(shminfo.shmid, 0, 0);
        shminfo.readOnly = False;
        XShmAttach(display, &shminfo);
        XSync(display,0);
        /* now deleting means making it temporary */
        shmctl(shminfo.shmid, IPC_RMID, 0);
#else
	bufmem = (char *)malloc(pixbytes * hsize * DISPLAY_Y);
	img = XCreateImage(display, vis, bitdepth, ZPixmap, 0, bufmem, hsize, DISPLAY_Y, 32, 0);
#endif

	cmap = XCreateColormap(display, rootwin, vis, AllocNone);

	XParseColor(display, cmap, "#000000", &black);
	if (!XAllocColor(display, cmap, &black))
		LOGE( "Whoops??\n");

	wattr.event_mask = eventmask;
	wattr.background_pixel = black.pixel;
	wattr.backing_store = Always;
	wattr.backing_planes = bitdepth;
	wattr.border_pixmap = None;
	wattr.border_pixel = black.pixel;
	wattr.colormap = cmap;

	mywin = XCreateWindow(display, rootwin, 0, 0, DISPLAY_X, DISPLAY_Y + 16, 0,
						  bitdepth, InputOutput, vis,
						  CWEventMask|CWBackPixel|CWBorderPixel|CWBackingStore
						  |CWBackingPlanes|CWColormap,
						  &wattr);
	XMapWindow(display, mywin);
	XStoreName(display, mywin, "Frodo");

	if ((hints = XAllocSizeHints()) != NULL) {
		hints->min_width = DISPLAY_X;
		hints->max_width = DISPLAY_X;
		hints->min_height = DISPLAY_Y + 16;
		hints->max_height = DISPLAY_Y + 16;
		hints->flags = PMinSize | PMaxSize;
		XSetWMNormalHints(display, mywin, hints);
		XFree((char *)hints);
	}

	black_gc = XCreateGC(display,mywin, 0, 0);
    	XSetForeground(display, black_gc, black.pixel);

	// Allocate colors for speedometer/LEDs
	if (!XAllocNamedColor(display, cmap, "rgb:d0/d0/d0", &fill_gray, &exact_color))
		return 0;
	if (!XAllocNamedColor(display, cmap, "rgb:e8/e8/e8", &shine_gray, &exact_color))
		return 0;
	if (!XAllocNamedColor(display, cmap, "rgb:98/98/98", &shadow_gray, &exact_color))
		return 0;
	if (!XAllocNamedColor(display, cmap, "rgb:f0/00/00", &red, &exact_color))
		return 0;
	if (!XAllocNamedColor(display, cmap, "rgb:00/f0/00", &green, &exact_color))
		return 0;

	// Load font for speedometer/LED labels
	led_font = XLoadFont(display, "-*-helvetica-medium-r-*-*-10-*");

	for(i=0; i<256; i++)
		keystate[i] = 0;

	return 1;       
}


/*
 *  Redraw bitmap
 */

void C64Display::Update(void)
{
	// Update C64 display
	XSync(display, 0);
#if defined(X_USE_SHM)
	XShmPutImage(display, mywin, black_gc, img, 0, 0, 0, 0, DISPLAY_X, DISPLAY_Y, 0);
#else
 	XPutImage(display, mywin, black_gc, img, 0, 0, 0, 0, DISPLAY_X, DISPLAY_Y);
#endif

	// Update drive LEDs
	for (int i=0; i<4; i++)
		if (led_state[i] != old_led_state[i]) {
			draw_led(i, led_state[i]);
			old_led_state[i] = led_state[i];
		}
}


/*
 *  Draw one drive LED
 */

void C64Display::draw_led(int num, int state)
{
	switch (state) {
		case LED_OFF:
		case LED_ERROR_OFF:
			XSetForeground(display, led_gc, black.pixel);
			break;
		case LED_ON:
			XSetForeground(display, led_gc, green.pixel);
			break;
		case LED_ERROR_ON:
			XSetForeground(display, led_gc, red.pixel);
			break;
	}
	XFillRectangle(display, mywin, led_gc, DISPLAY_X*(num+2)/5-23, DISPLAY_Y+5, 14, 6);
}


/*
 *  LED error blink
 */

void C64Display::pulse_handler(...)
{
	for (int i=0; i<4; i++)
		switch (c64_disp->led_state[i]) {
			case LED_ERROR_ON:
				c64_disp->led_state[i] = LED_ERROR_OFF;
				break;
			case LED_ERROR_OFF:
				c64_disp->led_state[i] = LED_ERROR_ON;
				break;
		}
}


/*
 *  Draw speedometer
 */

void C64Display::Speedometer(int speed)
{
	static int delay = 0;

	if (delay >= 20) {
		char str[16];
		sprintf(str, "%d%%", speed);
		XSetForeground(display, led_gc, fill_gray.pixel);
		XFillRectangle(display,mywin, led_gc, 1, DISPLAY_Y+1, DISPLAY_X/5-2, 14);
		XSetForeground(display, led_gc, black.pixel);
		XDrawString(display, mywin, led_gc, 24, DISPLAY_Y+12, str, strlen(str));
		delay = 0;
	} else
		delay++;
}


/*
 *  Return pointer to bitmap data
 */

uint8 *C64Display::BitmapBase(void)
{
	return (uint8 *)bufmem;
}


/*
 *  Return number of bytes per row
 */

int C64Display::BitmapXMod(void)
{
	return hsize;
}


/*
 *  Poll the keyboard
 */

void C64Display::PollKeyboard(uint8 *key_matrix, uint8 *rev_matrix, uint8 *joystick)
{
	static bool auto_rep = true;
	for(;;) {
		XEvent event;
		if (!XCheckMaskEvent(display, eventmask, &event)) 
			break;
       
		switch(event.type) {

			case KeyPress: {                
				int kc = keycode2c64((XKeyEvent *)&event);
				if (kc == -1)
					break;
				switch (kc) {

					case KEY_F9:	// F9: Invoke SAM
						SAM(TheC64);
						break;

					case KEY_F10:	// F10: Quit
						quit_requested = true;
						break;

					case KEY_F11:	// F11: NMI (Restore)
						TheC64->NMI();
						break;

					case KEY_F12:	// F12: Reset
						TheC64->Reset();
						break;

					case KEY_NUM_LOCK:	// NumLock: Toggle joyport
						num_locked = true;
						break;

					case KEY_FIRE:
						joystate &= ~0x10;
						break;
					case KEY_JD:
						joystate &= ~0x02;
						break;
					case KEY_JU:
						joystate &= ~0x01;
						break;
					case KEY_JL:
						joystate &= ~0x04;
						break;
					case KEY_JR:
						joystate &= ~0x08;
						break;
					case KEY_JUL:
						joystate &= ~0x05;
						break;
					case KEY_JUR:
						joystate &= ~0x09;
						break;
					case KEY_JDL:
						joystate &= ~0x06;
						break;
					case KEY_JDR:
						joystate &= ~0x0a;
						break;

					case KEY_KP_PLUS:	// '+' on keypad: Increase SkipFrames
						ThePrefs.SkipFrames++;
						break;

					case KEY_KP_MINUS:	// '-' on keypad: Decrease SkipFrames
						if (ThePrefs.SkipFrames > 1)
							ThePrefs.SkipFrames--;
						break;

					case KEY_KP_MULT:	// '*' on keypad: Toggle speed limiter
						ThePrefs.LimitSpeed = !ThePrefs.LimitSpeed;
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
				break;
			}

			case KeyRelease: {           
				int kc = keycode2c64((XKeyEvent *)&event);
				if (kc == -1)
					break;
				switch (kc) {

					case KEY_NUM_LOCK:
						num_locked = false;
						break;

					case KEY_FIRE:
						joystate |= 0x10;
						break;
					case KEY_JD:
						joystate |= 0x02;
						break;
					case KEY_JU:
						joystate |= 0x01;
						break;
					case KEY_JL:
						joystate |= 0x04;
						break;
					case KEY_JR:
						joystate |= 0x08;
						break;
					case KEY_JUL:
						joystate |= 0x05;
						break;
					case KEY_JUR:
						joystate |= 0x09;
						break;
					case KEY_JDL:
						joystate |= 0x06;
						break;
					case KEY_JDR:
						joystate |= 0x0a;
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

			case FocusIn:
				if (auto_rep) {
					XAutoRepeatOff(display);
					auto_rep = false;
				}
				break;

			case FocusOut:
				if (!auto_rep) {
					XAutoRepeatOn(display);
					auto_rep = true;
				}
				break;
		}
	}
	*joystick = joystate;
}


/*
 *  Check if NumLock is down (for switching the joystick keyboard emulation)
 */

bool C64Display::NumLock(void)
{
	return num_locked;
}


/*
 *  Open/close joystick drivers given old and new state of
 *  joystick preferences
 */

void C64::open_close_joysticks(int oldjoy1, int oldjoy2, int newjoy1, int newjoy2)
{
}


/*
 *  Poll joystick port, return CIA mask
 */

uint8 C64::poll_joystick(int port)
{
	return 0xff;
}


/*
 *  Allocate C64 colors
 */

void C64Display::InitColors(uint8 *colors)
{
	int i;
	XColor col;
	char str[20];

	for (i=0; i< 256; i++) {
		sprintf(str, "rgb:%x/%x/%x", palette_red[i & 0x0f], palette_green[i & 0x0f], palette_blue[i & 0x0f]);
		XParseColor(display, cmap, str, &col);
		if (XAllocColor(display, cmap, &col))
			colors[i] = col.pixel;
		else
			LOGE( "Couldn't get all colors\n");
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
