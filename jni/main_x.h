/*
 *  main_x.h - Main program, Unix specific stuff
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

#include "Version.h"
#include "C64.h"

#include <unistd.h>

#ifdef HAVE_GLADE
#include <gnome.h>
#endif

// Qtopia Windowing System
#ifdef QWS
extern "C" int main(int argc, char *argv[]);
#include <SDL.h>
#endif

extern int init_graphics(void);


// Global variables
char Frodo::prefs_path[256] = "";


/*
 *  Create application object and start it
 */

int main(int argc, char **argv)
{
#ifdef HAVE_GLADE
	gnome_program_init(PACKAGE_NAME, PACKAGE_VERSION, LIBGNOMEUI_MODULE, argc, argv,
	                   GNOME_PARAM_APP_DATADIR, DATADIR, NULL);
#endif

#ifdef __gp2x__
	//exit(0);
	//printf("starting frodo...\n");
	//FILE *debug_file=fopen("/mnt/sd/frodo.txt", "0777");
	//fprintf(debug_file, "test\n");
	//fclose(debug_file);
#endif

	timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);

#ifndef HAVE_GLADE
	/*printf(
		"%s Copyright (C) 1994-1997,2002-2005 Christian Bauer\n"
		"This is free software with ABSOLUTELY NO WARRANTY.\n"
		, VERSION_STRING
	);*/
#endif
	if (!init_graphics())
		return 1;
	fflush(stdout);

	Frodo *the_app = new Frodo();
	the_app->ArgvReceived(argc, argv);
	the_app->ReadyToRun();
	delete the_app;

#ifdef __gp2x__
	chdir("/usr/gp2x");
	execl("gp2xmenu", "gp2xmenu", NULL);
	//exit(1);
	//return 0;
#else
	return 0;
#endif
}

Frodo *the_app;


char kbd_feedbuf[255];
int kbd_feedbuf_pos;

void kbd_buf_feed(char *s) {
	strcpy(kbd_feedbuf, s);
	kbd_feedbuf_pos=0;
}

void kbd_buf_update(C64 *TheC64) {
	if(TheC64 && (kbd_feedbuf[kbd_feedbuf_pos]!=0)
			&& TheC64->RAM[198]==0) {
		TheC64->RAM[631]=kbd_feedbuf[kbd_feedbuf_pos];
		TheC64->RAM[198]=1;

		kbd_feedbuf_pos++;
	}
}

extern "C"
jint Java_org_ab_c64_FrodoC64_startEmulator( JNIEnv* env, jobject thiz)
{

	timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);
	char **argv;

	if (!init_graphics())
		return 1;

	
	Frodo *the_app = new Frodo();
	the_app->ArgvReceived(0,argv );
	the_app->ReadyToRun();
	delete the_app;


	return 0;
}

extern "C"
jint Java_org_ab_c64_FrodoC64_setFS( JNIEnv*  env, jobject thiz, jint fs) {

	LOGI("frameskip: %d", fs);

	Prefs *prefs = the_app->reload_prefs();
	prefs->SkipFrames = fs;
	ThePrefs.SkipFrames = fs;
	if (TheC64)
		TheC64->NewPrefs(prefs);
}

extern "C"
jint Java_org_ab_c64_FrodoC64_set1541( JNIEnv*  env, jobject thiz, jint boo) {

	Prefs *prefs = the_app->reload_prefs();
	prefs->Emul1541Proc = false;
	ThePrefs.Emul1541Proc = false;
	if (boo != 0)
	{
		prefs->Emul1541Proc = true;
		ThePrefs.Emul1541Proc = true;
	}
	if (TheC64)
		TheC64->NewPrefs(prefs);
}

extern "C"
void Java_org_ab_c64_FrodoC64_loadImage( JNIEnv* env, jobject thiz, jstring filename)
{
	const char *selected_game = (env)->GetStringUTFChars(filename, 0);
	//char *selected_game = strdup((env)->GetStringUTFChars(filename, 0));
	Prefs *prefs = the_app->reload_prefs();
	strncpy(prefs->DrivePath[0], selected_game, 255);

	LOGI("Loading %s", prefs->DrivePath[0]);

	if (TheC64)
		TheC64->NewPrefs(prefs);
	(env)->ReleaseStringUTFChars(filename, selected_game);
	
}

extern "C"
void Java_org_ab_c64_FrodoC64_saveSnapshot( JNIEnv* env, jobject thiz, jstring filename)
{
	if (TheC64)
		TheC64->Pause();
	usleep(200000);
	char *file = strdup((env)->GetStringUTFChars(filename, 0));
	if (TheC64)
		TheC64->SaveSnapshot(file);
	(env)->ReleaseStringUTFChars(filename, file);
	if (TheC64)
		TheC64->Resume();
}

extern "C"
void Java_org_ab_c64_FrodoC64_loadSnapshot( JNIEnv* env, jobject thiz, jstring filename)
{
	if (TheC64)
		TheC64->Pause();
	usleep(200000);
	char *file = strdup((env)->GetStringUTFChars(filename, 0));
	if (TheC64)
		TheC64->LoadSnapshot(file);
	(env)->ReleaseStringUTFChars(filename, file);
	LOGI("Finished to load snapshot");
	usleep(100000);
	if (TheC64)
		TheC64->Resume();
}

extern "C"
void Java_org_ab_c64_FrodoC64_restore( JNIEnv* env, jobject thiz)
{
	if (TheC64)
		TheC64->NMI();
}

extern "C"
void Java_org_ab_c64_FrodoC64_load81( JNIEnv* env, jobject thiz, jint i)
{
	if (i == 1)
		kbd_buf_feed("\rLOAD\":*\",8,1\rRUN\r");
	else
		kbd_buf_feed("\rLOAD\"*\",8\rRUN\r");
}

extern "C"
void Java_org_ab_c64_FrodoC64_loadX81( JNIEnv* env, jobject thiz, jstring filename, jstring drive, jint drive_present)
{
	char *imagefile = strdup((env)->GetStringUTFChars(filename, 0));
	char loadprg[255];
	char tmp_string[1024];
	int i, j;
	for(i=strlen(imagefile); i>0; i--) {
		if(imagefile[i]=='/') break;
	}
	//LOGI("i: %d", i);
	strcpy(loadprg, "\rLOAD\"");
	if (drive_present)
		i = -1;
	//LOGI("i: %d", i);
	for(j=0; imagefile[i+1+j]; j++) {
		unsigned char c;
		c=imagefile[i+1+j];
		if ((c >= 'A') && (c <= 'Z') || (c >= 'a') && (c <= 'z')) c ^= 0x20;
		tmp_string[j]=c;
	}
	tmp_string[j]='\0';
	strcat(loadprg, tmp_string);
	strcat(loadprg, "\",8,1\rRUN\r");
	imagefile[i]='\0';
	Prefs *prefs = the_app->reload_prefs();
	if (!drive_present)
	{
		strncpy(prefs->DrivePath[0], imagefile, 255);
		LOGI("Loading %s", prefs->DrivePath[0]);
	} else {
		const char *drive_name = (env)->GetStringUTFChars(drive, 0);
		strncpy(prefs->DrivePath[0], drive_name, 255);

		LOGI("Loading %s", prefs->DrivePath[0]);

		(env)->ReleaseStringUTFChars(filename, drive_name);
	}
	if (TheC64)
		TheC64->NewPrefs(prefs);
	kbd_buf_feed(loadprg);
	(env)->ReleaseStringUTFChars(filename, imagefile);
}

extern "C"
void Java_org_ab_c64_FrodoC64_pause( JNIEnv* env, jobject thiz)
{
	if (TheC64)
		TheC64->Pause();
}

extern "C"
void Java_org_ab_c64_FrodoC64_reset( JNIEnv* env, jobject thiz)
{
	LOGI("resetC64");
	if (TheC64)
		TheC64->Reset();
}

extern "C"
void Java_org_ab_c64_FrodoC64_resume( JNIEnv* env, jobject thiz)
{
	if (TheC64)
		TheC64->Resume();
}

extern "C"
void Java_org_ab_c64_FrodoC64_registerClass( JNIEnv* env, jobject caller, jobject callback )
{
	LOGI("--registerClass--");
	android_env = env;
	android_callback = (android_env)->NewGlobalRef(callback);
	jclass c64 = (android_env)->GetObjectClass(android_callback);
	LOGI("--frodoc64 registered--");
	initAudio = (android_env)->GetMethodID(c64, "initAudio", "(III)V");
	getEvent = (android_env)->GetMethodID(c64, "getEvent", "()I");
	/*refreshMainScreen = (android_env)->GetMethodID(c64, "refreshMainScreen", "()V");*/
	playAudio = (android_env)->GetMethodID(c64, "playAudio", "()V");
	pauseAudio = (android_env)->GetMethodID(c64, "pauseAudio", "()V");
	sendAudio = (android_env)->GetMethodID(c64, "sendAudio", "([S)V");
	emulatorReady = (android_env)->GetMethodID(c64, "emulatorReady", "()V");
	drawled = (android_env)->GetMethodID(c64, "drawled", "(II)V");
	requestRender = (android_env)->GetMethodID(c64, "requestRender", "()V");
	LOGI("--methods registered--");
}


extern "C"
jint Java_org_ab_c64_FrodoC64_stopEmulator( JNIEnv* env, jobject thiz )
{
	if (TheC64)
		TheC64->Quit();
	exit(0);
}

/*
 *  Constructor: Initialize member variables
 */

Frodo::Frodo()
{
	TheC64 = NULL;
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

	strcpy(AppDirPath, "/sdcard");

	// Create and start C64
	TheC64 = new C64;

	load_rom_files();
     TheC64->Run();
	delete TheC64;
}


Prefs *Frodo::reload_prefs(void)
{
	static Prefs newprefs;
	newprefs.Load(prefs_path);
	return &newprefs;
}


/*
 *  Determine whether path name refers to a directory
 */

bool IsDirectory(const char *path)
{
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}
