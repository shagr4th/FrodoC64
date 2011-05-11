/*
 *  Display.h - C64 graphics display, emulator window handling
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

#ifndef _DISPLAY_H
#define _DISPLAY_H

#ifdef __BEOS__
#include <InterfaceKit.h>
#endif

#ifdef AMIGA
#include <graphics/rastport.h>
#endif

#ifdef HAVE_SDL
struct SDL_Surface;
#endif

#ifdef WIN32
#include <ddraw.h>
#endif

#ifdef __riscos__
#include "ROlib.h"
#endif

// Display dimensions
#if defined(SMALL_DISPLAY)
const int DISPLAY_X = 0x168;
const int DISPLAY_Y = 0x110;
#else
const int DISPLAY_X = 0x180;
const int DISPLAY_Y = 0x110;
#endif



class C64Window;
class C64Screen;
class C64;
class Prefs;

// Class for C64 graphics display
class C64Display {
public:
	C64Display(C64 *the_c64);
	~C64Display();

	void Update(void);
	void UpdateLEDs(int l0, int l1, int l2, int l3);
	void Speedometer(int speed);
	uint8 *BitmapBase(void);
	int BitmapXMod(void);
#ifdef __riscos__
	void PollKeyboard(uint8 *key_matrix, uint8 *rev_matrix, uint8 *joystick, uint8 *joystick2);
#else
	void PollKeyboard(uint8 *key_matrix, uint8 *rev_matrix, uint8 *joystick);
#endif
	bool NumLock(void);
	void InitColors(uint8 *colors);
	void NewPrefs(Prefs *prefs);

	C64 *TheC64;

#ifdef __BEOS__
	void Pause(void);
	void Resume(void);
#endif

#ifdef __riscos__
	void ModeChange(void);
	unsigned int *GetColourTable(void);	// returns pointer to mode_cols
	bool CheckForUnpause(bool CheckLastState);

	ROScreen *screen;
	Joy_Keys JoystickKeys[2];		// it's easier making the joystick keys public
#endif

#ifdef __unix
	bool quit_requested;
#endif

private:
	int led_state[4];
	int old_led_state[4];

#ifdef __BEOS__
	C64Window *the_window;	// One of these is NULL
	C64Screen *the_screen;
	bool using_screen;		// Flag: Using the_screen
	key_info old_key_info;
	int draw_bitmap;		// Number of bitmap for the VIC to draw into
#endif

#ifdef AMIGA
	void draw_led_bar(void);	// Draw LED bar at the bottom of the window
	void draw_led(int num, int state);	// Draw one LED

	struct Window *the_window;	// Pointer to C64 display window
	struct Screen *the_screen;	// The window's screen
	struct RastPort *the_rp;	// The window's RastPort
	struct VisualInfo *the_visual_info;
	struct Menu *the_menus;
	struct TextFont *led_font;
	struct TextFont *speedo_font;
	struct RastPort temp_rp;	// For WritePixelArray8()
	struct BitMap *temp_bm;
	uint8 *chunky_buf;			// Chunky buffer for drawing into
	LONG pens[16];				// Pens for C64 colors
	int xo, yo;					// Window X/Y border size
	struct FileRequester *open_req, *save_req;	// File requesters for load/save snapshot
#endif

#ifdef HAVE_SDL
	char speedometer_string[16];		// Speedometer text
	void draw_string(SDL_Surface *s, int x, int y, const char *str, uint8 front_color, uint8 back_color);
#endif

#ifdef __sdlui__
	void draw_ascii_string(uint8 *s, int x, int y, const char *str, uint8 front_color, uint8 back_color);
	void Draw_Prefs(C64 *);
	void Draw_VKeyb(C64 *);
	char *FileReq(char *);
#endif

#ifdef __unix
	void draw_led(int num, int state);	// Draw one LED
	static void pulse_handler(...);		// LED error blinking
#endif

#ifdef WIN32
public:
	long ShowRequester(const char *str, const char *button1, const char *button2 = NULL);
	void WaitUntilActive();
	void NewPrefs();
	void Pause();
	void Resume();
	void Quit();

	struct DisplayMode {
		int x;
		int y;
		int depth;
		BOOL modex;
	};
	int GetNumDisplayModes() const;
	const DisplayMode *GetDisplayModes() const;

private:
	// Window members.
	void ResetKeyboardState();
	BOOL MakeWindow();
	static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	long WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static int VirtKey2C64(int virtkey, DWORD keydata);
	BOOL CalcViewPort();
	BOOL SetupWindow();
	BOOL SetupWindowMode(BOOL full_screen);
	BOOL RestoreWindow();
	BOOL ResizeWindow(int side, RECT *pRect);
	void WindowTitle();
	void CreateObjects();
	void DeleteObjects();

	// DirectDraw management members.
	BOOL StartDirectDraw();
	BOOL ResumeDirectDraw();
	BOOL ResetDirectDraw();
	BOOL StopDirectDraw();
	static HRESULT CALLBACK EnumModesCallback(LPDDSURFACEDESC pDDSD, LPVOID lpContext);
	HRESULT EnumModesCallback(LPDDSURFACEDESC pDDSD);
	static int CompareModes(const void *e1, const void *e2);
	BOOL Fail(const char *message);

	// DirectDraw worker members.
	BOOL SetPalettes();
	BOOL BuildColorTable();
	BOOL CopySurface(RECT &rcWork);
	BOOL FlipSurfaces();
	BOOL EraseSurfaces();
	BOOL RestoreSurfaces();

	void draw_led_bar(void);		// Draw LED bar on the window
	void draw_leds(BOOL force = false);	// Draw LEDs if force or changed
	void led_rect(int n, RECT &rc, RECT &led); // Compute LED rectangle
	void InsertNextDisk();			// should be a common func
	BOOL FileNameDialog(char *prefs_path, BOOL save = false);
	void OfferSave();			// Offer chance to save changes

	UBYTE *chunky_buf;			// Chunky buffer for drawing
	BOOL active;				// is application active?
	BOOL paused;				// is application paused?
	BOOL waiting;				// is application waiting?
	DWORD windowed_style;			// style of windowed window
	DWORD fullscreen_style;			// style of fullscreen window
	char failure_message[128];		// what when wrong
	int speed_index;			// look ma, no hands
	BOOL show_leds;				// cached prefs option
	BOOL full_screen;			// cached prefs option
	BOOL in_constructor;			// if we are being contructed 
	BOOL in_destructor;			// if we are being destroyed

	LPDIRECTDRAW pDD;			// DirectDraw object
	LPDIRECTDRAWSURFACE pPrimary;		// DirectDraw primary surface
	LPDIRECTDRAWSURFACE pBack;		// DirectDraw back surface
	LPDIRECTDRAWSURFACE pWork;		// DirectDraw working surface
	LPDIRECTDRAWCLIPPER pClipper;		// DirectDraw clipper
	LPDIRECTDRAWPALETTE pPalette;		// DirectDraw palette

	DWORD colors[256];			// our palette colors
	int colors_depth;			// depth of the colors table
#endif

#ifdef __riscos__
	unsigned int mode_cols[256];	// Colours in the current mode corresponding to C64's
	uint8 *bitmap;
	uint32 lastkeys[8];		// bitfield describing keys pressed last time.
#endif
};


// Exported functions
extern long ShowRequester(char *str, char *button1, char *button2 = NULL);


#endif
