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
#include <linux/soundcard.h>

// Catweasel ioctls (included here for convenience)
#include <linux/ioctl.h>
#define CWSID_IOCTL_TYPE ('S')
#define CWSID_IOCTL_RESET        _IO(CWSID_IOCTL_TYPE, 0)
#define CWSID_IOCTL_CARDTYPE	 _IOR(CWSID_IOCTL_TYPE, 4, int)
#define CWSID_IOCTL_PAL          _IO(CWSID_IOCTL_TYPE, 0x11)
#define CWSID_IOCTL_NTSC         _IO(CWSID_IOCTL_TYPE, 0x12)
#define CWSID_IOCTL_DOUBLEBUFFER _IOW(CWSID_IOCTL_TYPE, 0x21, int)
#define CWSID_IOCTL_DELAY        _IOW(CWSID_IOCTL_TYPE, 0x22, int)
#define CWSID_MAGIC 0x100
#define HAVE_CWSID 1

#include "VIC.h"


/*
 *  Initialization
 */

void DigitalRenderer::init_sound(void)
{
	int arg;
	unsigned long format;

	ready = false;
	devfd = open("/dev/dsp", O_WRONLY);
	if (devfd < 0)
		return;

	ioctl(devfd, SNDCTL_DSP_GETFMTS, &format);
	if (!(format & AFMT_S16_LE))
		return;
	format = AFMT_S16_LE;
	ioctl(devfd, SNDCTL_DSP_SETFMT, &format);

	// Buffer size: 2^9 == 512 bytes. Note that too large buffers will not work
	// very well: The speed of the C64 is slowed down to an average speed of
	// 100% by the blocking write() call in EmulateLine(). If you use a buffer
	// of, say 4096 bytes, that will happen only about every 4 frames, which
	// means that the emulation runs much faster in some frames, and much
	// slower in others.
	// On really fast machines, it might make sense to use an even smaller
	// buffer size.
	arg = 0x00100009;
	ioctl(devfd, SNDCTL_DSP_SETFRAGMENT, &arg);
	arg = 0;
	ioctl(devfd, SNDCTL_DSP_STEREO, &arg);
	arg = 44100;
	ioctl(devfd, SNDCTL_DSP_SPEED, &arg);
	ioctl(devfd, SOUND_PCM_READ_RATE, &arg);
	if (arg < 43000 || arg > 45000)
		return;

	ioctl(devfd, SNDCTL_DSP_GETBLKSIZE, &sndbufsize);
	sound_buffer = new int16[sndbufsize];
	ready = true;
}


/*
 *  Destructor
 */

DigitalRenderer::~DigitalRenderer()
{
	if (devfd >= 0)
		close(devfd);
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
 * Fill buffer, sample volume (for sampled voice)
 */

void DigitalRenderer::EmulateLine(void)
{
	static int divisor = 0;
	static int to_output = 0;
	static int buffer_pos = 0;

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
		write(devfd, sound_buffer, sndbufsize*2);
		buffer_pos = 0;
	}
}


/*
 *  Renderer for Catweasel card
 */

// Renderer class
class CatweaselRenderer : public SIDRenderer {
public:
	CatweaselRenderer();
	virtual ~CatweaselRenderer();

	virtual void Reset(void);
	virtual void EmulateLine(void) {}
	virtual void WriteRegister(uint16 adr, uint8 byte);
	virtual void NewPrefs(Prefs *prefs) {}
	virtual void Pause(void) {}
	virtual void Resume(void) {}

private:
	int cwsid_fh; // Catweasel device file handle
};

// Constructor: Open Catweasel device and reset SID
CatweaselRenderer::CatweaselRenderer()
{
	cwsid_fh = open("/dev/sid", O_WRONLY);
	if (cwsid_fh >= 0) {
		int i;
		if (ioctl(cwsid_fh, CWSID_IOCTL_CARDTYPE, &i) < 0 || i != CWSID_MAGIC) {
			close(cwsid_fh);
			cwsid_fh = -1;
		} else {
			ioctl(cwsid_fh, CWSID_IOCTL_RESET);
			ioctl(cwsid_fh, CWSID_IOCTL_DOUBLEBUFFER, 0);
		}
	}

	Reset();
}

// Destructor: Reset SID and close Catweasel device
CatweaselRenderer::~CatweaselRenderer()
{
	Reset();

	if (cwsid_fh >= 0) {
		close(cwsid_fh);
		cwsid_fh = -1;
	}
}

// Reset SID
void CatweaselRenderer::Reset(void)
{
	if (cwsid_fh >= 0) {
		uint8 zero = 0;
		ioctl(cwsid_fh, CWSID_IOCTL_RESET);
		lseek(cwsid_fh, 24, SEEK_SET);
		write(cwsid_fh, &zero, 1);
	}
}

// Write to register
void CatweaselRenderer::WriteRegister(uint16 adr, uint8 byte)
{
	if (cwsid_fh >= 0 && adr < 0x1a) {
		lseek(cwsid_fh, adr, SEEK_SET);
		write(cwsid_fh, &byte, 1);
		lseek(cwsid_fh, adr, SEEK_SET);
		write(cwsid_fh, &byte, 1);
	}
}
