/*
 *  SID_hp.h - 6581 emulation, HP-UX specific stuff
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

extern "C" {
	#include <sys/audio.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/ioctl.h>
}

#define TXBUFSIZE  16384 // bytes, not samples
#define TXFRAGSIZE 1024  // samples, not bytes
#define TXLOWWATER (TXBUFSIZE-(TXFRAGSIZE*2)) // bytes, not samples

/*
 *  Initialization
 */

void DigitalRenderer::init_sound(void)
{
	ready = false;
	
	if ((fd = open("/dev/audio", O_WRONLY | O_NDELAY, 0)) < 0) {
		LOGE( "unable to open /dev/audio -> no sound\n");
		return;
	}
	
	int flags;
	if ((flags = fcntl (fd, F_GETFL, 0)) < 0) {
		LOGE( "unable to set non-blocking mode for /dev/audio -> no sound\n");
		return;
	}
	flags |= O_NDELAY;
	if (fcntl (fd, F_SETFL, flags) < 0) {
		LOGE( "unable to set non-blocking mode for /dev/audio -> no sound\n");
		return;
	}
	
	if (ioctl(fd, AUDIO_SET_DATA_FORMAT, AUDIO_FORMAT_LINEAR16BIT)) {
		LOGE( "unable to select 16bit-linear sample format -> no sound\n");
		return;
	}
	
	if (ioctl(fd, AUDIO_SET_SAMPLE_RATE, 44100)) {
		LOGE( "unable to select 44.1kHz sample-rate -> no sound\n");
		return;
	}
	
	if (ioctl(fd, AUDIO_SET_CHANNELS, 1)) {
		LOGE( "unable to select 1-channel playback -> no sound\n");
		return;
	}
	
	// choose between:
	// AUDIO_OUT_SPEAKER
	// AUDIO_OUT_HEADPHONE
	// AUDIO_OUT_LINE
	if (ioctl(fd, AUDIO_SET_OUTPUT, AUDIO_OUT_SPEAKER)) {
		LOGE( "unable to select audio output -> no sound\n");
		return;
	}

	{
		// set volume:
		audio_describe description;
		audio_gains gains;
		if (ioctl(fd, AUDIO_DESCRIBE, &description)) {
			LOGE( "unable to get audio description -> no sound\n");
			return;
		}
		if (ioctl (fd, AUDIO_GET_GAINS, &gains)) {
			LOGE( "unable to get gain values -> no sound\n");
			return;
		}
		
		float volume = 1.0;
		gains.transmit_gain = (int)((float)description.min_transmit_gain +
                                  (float)(description.max_transmit_gain
                                          - description.min_transmit_gain)
                                          * volume);
		if (ioctl (fd, AUDIO_SET_GAINS, &gains)) {
			LOGE( "unable to set gain values -> no sound\n");
			return;
		}
	}
	
	if (ioctl(fd, AUDIO_SET_TXBUFSIZE, TXBUFSIZE)) {
		LOGE( "unable to set transmission buffer size -> no sound\n");
		return;
	}
	
	sound_calc_buf = new int16[TXFRAGSIZE];
	
	linecnt = 0;
	
	ready = true;
	return;
}


/*
 *  Destructor
 */

DigitalRenderer::~DigitalRenderer()
{
	if (fd > 0) {
		if (ready) {
			ioctl(fd, AUDIO_DRAIN);
		}
		ioctl(fd, AUDIO_RESET);
		close(fd);
	}
}


/*
 *  Pause sound output
 */

void DigitalRenderer::Pause(void)
{
}


/*
 * Resume sound output
 */

void DigitalRenderer::Resume(void)
{
}


/*
 *  Fill buffer, sample volume (for sampled voice)
 */

void DigitalRenderer::EmulateLine(void)
{
	sample_buf[sample_in_ptr] = volume;
	sample_in_ptr = (sample_in_ptr + 1) % SAMPLE_BUF_SIZE;

	// testing the audio status at each raster-line is
	// really not necessary. Let's do it every 50th one..
	// ought to be enough by far.

	linecnt--;
	if (linecnt < 0 && ready) {
		linecnt = 50;
		
		// check whether we should add some more data to the
		// transmission buffer.

		if (ioctl(fd, AUDIO_GET_STATUS, &status)) {
			LOGE("fatal: unable to get audio status\n");
			exit(20);
		}
		
		if (status.transmit_buffer_count < TXLOWWATER) {
			// add one sound fragment..
			calc_buffer(sound_calc_buf, TXFRAGSIZE*2);
			
			// since we've checked for enough space in the transmission buffer,
			// it is an error if the non-blocking write returns anything but
			// TXFRAGSIZE*2
			if (TXFRAGSIZE*2 != write (fd, sound_calc_buf, TXFRAGSIZE*2)) {
				LOGE("fatal: write to audio-device failed\n");
				exit(20);
			}
		}
	}
}

