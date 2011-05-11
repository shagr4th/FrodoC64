/*
 *  main_Be.h - Main program, BeOS specific stuff
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

#include <AppKit.h>
#include <StorageKit.h>
#include <Path.h>
#include <InterfaceKit.h>

#include "Version.h"


// Global variables
bool FromShell;
BEntry AppDirectory;	
BBitmap *AboutBitmap;
const BRect AboutFrame = BRect(0, 0, 383, 99);


/*
 *  Create application object and start it
 */

int main(int argc, char **argv)
{	
	Frodo *the_app;

	srand(real_time_clock());
	FromShell = (argc != 0);	// !! This doesn't work...

	the_app = new Frodo();
	if (the_app != NULL) {
		the_app->Run();
		delete the_app;
	}
	return 0;
}


/*
 *  Constructor: Initialize member variables
 */

Frodo::Frodo() : BApplication(APP_SIGNATURE), this_messenger(this)
{
	TheC64 = NULL;
	AboutBitmap = NULL;
	strcpy(prefs_path, "/boot/home/config/settings/Frodo_settings");
	prefs_showing = false;

	// Create file panels
	open_panel = new BFilePanel(B_OPEN_PANEL, &this_messenger, NULL, 0, false, new BMessage(MSG_OPEN_SNAPSHOT_RETURNED));
	open_panel->Window()->SetTitle("Frodo: Load snapshot");
	save_panel = new BFilePanel(B_SAVE_PANEL, &this_messenger, NULL, 0, false, new BMessage(MSG_SAVE_SNAPSHOT_RETURNED));
	save_panel->Window()->SetTitle("Frodo: Save snapshot");
}


/*
 *  Process command line arguments
 */

void Frodo::ArgvReceived(int32 argc, char **argv)
{
	if (argc == 2) {
		strncpy(prefs_path, argv[1], 1023);
		prefs_path[1023] = 0;
	}
}


/*
 *  Process Browser arguments
 */

void Frodo::RefsReceived(BMessage *message)
{
	// Set preferences path unless prefs editor is open or C64 is running
	if (!prefs_showing && !TheC64) {
		entry_ref the_ref;
		BEntry the_entry;

		if (message->FindRef("refs", &the_ref) == B_NO_ERROR)
			if (the_entry.SetTo(&the_ref) == B_NO_ERROR)
				if (the_entry.IsFile()) {
					BPath the_path;
					the_entry.GetPath(&the_path);
					strncpy(prefs_path, the_path.Path(), 1023);
					prefs_path[1023] = 0;
				}
	}
}


/*
 *  Arguments processed, prepare emulation and show preferences editor window
 */

void Frodo::ReadyToRun(void)
{
	// Find application directory and cwd to it
	app_info the_info;
	GetAppInfo(&the_info);
	BEntry the_file(&the_info.ref);
	the_file.GetParent(&AppDirectory);
	BPath the_path;
	AppDirectory.GetPath(&the_path);
	strncpy(AppDirPath, the_path.Path(), 1023);
	AppDirPath[1023] = 0;
	chdir(AppDirPath);

	// Set up "about" window bitmap
	AboutBitmap = new BBitmap(AboutFrame, B_COLOR_8_BIT);
	FILE *logofile = fopen("Frodo Logo", "rb");
	if (logofile != NULL) {
		fread(AboutBitmap->Bits(), 384*100, 1, logofile);
		fclose(logofile);
	}

	// Load preferences
	ThePrefs.Load(prefs_path);

	// Show preferences editor (sends MSG_STARTUP on close)
	prefs_showing = true;
	ThePrefs.ShowEditor(true, prefs_path);
}


/*
 *  Handle incoming messages
 */

void Frodo::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MSG_STARTUP: // Start the emulation

			// Preferences editor is not longer on screen
			prefs_showing = false;

			// Create everything
			TheC64 = new C64;

			// Load ROMs
			load_rom_files();

			// Run the 6510
			TheC64->Run();
			break;

		case MSG_PREFS:	// Show preferences editor
			if (TheC64 != NULL && !prefs_showing) {
				TheC64->Pause();
				TheC64->TheDisplay->Pause();

				Prefs *prefs = new Prefs(ThePrefs);
				prefs_showing = true;
				prefs->ShowEditor(false, prefs_path);	// Sends MSG_PREFS_DONE on close
			}
			break;

		case MSG_PREFS_DONE: { // Preferences editor closed
			Prefs *prefs;
			msg->FindPointer("prefs", (void **)&prefs);
			if (!msg->FindBool("canceled")) {
				TheC64->NewPrefs(prefs);
				ThePrefs = *prefs;
			}
			delete prefs;
			prefs_showing = false;

			TheC64->TheDisplay->Resume();
			TheC64->Resume();
			break;
		}

		case MSG_RESET:	// Reset C64
			if (TheC64 != NULL)
				TheC64->Reset();
			break;

		case MSG_NMI:	// NMI
			if (TheC64 != NULL)
				TheC64->NMI();
			break;

		case MSG_SAM:	// Invoke SAM
			if (TheC64 != NULL && !prefs_showing && FromShell) {
				TheC64->Pause();
				TheC64->TheDisplay->Pause();
				SAM(TheC64);
				TheC64->TheDisplay->Resume();
				TheC64->Resume();
			}
			break;

		case MSG_NEXTDISK:	// Insert next disk in drive 8
			if (TheC64 != NULL && !prefs_showing && strlen(ThePrefs.DrivePath[0]) > 4) {
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
					TheC64->Pause();
					Prefs *prefs = new Prefs(ThePrefs);
					strcpy(prefs->DrivePath[0], str);
					TheC64->NewPrefs(prefs);
					ThePrefs = *prefs;
					delete prefs;
					TheC64->Resume();
				}
			}
			break;

		case MSG_TOGGLE_1541:	// Toggle processor-level 1541 emulation
			if (TheC64 != NULL && !prefs_showing) {
				TheC64->Pause();
				Prefs *prefs = new Prefs(ThePrefs);
				prefs->Emul1541Proc = !prefs->Emul1541Proc;
				TheC64->NewPrefs(prefs);
				ThePrefs = *prefs;
				delete prefs;
				TheC64->Resume();
			}
			break;

		case MSG_OPEN_SNAPSHOT:
			if (TheC64 != NULL && !prefs_showing)
				open_panel->Show();
			break;

		case MSG_SAVE_SNAPSHOT:
			if (TheC64 != NULL && !prefs_showing)
				save_panel->Show();
			break;

		case MSG_OPEN_SNAPSHOT_RETURNED:
			if (TheC64 != NULL && !prefs_showing) {
				entry_ref the_ref;
				BEntry the_entry;
				if (msg->FindRef("refs", &the_ref) == B_NO_ERROR)
					if (the_entry.SetTo(&the_ref) == B_NO_ERROR)
						if (the_entry.IsFile()) {
							char path[1024];
							BPath the_path;
							the_entry.GetPath(&the_path);
							strncpy(path, the_path.Path(), 1023);
							path[1023] = 0;
							TheC64->Pause();
							TheC64->LoadSnapshot(path);
							TheC64->Resume();
						}
			}
			break;

		case MSG_SAVE_SNAPSHOT_RETURNED:
			if (TheC64 != NULL && !prefs_showing) {
				entry_ref the_ref;
				BEntry the_entry;
				if (msg->FindRef("directory", &the_ref) == B_NO_ERROR)
					if (the_entry.SetTo(&the_ref) == B_NO_ERROR) {
						char path[1024];
						BPath the_path;
						the_entry.GetPath(&the_path);
						strncpy(path, the_path.Path(), 1023);
						strncat(path, "/", 1023);
						strncat(path, msg->FindString("name"), 1023);
						path[1023] = 0;
						TheC64->Pause();
						TheC64->SaveSnapshot(path);
						TheC64->Resume();
					}
			}
			break;

		default:
			BApplication::MessageReceived(msg);
	}
}


/*
 *  Quit requested (either by menu or by closing the C64 display window)
 */

bool Frodo::QuitRequested(void)
{
	// Stop emulation
	if (TheC64) {
		TheC64->Quit();
		delete TheC64;
	}

	delete AboutBitmap;
	delete open_panel;
	delete save_panel;

	return BApplication::QuitRequested();
}


/*
 *  Display "about" window
 */

class AboutView : public BView {
public:
	AboutView() : BView(AboutFrame, "", B_FOLLOW_NONE, B_WILL_DRAW) {}

	virtual void AttachedToWindow(void)
	{
		SetHighColor(0, 0, 0);
	}

	virtual void Draw(BRect update)
	{
		DrawBitmap(AboutBitmap, update, update);

		SetFont(be_bold_font);
		SetDrawingMode(B_OP_OVER);
		MovePenTo(204, 20);
		DrawString(VERSION_STRING);

		SetFont(be_plain_font);
		MovePenTo(204, 40);
		DrawString("by Christian Bauer");
		MovePenTo(204, 52);
		DrawString("<Christian.Bauer@uni-mainz.de>");
		MovePenTo(204, 75);
		DrawString(B_UTF8_COPYRIGHT " Copyright 1994-1997,2002-2005");
		MovePenTo(204, 87);
		DrawString("Freely distributable.");
	}

	virtual void MouseDown(BPoint point)
	{
		Window()->PostMessage(B_QUIT_REQUESTED);
	}
};

class AboutWindow : public BWindow {
public:
	AboutWindow() : BWindow(AboutFrame, NULL, B_BORDERED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK)
	{
		Lock();
		MoveTo(100, 100);
		AboutView *view = new AboutView;
		AddChild(view);
		view->MakeFocus();
		Unlock();
		Show();
	}
};

void Frodo::AboutRequested(void)
{
	new AboutWindow();
}


/*
 *  Determine whether path name refers to a directory
 */

bool IsDirectory(const char *path)
{
	struct stat st;
	return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}
