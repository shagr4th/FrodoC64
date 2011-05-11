/*
 *  Prefs_WIN32.h - Global preferences, WIN32 specific stuff
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

#include <commctrl.h>
#include "resource.h"

Prefs *Prefs::edit_prefs;
char *Prefs::edit_prefs_name;
HWND Prefs::hDlg;

#define STANDARD_PAGE	0
#define WIN32_PAGE	1

bool Prefs::ShowEditor(bool /* startup */, char *prefs_name)
{
	edit_prefs = this;
	edit_prefs_name = prefs_name;

	PROPSHEETPAGE psp[2];

	// Set up standard preferences property page.
	psp[0].dwSize = sizeof(PROPSHEETPAGE);
	psp[0].dwFlags = PSP_HASHELP;
	psp[0].hInstance = hInstance;
	psp[0].pszTemplate = MAKEINTRESOURCE(IDD_PREFERENCES_STANDARD);
	psp[0].pszIcon = NULL;
	psp[0].pfnDlgProc = StandardDialogProc;
	psp[0].pszTitle = NULL;
	psp[0].lParam = 0;
	psp[0].pfnCallback = NULL;

	// Set up WIN32 preferences property page.
	psp[1].dwSize = sizeof(PROPSHEETPAGE);
	psp[1].dwFlags = PSP_HASHELP;
	psp[1].hInstance = hInstance;
	psp[1].pszTemplate = MAKEINTRESOURCE(IDD_PREFERENCES_WIN32);
	psp[1].pszIcon = NULL;
	psp[1].pfnDlgProc = WIN32DialogProc;
	psp[1].pszTitle = NULL;
	psp[1].lParam = 0;
	psp[1].pfnCallback = NULL;

	// Setup property sheet.
	PROPSHEETHEADER psh;
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
	psh.hwndParent = hwnd;
	psh.hInstance = hInstance;
	psh.pszIcon = NULL;
	psh.pszCaption = "Preferences";
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
	psh.nStartPage = 0;
	psh.ppsp = psp;
	psh.pfnCallback = NULL;

	int result = PropertySheet(&psh);

	if (result == -1)
		return FALSE;
	return result;
}

BOOL CALLBACK Prefs::StandardDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	return edit_prefs->DialogProc(STANDARD_PAGE, hDlg, message, wParam, lParam);
}

BOOL CALLBACK Prefs::WIN32DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	return edit_prefs->DialogProc(WIN32_PAGE, hDlg, message, wParam, lParam);
}

BOOL Prefs::DialogProc(int page, HWND hDlg_arg, UINT message, WPARAM wParam, LPARAM lParam)
{
	hDlg = hDlg_arg;

	switch (message) {

	case WM_INITDIALOG:
		SetupControls(page);
		SetValues(page);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {

		case IDC_BROWSE8:
			BrowseForDevice(IDC_DEVICE8);
			break;

		case IDC_BROWSE9:
			BrowseForDevice(IDC_DEVICE9);
			break;

		case IDC_BROWSE10:
			BrowseForDevice(IDC_DEVICE10);
			break;

		case IDC_BROWSE11:
			BrowseForDevice(IDC_DEVICE11);
			break;

		}
		return TRUE;

	case WM_NOTIFY:
		{
			NMHDR *pnmhdr = (NMHDR *) lParam;
			switch (pnmhdr->code) {

			case PSN_KILLACTIVE:
				SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
				break;

			case PSN_APPLY:
				GetValues(page);
				break;

			case PSN_HELP:
				PostMessage(hwnd, WM_COMMAND, ID_HELP_SETTINGS, 0);
				break;
			}
		}
		return TRUE;
	}

	return FALSE;
}

#define SetupSpin(id, upper, lower) SendMessage(GetDlgItem(hDlg, id), UDM_SETRANGE, 0, MAKELONG(upper, lower))
#define SetupSpinIncrement(id, increment) (udaccel.nSec = 0, udaccel.nInc = increment, SendMessage(GetDlgItem(hDlg, id), UDM_SETACCEL, 1, (LPARAM) &udaccel))
#define SetupComboClear(id) SendMessage(GetDlgItem(hDlg, id), CB_RESETCONTENT, 0, 0)
#define SetupComboAdd(id, string) SendMessage(GetDlgItem(hDlg, id), CB_ADDSTRING, 0, (LPARAM) string)

void Prefs::SetupControls(int page)
{
	UDACCEL udaccel;
	switch (page) {

	case STANDARD_PAGE:
		SetupSpin(IDC_NORMAL_SPIN, 200, 1);
		SetupSpin(IDC_BADLINES_SPIN, 200, 1);
		SetupSpin(IDC_CIA_SPIN, 200, 1);
		SetupSpin(IDC_FLOPPY_SPIN, 200, 1);
		SetupSpin(IDC_DRAWEVERY_SPIN, 10, 1);
		SetupComboClear(IDC_REUSIZE);
		SetupComboAdd(IDC_REUSIZE, "None");
		SetupComboAdd(IDC_REUSIZE, "128k");
		SetupComboAdd(IDC_REUSIZE, "256k");
		SetupComboAdd(IDC_REUSIZE, "512k");
		break;

	case WIN32_PAGE:
		SetupComboClear(IDC_VIEWPORT);
		SetupComboAdd(IDC_VIEWPORT, "Default");
		SetupComboAdd(IDC_VIEWPORT, "320x200");
		SetupComboAdd(IDC_VIEWPORT, "336x216");
		SetupComboAdd(IDC_VIEWPORT, "384x272");
		SetupComboClear(IDC_DISPLAYMODE);
		SetupComboAdd(IDC_DISPLAYMODE, "Default");
		{
			C64Display *TheDisplay = TheApp->TheC64->TheDisplay;
			int n = TheDisplay->GetNumDisplayModes();
			const C64Display::DisplayMode *modes =
				TheDisplay->GetDisplayModes();
			for (int i = 0; i < n; i++) {
				char mode[64];
				sprintf(mode, "%dx%dx%d%s",
					modes[i].x, modes[i].y, modes[i].depth,
					modes[i].modex ? " (ModeX)" : "");
				SetupComboAdd(IDC_DISPLAYMODE, mode);
			}
		}
		SetupSpin(IDC_SCALINGNUMERATOR_SPIN, 16, 1);
		SetupSpin(IDC_SCALINGDENOMINATOR_SPIN, 4, 1);
		SetupSpin(IDC_LATENCYMIN_SPIN, 1000, 20);
		SetupSpinIncrement(IDC_LATENCYMIN_SPIN, 20);
		SetupSpin(IDC_LATENCYMAX_SPIN, 1000, 20);
		SetupSpinIncrement(IDC_LATENCYMAX_SPIN, 20);
		SetupSpin(IDC_LATENCYAVG_SPIN, 1000, 20);
		SetupSpinIncrement(IDC_LATENCYAVG_SPIN, 20);
		break;
	}
}

#define SetText(id, val) SetDlgItemText(hDlg, id, val)
#define SetInteger(id, val) SetDlgItemInt(hDlg, id, val, FALSE)
#define SetCheckBox(id, val) CheckDlgButton(hDlg, id, (val) ? BST_CHECKED : BST_UNCHECKED)
#define SetCombo(id, val) SendMessage(GetDlgItem(hDlg, id), CB_SELECTSTRING, 0, (LPARAM) val)

void Prefs::SetValues(int page)
{
	const char *str;
	switch (page) {

	case STANDARD_PAGE:
		SetText(IDC_DEVICE8, DrivePath[0]);
		SetText(IDC_DEVICE9, DrivePath[1]);
		SetText(IDC_DEVICE10, DrivePath[2]);
		SetText(IDC_DEVICE11, DrivePath[3]);

		SetInteger(IDC_NORMAL, NormalCycles);
		SetInteger(IDC_BADLINES, BadLineCycles);
		SetInteger(IDC_CIA, CIACycles);
		SetInteger(IDC_FLOPPY, FloppyCycles);
		SetInteger(IDC_DRAWEVERY, SkipFrames);
		switch (REUSize) {
		case REU_NONE: str = "None"; break;
		case REU_128K: str = "128k"; break;
		case REU_256K: str = "256k"; break;
		case REU_512K: str = "512k"; break;
		}
		SetCombo(IDC_REUSIZE, str);

		SetCheckBox(IDC_LIMITSPEED, LimitSpeed);
		SetCheckBox(IDC_SPRITES, SpritesOn);
		SetCheckBox(IDC_SPRITECOLLISIONS, SpriteCollisions);
		SetCheckBox(IDC_JOYSTICK1, Joystick1On);
		SetCheckBox(IDC_JOYSTICK2, Joystick2On);
		SetCheckBox(IDC_SWAPJOYSTICKS, JoystickSwap);
		SetCheckBox(IDC_FASTRESET, FastReset);
		SetCheckBox(IDC_CIAIRQHACK, CIAIRQHack);
		SetCheckBox(IDC_MAPSLASH, MapSlash);
		SetCheckBox(IDC_SIDEMULATION, SIDType == SIDTYPE_DIGITAL);
		SetCheckBox(IDC_SIDFILTERS, SIDFilters);
		SetCheckBox(IDC_1541EMULATION, Emul1541Proc);
		break;

	case WIN32_PAGE:
		SetCheckBox(IDC_FULLSCREEN, DisplayType == DISPTYPE_SCREEN);
		SetCheckBox(IDC_SYSTEMMEMORY, SystemMemory);
		SetCheckBox(IDC_ALWAYSCOPY, AlwaysCopy);
		SetText(IDC_VIEWPORT, ViewPort);
		SetText(IDC_DISPLAYMODE, DisplayMode);

		SetCheckBox(IDC_HIDECURSOR, HideCursor);
		SetCheckBox(IDC_SYSTEMKEYS, SystemKeys);
		SetInteger(IDC_SCALINGNUMERATOR, ScalingNumerator);
		SetInteger(IDC_SCALINGDENOMINATOR, ScalingDenominator);

		SetCheckBox(IDC_DIRECTSOUND, DirectSound);
		SetCheckBox(IDC_EXCLUSIVESOUND, ExclusiveSound);
		SetInteger(IDC_LATENCYMIN, LatencyMin);
		SetInteger(IDC_LATENCYMAX, LatencyMax);
		SetInteger(IDC_LATENCYAVG, LatencyAvg);

		SetCheckBox(IDC_AUTOPAUSE, AutoPause);
		SetCheckBox(IDC_PREFSATSTARTUP, PrefsAtStartup);
		SetCheckBox(IDC_SHOWLEDS, ShowLEDs);
		break;
	}
}

#define GetCheckBox(id, val) (val = IsDlgButtonChecked(hDlg, id) == BST_CHECKED)
#define GetInteger(id, val) (val = GetDlgItemInt(hDlg, id, NULL, FALSE))
#define GetText(id, val) GetDlgItemText(hDlg, id, val, sizeof(val))

void Prefs::GetValues(int page)
{
	BOOL temp;
	char str[256];
	switch (page) {

	case STANDARD_PAGE:
		GetText(IDC_DEVICE8, DrivePath[0]);
		GetText(IDC_DEVICE9, DrivePath[1]);
		GetText(IDC_DEVICE10, DrivePath[2]);
		GetText(IDC_DEVICE11, DrivePath[3]);

		GetInteger(IDC_NORMAL, NormalCycles);
		GetInteger(IDC_BADLINES, BadLineCycles);
		GetInteger(IDC_CIA, CIACycles);
		GetInteger(IDC_FLOPPY, FloppyCycles);
		GetInteger(IDC_DRAWEVERY, SkipFrames);
		GetText(IDC_REUSIZE, str);
		if (strcmp(str, "None") == 0)
			REUSize = REU_NONE;
		else if (strcmp(str, "128k") == 0)
			REUSize = REU_128K;
		else if (strcmp(str, "256k") == 0)
			REUSize = REU_256K;
		else if (strcmp(str, "512k") == 0)
			REUSize = REU_512K;

		GetCheckBox(IDC_LIMITSPEED, LimitSpeed);
		GetCheckBox(IDC_SPRITES, SpritesOn);
		GetCheckBox(IDC_SPRITECOLLISIONS, SpriteCollisions);
		GetCheckBox(IDC_JOYSTICK1, Joystick1On);
		GetCheckBox(IDC_JOYSTICK2, Joystick2On);
		GetCheckBox(IDC_SWAPJOYSTICKS, JoystickSwap);
		GetCheckBox(IDC_FASTRESET, FastReset);
		GetCheckBox(IDC_CIAIRQHACK, CIAIRQHack);
		GetCheckBox(IDC_MAPSLASH, MapSlash);
		GetCheckBox(IDC_SIDEMULATION, temp);
		SIDType = temp ? SIDTYPE_DIGITAL : SIDTYPE_NONE;
		GetCheckBox(IDC_SIDFILTERS, SIDFilters);
		GetCheckBox(IDC_1541EMULATION, Emul1541Proc);
		break;

	case WIN32_PAGE:
		GetCheckBox(IDC_FULLSCREEN, temp);
		DisplayType = temp ? DISPTYPE_SCREEN : DISPTYPE_WINDOW;
		GetCheckBox(IDC_SYSTEMMEMORY, SystemMemory);
		GetCheckBox(IDC_ALWAYSCOPY, AlwaysCopy);
		GetText(IDC_VIEWPORT, ViewPort);
		GetText(IDC_DISPLAYMODE, DisplayMode);

		GetCheckBox(IDC_HIDECURSOR, HideCursor);
		GetCheckBox(IDC_SYSTEMKEYS, SystemKeys);
		GetInteger(IDC_SCALINGNUMERATOR, ScalingNumerator);
		GetInteger(IDC_SCALINGDENOMINATOR, ScalingDenominator);

		GetCheckBox(IDC_DIRECTSOUND, DirectSound);
		GetCheckBox(IDC_EXCLUSIVESOUND, ExclusiveSound);
		GetInteger(IDC_LATENCYMIN, LatencyMin);
		GetInteger(IDC_LATENCYMAX, LatencyMax);
		GetInteger(IDC_LATENCYAVG, LatencyAvg);

		GetCheckBox(IDC_AUTOPAUSE, AutoPause);
		GetCheckBox(IDC_PREFSATSTARTUP, PrefsAtStartup);
		GetCheckBox(IDC_SHOWLEDS, ShowLEDs);
		break;
	}

	for (int i = 0; i < 4; i++) {
		DriveType[i] = DRVTYPE_DIR;
		int length = strlen(DrivePath[i]);
		if (length >= 4) {
			char *suffix = DrivePath[i] + length - 4;
			if (stricmp(suffix, ".t64") == 0)
				DriveType[i] = DRVTYPE_T64;
			else if (stricmp(suffix, ".d64") == 0)
				DriveType[i] = DRVTYPE_D64;
		}
	}
}

void Prefs::BrowseForDevice(int id)
{
	char filename[256];
	GetDlgItemText(hDlg, id, filename, sizeof(filename));
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hDlg;
	ofn.hInstance = hInstance;
	ofn.lpstrFilter =
		"All Files (*.*)\0*.*\0"
		"All C64 Files (*.d64;*.t64)\0*.d64;*.t64\0"
		"D64 Files (*.d64)\0*.d64\0"
		"T64 Files (*.t64)\0*.t64\0"
		;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = "Set Device";
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOTESTFILECREATE |
		OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_SHAREAWARE;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = NULL;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
	BOOL result = GetOpenFileName(&ofn);
	if (result) {
		const char *ext = filename + ofn.nFileExtension;
		if (stricmp(ext, "d64") != 0 && stricmp(ext, "t64") != 0)
			filename[ofn.nFileOffset - 1] = '\0';
		char cwd[256];
		GetCurrentDirectory(sizeof(cwd), cwd);
		int cwd_len = strlen(cwd);
		if (cwd_len > 0 && cwd[cwd_len - 1] != '\\') {
			strcat(cwd, "\\");
			cwd_len++;
		}
		if (strnicmp(filename, cwd, cwd_len) == 0)
			SetDlgItemText(hDlg, id, filename + cwd_len);
		else
			SetDlgItemText(hDlg, id, filename);
	}
}
