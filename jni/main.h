/*
 *  main.h - Main program
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

#ifndef _MAIN_H
#define _MAIN_H


class C64;

// Global variables
extern char AppDirPath[1024];	// Path of application directory

extern void kbd_buf_update(C64 *TheC64);
/*
 *  BeOS specific stuff
 */

#ifdef __BEOS__
#include <AppKit.h>
#include <StorageKit.h>

// Global variables
extern bool FromShell;			// true: Started from shell, SAM can be used
extern BEntry AppDirectory;		// Application directory


// Message codes
const uint32 MSG_STARTUP = 'strt';			// Start emulation
const uint32 MSG_PREFS = 'pref';			// Show preferences editor
const uint32 MSG_PREFS_DONE = 'pdon';		// Preferences editor closed
const uint32 MSG_RESET = 'rset';			// Reset C64
const uint32 MSG_NMI = 'nmi ';				// Raise NMI
const uint32 MSG_SAM = 'sam ';				// Invoke SAM
const uint32 MSG_NEXTDISK = 'ndsk';		// Insert next disk in drive 8
const uint32 MSG_TOGGLE_1541 = '1541';		// Toggle processor-level 1541 emulation
const uint32 MSG_OPEN_SNAPSHOT = 'opss';	// Open snapshot file
const uint32 MSG_SAVE_SNAPSHOT = 'svss';	// Save snapshot file
const uint32 MSG_OPEN_SNAPSHOT_RETURNED = 'opsr';	// Open snapshot file panel returned
const uint32 MSG_SAVE_SNAPSHOT_RETURNED = 'svsr';	// Save snapshot file panel returned


// Application signature
const char APP_SIGNATURE[] = "application/x-vnd.cebix-Frodo";


// Application class
class Frodo : public BApplication {
public:
	Frodo();
	virtual void ArgvReceived(int32 argc, char **argv);
	virtual void RefsReceived(BMessage *message);
	virtual void ReadyToRun(void);
	virtual void MessageReceived(BMessage *msg);
	virtual bool QuitRequested(void);
	virtual void AboutRequested(void);

private:
	void load_rom(const char *which, const char *path, uint8 *where, size_t size, const uint8 *builtin);
	void load_rom_files();

	char prefs_path[1024];	// Pathname of current preferences file
	bool prefs_showing;		// true: Preferences editor is on screen

	BMessenger this_messenger;
	BFilePanel *open_panel;
	BFilePanel *save_panel;
};

#endif


/*
 *  AmigaOS specific stuff
 */

#ifdef AMIGA

class Frodo {
public:
	Frodo();
	void ArgvReceived(int argc, char **argv);
	void ReadyToRun(void);
	void RunPrefsEditor(void);

private:
	void load_rom(const char *which, const char *path, uint8 *where, size_t size, const uint8 *builtin);
	void load_rom_files();

	char prefs_path[256];	// Pathname of current preferences file
};

// Global variables
extern Frodo *be_app;	// Pointer to Frodo object

#endif


/*
 *  X specific stuff
 */

#ifdef __unix

class Prefs;

class Frodo {
public:
	Frodo();
	void ArgvReceived(int argc, char **argv);
	void ReadyToRun(void);
	static Prefs *reload_prefs(void);

private:
	void load_rom(const char *which, const char *path, uint8 *where, size_t size, const uint8 *builtin);
	void load_rom_files();

	static char prefs_path[256];	// Pathname of current preferences file
};

#endif


/*
 *  Mac specific stuff
 */

#ifdef __mac__

class Frodo {
public:
	Frodo();

	void Run(void);

private:
	void load_rom(const char *which, const char *path, uint8 *where, size_t size, const uint8 *builtin);
	void load_rom_files();
};

#endif


/*
 *  WIN32 specific stuff
 */

#ifdef WIN32

class Frodo {
public:
	Frodo();
	~Frodo();
	void ArgvReceived(int argc, char **argv);
	void ReadyToRun();
	void RunPrefsEditor();

	char prefs_path[256];	// Pathname of current preferences file

private:
	void load_rom(const char *which, const char *path, uint8 *where, size_t size, const uint8 *builtin);
	void load_rom_files();
};

// Global variables
extern Frodo *TheApp;	// Pointer to Frodo object
extern HINSTANCE hInstance;
extern int nCmdShow;
extern HWND hwnd;

// Command line options.
extern BOOL full_screen;

#endif


/*
 *  RiscOS specific stuff
 */

#ifdef __riscos__

class Frodo
{
public:
	Frodo();
	~Frodo();
	void ReadyToRun(void);

private:
	void load_rom(const char *which, const char *path, uint8 *where, size_t size, const uint8 *builtin);
	void load_rom_files();
};

#endif

// Global C64 object
extern C64 *TheC64;


/*
 *  Functions
 */

// Determine whether path name refers to a directory
extern bool IsDirectory(const char *path);

#endif
