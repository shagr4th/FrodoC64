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


//static Display *display;
//static int screen;
//static Window rootwin, mywin;

//static GC black_gc, led_gc;
//static XColor black, fill_gray, shine_gray, shadow_gray, red, green;
//static Colormap cmap;
//static Font led_font;

//static XImage *img;
//static Visual *vis;
//static XVisualInfo visualInfo;
static int bitdepth;
static char *bufmem;
//static char *bufmem_copy;
static int hsize;

// For LED error blinking
static C64Display *c64_disp;
static struct sigaction pulse_sa;
static itimerval pulse_tv;

// Keyboard and joystick
static int keystate[256];
static int joystate = 0xFF;
static bool num_locked = false;

//static const long int eventmask = (KeyPressMask|KeyReleaseMask|FocusChangeMask|ExposureMask);

#define PALETTE_SIZE 16
#define BITMAP_SIZE DISPLAY_X*DISPLAY_Y
unsigned short palette[PALETTE_SIZE];


/*
  C64 keyboard matrix:

    Bit 7   6   5   4   3   2   1   0
  0    CUD  F5  F3  F1  F7 CLR RET DEL
  1    SHL  E   S   Z   4   A   W   3
  2     X   T   F   C   6   D   R   5
  3     V   U   H   B   8   G   Y   7
  4     N   O   K   M   0   J   I   9
  5     ,   @   :   .   -   L   P   +
  6     /   ^   =  SHR HOM  ;   *   �
  7    R/S  Q   C= SPC  2  CTL  <-  1
*/

#define MATRIX(a,b) (((a) << 3) | (b))

#define KEY_F9 512
#define KEY_F10 513
#define KEY_F11 514
#define KEY_F12 515

#define KEY_FIRE 516
#define KEY_JU 517
#define KEY_JD 518
#define KEY_JL 519
#define KEY_JR 520

#define KEY_JUL 521
#define KEY_JUR 522
#define KEY_JDL 523
#define KEY_JDR 524


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

	for (i=0; i<4; i++) {
		sprintf(str, "Drive %d", i+8);
		draw_led(i, LED_OFF);
	}

	// Start timer for LED error blinking
	c64_disp = this;
	/*pulse_sa.sa_handler = (void (*)(int))pulse_handler;
	pulse_sa.sa_flags = SA_RESTART;
	sigemptyset(&pulse_sa.sa_mask);
	sigaction(SIGALRM, &pulse_sa, NULL);
	pulse_tv.it_interval.tv_sec = 0;
	pulse_tv.it_interval.tv_usec = 400000;
	pulse_tv.it_value.tv_sec = 0;
	pulse_tv.it_value.tv_usec = 400000;
	setitimer(ITIMER_REAL, &pulse_tv, NULL);*/
}


/*
 *  Display destructor
 */

C64Display::~C64Display()
{
	//XAutoRepeatOn(display);
	//XSync(display, 0);
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
	int pixbytes;
	pixbytes = 1;
	//hsize = (DISPLAY_X + 3) & ~3;
	hsize = DISPLAY_X;
	bufmem = (char *)malloc(pixbytes * hsize * DISPLAY_Y);
	//bufmem_copy = (char *)malloc(pixbytes * hsize * DISPLAY_Y);

	for(i=0; i<256; i++)
		keystate[i] = 0;

	LOGI("init_graphics");

	return 1;       
}


/*
 *  Redraw bitmap
 */

short* bufferCopy;
int w;
int h;

int flipped = 0;

int border_present = 0;
void C64Display::Update(void)
{
	kbd_buf_update(TheC64);
	
	// Update drive LEDs
	for (int i=0; i<4; i++)
		if (led_state[i] != old_led_state[i]) {
			draw_led(i, led_state[i]);
			old_led_state[i] = led_state[i];
		}

		
		flipped = 1;

	
	if (bufferCopy)
	{
		if (border_present) {
			short *buf = bufferCopy;

			int delay = 0;
			if (w == 512)
			{
				delay = 128;
			}

			for(int y=0;y<DISPLAY_Y-0;y++) {
				int a = y*DISPLAY_X;
				for(int x=a+0;x<a+DISPLAY_X-0;x++) {
					*buf++ = palette[bufmem[x]];
				}
				buf += delay;
			}
			(android_env)->CallVoidMethod(android_callback, requestRender);
		} else {
			short *buf = bufferCopy;

			int delay = 0;
			if (w == 512)
			{
				delay = 192;
			}

			for(int y=35;y<DISPLAY_Y-37;y++) {
				int a = y*DISPLAY_X;
				for(int x=a+32;x<a+DISPLAY_X-32;x++) {
					*buf++ = palette[bufmem[x]];
				}
				buf += delay;
			}
			(android_env)->CallVoidMethod(android_callback, requestRender);
		}
	}
		
}

extern "C"
jint Java_org_ab_c64_FrodoC64_setNumLock( JNIEnv*  env, jobject thiz, jint nl) {
	num_locked = false;
	if (nl != 0)
	{
		num_locked = true;
	}
}

extern "C"
jint Java_org_ab_c64_FrodoC64_setBorder( JNIEnv*  env, jobject thiz, jint border) {
	border_present = border;
}

#define SIZE 500
int queue [SIZE];
int *write_index = queue, *tos = queue, *read_index = queue;
void push_circular_queue(int a) {
	write_index++;
	if(write_index == (tos+SIZE)) // circular
		write_index = tos;
	*write_index = a;
}
int pop_circular_queue() {
	if (read_index == write_index)
		return -1; 	read_index++;
	if(read_index == (tos+SIZE)) // circular
		read_index = tos;
	return *read_index;
}


extern "C"
jint Java_org_ab_c64_FrodoC64_initScreenData( JNIEnv*  env, jobject thiz, jobject shortBuffer, jint width, jint height, jint ccode1, jint ccode2, jint border) {
	w = width;
	h = height;
	bufferCopy = (jshort*) (env)->GetDirectBufferAddress(shortBuffer);
	border_present = border;
	LOGI("Init buffer");
	return 0;
}

extern "C"
jint Java_org_ab_c64_FrodoC64_sendKey( JNIEnv*  env, jobject thiz, jint ccode) {
	push_circular_queue(ccode);
}

extern "C"
jint Java_org_ab_c64_FrodoC64_fillScreenData( JNIEnv*  env, jobject thiz, jobject shortBuffer, jint width, jint height, jint ccode1, jint ccode2) {
	//push_circular_queue(ccode1);
	//push_circular_queue(ccode2);
	if (flipped)
	{

		jshort* buf = (jshort*) (env)->GetDirectBufferAddress(shortBuffer); 
		if (border_present)
		{
		
		
			int delay = 0;
			if (width == 512)
			{
				delay = 128;
			}

			for(int y=0;y<DISPLAY_Y;y++) {
				int a = y*DISPLAY_X;
				for(int x=0;x<DISPLAY_X;x++) {
					*buf++ = palette[bufmem[a+x]];
				}
				buf += delay;
			}
		} else {
			

			int delay = 0;
			if (width == 512)
			{
				delay = 192;
			}
			
			for(int y=35;y<DISPLAY_Y-37;y++) {
				int a = y*DISPLAY_X;
				for(int x=32;x<DISPLAY_X-32;x++) {
					*buf++ = palette[bufmem[a+x]];
				}
				buf += delay;
			}
			
		}
		flipped = 0;
		return 1;
	}
	return 0;

	
}
/*
 *  Draw one drive LED
 */

void C64Display::draw_led(int num, int state)
{
	/*switch (state) {
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
	XFillRectangle(display, mywin, led_gc, DISPLAY_X*(num+2)/5-23, DISPLAY_Y+5, 14, 6);*/
//LOGI("draw_led: %d", state);
	//(android_env)->CallVoidMethod(android_callback, drawled, num, state);
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
		/*char str[16];
		sprintf(str, "%d%%", speed);
		XSetForeground(display, led_gc, fill_gray.pixel);
		XFillRectangle(display,mywin, led_gc, 1, DISPLAY_Y+1, DISPLAY_X/5-2, 14);
		XSetForeground(display, led_gc, black.pixel);
		XDrawString(display, mywin, led_gc, 24, DISPLAY_Y+12, str, strlen(str));*/
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

	int code = pop_circular_queue();
	//int code = (android_env)->CallIntMethod(android_callback, getEvent);
	//int code = -1;
	if (code != -1)
	{
		//LOGI("event received: %d", code);
		int kc;
		if (code < -1)
		{
			kc = -code - 2;
			switch(kc) {
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
		} else if (code >= 0)
		{
			kc = code;
			switch(kc) {
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
	/*int i;
	XColor col;
	char str[20];

	for (i=0; i< 256; i++) {
		sprintf(str, "rgb:%x/%x/%x", palette_red[i & 0x0f], palette_green[i & 0x0f], palette_blue[i & 0x0f]);
		XParseColor(display, cmap, str, &col);
		if (XAllocColor(display, cmap, &col))
			colors[i] = col.pixel;
		else
			LOGE( "Couldn't get all colors\n");
	}*/
	for (int i=0; i<16; i++) {
		/*int r = (int)palette_red[i];
		int g = (int)palette_green[i];
		int b = (int)palette_blue[i];
        palette[i] = (0xff << 24) | ((r) << 16) | ((g) << 8) | (b);*/

		//palette[i] = (0xff << 24) | (palette_red[i] << 16) | (palette_green[i] << 8) | (palette_blue[i]);
		palette[i] = ((palette_red[i]>>3) << 11) | ((palette_green[i]>>2) << 5) | (palette_blue[i] >>3);
	}

	for (int i=0; i<256; i++)
		colors[i] = i & 0x0f;
}


/*
 *  Show a requester (error message)
 */

long int ShowRequester(char *a,char *b,char *)
{
	printf("%s: %s\n", a, b);
	return 1;
}
