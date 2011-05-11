/*
 *  main_WIN32.h - Main program, WIN32 specific stuff
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

#include <math.h>

// The application.
Frodo *TheApp;

// WinMain args.
HINSTANCE hInstance;
int nCmdShow;
HWND hwnd;

int PASCAL WinMain(HINSTANCE hInstance_arg, HINSTANCE /* hPrevInstance */, LPSTR lpCmdLine, int nCmdShow_arg)
{
	hInstance = hInstance_arg;
	nCmdShow = nCmdShow_arg;
	TheApp = new Frodo();
	TheApp->ArgvReceived(__argc, __argv);
	TheApp->ReadyToRun();
	delete TheApp;
	DestroyWindow(hwnd);
	return 0;
}

/*
 *  Constructor: Initialize member variables
 */

Frodo::Frodo()
{
	TheC64 = NULL;
	prefs_path[0] = 0;
}


Frodo::~Frodo()
{
	delete TheC64;
}


/*
 *  Process command line arguments
 */

void Frodo::ArgvReceived(int argc, char **argv)
{
	char *progname = argv[0];
	argc--, argv++;
	if (argc >= 1 && argv[0][0] != '\0') {

		// XXX: This should be a function.
		char cwd[256];
		GetCurrentDirectory(sizeof(cwd), cwd);
		int cwd_len = strlen(cwd);
		if (cwd_len > 0 && cwd[cwd_len - 1] != '\\') {
			strcat(cwd, "\\");
			cwd_len++;
		}
		if (strnicmp(argv[0], cwd, cwd_len) == 0)
			strncpy(prefs_path, argv[0] + cwd_len, 255);
		else
			strncpy(prefs_path, argv[0], 255);
		int length = strlen(prefs_path);
		if (length > 4 && strchr(prefs_path, '.') == NULL)
			strcat(prefs_path, ".fpr");
	}
}


/*
 *  Arguments processed, run emulation
 */

void Frodo::ReadyToRun(void)
{
	getcwd(AppDirPath, 256);

	// Load preferences
	if (!prefs_path[0])
		strcpy(prefs_path, "Frodo.fpr");
	ThePrefs.Load(prefs_path);

	if (ThePrefs.PrefsAtStartup) {
		if (!ThePrefs.ShowEditor(TRUE, prefs_path))
			return;
	}

	// Create and start C64
	TheC64 = new C64;
	load_rom_files();
	TheC64->Run();
}


/*
 *  Run preferences editor
 */

void Frodo::RunPrefsEditor(void)
{
	Prefs *prefs = new Prefs(ThePrefs);
	if (prefs->ShowEditor(FALSE, prefs_path)) {
		TheC64->NewPrefs(prefs);
		ThePrefs = *prefs;
	}
	delete prefs;
}
