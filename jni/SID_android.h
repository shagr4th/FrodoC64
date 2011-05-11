/*
 *  SID_linux.h - 6581 emulation, Linux specific stuff
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

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>



#include "VIC.h"


/*
 *  Initialization
 */

 int sound_present = 0;


extern "C"
jint Java_org_ab_c64_FrodoC64_setSound( JNIEnv*  env, jobject thiz, jint sound) {
	sound_present = sound;
}

FILE *f;

void DigitalRenderer::init_sound(void)
{
	int arg;
	
	ready = false;
	
	// Buffer size: 2^9 == 512 bytes. Note that too large buffers will not work
	// very well: The speed of the C64 is slowed down to an average speed of
	// 100% by the blocking write() call in EmulateLine(). If you use a buffer
	// of, say 4096 bytes, that will happen only about every 4 frames, which
	// means that the emulation runs much faster in some frames, and much
	// slower in others.
	// On really fast machines, it might make sense to use an even smaller
	// buffer size.
	arg = 0x00100009;
	
	//ioctl(devfd, SNDCTL_DSP_GETBLKSIZE, &sndbufsize);
	sndbufsize = 512;
	sound_buffer = new int16[sndbufsize];
	(android_env)->CallVoidMethod(android_callback, initAudio, SAMPLE_FREQ, 16, sndbufsize);
	ready = true;

	//f = fopen("/sdcard/audio.raw", "w");
}


/*
 *  Destructor
 */

DigitalRenderer::~DigitalRenderer()
{
	
}


/*
 *  Pause sound output
 */

void DigitalRenderer::Pause(void)
{
	//(android_env)->CallVoidMethod(android_callback, pauseAudio);
}


/*
 * Resume sound output
 */

void DigitalRenderer::Resume(void)
{
	//(android_env)->CallVoidMethod(android_callback, playAudio);
}

jshortArray audioarray;

/*
 * Fill buffer, sample volume (for sampled voice)
 */



void DigitalRenderer::EmulateLine(void)
{
	if (!sound_present)
	{
		return;
	}
	static int divisor = 0;
	static int to_output = 0;
	static int buffer_pos = 0;
	static int loop_n = 2;
	static int loop_c = 0;
	if (!ready)
		return;

	sample_buf[sample_in_ptr] = volume;
	sample_in_ptr = (sample_in_ptr + 1) % SAMPLE_BUF_SIZE;

	// Now see how many samples have to be added for this line
	divisor += SAMPLE_FREQ;
	while (divisor >= 0)
		divisor -= TOTAL_RASTERS*SCREEN_FREQ, to_output++;

	// Calculate the sound data only when we have enough to fill
	// the buffer entirely
	if ((buffer_pos + to_output) >= sndbufsize) {

		int datalen = sndbufsize - buffer_pos;
		to_output -= datalen;
		calc_buffer(sound_buffer + buffer_pos, datalen*2);
		
		
		if (!audioarray)
		{
			audioarray = (android_env)->NewShortArray(sndbufsize*loop_n);
		}

		(android_env)->SetShortArrayRegion(audioarray, loop_c*sndbufsize, sndbufsize, sound_buffer);
		loop_c++;
		if (loop_c == loop_n)
		{
			(android_env)->CallVoidMethod(android_callback, sendAudio, audioarray);
			loop_c = 0;
		}
		
		
		buffer_pos = 0;
		
	}
}

