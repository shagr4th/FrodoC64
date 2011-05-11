/*
 *  Prefs_Amiga.h - Global preferences, Amiga specific stuff
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

#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/asl.h>
#include <proto/dos.h>

extern "C" {
#include "AmigaGUI.h"
}


// Flag: All done, close prefs window
static bool done, result;

// Pointer to preferences being edited
static Prefs *prefs;

// Pointer to prefs file name
static char *path;

// File requesters
struct FileRequester *open_req, *save_req, *drive_req;

// Prototypes
static void set_values(void);
static void get_values(void);
static void ghost_gadgets(void);
static void get_drive(int i);


/*
 *  Show preferences editor (synchronously)
 *  prefs_name points to the file name of the preferences (which may be changed)
 */

bool Prefs::ShowEditor(bool startup, char *prefs_name)
{
	done = result = FALSE;
	prefs = this;
	path = prefs_name;
	open_req = save_req = NULL;

	// Open prefs window
	if (!SetupScreen()) {
		if (!OpenPrefsWindow()) {

			// Allocate file requesters
			open_req = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
				ASLFR_Window, (ULONG)PrefsWnd,
				ASLFR_SleepWindow, TRUE,
				ASLFR_TitleText, (ULONG)"Frodo: Open preferences...",
				ASLFR_RejectIcons, TRUE,
				TAG_DONE);
			save_req = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
				ASLFR_Window, (ULONG)PrefsWnd,
				ASLFR_SleepWindow, TRUE,
				ASLFR_TitleText, (ULONG)"Frodo: Save preferences as...",
				ASLFR_DoSaveMode, TRUE,
				ASLFR_RejectIcons, TRUE,
				TAG_DONE);
			drive_req = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
				ASLFR_Window, (ULONG)PrefsWnd,
				ASLFR_SleepWindow, TRUE,
				ASLFR_RejectIcons, TRUE,
				TAG_DONE);

			// Handle prefs window
			set_values();
			while (!done) {
				WaitPort(PrefsWnd->UserPort);
				HandlePrefsIDCMP();
			}

			// Free file requesters
			FreeAslRequest(open_req);
			FreeAslRequest(save_req);
			FreeAslRequest(drive_req);
		}
		ClosePrefsWindow();
	}
	CloseDownScreen();

	return result;
}


/*
 *  Set the values of the gadgets
 */

static void set_values(void)
{
	prefs->Check();

	GT_SetGadgetAttrs(PrefsGadgets[GDX_NormalCycles], PrefsWnd, NULL, GTIN_Number, prefs->NormalCycles, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_BadLineCycles], PrefsWnd, NULL, GTIN_Number, prefs->BadLineCycles, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_CIACycles], PrefsWnd, NULL, GTIN_Number, prefs->CIACycles, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_FloppyCycles], PrefsWnd, NULL, GTIN_Number, prefs->FloppyCycles, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_SkipFrames], PrefsWnd, NULL, GTIN_Number, prefs->SkipFrames, TAG_DONE);

	GT_SetGadgetAttrs(PrefsGadgets[GDX_SIDType], PrefsWnd, NULL, GTCY_Active, prefs->SIDType, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_REUSize], PrefsWnd, NULL, GTCY_Active, prefs->REUSize, TAG_DONE);

	GT_SetGadgetAttrs(PrefsGadgets[GDX_SpritesOn], PrefsWnd, NULL, GTCB_Checked, prefs->SpritesOn, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_SpriteCollisions], PrefsWnd, NULL, GTCB_Checked, prefs->SpriteCollisions, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_Joystick2On], PrefsWnd, NULL, GTCB_Checked, prefs->Joystick2On, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_JoystickSwap], PrefsWnd, NULL, GTCB_Checked, prefs->JoystickSwap, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_LimitSpeed], PrefsWnd, NULL, GTCB_Checked, prefs->LimitSpeed, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_FastReset], PrefsWnd, NULL, GTCB_Checked, prefs->FastReset, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_CIAIRQHack], PrefsWnd, NULL, GTCB_Checked, prefs->CIAIRQHack, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_SIDFilters], PrefsWnd, NULL, GTCB_Checked, prefs->SIDFilters, TAG_DONE);

	GT_SetGadgetAttrs(PrefsGadgets[GDX_DrivePath8], PrefsWnd, NULL, GTST_String, (ULONG)prefs->DrivePath[0], TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_DrivePath9], PrefsWnd, NULL, GTST_String, (ULONG)prefs->DrivePath[1], TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_DrivePath10], PrefsWnd, NULL, GTST_String, (ULONG)prefs->DrivePath[2], TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_DrivePath11], PrefsWnd, NULL, GTST_String, (ULONG)prefs->DrivePath[3], TAG_DONE);

	GT_SetGadgetAttrs(PrefsGadgets[GDX_MapSlash], PrefsWnd, NULL, GTCB_Checked, prefs->MapSlash, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_Emul1541Proc], PrefsWnd, NULL, GTCB_Checked, prefs->Emul1541Proc, TAG_DONE);

	ghost_gadgets();
}


/*
 *  Get the values of the gadgets
 */

static void get_values(void)
{
	prefs->NormalCycles = GetNumber(PrefsGadgets[GDX_NormalCycles]);
	prefs->BadLineCycles = GetNumber(PrefsGadgets[GDX_BadLineCycles]);
	prefs->CIACycles = GetNumber(PrefsGadgets[GDX_CIACycles]);
	prefs->FloppyCycles = GetNumber(PrefsGadgets[GDX_FloppyCycles]);
	prefs->SkipFrames = GetNumber(PrefsGadgets[GDX_SkipFrames]);

	strcpy(prefs->DrivePath[0], GetString(PrefsGadgets[GDX_DrivePath8]));
	strcpy(prefs->DrivePath[1], GetString(PrefsGadgets[GDX_DrivePath9]));
	strcpy(prefs->DrivePath[2], GetString(PrefsGadgets[GDX_DrivePath10]));
	strcpy(prefs->DrivePath[3], GetString(PrefsGadgets[GDX_DrivePath11]));

	prefs->Check();
}


/*
 *  Enable/disable certain gadgets
 */

static void ghost_gadgets(void)
{
	GT_SetGadgetAttrs(PrefsGadgets[GDX_NormalCycles], PrefsWnd, NULL, GA_Disabled, IsFrodoSC, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_BadLineCycles], PrefsWnd, NULL, GA_Disabled, IsFrodoSC, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_CIACycles], PrefsWnd, NULL, GA_Disabled, IsFrodoSC, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_FloppyCycles], PrefsWnd, NULL, GA_Disabled, IsFrodoSC, TAG_DONE);
	GT_SetGadgetAttrs(PrefsGadgets[GDX_CIAIRQHack], PrefsWnd, NULL, GA_Disabled, IsFrodoSC, TAG_DONE);
}


/*
 *  Handle gadgets
 */

int SpritesOnClicked(void)
{
	prefs->SpritesOn = !prefs->SpritesOn;
}

int SpriteCollisionsClicked(void)
{
	prefs->SpriteCollisions = !prefs->SpriteCollisions;
}

int Joystick2OnClicked(void)
{
	prefs->Joystick2On = !prefs->Joystick2On;
}

int JoystickSwapClicked(void)
{
	prefs->JoystickSwap = !prefs->JoystickSwap;
}

int LimitSpeedClicked(void)
{
	prefs->LimitSpeed = !prefs->LimitSpeed;
}

int FastResetClicked(void)
{
	prefs->FastReset = !prefs->FastReset;
}

int CIAIRQHackClicked(void)
{
	prefs->CIAIRQHack = !prefs->CIAIRQHack;
}

int SIDFiltersClicked(void)
{
	prefs->SIDFilters = !prefs->SIDFilters;
}

int NormalCyclesClicked(void) {}
int BadLineCyclesClicked(void) {}
int CIACyclesClicked(void) {}
int FloppyCyclesClicked(void) {}
int SkipFramesClicked(void) {}
int DrivePath8Clicked(void) {}
int DrivePath9Clicked(void) {}
int DrivePath10Clicked(void) {}
int DrivePath11Clicked(void) {}

int SIDTypeClicked(void)
{
	prefs->SIDType = PrefsMsg.Code;
}

int REUSizeClicked(void)
{
	prefs->REUSize = PrefsMsg.Code;
}

void get_drive(int i)
{
	bool result = FALSE;

	if (drive_req == NULL)
		return;

	get_values();

	if (prefs->DriveType[i] == DRVTYPE_DIR)
		result = AslRequestTags(drive_req,
			ASLFR_TitleText, (ULONG)"Frodo: Select directory",
			ASLFR_DrawersOnly, TRUE,
			ASLFR_DoPatterns, FALSE,
			ASLFR_InitialPattern, (ULONG)"#?",
			ASLFR_InitialDrawer, (ULONG)prefs->DrivePath[i],
			ASLFR_InitialFile, (ULONG)"",
			TAG_DONE);
	else {

		// Separate path and file
		char dir[256], file[256];
		strncpy(dir, prefs->DrivePath[i], 255);
		char *s = FilePart(dir);
		strncpy(file, s, 255);
		*s = 0;

		result = AslRequestTags(drive_req,
			ASLFR_TitleText, (ULONG)"Frodo: Select disk image or archive file",
			ASLFR_DrawersOnly, FALSE,
			ASLFR_DoPatterns, TRUE,
			ASLFR_InitialPattern, (ULONG)"#?.(d64|x64|t64|LNX|P00)",
			ASLFR_InitialDrawer, (ULONG)dir,
			ASLFR_InitialFile, (ULONG)file,
			TAG_DONE);
	}

	if (result) {
		strncpy(prefs->DrivePath[i], drive_req->fr_Drawer, 255);
		if (prefs->DriveType[i] != DRVTYPE_DIR)
			AddPart(prefs->DrivePath[i], drive_req->fr_File, 255);
		set_values();
	}
}

int GetDrive8Clicked(void)
{
	get_drive(0);
}

int GetDrive9Clicked(void)
{
	get_drive(1);
}

int GetDrive10Clicked(void)
{
	get_drive(2);
}

int GetDrive11Clicked(void)
{
	get_drive(3);
}

int MapSlashClicked(void)
{
	prefs->MapSlash = !prefs->MapSlash;
}

int Emul1541ProcClicked(void)
{
	prefs->Emul1541Proc = !prefs->Emul1541Proc;
}

int OKClicked(void)
{
	get_values();
	done = result = TRUE;
}

int CancelClicked(void)
{
	done = TRUE;
	result = FALSE;
}


/*
 *  Handle menus
 */

int PrefsOpen(void)
{
	if (open_req != NULL && AslRequest(open_req, NULL)) {
		strncpy(path, open_req->fr_Drawer, 255);
		AddPart(path, open_req->fr_File, 255);

		get_values();	// Useful if Load() is unsuccessful
		prefs->Load(path);
		set_values();
	}
}

int PrefsRevert(void)
{
	get_values();	// Useful if Load() is unsuccessful
	prefs->Load(path);
	set_values();
}

int PrefsSaveAs(void)
{
	if (save_req != NULL && AslRequest(save_req, NULL)) {
		strncpy(path, save_req->fr_Drawer, 255);
		AddPart(path, save_req->fr_File, 255);

		get_values();
		prefs->Save(path);
	}
}

int PrefsSave(void)
{
	get_values();
	prefs->Save(path);
}

int PrefsOK(void)
{
	return OKClicked();
}

int PrefsCancel(void)
{
	return CancelClicked();
}


/*
 *  Handle keys
 */

int PrefsVanillaKey(void)
{
	switch (PrefsMsg.Code) {
		case 'o': case 'O':
			return OKClicked();
		case 'c': case 'C':
			return CancelClicked();
	}
}
