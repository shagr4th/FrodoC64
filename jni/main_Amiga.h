/*
 *  main_Amiga.h - Main program, AmigaOS specific stuff
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
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>


// Global variables
Frodo *be_app;	// Pointer to Frodo object

// Library bases
extern ExecBase *SysBase;
struct GfxBase *GfxBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct Library *GadToolsBase = NULL;
struct Library *DiskfontBase = NULL;
struct Library *AslBase = NULL;

// Prototypes
void error_exit(char *str);
void open_libs(void);
void close_libs(void);


/*
 *  Create application object and start it
 */

int main(int argc, char **argv)
{
	if ((SysBase->AttnFlags & (AFF_68040 | AFF_68881)) != (AFF_68040 | AFF_68881))
		error_exit("68040/68881 or higher required.\n");
	open_libs();

	ULONG secs, micros;
	CurrentTime(&secs, &micros);
	srand(micros);

	be_app = new Frodo();
	be_app->ArgvReceived(argc, argv);
	be_app->ReadyToRun();
	delete be_app;

	close_libs();
	return 0;
}


/*
 *  Low-level failure
 */

void error_exit(char *str)
{
	printf(str);
	close_libs();
	exit(20);
}


/*
 *  Open libraries
 */

void open_libs(void)
{
	if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)))
		error_exit("Couldn't open Gfx V39.\n");
	if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
		error_exit("Couldn't open Intuition V39.\n");
	if (!(GadToolsBase = OpenLibrary("gadtools.library", 39)))
		error_exit("Couldn't open GadTools V39.\n");
	if (!(DiskfontBase = OpenLibrary("diskfont.library", 39)))
		error_exit("Couldn't open Diskfont V39.\n");
	if (!(AslBase = OpenLibrary("asl.library", 39)))
		error_exit("Couldn't open ASL V39.\n");
}


/*
 *  Close libraries
 */

void close_libs(void)
{
	if (AslBase)
		CloseLibrary(AslBase);
	if (DiskfontBase)
		CloseLibrary(DiskfontBase);
	if (GadToolsBase)
		CloseLibrary(GadToolsBase);
	if (IntuitionBase)
		CloseLibrary((struct Library *)IntuitionBase);
	if (GfxBase)
		CloseLibrary((struct Library *)GfxBase);
}


/*
 *  Constructor: Initialize member variables
 */

Frodo::Frodo()
{
	TheC64 = NULL;
	prefs_path[0] = 0;
}


/*
 *  Process command line arguments
 */

void Frodo::ArgvReceived(int argc, char **argv)
{
	if (argc == 2)
		strncpy(prefs_path, argv[1], 255);
}


/*
 *  Arguments processed, run emulation
 */

void Frodo::ReadyToRun(void)
{
	getcwd(AppDirPath, 256);

	// Load preferences
	if (!prefs_path[0])
		strcpy(prefs_path, "Frodo Prefs");
	ThePrefs.Load(prefs_path);

	// Show preferences editor
	if (ThePrefs.ShowEditor(TRUE, prefs_path)) {

		// Create and start C64
		TheC64 = new C64;
		load_rom_files();
		TheC64->Run();
		delete TheC64;
	}
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
