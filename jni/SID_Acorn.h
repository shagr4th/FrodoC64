/*
 *  SID_Acorn.h - 6581 emulation, RISC OS specific stuff
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


void DigitalRenderer::init_sound(void)
{
  _kernel_oserror *err;

  ready = false; sound_buffer = NULL;
  if ((DigitalRenderer_ReadState() & DRState_Active) != 0)
  {
    _kernel_oserror dra;

    dra.errnum = 0; sprintf(dra.errmess,"Can't claim sound system -- already active!");
    Wimp_ReportError(&dra,1,TASKNAME); return;
  }
  // Try starting up the renderer
  sndbufsize = 2*224; linecnt = 0;
  if ((err = DigitalRenderer_Activate(1,sndbufsize,1000000/SAMPLE_FREQ)) != NULL)
  {
    Wimp_ReportError(err,1,TASKNAME); return;
  }
  sound_buffer = new uint8[sndbufsize];
  ready = true;
}




DigitalRenderer::~DigitalRenderer()
{
  if (ready)
  {
    _kernel_oserror *err;

    delete sound_buffer;
    if ((err = DigitalRenderer_Deactivate()) != NULL)
    {
      Wimp_ReportError(err,1,TASKNAME);
    }
  }
}




void DigitalRenderer::EmulateLine(void)
{
  if (ready)
  {
    sample_buf[sample_in_ptr++] = volume;
    // faster than modulo; usually there shouldn't be a loop (while)...
    while (sample_in_ptr >= SAMPLE_BUF_SIZE) {sample_in_ptr -= SAMPLE_BUF_SIZE;}

    // A similar approach to the HP variant: check every <number> of lines if
    // new sample needed.
    if (--linecnt < 0)
    {
      int status;

      linecnt = the_c64->PollSoundAfter;
      if ((status = DigitalRenderer_ReadState()) > 0)
      {
        if ((status & DRState_NeedData) != 0)
        {
          calc_buffer(sound_buffer, sndbufsize);
          DigitalRenderer_NewSample(sound_buffer);
        }
      }
    }
  }
}




void DigitalRenderer::Pause(void)
{
  if (ready) {DigitalRenderer_Pause();}
}




void DigitalRenderer::Resume(void)
{
  if (ready) {DigitalRenderer_Resume();}
}
