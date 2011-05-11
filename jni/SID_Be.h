/*
 *  SID_Be.h - 6581 emulation, Be specific stuff
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

#include "C64.h"


/*
 *  Initialization, open subscriber
 */

#if B_HOST_IS_LENDIAN
const media_raw_audio_format audio_format = {SAMPLE_FREQ, 1, media_raw_audio_format::B_AUDIO_SHORT, B_MEDIA_LITTLE_ENDIAN, SAMPLE_FREQ / CALC_FREQ * 2};
#else
const media_raw_audio_format audio_format = {SAMPLE_FREQ, 1, media_raw_audio_format::B_AUDIO_SHORT, B_MEDIA_BIG_ENDIAN, SAMPLE_FREQ / CALC_FREQ * 2};
#endif

void DigitalRenderer::init_sound(void)
{
	the_player = new BSoundPlayer(&audio_format, "Frodo", buffer_proc, NULL, this);
	the_player->SetHasData(true);
	the_player->Start();
	player_stopped = false;
	ready = true;
}


/*
 *  Destructor, close subscriber
 */

DigitalRenderer::~DigitalRenderer()
{
	if (the_player) {
		the_player->Stop();
		delete the_player;
	}
}


/*
 *  Sample volume (for sampled voice)
 */

void DigitalRenderer::EmulateLine(void)
{
	sample_buf[sample_in_ptr] = volume;
	sample_in_ptr = (sample_in_ptr + 1) % SAMPLE_BUF_SIZE;
}


/*
 *  Pause sound output
 */

void DigitalRenderer::Pause(void)
{
	if (!player_stopped) {
		the_player->Stop();
		player_stopped = true;
	}
}


/*
 *  Resume sound output
 */

void DigitalRenderer::Resume(void)
{
	if (player_stopped) {
		the_player->Start();
		player_stopped = false;
	}
}


/*
 *  Stream function 
 */

void DigitalRenderer::buffer_proc(void *cookie, void *buffer, size_t size, const media_raw_audio_format &format)
{
	((DigitalRenderer *)cookie)->calc_buffer((int16 *)buffer, size);
	((DigitalRenderer *)cookie)->the_c64->SoundSync();
}
