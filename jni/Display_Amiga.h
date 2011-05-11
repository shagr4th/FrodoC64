/*
 *  Display_Amiga.h - C64 graphics display, emulator window handling,
 *                    Amiga specific stuff
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

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/diskfont.h>
#include <proto/asl.h>

#include "C64.h"
#include "SAM.h"
#include "Version.h"


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


/*
  Tables for key translation
  Bit 0..2: row/column in C64 keyboard matrix
  Bit 3   : implicit shift
  Bit 5   : joystick emulation (bit 0..4: mask)
*/

const int key_byte[128] = {
	 7,  7,  7,  1,  1,   2,   2,   3,
	 3,  4,  4,  5,  5,   6,  -1,0x30,
	 7,  1,  1,  2,  2,   3,   3,   4,
	 4,  5,  5,  6, -1,0x26,0x22,0x2a,
	 1,  1,  2,  2,  3,   3,   4,   4,
	 5,  5,  6,  6, -1,0x24,0x30,0x28,
	 6,  1,  2,  2,  3,   3,   4,   4,
	 5,  5,  6, -1, -1,0x25,0x21,0x29,
	 7,  0, -1,  0,  0,   7,   6,  -1,
	-1, -1, -1, -1,8+0,   0,   0, 8+0,
	 0,8+0,  0,8+0,  0, 8+0,   0, 8+0,
	-1, -1,  6,  6, -1,  -1,  -1,  -1,
	 1,  6,  1,  7,  7,   7,   7,  -1,
	-1, -1, -1, -1, -1,  -1,  -1,  -1,
	-1, -1, -1, -1, -1,  -1,  -1,  -1,
	-1, -1, -1, -1, -1,  -1,  -1,  -1
};

const int key_bit[128] = {
	 1,  0,  3,  0,  3,  0,  3,  0,
	 3,  0,  3,  0,  3,  0, -1, -1,
	 6,  1,  6,  1,  6,  1,  6,  1,
	 6,  1,  6,  1, -1, -1, -1, -1,
	 2,  5,  2,  5,  2,  5,  2,  5,
     2,  5,  2,  5, -1, -1, -1, -1,
	 6,  4,  7,  4,  7,  4,  7,  4,
	 7,  4,  7, -1, -1, -1, -1, -1,
	 4,  0, -1,  1,  1,  7,  3, -1,
	-1, -1, -1, -1,  7,  7,  2,  2,
	 4,  4,  5,  5,  6,  6,  3,  3,
	-1, -1,  6,  5, -1, -1, -1, -1,
	 7,  4,  7,  2,  5,  5,  5, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1
};


/*
 *  Menu definitions
 */

const struct NewMenu new_menus[] = {
	NM_TITLE, "Frodo", NULL, 0, 0, NULL,
	NM_ITEM, "About Frodo...", NULL, 0, 0, NULL,
	NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL,
	NM_ITEM, "Preferences...", "P", 0, 0, NULL,
	NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL,
	NM_ITEM, "Reset C64", NULL, 0, 0, NULL,
	NM_ITEM, "Insert next disk", "D", 0, 0, NULL,
	NM_ITEM, "SAM...", "M", 0, 0, NULL,
	NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL,
	NM_ITEM, "Load snapshot...", "O", 0, 0, NULL,
	NM_ITEM, "Save snapshot...", "S", 0, 0, NULL,
	NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL,
	NM_ITEM, "Quit Frodo", "Q", 0, 0, NULL,
	NM_END, NULL, NULL, 0, 0, NULL
};


/*
 *  Font attributes
 */

const struct TextAttr led_font_attr = {
	"Helvetica.font", 11, FS_NORMAL, 0
};

const struct TextAttr speedo_font_attr = {
	"Courier.font", 11, FS_NORMAL, 0
};


/*
 *  Display constructor: Create window/screen
 */

C64Display::C64Display(C64 *the_c64) : TheC64(the_c64)
{
	int i;

	// LEDs off
	for (i=0; i<4; i++)
		led_state[i] = old_led_state[i] = LED_OFF;

	// Allocate chunky buffer to draw into
	chunky_buf = new UBYTE[DISPLAY_X * DISPLAY_Y];

	// Open fonts
	led_font = OpenDiskFont(&led_font_attr);
	speedo_font = OpenDiskFont(&speedo_font_attr);

	// Open window on default pubscreen
	the_window = OpenWindowTags(NULL,
		WA_Left, 0,
		WA_Top, 0,
		WA_InnerWidth, DISPLAY_X,
		WA_InnerHeight, DISPLAY_Y + 16,
		WA_Title, (ULONG)"Frodo",
		WA_ScreenTitle, (ULONG)"Frodo C64 Emulator",
		WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_RAWKEY | IDCMP_MENUPICK | IDCMP_REFRESHWINDOW,
		WA_DragBar, TRUE,
		WA_DepthGadget, TRUE,
		WA_CloseGadget, TRUE,
		WA_SimpleRefresh, TRUE,
		WA_Activate, TRUE,
		WA_NewLookMenus, TRUE,
		TAG_DONE);
	the_screen = the_window->WScreen;
	the_rp = the_window->RPort;
	xo = the_window->BorderLeft;
	yo = the_window->BorderTop;

	// Create menus
	the_visual_info = GetVisualInfo(the_screen, NULL);
	the_menus = CreateMenus(new_menus, GTMN_FullMenu, TRUE, TAG_DONE);
	LayoutMenus(the_menus, the_visual_info, GTMN_NewLookMenus, TRUE, TAG_DONE);
	SetMenuStrip(the_window, the_menus);

	// Obtain 16 pens for the C64 colors
	for (i=0; i<16; i++)
		pens[i] = ObtainBestPen(the_screen->ViewPort.ColorMap,
			palette_red[i] * 0x01010101, palette_green[i] * 0x01010101, palette_blue[i] * 0x01010101);

	// Allocate temporary RastPort for WritePixelArra8()
	temp_bm = AllocBitMap(DISPLAY_X, 1, 8, 0, NULL);
	InitRastPort(&temp_rp);
	temp_rp.BitMap = temp_bm;

	// Draw LED bar
	draw_led_bar();

	// Allocate file requesters
	open_req = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
		ASLFR_Window, (ULONG)the_window,
		ASLFR_SleepWindow, TRUE,
		ASLFR_TitleText, (ULONG)"Frodo: Load snapshot...",
		ASLFR_RejectIcons, TRUE,
		TAG_DONE);
	save_req = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
		ASLFR_Window, (ULONG)the_window,
		ASLFR_SleepWindow, TRUE,
		ASLFR_TitleText, (ULONG)"Frodo: Save snapshot...",
		ASLFR_DoSaveMode, TRUE,
		ASLFR_RejectIcons, TRUE,
		TAG_DONE);
}


/*
 *  Display destructor
 */

C64Display::~C64Display()
{
	// Free file requesters
	if (open_req != NULL)
		FreeAslRequest(open_req);
	if (save_req != NULL)
		FreeAslRequest(save_req);

	// Free temporary RastPort
	if (temp_bm != NULL)
		FreeBitMap(temp_bm);

	// Free pens
	for (int i=0; i<16; i++)
		ReleasePen(the_screen->ViewPort.ColorMap, pens[i]);

	// Delete menus
	if (the_menus != NULL) {
		if (the_window != NULL)
			ClearMenuStrip(the_window);
		FreeMenus(the_menus);
	}

	// Delete VisualInfo
	if (the_visual_info != NULL)
		FreeVisualInfo(the_visual_info);

	// Close window
	if (the_window != NULL)
		CloseWindow(the_window);

	// Close fonts
	CloseFont(speedo_font);
	CloseFont(led_font);

	// Free chunky buffer
	delete chunky_buf;
}


/*
 *  Prefs may have changed
 */

void C64Display::NewPrefs(Prefs *prefs)
{
}


/*
 *  Redraw bitmap
 */

void C64Display::Update(void)
{
	// Update C64 display
	WritePixelArray8(the_rp, xo, yo, DISPLAY_X + xo - 1, DISPLAY_Y + yo - 1,
		chunky_buf, &temp_rp);

	// Update drive LEDs
	for (int i=0; i<4; i++)
		if (led_state[i] != old_led_state[i]) {
			draw_led(i, led_state[i]);
			old_led_state[i] = led_state[i];
		}
}


/*
 *  Draw LED bar at the bottom of the window
 */

void C64Display::draw_led_bar(void)
{
	int i;
	char str[16];

	SetAPen(the_rp, pens[15]);	// Light gray
	SetBPen(the_rp, pens[15]);	// Light gray
	RectFill(the_rp, xo, yo+DISPLAY_Y, xo+DISPLAY_X-1, yo+DISPLAY_Y+15);

	SetAPen(the_rp, pens[1]);	// White
	Move(the_rp, xo, yo+DISPLAY_Y); Draw(the_rp, xo+DISPLAY_X-1, yo+DISPLAY_Y);
	for (i=0; i<5; i++) {
		Move(the_rp, xo+DISPLAY_X*i/5, yo+DISPLAY_Y); Draw(the_rp, xo+DISPLAY_X*i/5, yo+DISPLAY_Y+14);
	}
	for (i=2; i<6; i++) {
		Move(the_rp, xo+DISPLAY_X*i/5-23, yo+DISPLAY_Y+11); Draw(the_rp, xo+DISPLAY_X*i/5-9, yo+DISPLAY_Y+11);
		Move(the_rp, xo+DISPLAY_X*i/5-9, yo+DISPLAY_Y+11); Draw(the_rp, xo+DISPLAY_X*i/5-9, yo+DISPLAY_Y+5);
	}

	SetAPen(the_rp, pens[12]);	// Medium gray
	Move(the_rp, xo, yo+DISPLAY_Y+15); Draw(the_rp, xo+DISPLAY_X-1, yo+DISPLAY_Y+15);
	for (i=1; i<6; i++) {
		Move(the_rp, xo+DISPLAY_X*i/5-1, yo+DISPLAY_Y+1); Draw(the_rp, xo+DISPLAY_X*i/5-1, yo+DISPLAY_Y+15);
	}
	for (i=2; i<6; i++) {
		Move(the_rp, xo+DISPLAY_X*i/5-24, yo+DISPLAY_Y+11); Draw(the_rp, xo+DISPLAY_X*i/5-24, yo+DISPLAY_Y+4);
		Move(the_rp, xo+DISPLAY_X*i/5-24, yo+DISPLAY_Y+4); Draw(the_rp, xo+DISPLAY_X*i/5-9, yo+DISPLAY_Y+4);
	}

	SetFont(the_rp, led_font);
	for (i=0; i<4; i++) {
		sprintf(str, "Drive %d", i+8);
		SetAPen(the_rp, pens[0]);	// Black
		Move(the_rp, xo+DISPLAY_X*(i+1)/5+8, yo+DISPLAY_Y+11);
		Text(the_rp, str, strlen(str));
		draw_led(i, LED_OFF);
	}
}


/*
 *  Draw one LED
 */

void C64Display::draw_led(int num, int state)
{
	switch (state) {
		case LED_OFF:
		case LED_ERROR_OFF:
			SetAPen(the_rp, pens[0]);	// Black;
			break;
		case LED_ON:
			SetAPen(the_rp, pens[5]);	// Green
			break;
		case LED_ERROR_ON:
			SetAPen(the_rp, pens[2]);	// Red
			break;
	}
	RectFill(the_rp, xo+DISPLAY_X*(num+2)/5-23, yo+DISPLAY_Y+5, xo+DISPLAY_X*(num+2)/5-10, yo+DISPLAY_Y+10);
}


/*
 *  Update speedometer
 */

void C64Display::Speedometer(int speed)
{
	static int delay = 0;

	if (delay >= 20) {
		char str[16];
		sprintf(str, "%d%%", speed);
		SetAPen(the_rp, pens[15]);	// Light gray
		RectFill(the_rp, xo+1, yo+DISPLAY_Y+1, xo+DISPLAY_X/5-2, yo+DISPLAY_Y+14);
		SetAPen(the_rp, pens[0]);	// Black
		SetFont(the_rp, speedo_font);
		Move(the_rp, xo+24, yo+DISPLAY_Y+10);
		Text(the_rp, str, strlen(str));
		delay = 0;
	} else
		delay++;
}


/*
 *  Return pointer to bitmap data
 */

UBYTE *C64Display::BitmapBase(void)
{
	return chunky_buf;
}


/*
 *  Return number of bytes per row
 */

int C64Display::BitmapXMod(void)
{
	return DISPLAY_X;
}


/*
 *  Handle IDCMP messages
 */

void C64Display::PollKeyboard(UBYTE *key_matrix, UBYTE *rev_matrix, UBYTE *joystick)
{
	struct IntuiMessage *msg;

	// Get and analyze all pending window messages
	while ((msg = (struct IntuiMessage *)GetMsg(the_window->UserPort)) != NULL) {

		// Extract data and reply message
		ULONG iclass = msg->Class;
		USHORT code = msg->Code;
		ReplyMsg((struct Message *)msg);

		// Action depends on message class
		switch (iclass) {

			case IDCMP_CLOSEWINDOW:	// Closing the window quits Frodo
				TheC64->Quit();
				break;

			case IDCMP_RAWKEY:
				switch (code) {

					case 0x58:	// F9: NMI (Restore)
						TheC64->NMI();
						break;

					case 0x59:	// F10: Reset
						TheC64->Reset();
						break;

					case 0x5e:	// '+' on keypad: Increase SkipFrames
						ThePrefs.SkipFrames++;
						break;

					case 0x4a:	// '-' on keypad: Decrease SkipFrames
						if (ThePrefs.SkipFrames > 1)
							ThePrefs.SkipFrames--;
						break;

					case 0x5d:	// '*' on keypad: Toggle speed limiter
						ThePrefs.LimitSpeed = !ThePrefs.LimitSpeed;
						break;

					case 0x5c:{	// '/' on keypad: Toggle processor-level 1541 emulation
						Prefs *prefs = new Prefs(ThePrefs);
						prefs->Emul1541Proc = !prefs->Emul1541Proc;
						TheC64->NewPrefs(prefs);
						ThePrefs = *prefs;
						delete prefs;
						break;
					}

					default:{
						// Convert Amiga keycode to C64 row/column
						int c64_byte = key_byte[code & 0x7f];
						int c64_bit = key_bit[code & 0x7f];

						if (c64_byte != -1) {
							if (!(c64_byte & 0x20)) {

								// Normal keys
								bool shifted = c64_byte & 8;
								c64_byte &= 7;
								if (!(code & 0x80)) {

									// Key pressed
									if (shifted) {
										key_matrix[6] &= 0xef;
										rev_matrix[4] &= 0xbf;
									}
									key_matrix[c64_byte] &= ~(1 << c64_bit);
									rev_matrix[c64_bit] &= ~(1 << c64_byte);
								} else {

									// Key released
									if (shifted) {
										key_matrix[6] |= 0x10;
										rev_matrix[4] |= 0x40;
									}
									key_matrix[c64_byte] |= (1 << c64_bit);
									rev_matrix[c64_bit] |= (1 << c64_byte);
								}
							} else {

								// Joystick emulation
								c64_byte &= 0x1f;
								if (code & 0x80)
									*joystick |= c64_byte;
								else
									*joystick &= ~c64_byte;
							}
						}
					}
				}
				break;

			case IDCMP_MENUPICK:{
				if (code == MENUNULL)
					break;

				// Get item number
				int item_number = ITEMNUM(code);
				switch (item_number) {

					case 0: {	// About Frodo
						TheC64->Pause();
						char str[256];
						sprintf(str, "%s by Christian Bauer\n<Christian.Bauer@uni-mainz.de>\n© Copyright 1994-1997,2002-2005", VERSION_STRING);
						ShowRequester(str, "OK");
						TheC64->Resume();
						break;
					}

					case 2:		// Preferences
						TheC64->Pause();
						be_app->RunPrefsEditor();
						TheC64->Resume();
						break;

					case 4:		// Reset C64
						TheC64->Reset();
						break;

					case 5:		// Insert next disk
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
								Prefs *prefs = new Prefs(ThePrefs);
								strcpy(prefs->DrivePath[0], str);
								TheC64->NewPrefs(prefs);
								ThePrefs = *prefs;
								delete prefs;
							}
						}
						break;

					case 6:		// SAM
						TheC64->Pause();
						SAM(TheC64);
						TheC64->Resume();
						break;

					case 8:		// Load snapshot
						if (open_req != NULL && AslRequest(open_req, NULL)) {
							char path[256];
							strncpy(path, open_req->fr_Drawer, 255);
							AddPart(path, open_req->fr_File, 255);
							TheC64->Pause();
							TheC64->LoadSnapshot(path);
							TheC64->Resume();
						}
						break;

					case 9:		// Save snapshot
						if (save_req != NULL && AslRequest(save_req, NULL)) {
							char path[256];
							strncpy(path, save_req->fr_Drawer, 255);
							AddPart(path, save_req->fr_File, 255);
							TheC64->Pause();
							TheC64->SaveSnapshot(path);
							TheC64->Resume();
						}
						break;

					case 11:	// Quit Frodo
						TheC64->Quit();
						break;
				}
				break;
			}

			case IDCMP_REFRESHWINDOW:
				BeginRefresh(the_window);
				draw_led_bar();
				EndRefresh(the_window, TRUE);
				break;
		}
	}
}


/*
 *  Check if NumLock is down (for switching the joystick keyboard emulation)
 */

bool C64Display::NumLock(void)
{
	return FALSE;
}


/*
 *  Allocate C64 colors
 */

void C64Display::InitColors(UBYTE *colors)
{
	// Spread pens into colors array
	for (int i=0; i<256; i++)
		colors[i] = pens[i & 0x0f];
}


/*
 *  Show a requester
 */

long ShowRequester(char *str, char *button1, char *button2)
{
	struct EasyStruct es;
	char gads[256];

	strcpy(gads, button1);
	if (button2) {
		strcat(gads, "|");
		strcat(gads, button2);
	}

	es.es_StructSize = sizeof(struct EasyStruct);
	es.es_Flags = 0;
	es.es_Title = "Frodo";
	es.es_TextFormat = str;
	es.es_GadgetFormat = gads;

	return EasyRequestArgs(NULL, &es, NULL, NULL) % 1;
}
