/*
 *  Display_WIN32.h - C64 graphics display, emulator window handling,
 *                    WIN32 specific stuff
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

#include <shellapi.h>
#include <commctrl.h>

#include "C64.h"
#include "SAM.h"
#include "Version.h"
#include "VIC.h"
#include "resource.h"

#define NAME "Frodo"
#define TITLE (IsFrodoSC ? "FrodoSC" : "Frodo")

#ifdef DEBUG

class TimeScope
{

public:
	TimeScope(const char *s)
	{
		tag = s;
		QueryPerformanceCounter(&liStart);
	}
	~TimeScope()
	{
		QueryPerformanceCounter(&liFinish);
		OutputTime();
	}

private:
	void
	OutputTime()
	{
		LARGE_INTEGER liFreq;

		QueryPerformanceFrequency(&liFreq);
		Debug("%s: %.0f usec\n", tag,
			double(liFinish.LowPart - liStart.LowPart)
			/liFreq.LowPart*1000000);
	}
	const char *tag;
	LARGE_INTEGER liStart, liFinish;
};

#define TIMESCOPE(var, tag) TimeScope var(tag)

#else

#define TIMESCOPE(var, tag)

#endif

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

#define KEY_F9		256
#define KEY_F10		257
#define KEY_F11		258
#define KEY_F12		259

#define KEY_FIRE	260
#define KEY_JUP		261
#define KEY_JDN		262
#define KEY_JLF		263
#define KEY_JRT		264
#define KEY_JUPLF	265
#define KEY_JUPRT	266
#define KEY_JDNLF	267
#define KEY_JDNRT	268
#define KEY_CENTER	269

#define KEY_NUMLOCK	270

#define KEY_KPPLUS	271
#define KEY_KPMINUS	272
#define KEY_KPMULT	273
#define KEY_KPDIV	274
#define KEY_KPENTER	275
#define KEY_KPPERIOD	276

#define KEY_PAUSE	277
#define KEY_ALTENTER 	278
#define KEY_CTRLENTER	279

#define VK_bracketleft	0xdb
#define VK_bracketright	0xdd
#define VK_comma	0xbc
#define VK_period	0xbe
#define VK_slash	0xbf
#define VK_semicolon	0xba
#define VK_grave	0xc0
#define VK_minus	0xbd
#define VK_equal	0xbb
#define VK_quote	0xde
#define VK_backslash	0xdc

static C64Display *TheDisplay;
static int keystate[256];
static UBYTE rev_matrix[8], key_matrix[8];
static int quit = 0;
static int numlock = 0;
static int joystate = 0xff;

static RECT rcScreen;
static RECT rcLast;
static RECT rcWindow;
static RECT rcWork;
static BOOL need_new_color_table = FALSE;
static int view_x, view_y;

static int led_rows = 16;

static HCURSOR invisible_cursor;
static HCURSOR arrow_cursor;

static HFONT led_font;

static HPEN led_highlight;
static HPEN led_shadow;

static HBRUSH led_brush;
static HBRUSH off_brush;
static HBRUSH error_off_brush;
static HBRUSH on_brush;
static HBRUSH error_on_brush;

// Not fully working yet.
#ifdef WORKBUFFER_BITMAP
static BOOL workbuffer_bitmap = FALSE;
static BOOL workbuffer_locked = FALSE;
static DDSURFACEDESC bitmap_ddsd;
#endif

C64Display::DisplayMode default_modes[] = {
	{ 320, 200, 8 },
	{ 320, 240, 8 },
	{ 512, 384, 8 },
	{ 640, 400, 8 },
	{ 640, 480, 8 },
	{ 320, 200, 16 },
	{ 320, 240, 16 },
	{ 512, 384, 16 },
	{ 640, 400, 16 },
	{ 640, 480, 16 },
};
static int num_default_modes =
	sizeof(default_modes)/sizeof(C64Display::DisplayMode);

static C64Display::DisplayMode *display_modes = NULL;
static int num_display_modes = 0;
static int max_display_modes = 16;

int C64Display::GetNumDisplayModes() const
{
	if (num_display_modes == 0)
		return num_default_modes;
	return num_display_modes;
}

const C64Display::DisplayMode *C64Display::GetDisplayModes() const
{
	if (num_display_modes == 0)
		return default_modes;
	return display_modes;
}

long ShowRequester(char *str, char *button1, char *button2)
{
	if (!TheDisplay) {
		MessageBox(hwnd, str, "Frodo", MB_OK | MB_ICONSTOP);
		return FALSE;
	}
	return TheDisplay->ShowRequester(str, button1, button2);
}

/*
 *  Display constructor: Create window/screen
 */

C64Display::C64Display(C64 *the_c64) : TheC64(the_c64)
{
	in_constructor = TRUE;
	in_destructor = FALSE;

	TheDisplay = this;
	speed_index = 0;

	pDD = NULL;
	pPrimary = NULL;
	pBack = NULL;
	pWork = NULL;
	pClipper = NULL;
	pPalette = NULL;
	active = FALSE;
	paused = FALSE;
	waiting = FALSE;
	show_leds = ThePrefs.ShowLEDs;
	full_screen = ThePrefs.DisplayType == DISPTYPE_SCREEN;

	// Turn LEDs off.
	for (int i = 0; i < 4; i++)
		led_state[i] = old_led_state[i] = LED_OFF;

	// Allocate chunky buffer to draw into.
	chunky_buf = new UBYTE[DISPLAY_X * DISPLAY_Y];

	CalcViewPort();

	ResetKeyboardState();

	if (!MakeWindow()) {
		ShowRequester("Failed to create window.", "Quit");
		Quit();
	}
	else {
		WindowTitle();

		if (!StartDirectDraw()) {
			ShowRequester(failure_message, "Quit");
			Quit();
		}
		else
			draw_led_bar();
	}

	in_constructor = FALSE;
}

/*
 *  Display destructor
 */

C64Display::~C64Display()
{
	in_destructor = TRUE;

	Debug("~C64Display\n");

	StopDirectDraw();

	// Offer to save now that we are not in full screen mode.
	OfferSave();

	// Free the display modes table.
	delete[] display_modes;

	// Free chunky buffer
	delete chunky_buf;

	// Destroy the main window.
	DestroyWindow(hwnd);

	// Drain the window message queue.
	for (;;)
	{
		MSG msg;
		if (!GetMessage(&msg, NULL, 0, 0))
			break;
		if (ThePrefs.SystemKeys)
			TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DeleteObjects();

	in_destructor = FALSE;
}

/*
 *  Prefs may have changed
 */

void C64Display::NewPrefs(Prefs *prefs)
{
}

void C64Display::DeleteObjects()
{
	// Delete objects we created.
	DeleteObject(led_highlight);
	DeleteObject(led_shadow);
	DeleteObject(led_font);
	DeleteObject(led_brush);
	DeleteObject(off_brush);
	DeleteObject(error_off_brush);
	DeleteObject(on_brush);
	DeleteObject(error_on_brush);
}

BOOL C64Display::CalcViewPort()
{
	int old_view_x = view_x, old_view_y = view_y;
	const char *view_port = ThePrefs.ViewPort;
	if (view_port[0] == '\0' ||
	    stricmp(view_port, "Default") == 0)
		view_port = NULL;
	if (!view_port || sscanf(view_port, "%dx%d", &view_x, &view_y) != 2) {
		view_x = DISPLAY_X;
		view_y = DISPLAY_Y;
	}
	SetRect(&rcWork, 0, 0, view_x, view_y);
	if (view_x != old_view_x || view_y != old_view_y)
		return TRUE;
	return FALSE;
}


BOOL C64Display::ResizeWindow(int side, RECT *pRect)
{
	// Compute size of non-client borders.
	DWORD style = GetWindowLong(hwnd, GWL_STYLE);
	RECT rc;
	SetRect(&rc, 0, 0, view_x, view_y);
	BOOL has_menu = GetMenu(hwnd) != NULL;
	AdjustWindowRect(&rc, style, has_menu);
	if (ThePrefs.ShowLEDs)
		rc.bottom += led_rows;
	int nc_x = rc.right - rc.left - view_x;
	int nc_y = rc.bottom - rc.top - view_y;

	// Compute client area corresponding to resizing.
	int old_x = pRect->right - pRect->left - nc_x;
	int old_y = pRect->bottom - pRect->top - nc_y;

	// Compute nearest integral scaling numerators.
	int d = ThePrefs.ScalingDenominator;
	int x = (old_x + view_x/d/2)/(view_x/d);
	if (x == 0)
		x = 1;
	int y = (old_y + view_y/4)/(view_y/d);
	if (y == 0)
		y = 1;

	// When resizing corners make the scale factors agree.
	switch (side) {
	case WMSZ_BOTTOMRIGHT:
	case WMSZ_BOTTOMLEFT:
	case WMSZ_TOPRIGHT:
	case WMSZ_TOPLEFT:
		if (x < y)
			y = x;
		else
			x = y;
	}

	// Compute the quantized size of the window area.
	int new_x = x*(view_x/d) + nc_x;
	int new_y = y*(view_y/d) + nc_y;

	// Adjust the resizing rectangle.
	switch (side) {

	case WMSZ_BOTTOMRIGHT:
	case WMSZ_BOTTOMLEFT:
	case WMSZ_BOTTOM:
		pRect->bottom = pRect->top + new_y;
		break;

	case WMSZ_TOPRIGHT:
	case WMSZ_TOPLEFT:
	case WMSZ_TOP:
		pRect->top = pRect->bottom - new_y;
		break;
	}
	switch (side) {

	case WMSZ_TOPRIGHT:
	case WMSZ_BOTTOMRIGHT:
	case WMSZ_RIGHT:
		pRect->right = pRect->left + new_x;
		break;

	case WMSZ_TOPLEFT:
	case WMSZ_BOTTOMLEFT:
	case WMSZ_LEFT:
		pRect->left = pRect->right - new_x;
		break;
	}

	return TRUE;
}

/*
 *  Update speedometer
 */

void C64Display::Speedometer(int speed)
{
	Debug("speed = %d %%\n", speed);
	speed_index = speed;

	if (full_screen)
		return;

	if (!ThePrefs.ShowLEDs) {
		WindowTitle();
		return;
	}

	if (speed_index == 0)
		return;

	HDC hdc = GetDC(hwnd);
	RECT rc;
	GetClientRect(hwnd, &rc);
	rc.top = rc.bottom - led_rows;
	rc.right = rc.left + (rc.right - rc.left)/5;
	FillRect(hdc, &rc, led_brush);
	SelectObject(hdc, led_font);
	SetTextAlign(hdc, TA_TOP | TA_LEFT);
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, (COLORREF) GetSysColor(COLOR_MENUTEXT));
	char str[128];
	if (IsFrodoSC)
		sprintf(str, "%d%%", speed_index);
	else
		sprintf(str, "%d%%", speed_index);
	int x = rc.left + 4;
	int y = rc.top + 2;
	TextOut(hdc, x, y, str, strlen(str));
	ReleaseDC(hwnd, hdc);
}


/*
 *  Return pointer to bitmap data
 */

UBYTE *C64Display::BitmapBase()
{
#ifdef WORKBUFFER_BITMAP
	if (colors_depth == 8 && pWork) {
		if (workbuffer_locked) {
			pWork->Unlock(NULL);
			workbuffer_locked = FALSE;
		}
		HRESULT ddrval;
		for (;;) {
			bitmap_ddsd.dwSize = sizeof(bitmap_ddsd);
			ddrval = pWork->Lock(NULL, &bitmap_ddsd, 0, NULL);
			if (ddrval != DDERR_WASSTILLDRAWING)
				break;
		}
		if (ddrval == DD_OK) {
			workbuffer_locked = TRUE;
			workbuffer_bitmap = TRUE;
			return (UBYTE *) bitmap_ddsd.lpSurface;
		}
	}
	workbuffer_bitmap = FALSE;
#endif
	return chunky_buf;
}


/*
 *  Return number of bytes per row
 */

int C64Display::BitmapXMod()
{
#ifdef WORKBUFFER_BITMAP
	if (workbuffer_locked)
		return bitmap_ddsd.lPitch;
#endif
	return DISPLAY_X;
}


/*
 *  Freshen keyboard state
 */

void C64Display::PollKeyboard(UBYTE *CIA_key_matrix, UBYTE *CIA_rev_matrix, UBYTE *joystick)
{
	//Debug("Display::PollKeyboard\n");

#ifdef WORKBUFFER_BITMAP
	if (workbuffer_locked) {
		pWork->Unlock(NULL);
		workbuffer_locked = FALSE;
	}
#endif

	for (;;)
	{
		MSG msg;
		if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			break;
		if (ThePrefs.SystemKeys)
			TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	*joystick = joystate;
	memcpy(CIA_key_matrix, key_matrix, sizeof(key_matrix));
	memcpy(CIA_rev_matrix, rev_matrix, sizeof(rev_matrix));
}

/*
 *  Check if NumLock is down (for switching the joystick keyboard emulation)
 */

bool C64Display::NumLock()
{
	return numlock;
}


/*
 *  Allocate C64 colors
 */

void C64Display::InitColors(UBYTE *array)
{
	if (colors_depth == 8) {
		for (int i = 0; i < 256; i++)
			array[i] = colors[i & 0x0f];
	}
	else {
		for (int i = 0; i < 256; i++)
			array[i] = i & 0x0f;
	}
}


long C64Display::ShowRequester(const char *str, const char *button1, const char *button2)
{
	// This could be a lot nicer but quick and dirty is fine with me.
	char message[1024];
	strcpy(message, str);
	strcat(message, "\nPress OK to ");
	strcat(message, button1);
	if (button2) {
		strcat(message, ", Cancel to ");
		strcat(message, button2);
	}
	strcat(message, ".");
	UINT type;
	if (button2) 
		type = MB_OKCANCEL | MB_ICONQUESTION;
	else
		type = MB_OK | MB_ICONSTOP;
	Pause();
	if (full_screen)
		StopDirectDraw();
	int result = MessageBox(hwnd, message, NAME " Error", type);
	if (full_screen)
		StartDirectDraw();
	Resume();
	if (result == IDCANCEL)
		return TRUE;
	return FALSE;
}

void C64Display::WaitUntilActive()
{
	Debug("waiting until not paused...\n");
	waiting = TRUE;
	WindowTitle();
	for (;;) {

		// Check for termination condition.
		if (!paused || quit)
			break;

		// Process message queue.
		MSG msg;
		if (GetMessage(&msg, NULL, 0, 0) != TRUE)
			break;

		// Always translate system keys while paused.
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	waiting = FALSE;
	Debug("...done waiting\n");
	WindowTitle();
	ResetKeyboardState();
}

void C64Display::ResetKeyboardState()
{
	memset(keystate, 0, sizeof(keystate));
	memset(key_matrix, 0xff, sizeof(key_matrix));
	memset(rev_matrix, 0xff, sizeof(rev_matrix));
	joystate = 0xff;
}

BOOL C64Display::MakeWindow()
{
	// Set up and register window class.
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = StaticWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(FRODO_ICON));
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAIN_MENU);
	wc.lpszClassName = NAME;
	RegisterClass(&wc);

	// Set up our preferred styles for our window depending on the mode.
	windowed_style = WS_VISIBLE | WS_SYSMENU | WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_THICKFRAME;
	fullscreen_style = WS_POPUP | WS_VISIBLE;

	// Compute the initial window size.
	DWORD style = windowed_style;
	RECT rc;
	int n = ThePrefs.ScalingNumerator;
	int d = ThePrefs.ScalingDenominator;
	SetRect(&rc, 0, 0, n*view_x/d, n*view_y/d);
	BOOL has_menu = wc.lpszMenuName != NULL;
	AdjustWindowRect(&rc, style, has_menu);
	if (ThePrefs.ShowLEDs)
		rc.bottom += led_rows;
	int x_size = rc.right - rc.left;
	int y_size = rc.bottom - rc.top;

	// Create the window and save the initial position.
	hwnd = CreateWindowEx(0, NAME, TITLE, style, CW_USEDEFAULT, 0, x_size, y_size, NULL, NULL, hInstance, NULL);
	GetWindowRect(hwnd, &rcLast);
	SetRect(&rcScreen, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

	// Load cursors.
	invisible_cursor = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_INVISIBLE));
	arrow_cursor = LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW));

	// Create fonts, pens, brushes, etc.
	CreateObjects();

	if (!hwnd)
		return FALSE;

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	return TRUE;
}

void C64Display::CreateObjects()
{
	// Create fonts.
	led_font = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		VARIABLE_PITCH | FF_SWISS, "");

	// Create pens.
	led_highlight = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DHIGHLIGHT));
	led_shadow = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DSHADOW));

	// Create brushes.
	LOGBRUSH logbrush;
	logbrush.lbStyle = BS_SOLID;
	logbrush.lbHatch = 0;
	logbrush.lbColor = GetSysColor(COLOR_MENU);
	led_brush = CreateBrushIndirect(&logbrush);
	logbrush.lbColor = RGB(0x00, 0x00, 0x00); // black
	off_brush = CreateBrushIndirect(&logbrush);
	logbrush.lbColor = RGB(0x00, 0x00, 0x00); // black
	error_off_brush = CreateBrushIndirect(&logbrush);
	logbrush.lbColor = RGB(0x00, 0xff, 0x00); // green
	on_brush = CreateBrushIndirect(&logbrush);
	logbrush.lbColor = RGB(0xff, 0x00, 0x00); // red
	error_on_brush = CreateBrushIndirect(&logbrush);
}

HRESULT CALLBACK C64Display::StaticWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return TheDisplay->WindowProc(hWnd, message, wParam, lParam);
}

long C64Display::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Debug("window message: 0x%x\n", message);

	switch (message) {

	case WM_SYSCOLORCHANGE:
		DeleteObjects();
		CreateObjects();
		InvalidateRect(hwnd, NULL, FALSE);
		break;

	case WM_MOUSEMOVE:
		SetCursor(ThePrefs.HideCursor ? invisible_cursor : arrow_cursor);
		break;
		
	case WM_MENUCHAR:
		// Eat Alt-foo characters so that it doesn't beep.
		if (HIWORD(wParam) == 0)
			return MAKELONG(0, 1);
		break;
		
	case WM_ENTERSIZEMOVE:
		Pause();
		break;

	case WM_EXITSIZEMOVE:
		Resume();
		break;

	case WM_SIZING:
		ResizeWindow(wParam, (RECT *) lParam);
		return TRUE;

	case WM_SIZE:
	case WM_MOVE:
		if (full_screen)
			SetRect(&rcWindow, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		else {
			GetClientRect(hWnd, &rcWindow);
			if (ThePrefs.ShowLEDs)
				rcWindow.bottom -= led_rows;
			ClientToScreen(hWnd, (LPPOINT) &rcWindow);
			ClientToScreen(hWnd, (LPPOINT) &rcWindow + 1);

			// Align the client rect to a four-byte
			// boundary because this can triple the
			// speed of a memcpy to the display.
			int align_to = 4;
			int misalignment = rcWindow.left % align_to;
			if (misalignment == 0)
				Update();
			else {
				if (misalignment > align_to/2)
					misalignment -= align_to;
				RECT rc;
				GetWindowRect(hwnd, &rc);
				MoveWindow(hwnd, rc.left - misalignment,
					rc.top, rc.right - rc.left,
					rc.bottom - rc.top, TRUE);
			}
		}
		break;

	case WM_DISPLAYCHANGE:
		if (!full_screen)
			ResumeDirectDraw();
		SetRect(&rcScreen, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		break;

	case WM_ACTIVATE:
		Debug("WM_ACTIVATE\n");
		{
			int old_active = active;
			active = LOWORD(wParam) != WA_INACTIVE;
			if (ThePrefs.AutoPause && active != old_active) {
				if (!active)
					Pause();
				else
					Resume();
			}
		}
		if (active) {
			ResumeDirectDraw();
			ResetKeyboardState();
		}

		// Kick the message loop since this was sent to us, not posted.
		PostMessage(hWnd, WM_USER, 0, 0);
		break;

	case WM_COMMAND:
		{
			int id = LOWORD(wParam);
			switch (id) {

			case ID_FILE_NEW:
				{
					OfferSave();
					Prefs *prefs = new Prefs;
					TheC64->NewPrefs(prefs);
					ThePrefs = *prefs;
					ThePrefsOnDisk = ThePrefs;
					delete prefs;
					strcpy(TheApp->prefs_path, "Untitled.fpr");
					NewPrefs();
				}
				break;

			case ID_FILE_OPEN:
				Pause();
				OfferSave();
				if (FileNameDialog(TheApp->prefs_path)) {
					Prefs *prefs = new Prefs;
					prefs->Load(TheApp->prefs_path);
					TheC64->NewPrefs(prefs);
					ThePrefs = *prefs;
					delete prefs;
					NewPrefs();
				}
				Resume();
				break;

			case ID_FILE_SAVE:
				ThePrefs.Save(TheApp->prefs_path);
				break;

			case ID_FILE_SAVEAS:
				Pause();
				if (FileNameDialog(TheApp->prefs_path, TRUE)) {
					ThePrefs.Save(TheApp->prefs_path);
					WindowTitle();
				}
				Resume();
				break;

			case ID_FILE_EX:
				PostMessage(hWnd, WM_CLOSE, 0, 0);
				break;

			case ID_TOOLS_PREFERENCES:
				Pause();
				TheApp->RunPrefsEditor();
				NewPrefs();
				Resume();
				break;

			case ID_TOOLS_FULLSCREEN:
				Pause();
				StopDirectDraw();
				full_screen = !full_screen;
				if (!StartDirectDraw()) {
					StopDirectDraw();
					full_screen = !full_screen;
					StartDirectDraw();
					if (!full_screen)
						ShowRequester(failure_message, "Continue");
				}
				Resume();
				CheckMenuItem(GetMenu(hWnd), ID_TOOLS_FULLSCREEN, full_screen ? MF_CHECKED : MF_UNCHECKED);
				if (paused)
					Update();
				break;

			case ID_TOOLS_RESETDIRECTDRAW:
				ResetDirectDraw();
				break;

			case ID_TOOLS_PAUSE:
				if (!paused)
					Pause();
				else {
					// XXX: Shouldn't happen but be safe.
					while (paused)
						Resume();
					ResetKeyboardState();
				}
				CheckMenuItem(GetMenu(hWnd), ID_TOOLS_PAUSE, paused ? MF_CHECKED : MF_UNCHECKED);
				break;

			case ID_TOOLS_RESETC64:
				TheC64->Reset();
				break;

			case ID_TOOLS_INSERTNEXTDISK:
				InsertNextDisk();
				break;

			case ID_TOOLS_SAM:
				Pause();
				MessageBox(hWnd, "SAM not yet implemented.", NAME, MB_OK);
				Resume();
				break;

			case ID_HELP_CONTENTS:
			case ID_HELP_KEYBOARD:
			case ID_HELP_SETTINGS:
				{
					const char *html;
					switch (id) {
					case ID_HELP_CONTENTS: html = "Main"; break;
					case ID_HELP_KEYBOARD: html = "keyboard"; break;
					case ID_HELP_SETTINGS: html = "settings"; break;
					}
					char helpfile[256];
					sprintf(helpfile, "%s\\Docs\\%s.html", AppDirPath, html);
					ShellExecute(0, 0, helpfile, 0, 0, SW_NORMAL);
				}
				break;

			case ID_HELP_ABOUT:
				{
					Pause();
					char message[256];
					sprintf(message, "%s by %s\n%s by %s",
						VERSION_STRING,
						"Christian Bauer",
						"WIN32 port",
						"J. Richard Sladkey");
					MessageBox(hWnd, message, NAME, MB_OK);
					Resume();
				}
				break;
			}
			ResetKeyboardState();
		}
		break;

	case WM_CLOSE:
		Quit();
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_QUERYNEWPALETTE:
		if (!full_screen && pPalette && pPrimary) {
			SetPalettes();
			BuildColorTable();
			if (!active)
				Update();
		}
		break;

	case WM_PALETTECHANGED:
		if (!full_screen) {
			if ((HWND) wParam != hWnd) {
				need_new_color_table = TRUE;
				InvalidateRect(hwnd, NULL, FALSE);
			}
		}
		break;

	case WM_PAINT:
		if (!full_screen)
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			if (need_new_color_table) {
				BuildColorTable();
				need_new_color_table = FALSE;
			}
			if (paused)
				Update();
			draw_led_bar();
			Speedometer(speed_index);
			return 0;
		}
		break;

	case WM_ENTERMENULOOP:
		Pause();
		break;

	case WM_EXITMENULOOP:
		Resume();
		ResetKeyboardState();
		break;

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		Debug("Display::WindowProc: KEYDOWN: 0x%x\n", wParam);
		{
			int kc = VirtKey2C64(wParam, lParam);
			switch (kc) {

			case KEY_PAUSE:
				PostMessage(hWnd, WM_COMMAND, ID_TOOLS_PAUSE, 0);
				break;

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
				{
					Prefs *prefs = new Prefs(ThePrefs);
					prefs->Emul1541Proc = !prefs->Emul1541Proc;
					TheC64->NewPrefs(prefs);
					ThePrefs = *prefs;
					delete prefs;
				}
				break;

			case KEY_KPPERIOD:
				ThePrefs.JoystickSwap = !ThePrefs.JoystickSwap;
				break;

			case KEY_F9:
				PostMessage(hWnd, WM_COMMAND, ID_TOOLS_INSERTNEXTDISK, 0);
				break;

			case KEY_ALTENTER:
				PostMessage(hWnd, WM_COMMAND, ID_TOOLS_FULLSCREEN, 0);
				break;

			case KEY_CTRLENTER:
				PostMessage(hWnd, WM_COMMAND, ID_TOOLS_RESETDIRECTDRAW, 0);
				break;

			case KEY_F10:
				PostMessage(hWnd, WM_CLOSE, 0, 0);
				break;

			case KEY_F11:
				if (!paused)
					TheC64->NMI();
				break;

			case KEY_F12:
				if (!paused)
					TheC64->Reset();
				break;

			case KEY_FIRE:
				joystate &= ~0x10;
				break;

			case KEY_JUP:
				joystate |=  0x02;
				joystate &= ~0x01;
				break;

			case KEY_JDN:
				joystate |=  0x01;
				joystate &= ~0x02;
				break;

			case KEY_JLF:
				joystate |=  0x08;
				joystate &= ~0x04;
				break;

			case KEY_JRT:
				joystate |=  0x04;
				joystate &= ~0x08;
				break;

			case KEY_JUPLF:
				joystate |=  0x0a;
				joystate &= ~0x05;
				break;

			case KEY_JUPRT:
				joystate |=  0x06;
				joystate &= ~0x09;
				break;

			case KEY_JDNLF:
				joystate |=  0x09;
				joystate &= ~0x06;
				break;

			case KEY_JDNRT:
				joystate |=  0x05;
				joystate &= ~0x0a;
				break;

			case KEY_CENTER:
				joystate |=  0x0f;
				break;

			default:
				if (kc < 0 || kc >= 256)
					break;
				if (keystate[kc])
					break;
				keystate[kc] = 1;
				int c64_byte = kc >> 3;
				int c64_bit = kc & 7;
				int shifted = kc & 128;
				c64_byte &= 7;
				if (shifted) {
					key_matrix[6] &= 0xef;
					rev_matrix[4] &= 0xbf;
				}
				key_matrix[c64_byte] &= ~(1 << c64_bit);
				rev_matrix[c64_bit] &= ~(1 << c64_byte);
				break;
			}
			return 0;
		}
		break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		Debug("Display::WindowProc: KEYUP: 0x%x\n", wParam);
		{
			int kc = VirtKey2C64(wParam, lParam);
			switch (kc) {

			case KEY_FIRE:
				joystate |= 0x10;
				break;

			case KEY_JUP:
				joystate |= 0x01;
				break;

			case KEY_JDN:
				joystate |= 0x02;
				break;

			case KEY_JLF:
				joystate |= 0x04;
				break;

			case KEY_JRT:
				joystate |= 0x08;
				break;

			case KEY_JUPLF:
				joystate |= 0x05;
				break;

			case KEY_JUPRT:
				joystate |= 0x09;
				break;

			case KEY_JDNLF:
				joystate |= 0x06;
				break;

			case KEY_JDNRT:
				joystate |= 0x0a;
				break;

			default:
				if (kc < 0 || kc >= 256)
					break;
				if (!keystate[kc])
					break;
				keystate[kc] = 0;
				int c64_byte = kc >> 3;
				int c64_bit = kc & 7;
				int shifted = kc & 128;
				c64_byte &= 7;
				if (shifted) {
					key_matrix[6] |= 0x10;
					rev_matrix[4] |= 0x40;
				}
				key_matrix[c64_byte] |= (1 << c64_bit);
				rev_matrix[c64_bit] |= (1 << c64_byte);
				break;
			}
			return 0;
		}
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

int C64Display::VirtKey2C64(int virtkey, DWORD keydata)
{
	int ext = keydata & 0x01000000;
	int sc = (keydata & 0x00ff0000) >> 16;
	int result = -1;

	switch (virtkey) {

	case VK_NUMPAD0: numlock = 1; return KEY_FIRE;
	case VK_NUMPAD1: numlock = 1; return KEY_JDNLF;
	case VK_NUMPAD2: numlock = 1; return KEY_JDN;
	case VK_NUMPAD3: numlock = 1; return KEY_JDNRT;
	case VK_NUMPAD4: numlock = 1; return KEY_JLF;
	case VK_NUMPAD5: numlock = 1; return KEY_CENTER;
	case VK_NUMPAD6: numlock = 1; return KEY_JRT;
	case VK_NUMPAD7: numlock = 1; return KEY_JUPLF;
	case VK_NUMPAD8: numlock = 1; return KEY_JUP;
	case VK_NUMPAD9: numlock = 1; return KEY_JUPRT;

	case VK_NUMLOCK: return KEY_NUMLOCK;
	case VK_MULTIPLY: return KEY_KPMULT;
	case VK_DIVIDE: return KEY_KPDIV;
	case VK_SUBTRACT: return KEY_KPMINUS;
	case VK_ADD: return KEY_KPPLUS;
	case VK_DECIMAL: return KEY_KPPERIOD;

	case VK_F9: return KEY_F9;
	case VK_F10: return KEY_F10;
	case VK_F11: return KEY_F11;
	case VK_F12: return KEY_F12;
	case VK_PAUSE: return KEY_PAUSE;

	case VK_BACK: return MATRIX(0,0);
	case VK_DELETE: return ext ? MATRIX(0,0) : /*KP*/ KEY_KPPERIOD;
	case VK_TAB: return -1;
	case VK_RETURN:
		if ((GetKeyState(VK_MENU) & 0x8000))
			return KEY_ALTENTER;
		if ((GetKeyState(VK_CONTROL) & 0x8000))
			return KEY_CTRLENTER;
		return ext ? /*KP*/ MATRIX(0,1) : MATRIX(0,1);
	case VK_SPACE: return MATRIX(7,4);
	case VK_ESCAPE: return MATRIX(7,7);
	case VK_INSERT: if (!ext) numlock = 0; return ext ? MATRIX(0,0) | 0x80 : /*KP*/ KEY_FIRE;
	case VK_HOME: if (!ext) numlock = 0; return ext ? MATRIX(6,3) : /*KP*/ KEY_JUPLF;
	case VK_END: if (!ext) numlock = 0; return ext ? MATRIX(6,0) : /*KP*/ KEY_JDNLF;
	case VK_PRIOR: if (!ext) numlock = 0; return ext ? MATRIX(6,6) : /*KP*/ KEY_JUPRT;
	case VK_NEXT: if (!ext) numlock = 0; return ext ? MATRIX(6,5) : /*KP*/ KEY_JDNRT;
	case VK_CLEAR: return KEY_CENTER;

	case VK_SHIFT: return sc == 0x36 ? /*R*/ MATRIX(6,4) : MATRIX(1,7);
	case VK_CONTROL: return ext ? /*R*/ MATRIX(7,5) : MATRIX(7,2);
	case VK_MENU: return ext ? /*R*/ MATRIX(7,5) : MATRIX(7,5);

	case VK_UP: if (!ext) numlock = 0; return ext ? MATRIX(0,7) | 0x80 : /*KP*/ KEY_JUP;
	case VK_DOWN: if (!ext) numlock = 0; return ext ? MATRIX(0,7) : /*KP*/ KEY_JDN;
	case VK_LEFT: if (!ext) numlock = 0; return ext ? MATRIX(0,2) | 0x80 : /*KP*/ KEY_JLF;
	case VK_RIGHT: if (!ext) numlock = 0; return ext ? MATRIX(0,2) : /*KP*/ KEY_JRT;

	case VK_F1: return MATRIX(0,4);
	case VK_F2: return MATRIX(0,4) | 0x80;
	case VK_F3: return MATRIX(0,5);
	case VK_F4: return MATRIX(0,5) | 0x80;
	case VK_F5: return MATRIX(0,6);
	case VK_F6: return MATRIX(0,6) | 0x80;
	case VK_F7: return MATRIX(0,3);
	case VK_F8: return MATRIX(0,3) | 0x80;

	case '0': return MATRIX(4,3);
	case '1': return MATRIX(7,0);
	case '2': return MATRIX(7,3);
	case '3': return MATRIX(1,0);
	case '4': return MATRIX(1,3);
	case '5': return MATRIX(2,0);
	case '6': return MATRIX(2,3);
	case '7': return MATRIX(3,0);
	case '8': return MATRIX(3,3);
	case '9': return MATRIX(4,0);

	case VK_bracketleft: return MATRIX(5,6);
	case VK_bracketright: return MATRIX(6,1);
	case VK_slash: return MATRIX(6,7);
	case VK_semicolon: return MATRIX(5,5);
	case VK_grave: return MATRIX(7,1);
	case VK_minus: return MATRIX(5,0);
	case VK_equal: return MATRIX(5,3);
	case VK_comma: return MATRIX(5,7);
	case VK_period: return MATRIX(5,4);
	case VK_quote: return MATRIX(6,2);
	case VK_backslash: return MATRIX(6,6);

	case 'A': result = MATRIX(1,2); break;
	case 'B': result = MATRIX(3,4); break;
	case 'C': result = MATRIX(2,4); break;
	case 'D': result = MATRIX(2,2); break;
	case 'E': result = MATRIX(1,6); break;
	case 'F': result = MATRIX(2,5); break;
	case 'G': result = MATRIX(3,2); break;
	case 'H': result = MATRIX(3,5); break;
	case 'I': result = MATRIX(4,1); break;
	case 'J': result = MATRIX(4,2); break;
	case 'K': result = MATRIX(4,5); break;
	case 'L': result = MATRIX(5,2); break;
	case 'M': result = MATRIX(4,4); break;
	case 'N': result = MATRIX(4,7); break;
	case 'O': result = MATRIX(4,6); break;
	case 'P': result = MATRIX(5,1); break;
	case 'Q': result = MATRIX(7,6); break;
	case 'R': result = MATRIX(2,1); break;
	case 'S': result = MATRIX(1,5); break;
	case 'T': result = MATRIX(2,6); break;
	case 'U': result = MATRIX(3,6); break;
	case 'V': result = MATRIX(3,7); break;
	case 'W': result = MATRIX(1,1); break;
	case 'X': result = MATRIX(2,7); break;
	case 'Y': result = MATRIX(3,1); break;
	case 'Z': result = MATRIX(1,4); break;

	}

	if (result != -1 && GetKeyState(VK_CAPITAL))
		result |= 0x80;

	return result;
}

BOOL C64Display::SetupWindow()
{
	// Setup the window.
	SetupWindowMode(full_screen);

	UpdateWindow(hwnd);

	if (full_screen)
		ShowCursor(FALSE);

	return TRUE;
}

BOOL C64Display::SetupWindowMode(BOOL full_screen_mode)
{
	DWORD style;
	int x0, y0, x, y;
	if (full_screen_mode) {
		style = fullscreen_style;
		x0 = 0;
		y0 = 0;
		x = GetSystemMetrics(SM_CXSCREEN);
		y = GetSystemMetrics(SM_CYSCREEN);
	}
	else {
		style = windowed_style;
		x0 = rcLast.left;
		y0 = rcLast.top;
		x = rcLast.right - rcLast.left;
		y = rcLast.bottom - rcLast.top;
	}
	SetWindowLong(hwnd, GWL_STYLE, style);
	SetWindowPos(hwnd, NULL, x0, y0, x, y, SWP_NOZORDER | SWP_NOACTIVATE);
	SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
	GetClientRect(hwnd, &rcWindow);
	if (!full_screen_mode && ThePrefs.ShowLEDs)
		rcWindow.bottom -= led_rows;
	ClientToScreen(hwnd, (LPPOINT) &rcWindow);
	ClientToScreen(hwnd, (LPPOINT) &rcWindow + 1);

	// Windowed mode has a menu, full screen mode doesn't.
	HMENU old_menu = GetMenu(hwnd);
	if (old_menu) {
		SetMenu(hwnd, NULL);
		DestroyMenu(old_menu);
	}
	if (!full_screen_mode) {
		HMENU new_menu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAIN_MENU));
		SetMenu(hwnd, new_menu);
	}

	return TRUE;
}

BOOL C64Display::RestoreWindow()
{
	if (full_screen)
		ShowCursor(TRUE);

	if (!full_screen)
		GetWindowRect(hwnd, &rcLast);

	SetupWindowMode(FALSE);

	return TRUE;
}

HRESULT CALLBACK C64Display::EnumModesCallback(LPDDSURFACEDESC pDDSD, LPVOID lpContext)
{
	C64Display *pDisplay = (C64Display *) lpContext;
	return pDisplay->EnumModesCallback(pDDSD);
}

HRESULT C64Display::EnumModesCallback(LPDDSURFACEDESC pDDSD)
{
	DisplayMode mode;
	mode.x = pDDSD->dwWidth;
	mode.y = pDDSD->dwHeight;
	mode.depth = pDDSD->ddpfPixelFormat.dwRGBBitCount;
	mode.modex = (pDDSD->ddsCaps.dwCaps & DDSCAPS_MODEX) != 0;
	Debug("EnumModesCallback: %dx%dx%d (modex: %d)\n",
		mode.x, mode.y, mode.depth, mode.modex);
	if (display_modes == NULL)
		display_modes = new DisplayMode[max_display_modes];
	if (num_display_modes == max_display_modes) {
		int old_max = max_display_modes;
		max_display_modes *= 2;
		DisplayMode *new_modes = new DisplayMode[max_display_modes];
		memcpy(new_modes, display_modes, sizeof(DisplayMode)*old_max);
		delete[] display_modes;
		display_modes = new_modes;
	}
	display_modes[num_display_modes++] = mode;
	return DDENUMRET_OK;
}

int C64Display::CompareModes(const void *e1, const void *e2)
{
	DisplayMode *m1 = (DisplayMode *) e1;
	DisplayMode *m2 = (DisplayMode *) e2;
	if (m1->depth != m2->depth)
		return m1->depth - m2->depth;
	if (m1->x != m2->x)
		return m1->x - m2->x;
	if (m1->y != m2->y)
		return m1->y - m2->y;
	if (m1->modex != m2->modex)
		return int(m1->modex) - int(m2->modex);
	return 0;
}

BOOL C64Display::StartDirectDraw()
{
	// Setup our window size, position, style, etc.
	SetupWindow();

	// Create the main DirectDraw object.
	HRESULT ddrval = DirectDrawCreate(NULL, &pDD, NULL);
	if (ddrval != DD_OK) {
		DebugResult("DirectDrawCreate failed", ddrval);
		return Fail("Failed to initialize direct draw.");
	}

	if (full_screen) {

		// Set exclusive mode.
		ddrval = pDD->SetCooperativeLevel(hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWMODEX);
		if (ddrval != DD_OK) {
			DebugResult("SetCooperativeLevel failed", ddrval);
			return Fail("Failed to set exclusive cooperative level.");
		}

		if (!display_modes) {

			// Get all available video modes and sort them.
			num_display_modes = 0;
			pDD->EnumDisplayModes(0, NULL, this, EnumModesCallback);
			qsort(display_modes, num_display_modes, sizeof(DisplayMode), CompareModes);
		}

		// Set the video mode.
		const char *display_mode = ThePrefs.DisplayMode;
		if (display_mode[0] == '\0' ||
		    stricmp(display_mode, "Default") == 0)
			display_mode = NULL;
		if (display_mode) {
			int x, y, depth = 8;
			if (sscanf(display_mode, "%dx%dx%d", &x, &y, &depth) < 2)
				return Fail("Invalid command line mode format.");
			ddrval = pDD->SetDisplayMode(x, y, depth);
			if (ddrval != DD_OK) {
				DebugResult("SetDisplayMode failed", ddrval);
				return Fail("Failed to set the video mode.");
			}
		}
		else {
			for (int i = 0; i < num_display_modes; i++) {
				DisplayMode *mode = &display_modes[i];
				if (mode->x < view_x || mode->y < view_y)
					continue;
				ddrval = pDD->SetDisplayMode(mode->x, mode->y, mode->depth);
				if (ddrval == DD_OK)
					break;
			}
			if (i == num_display_modes)
				return Fail("Failed to find a suitable video mode.");
		}
	}
	else {

		// Set normal mode.
		ddrval = pDD->SetCooperativeLevel(hwnd, DDSCL_NORMAL);
		if (ddrval != DD_OK)
			return Fail("Failed to set normal cooperative level.");
	}

	// Create the primary surface with one back buffer.
	DDSURFACEDESC ddsd;
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
	ddsd.dwBackBufferCount = 1;
	ddrval = pDD->CreateSurface(&ddsd, &pPrimary, NULL);
	if (ddrval != DD_OK) {
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		ddrval = pDD->CreateSurface(&ddsd, &pPrimary, NULL);
		if (ddrval != DD_OK)
			return Fail("Failed to create primary surface.");
	}

	if (ddsd.dwBackBufferCount == 1) {
		DDSCAPS ddscaps;
		ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
		ddrval = pPrimary->GetAttachedSurface(&ddscaps, &pBack);
		if (ddrval != DD_OK)
			return Fail("Failed to get attached surface.");
	}

	// Create work surface.  It displays correctly without
	// this but doesn't handle clipping.  We would have to
	// do that ourselves.
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	if (ThePrefs.SystemMemory)
		ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
	ddsd.dwHeight = DISPLAY_Y;
	ddsd.dwWidth = DISPLAY_X;
	ddrval = pDD->CreateSurface(&ddsd, &pWork, NULL);
	if (ddrval != DD_OK) {
		//return Fail("Failed to create work surface.");
		Debug("cannot create work surface: %d\n", ddrval);
	}
	if (pWork) {
		pWork->GetCaps(&ddsd.ddsCaps);
		if (ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
			Debug("Work surface is in video memory.\n");
		else if (ddsd.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
			Debug("Work surface is in system memory.\n");
		else
			Debug("Work surface is in unknown memory.\n");
	}

	if (!full_screen) {

		// Create clipper object.
		ddrval = pDD->CreateClipper(0, &pClipper, NULL);
		if (ddrval != DD_OK)
			return Fail("Failed to create direct draw clipper.");
		ddrval = pClipper->SetHWnd(0, hwnd);
		if (ddrval != DD_OK)
			return Fail("Failed setting clipper window handle.");
		ddrval = pPrimary->SetClipper(pClipper);
		if (ddrval != DD_OK)
			return Fail("Failed setting primary surface clipper.");
	}

	// We need to use a 256 color palette otherwise we get an
	// invalid pixel format error when trying to set the palette
	// on a windowed surface.
	PALETTEENTRY ape[256];
	HDC hdc = GetDC(NULL);
	int entries = GetSystemPaletteEntries(hdc, 0, 256, ape);
	ReleaseDC(NULL, hdc);
	if (entries != 256) {
		Debug("failed to get 256 system palette entries: %d (%d)\n",
			entries, GetLastError());

		// Build a 332 palette as the default.  This makes it easy for
		// other apps to find colors when they aren't the foreground.
		for (int i = 0; i < 256; i++) {
			ape[i].peRed   = (BYTE)(((i >> 5) & 0x07) * 255 / 7);
			ape[i].peGreen = (BYTE)(((i >> 2) & 0x07) * 255 / 7);
			ape[i].peBlue  = (BYTE)(((i >> 0) & 0x03) * 255 / 3);
			ape[i].peFlags = 0;
		}
	}

	// Now override the first 16 entries with the C64 colors.
	// If we were really obsessive we could try to find the
	// nearest matches and replace them instead.
	for (int i = 0; i < 16; i++) {
		ape[i].peRed = palette_red[i];
		ape[i].peGreen = palette_green[i];
		ape[i].peBlue = palette_blue[i];
		ape[i].peFlags = 0;
	}

	// Create the palette and set it on all surfaces.
	ddrval = pDD->CreatePalette(DDPCAPS_8BIT, ape, &pPalette, NULL);
	if (ddrval != DD_OK)
		return Fail("Failed to create palette.");
	if (!SetPalettes())
		return Fail("Failed to set palettes.");
	if (!BuildColorTable())
		return Fail("Failed to build color table.");

	// Start with a clean slate.
	if (!EraseSurfaces()) {
		// Some display drivers have bugs, I guess.
		// What's a little problem erasing gonna hurt.
#if 0
		return Fail("Failed to erase surfaces.");
#endif
	}


	return TRUE;
}

BOOL C64Display::ResumeDirectDraw()
{
	if (!RestoreSurfaces())
		ResetDirectDraw();

	return TRUE;
}

BOOL C64Display::ResetDirectDraw()
{
	Pause();
	StopDirectDraw();
	StartDirectDraw();
	Resume();
	if (paused)
		Update();

	return TRUE;
}

BOOL C64Display::StopDirectDraw()
{
	if (pDD != NULL) {
		if (pClipper != NULL) {
			pClipper->Release();
			pClipper = NULL;
		}
		if (pWork != NULL) {
			pWork->Release();
			pWork = NULL;
		}
		if (pBack != NULL) {
			pBack->Release();
			pBack = NULL;
		}
		if (pPrimary != NULL) {
			pPrimary->Release();
			pPrimary = NULL;
		}
		if (pPalette != NULL) {
			pPalette->Release();
			pPalette = NULL;
		}
		pDD->RestoreDisplayMode();
		pDD->Release();
		pDD = NULL;
	}

	// Restore windowing state, window position, etc.
	RestoreWindow();

	return TRUE;
}


/*
 * This function is called if the initialization function fails
 */
BOOL C64Display::Fail(const char *error)
{
	Debug(error);
	Debug("\n");
	strcpy(failure_message, error);
	return FALSE;
}


BOOL C64Display::SetPalettes()
{
	// Only try to set palettes when in 256 color mode.
	HDC hdc = GetDC(NULL);
	int depth = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
	ReleaseDC(NULL, hdc);
	if (depth != 8)
		return TRUE;

	// Set palette on primary surface.
	HRESULT ddrval = pPrimary->SetPalette(pPalette);
	if (ddrval == DDERR_SURFACELOST) {
		pPrimary->Restore();
		ddrval = pPrimary->SetPalette(pPalette);
	}
	if (ddrval == DDERR_NOT8BITCOLOR)
		return TRUE;
	if (ddrval != DD_OK) {
		DebugResult("failed to set palette on primary", ddrval);
		return FALSE;
	}

	// Set palette on back surface.
	if (pBack) {
		FlipSurfaces();
		pPrimary->SetPalette(pPalette);
		if (ddrval == DDERR_SURFACELOST) {
			pPrimary->Restore();
			ddrval = pPrimary->SetPalette(pPalette);
		}
		if (ddrval != DD_OK) {
			DebugResult("failed to set palette on back", ddrval);
			return FALSE;
		}
	}

	// Set palette on work surface.
	if (pWork) {
		ddrval = pWork->SetPalette(pPalette);
		if (ddrval == DDERR_SURFACELOST) {
			pWork->Restore();
			ddrval = pWork->SetPalette(pPalette);
		}
		if (ddrval != DD_OK) {
			DebugResult("failed to set palette on work", ddrval);
			return FALSE;
		}
	}

	return TRUE;
}

BOOL C64Display::BuildColorTable()
{
	if (!pPrimary)
		return FALSE;

	// Determine the physical colors corresponding to the 16 C64 colors.
	for (int j = 0; j < 16; j++) {

		// Compute the true color in RGB format.
		int red = palette_red[j];
		int green = palette_green[j];
		int blue = palette_blue[j];
		COLORREF rgb = RGB(red, green, blue);

		// Set pixel(0, 0) to that value.
		LPDIRECTDRAWSURFACE pSurface = pBack ? pBack : pPrimary;
		HDC hdc;
		if (pSurface->GetDC(&hdc) != DD_OK)
			return Fail("Failed getting direct draw device context.");
		COLORREF new_rgb = SetPixel(hdc, 0, 0, PALETTERGB(red, green, blue));
		Debug("new: %.8x, old %.8x\n", new_rgb, rgb);
		pSurface->ReleaseDC(hdc);

		// Read the physical color from linear memory.
		DDSURFACEDESC ddsd;
		ddsd.dwSize = sizeof(ddsd);
		HRESULT ddrval;
		for (;;) {
			ddrval = pSurface->Lock(NULL, &ddsd, 0, NULL);
			if (ddrval != DDERR_WASSTILLDRAWING)
				break;
		}
		if (ddrval != DD_OK)
			return Fail("Failed to lock surface.");
		colors_depth = ddsd.ddpfPixelFormat.dwRGBBitCount;
		DWORD dw = *(DWORD *) ddsd.lpSurface;
		Debug("DWORD = %.8x, depth = %d\n", dw, colors_depth);
		if (colors_depth != 32)
			dw &= (1 << colors_depth) - 1;
		pSurface->Unlock(NULL);

		// Store the physical color in the colors array.
		colors[j] = dw;
		Debug("colors[%d] = %d\n", j, dw);
	}

	// Replicate the physical colors into the rest of the color array.
	for (int k = 16; k < 256; k++)
		colors[k] = colors[k & 0x0f];

	// Tell the VIC all about it;
	if (!in_constructor)
		TheC64->TheVIC->ReInitColors();

	return TRUE;
}

/*
 *  Redraw bitmap using double buffering when possible.
 */

void C64Display::Update()
{
	TIMESCOPE(ts0, "Update");

	//Debug("Display::Update\n");

	if (full_screen && !active)
		return;

	if (!pPrimary)
		return;

#ifdef WORKBUFFER_BITMAP
	// Special case for using the workbuffer as a bitmap.
	if (workbuffer_bitmap) {
		if (workbuffer_locked) {
			pWork->Unlock(NULL);
			workbuffer_locked = FALSE;
		}
		RECT rc;
		rc.left = (DISPLAY_X - view_x)/2;
		rc.top = (DISPLAY_Y - view_y)/2 - 1;
		if (rc.top < 0)
			rc.top = 0;
		rc.right = rc.left + view_x;
		rc.bottom = rc.top + view_y;
		CopySurface(rc);
		draw_leds();
		return;

	}
#endif

	// Work on the backing surface unless there isn't one.
	// We'll flip to it when we're done.
	LPDIRECTDRAWSURFACE pSurface = pBack ? pBack : pPrimary;

	// Use a work surface when we have to:
	// * when always copy is on
	// * when possibly clipped
	// * when streching
	// * when partially offscreen

	if (!full_screen && pWork) {
		if (ThePrefs.AlwaysCopy || !active || paused ||
#if 0
		    GetForegroundWindow() != hwnd ||
#endif
		    rcWindow.right - rcWindow.left != view_x ||
		    rcWindow.bottom - rcWindow.top != view_y ||
		    rcWindow.left < rcScreen.left ||
		    rcWindow.top < rcScreen.top ||
		    rcWindow.right > rcScreen.right ||
		    rcWindow.bottom > rcScreen.bottom) {
			pSurface = pWork;
			//Debug("using work surface\n");
		}
	}

	// Lock the surface.
	DDSURFACEDESC ddsd;
	ddsd.dwSize = sizeof(ddsd);

	for (;;) {
		HRESULT ddrval = pSurface->Lock(NULL, &ddsd, 0, NULL);
		if (ddrval == DD_OK)
			break;
		if (ddrval == DDERR_SURFACELOST) {
			Debug("surface lost\n");
			if (pSurface == pWork)
				ddrval = pWork->Restore();
			else
				ddrval = pPrimary->Restore();
			if (ddrval != DD_OK) {
				DebugResult("surface Restore failed", ddrval);
				return;
			}
			EraseSurfaces();
			BuildColorTable();
		}
		else if (ddrval != DDERR_WASSTILLDRAWING) {
			if (pWork && pSurface != pWork)
				pSurface = pWork;
			else {
				DebugResult("surface Lock failed", ddrval);
				return;
			}
		}
		Debug("was still drawing\n");
	}

	// Compute the optimal placement of our window depending on
	// the screen dimensions.
	int x_off, y_off;
	int x_beg, y_beg;
	int x_siz, y_siz;

	// XXX: Do these calculations only when the parameters change.
	if (full_screen) {
		if (rcWindow.right >= view_x) {
			x_off = (rcWindow.right - view_x)/2;
			x_beg = (DISPLAY_X - view_x)/2;
			x_siz = view_x;
		}
		else {
			x_off = 0;
			x_beg = (DISPLAY_X - rcWindow.right)/2;
			x_siz = rcWindow.right;
		}
		if (rcWindow.bottom >= view_y) {
			y_off = (rcWindow.bottom - view_y)/2;
			y_beg = (DISPLAY_Y - view_y)/2 - 1;
			y_siz = view_y;
		}
		else {
			y_off = 0;
			y_beg = (DISPLAY_Y - rcWindow.bottom)/2 - 1;
			y_siz = rcWindow.bottom;
		}
	}
	else {
		if (pSurface == pWork) {
			x_off = 0;
			y_off = 0;
		}
		else {
			x_off = rcWindow.left;
			y_off = rcWindow.top;
		}
		x_beg = (DISPLAY_X - view_x)/2;
		y_beg = (DISPLAY_Y - view_y)/2 - 1;
		x_siz = view_x;
		y_siz = view_y;
	}
	if (y_beg < 0)
		y_beg = 0;

	// Translate chunky colors into the surface's linear memory.
	int pitch = ddsd.lPitch;
	int depth = ddsd.ddpfPixelFormat.dwRGBBitCount;
	BYTE *surface = (BYTE *) ddsd.lpSurface + pitch*y_off + x_off*(depth/8);
	BYTE *chunky = chunky_buf + DISPLAY_X*y_beg + x_beg;

	// These tight loops are where the display speed action is at.
	// Note that MSVC optimizes out the mulitiplications and
	// reverses the direction of the loop counters automatically.
	if (depth == 8) {

		// Since the VIC is using our palette entries we just copy.
		//TIMESCOPE(ts1, "hand blt 8");
		BYTE *scanline = surface;
		BYTE *scanbuf = chunky;
		//Debug("scanline = %8p, scanbuf = %8p\n", scanline, scanbuf);
		for (int j = 0; j < y_siz; j++) {
			memcpy(scanline, scanbuf, x_siz);
			scanline += pitch;
			scanbuf += DISPLAY_X;
		}
	}
	else if (depth == 16) {
		//TIMESCOPE(ts1, "hand blt 16");
		for (int j = 0; j < y_siz; j++) {
			WORD *scanline = (WORD *) (surface + pitch*j);
			BYTE *scanbuf = chunky + +DISPLAY_X*j;
			for (int i = 0; i < x_siz; i++)
				*scanline++ = (WORD) colors[*scanbuf++];
		}
	}
	else if (depth == 24) {

		// XXX: Works for little-endian only.
		//TIMESCOPE(ts1, "hand blt 24");
		for (int j = 0; j < y_siz; j++) {
			BYTE *scanline = surface + pitch*j;
			BYTE *scanbuf = chunky + +DISPLAY_X*j;
			for (int i = 0; i < x_siz; i++) {
				*((DWORD *) scanline) = colors[*scanbuf++];
				scanline += 3;
			}
		}
	}
	else if (depth == 32) {
		//TIMESCOPE(ts1, "hand blt 32");
		for (int j = 0; j < y_siz; j++) {
			DWORD *scanline = (DWORD *) (surface + pitch*j);
			BYTE *scanbuf = chunky + +DISPLAY_X*j;
			for (int i = 0; i < x_siz; i++)
				*scanline++ = colors[*scanbuf++];
		}
	}
	else
		Debug("PixelCount not 8, 16, 24, or 32\n");

	// Unlock the surface.
	HRESULT ddrval = pSurface->Unlock(NULL);
	if (ddrval != DD_OK)
		Debug("DirectDrawSurface::Unlock failed\n");

	// Now flip from the primary surface to the backing surface.
	if (pSurface == pWork)
		CopySurface(rcWork);
	else if (full_screen && pBack)
		FlipSurfaces();

	// Update drive LEDs
	draw_leds();
}


BOOL C64Display::CopySurface(RECT &rcWork)
{
	// Copy work surface to primary.
	for (;;) {
		HRESULT ddrval = pPrimary->Blt(&rcWindow, pWork, &rcWork, DDBLT_WAIT, NULL);
		if (ddrval == DD_OK)
			break;
		if (ddrval == DDERR_SURFACELOST) {
			ddrval = pPrimary->Restore();
			if (ddrval != DD_OK) {
				DebugResult("CopySurface Restore failed", ddrval);
				return FALSE;
			}
		}
		else if (ddrval != DDERR_WASSTILLDRAWING) {
			DebugResult("CopySurface Blt failed", ddrval);
			return FALSE;
		}
	}
	return TRUE;
}

BOOL C64Display::FlipSurfaces()
{
	// Flip buffers.
	for (;;) {
		HRESULT ddrval = pPrimary->Flip(NULL, 0);
		if (ddrval == DD_OK)
			break;
		if (ddrval == DDERR_SURFACELOST) {
			ddrval = pPrimary->Restore();
			if (ddrval != DD_OK) {
				Debug("Restore failed\n");
				return FALSE;
			}
		}
		else if (ddrval != DDERR_WASSTILLDRAWING)
			return FALSE;
	}
	return TRUE;
}

BOOL C64Display::EraseSurfaces()
{
	DDBLTFX ddbltfx;
	ddbltfx.dwSize = sizeof(ddbltfx);
	ddbltfx.dwFillColor = 0;

	// Erase the backing surface.
	for (;;) {
		if (!pBack)
			break;
		HRESULT ddrval = pBack->Blt(&rcWindow, NULL, NULL, DDBLT_COLORFILL, &ddbltfx);

		if (ddrval == DD_OK)
			break;

		if (ddrval == DDERR_SURFACELOST) {
			ddrval = pPrimary->Restore();
			if (ddrval != DD_OK) {
				DebugResult("Restore primary failed", ddrval);
				return FALSE;
			}
		}
		else if (ddrval != DDERR_WASSTILLDRAWING) {
			DebugResult("Blt erase back failed", ddrval);
			return FALSE;
		}
	}

	// Erase the primary surface.
	for (;;) {
		HRESULT ddrval = pPrimary->Blt(&rcWindow, NULL, NULL, DDBLT_COLORFILL, &ddbltfx);

		if (ddrval == DD_OK)
			break;

		if (ddrval == DDERR_SURFACELOST) {
			ddrval = pPrimary->Restore();
			if (ddrval != DD_OK) {
				DebugResult("Restore primary failed", ddrval);
				return FALSE;
			}
		}
		else if (ddrval != DDERR_WASSTILLDRAWING) {
			DebugResult("Blt erase primary failed", ddrval);
			return FALSE;
		}
	}

	return TRUE;
}

BOOL C64Display::RestoreSurfaces()
{
	if (pPrimary) {
		HRESULT ddrval = pPrimary->Restore();
		if (ddrval != DD_OK)
			return FALSE;
	}

	if (pWork) {
		HRESULT ddrval = pWork->Restore();
		if (ddrval != DD_OK)
			return FALSE;
	}

	return TRUE;
}

/*
 *  Draw LED bar at the bottom of the window
 */

void C64Display::draw_led_bar()
{
	if (full_screen || !ThePrefs.ShowLEDs)
		return;

	HDC hdc = GetDC(hwnd);
	RECT rc;
	GetClientRect(hwnd, &rc);
	rc.top = rc.bottom - led_rows;
	FillRect(hdc, &rc, led_brush);
	if (rc.right - rc.left > view_x)
		rc.left = rc.right - view_x;
	SelectObject(hdc, led_font);
	SetTextAlign(hdc, TA_TOP | TA_RIGHT);
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, (COLORREF) GetSysColor(COLOR_MENUTEXT));
	for (int i = 0; i < 4; i++) {
		char str[128];
		if (rc.right - rc.left < view_x)
			sprintf(str, "%d", i + 8);
		else
			sprintf(str, "Drive %d", i + 8);
		RECT led;
		led_rect(i, rc, led);
		SelectObject(hdc, led_shadow);
		MoveToEx(hdc, led.left - 1, led.bottom - 1, NULL);
		LineTo(hdc, led.left - 1, led.top - 1);
		LineTo(hdc, led.right, led.top - 1);
		SelectObject(hdc, led_highlight);
		LineTo(hdc, led.right, led.bottom);
		LineTo(hdc, led.left - 2, led.bottom);
		TextOut(hdc, led.left - 4, rc.top + 2, str, strlen(str));
	}
	ReleaseDC(hwnd, hdc);
	draw_leds(TRUE);
}

/*
 *  Draw one LED
 */

void C64Display::draw_leds(BOOL force)
{
	if (full_screen || !ThePrefs.ShowLEDs)
		return;

	if (!force) {
		int i;
		for (i = 0; i < 4; i++) {
			if (led_state[i] != old_led_state[i])
				break;
		}
		if (i == 4)
			return;
	}

	HDC hdc = GetDC(hwnd);
	RECT rc;
	GetClientRect(hwnd, &rc);
	rc.top = rc.bottom - led_rows;
	if (rc.right - rc.left > view_x)
		rc.left = rc.right - view_x;
	for (int i = 0; i < 4; i++) {
		old_led_state[i] = led_state[i];
		HBRUSH brush;
		switch (led_state[i]) {
		case LED_OFF: brush = off_brush; break;
		case LED_ERROR_OFF: brush = error_off_brush; break;
		case LED_ON: brush = on_brush; break;
		case LED_ERROR_ON: brush = error_on_brush; break;
		}
		RECT led;
		led_rect(i, rc, led);
		FillRect(hdc, &led, brush);
	}
	ReleaseDC(hwnd, hdc);
}

void C64Display::led_rect(int n, RECT &rc, RECT &led)
{
	int x = rc.left + (rc.right - rc.left)*(n + 2)/5 - 20;
	int y = rc.top + 2 + led_rows/3;
	SetRect(&led, x, y, x + 13, y + led_rows/3);
}

void C64Display::InsertNextDisk()
{
	if (strlen(ThePrefs.DrivePath[0]) > 4) {
		char str[256];
		strcpy(str, ThePrefs.DrivePath[0]);
		char *p = str + strlen(str) - 5;

		// If path matches "*.?64", increment character before the '.'
		if (p[1] == '.' && p[3] == '6' && p[4] == '4') {
			p[0]++;

			// If no such file exists, set character before the '.' to '1', 'a' or 'A'
			FILE *file;
			if ((file = fopen(str, "rb")) == NULL) {
				if (isdigit(p[0]))
					p[0] = '1';
				else if (isupper(p[0]))
					p[0] = 'A';
				else
					p[0] = 'a';
			} else
				fclose(file);

			// Set new prefs
			Pause();
			Prefs *prefs = new Prefs(ThePrefs);
			strcpy(prefs->DrivePath[0], str);
			TheC64->NewPrefs(prefs);
			ThePrefs = *prefs;
			delete prefs;
			Resume();
		}
	}
}

BOOL C64Display::FileNameDialog(char *prefs_path, BOOL save)
{
	char filename[256];
	strcpy(filename, prefs_path);
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.hInstance = hInstance;
	ofn.lpstrFilter =
		"Preferences Files (*.fpr)\0*.fpr\0"
		"All Files (*.*)\0*.*\0"
		;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL;
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOTESTFILECREATE |
		OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_SHAREAWARE;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = "fpr";
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
	BOOL result = save ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn);
	if (result) {
		char cwd[256];
		GetCurrentDirectory(sizeof(cwd), cwd);
		int cwd_len = strlen(cwd);
		if (cwd_len > 0 && cwd[cwd_len - 1] != '\\') {
			strcat(cwd, "\\");
			cwd_len++;
		}
		if (strnicmp(filename, cwd, cwd_len) == 0)
			strcpy(prefs_path, filename + cwd_len);
		else
			strcpy(prefs_path, filename);
	}
	return result;
}

void C64Display::WindowTitle()
{
	// Show the program name, the current preferences file,
	// and the paused state or the speedometer.
	const char *prefs_path = TheApp->prefs_path;
	int prefs_path_length = strlen(prefs_path);
	if (prefs_path_length > 4 &&
	    stricmp(prefs_path + prefs_path_length - 4, ".fpr") == 0)
		prefs_path_length -= 4;
	const char *info = NULL;
	char tmp[128];
	if (waiting)
		info = "PAUSED";
	else if (!ThePrefs.ShowLEDs && speed_index != 0) {
		if (IsFrodoSC)
			sprintf(tmp, "%.1f%%", speed_index);
		else
			sprintf(tmp, "%.0f%%", speed_index);
		info = tmp;
	}
	const char *sep1 = info ? " (" : "";
	const char *sep2 = info ? ")" : "";
	char title[256];
	sprintf(title, "%s - %.*s%s%s%s", TITLE,
		prefs_path_length, prefs_path, sep1, info ? info : "", sep2);
	SetWindowText(hwnd, title);
}

void C64Display::NewPrefs()
{
	// Resize the window to the new viewport while preserving
	// as closely as possible the previous scaling factors.
	RECT rc;
	GetWindowRect(hwnd, &rc);
	int x_nc = rc.right - rc.left - (rcWindow.right - rcWindow.left);
	int y_nc = rc.bottom - rc.top - (rcWindow.bottom - rcWindow.top);
	if (show_leds)
		y_nc -= led_rows;
	double x_scale = double(rcWindow.right - rcWindow.left)/view_x;
	double y_scale = double(rcWindow.bottom - rcWindow.top)/view_y;
	if (CalcViewPort() || show_leds != ThePrefs.ShowLEDs) {
		show_leds = ThePrefs.ShowLEDs;
		rc.right = int(rc.left + x_scale*view_x + x_nc);
		rc.bottom = int(rc.top + y_scale*view_y + y_nc);
		if (show_leds)
			rc.bottom += led_rows;
		ResizeWindow(WMSZ_BOTTOMRIGHT, &rc);
		MoveWindow(hwnd, rc.left, rc.top,
			rc.right - rc.left,
			rc.bottom - rc.top, TRUE);
	}

	// The prefs filename might have changed.
	WindowTitle();
}

void C64Display::OfferSave()
{
	if (ThePrefs == ThePrefsOnDisk)
		return;
	const char *str = "Preferences have changed.\nSave preferences now?";
	int result = MessageBox(hwnd, str, "Frodo", MB_YESNO | MB_ICONQUESTION);
	if (result == IDYES)
		ThePrefs.Save(TheApp->prefs_path);
}

void C64Display::Pause()
{
	// It's not safe to call this from the contructor or destructor.
	if (in_constructor || in_destructor)
		return;

	if (paused == 0)
		TheC64->Pause();
	paused++;
}

void C64Display::Resume()
{
	// It's not safe to call this from the contructor or destructor.
	if (in_constructor || in_destructor)
		return;

	if (paused > 0) {
		paused--;
		if (!paused)
			TheC64->Resume();
	}
	else
		_ASSERTE(paused > 0);
}

void C64Display::Quit()
{
	quit = 1;
	TheC64->Quit();
}
