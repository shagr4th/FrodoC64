/*
 *  VIC_SC.cpp - 6569R5 emulation (cycle based)
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

/*
 * Incompatibilities:
 * ------------------
 *
 *  - Color of $ff bytes read when BA is low and AEC is high
 *    is not correct
 *  - Changes to border/background color are visible 7 pixels
 *    too late
 *  - Sprite data access doesn't respect BA
 *  - Sprite collisions are only detected within the visible
 *    screen area (excluding borders)
 *  - Sprites are only drawn if they completely fit within the
 *    left/right limits of the chunky bitmap
 */

#include "sysdeps.h"

#include "VIC.h"
#include "C64.h"
#include "CPUC64.h"
#include "Display.h"
#include "Prefs.h"


// First and last displayed line
const int FIRST_DISP_LINE = 0x10;
const int LAST_DISP_LINE = 0x11f;

// First and last possible line for Bad Lines
const int FIRST_DMA_LINE = 0x30;
const int LAST_DMA_LINE = 0xf7;

// Display window coordinates
const int ROW25_YSTART = 0x33;
const int ROW25_YSTOP = 0xfb;
const int ROW24_YSTART = 0x37;
const int ROW24_YSTOP = 0xf7;

#if defined(SMALL_DISPLAY)
/* This does not work yet, the sprite code doesn't know about it. */
const int COL40_XSTART = 0x14;
const int COL40_XSTOP = 0x154;
const int COL38_XSTART = 0x1B;
const int COL38_XSTOP = 0x14B;
#else
const int COL40_XSTART = 0x20;
const int COL40_XSTOP = 0x160;
const int COL38_XSTART = 0x27;
const int COL38_XSTOP = 0x157;
#endif


// Tables for sprite X expansion
uint16 ExpTable[256] = {
	0x0000, 0x0003, 0x000C, 0x000F, 0x0030, 0x0033, 0x003C, 0x003F,
	0x00C0, 0x00C3, 0x00CC, 0x00CF, 0x00F0, 0x00F3, 0x00FC, 0x00FF,
	0x0300, 0x0303, 0x030C, 0x030F, 0x0330, 0x0333, 0x033C, 0x033F,
	0x03C0, 0x03C3, 0x03CC, 0x03CF, 0x03F0, 0x03F3, 0x03FC, 0x03FF,
	0x0C00, 0x0C03, 0x0C0C, 0x0C0F, 0x0C30, 0x0C33, 0x0C3C, 0x0C3F,
	0x0CC0, 0x0CC3, 0x0CCC, 0x0CCF, 0x0CF0, 0x0CF3, 0x0CFC, 0x0CFF,
	0x0F00, 0x0F03, 0x0F0C, 0x0F0F, 0x0F30, 0x0F33, 0x0F3C, 0x0F3F,
	0x0FC0, 0x0FC3, 0x0FCC, 0x0FCF, 0x0FF0, 0x0FF3, 0x0FFC, 0x0FFF,
	0x3000, 0x3003, 0x300C, 0x300F, 0x3030, 0x3033, 0x303C, 0x303F,
	0x30C0, 0x30C3, 0x30CC, 0x30CF, 0x30F0, 0x30F3, 0x30FC, 0x30FF,
	0x3300, 0x3303, 0x330C, 0x330F, 0x3330, 0x3333, 0x333C, 0x333F,
	0x33C0, 0x33C3, 0x33CC, 0x33CF, 0x33F0, 0x33F3, 0x33FC, 0x33FF,
	0x3C00, 0x3C03, 0x3C0C, 0x3C0F, 0x3C30, 0x3C33, 0x3C3C, 0x3C3F,
	0x3CC0, 0x3CC3, 0x3CCC, 0x3CCF, 0x3CF0, 0x3CF3, 0x3CFC, 0x3CFF,
	0x3F00, 0x3F03, 0x3F0C, 0x3F0F, 0x3F30, 0x3F33, 0x3F3C, 0x3F3F,
	0x3FC0, 0x3FC3, 0x3FCC, 0x3FCF, 0x3FF0, 0x3FF3, 0x3FFC, 0x3FFF,
	0xC000, 0xC003, 0xC00C, 0xC00F, 0xC030, 0xC033, 0xC03C, 0xC03F,
	0xC0C0, 0xC0C3, 0xC0CC, 0xC0CF, 0xC0F0, 0xC0F3, 0xC0FC, 0xC0FF,
	0xC300, 0xC303, 0xC30C, 0xC30F, 0xC330, 0xC333, 0xC33C, 0xC33F,
	0xC3C0, 0xC3C3, 0xC3CC, 0xC3CF, 0xC3F0, 0xC3F3, 0xC3FC, 0xC3FF,
	0xCC00, 0xCC03, 0xCC0C, 0xCC0F, 0xCC30, 0xCC33, 0xCC3C, 0xCC3F,
	0xCCC0, 0xCCC3, 0xCCCC, 0xCCCF, 0xCCF0, 0xCCF3, 0xCCFC, 0xCCFF,
	0xCF00, 0xCF03, 0xCF0C, 0xCF0F, 0xCF30, 0xCF33, 0xCF3C, 0xCF3F,
	0xCFC0, 0xCFC3, 0xCFCC, 0xCFCF, 0xCFF0, 0xCFF3, 0xCFFC, 0xCFFF,
	0xF000, 0xF003, 0xF00C, 0xF00F, 0xF030, 0xF033, 0xF03C, 0xF03F,
	0xF0C0, 0xF0C3, 0xF0CC, 0xF0CF, 0xF0F0, 0xF0F3, 0xF0FC, 0xF0FF,
	0xF300, 0xF303, 0xF30C, 0xF30F, 0xF330, 0xF333, 0xF33C, 0xF33F,
	0xF3C0, 0xF3C3, 0xF3CC, 0xF3CF, 0xF3F0, 0xF3F3, 0xF3FC, 0xF3FF,
	0xFC00, 0xFC03, 0xFC0C, 0xFC0F, 0xFC30, 0xFC33, 0xFC3C, 0xFC3F,
	0xFCC0, 0xFCC3, 0xFCCC, 0xFCCF, 0xFCF0, 0xFCF3, 0xFCFC, 0xFCFF,
	0xFF00, 0xFF03, 0xFF0C, 0xFF0F, 0xFF30, 0xFF33, 0xFF3C, 0xFF3F,
	0xFFC0, 0xFFC3, 0xFFCC, 0xFFCF, 0xFFF0, 0xFFF3, 0xFFFC, 0xFFFF
};

uint16 MultiExpTable[256] = {
	0x0000, 0x0005, 0x000A, 0x000F, 0x0050, 0x0055, 0x005A, 0x005F,
	0x00A0, 0x00A5, 0x00AA, 0x00AF, 0x00F0, 0x00F5, 0x00FA, 0x00FF,
	0x0500, 0x0505, 0x050A, 0x050F, 0x0550, 0x0555, 0x055A, 0x055F,
	0x05A0, 0x05A5, 0x05AA, 0x05AF, 0x05F0, 0x05F5, 0x05FA, 0x05FF,
	0x0A00, 0x0A05, 0x0A0A, 0x0A0F, 0x0A50, 0x0A55, 0x0A5A, 0x0A5F,
	0x0AA0, 0x0AA5, 0x0AAA, 0x0AAF, 0x0AF0, 0x0AF5, 0x0AFA, 0x0AFF,
	0x0F00, 0x0F05, 0x0F0A, 0x0F0F, 0x0F50, 0x0F55, 0x0F5A, 0x0F5F,
	0x0FA0, 0x0FA5, 0x0FAA, 0x0FAF, 0x0FF0, 0x0FF5, 0x0FFA, 0x0FFF,
	0x5000, 0x5005, 0x500A, 0x500F, 0x5050, 0x5055, 0x505A, 0x505F,
	0x50A0, 0x50A5, 0x50AA, 0x50AF, 0x50F0, 0x50F5, 0x50FA, 0x50FF,
	0x5500, 0x5505, 0x550A, 0x550F, 0x5550, 0x5555, 0x555A, 0x555F,
	0x55A0, 0x55A5, 0x55AA, 0x55AF, 0x55F0, 0x55F5, 0x55FA, 0x55FF,
	0x5A00, 0x5A05, 0x5A0A, 0x5A0F, 0x5A50, 0x5A55, 0x5A5A, 0x5A5F,
	0x5AA0, 0x5AA5, 0x5AAA, 0x5AAF, 0x5AF0, 0x5AF5, 0x5AFA, 0x5AFF,
	0x5F00, 0x5F05, 0x5F0A, 0x5F0F, 0x5F50, 0x5F55, 0x5F5A, 0x5F5F,
	0x5FA0, 0x5FA5, 0x5FAA, 0x5FAF, 0x5FF0, 0x5FF5, 0x5FFA, 0x5FFF,
	0xA000, 0xA005, 0xA00A, 0xA00F, 0xA050, 0xA055, 0xA05A, 0xA05F,
	0xA0A0, 0xA0A5, 0xA0AA, 0xA0AF, 0xA0F0, 0xA0F5, 0xA0FA, 0xA0FF,
	0xA500, 0xA505, 0xA50A, 0xA50F, 0xA550, 0xA555, 0xA55A, 0xA55F,
	0xA5A0, 0xA5A5, 0xA5AA, 0xA5AF, 0xA5F0, 0xA5F5, 0xA5FA, 0xA5FF,
	0xAA00, 0xAA05, 0xAA0A, 0xAA0F, 0xAA50, 0xAA55, 0xAA5A, 0xAA5F,
	0xAAA0, 0xAAA5, 0xAAAA, 0xAAAF, 0xAAF0, 0xAAF5, 0xAAFA, 0xAAFF,
	0xAF00, 0xAF05, 0xAF0A, 0xAF0F, 0xAF50, 0xAF55, 0xAF5A, 0xAF5F,
	0xAFA0, 0xAFA5, 0xAFAA, 0xAFAF, 0xAFF0, 0xAFF5, 0xAFFA, 0xAFFF,
	0xF000, 0xF005, 0xF00A, 0xF00F, 0xF050, 0xF055, 0xF05A, 0xF05F,
	0xF0A0, 0xF0A5, 0xF0AA, 0xF0AF, 0xF0F0, 0xF0F5, 0xF0FA, 0xF0FF,
	0xF500, 0xF505, 0xF50A, 0xF50F, 0xF550, 0xF555, 0xF55A, 0xF55F,
	0xF5A0, 0xF5A5, 0xF5AA, 0xF5AF, 0xF5F0, 0xF5F5, 0xF5FA, 0xF5FF,
	0xFA00, 0xFA05, 0xFA0A, 0xFA0F, 0xFA50, 0xFA55, 0xFA5A, 0xFA5F,
	0xFAA0, 0xFAA5, 0xFAAA, 0xFAAF, 0xFAF0, 0xFAF5, 0xFAFA, 0xFAFF,
	0xFF00, 0xFF05, 0xFF0A, 0xFF0F, 0xFF50, 0xFF55, 0xFF5A, 0xFF5F,
	0xFFA0, 0xFFA5, 0xFFAA, 0xFFAF, 0xFFF0, 0xFFF5, 0xFFFA, 0xFFFF
};

#ifdef GLOBAL_VARS
static uint16 mx[8];						// VIC registers
static uint8 my[8];
static uint8 mx8;
static uint8 ctrl1, ctrl2;
static uint8 lpx, lpy;
static uint8 me, mxe, mye, mdp, mmc;
static uint8 vbase;
static uint8 irq_flag, irq_mask;
static uint8 clx_spr, clx_bgr;
static uint8 ec, b0c, b1c, b2c, b3c, mm0, mm1;
static uint8 sc[8];

static uint8 *ram, *char_rom, *color_ram; // Pointers to RAM and ROM
static C64 *the_c64;					// Pointer to C64
static C64Display *the_display;			// Pointer to C64Display
static MOS6510 *the_cpu;				// Pointer to 6510
static MOS6569 *the_vic;				// Pointer to self

static uint8 colors[256];				// Indices of the 16 C64 colors (16 times mirrored to avoid "& 0x0f")

static uint8 ec_color, b0c_color, b1c_color, b2c_color, b3c_color; // Indices for exterior/background colors
static uint8 mm0_color, mm1_color;		// Indices for MOB multicolors
static uint8 spr_color[8];				// Indices for MOB colors

static uint8 matrix_line[40];			// Buffer for video line, read in Bad Lines
static uint8 color_line[40];			// Buffer for color line, read in Bad Lines

#ifdef __POWERPC__
static double chunky_tmp[DISPLAY_X/8];	// Temporary line buffer for GameKit speedup
#endif
static uint8 *chunky_ptr;				// Pointer in chunky bitmap buffer
static uint8 *chunky_line_start;		// Pointer to start of current line in bitmap buffer
static uint8 *fore_mask_ptr;			// Pointer in fore_mask_buf
static int xmod;						// Number of bytes per row

static uint16 raster_x;					// Current raster x position
static uint16 raster_y;					// Current raster line
static uint16 irq_raster;				// Interrupt raster line
static uint16 dy_start;					// Comparison values for border logic
static uint16 dy_stop;
static uint16 rc;						// Row counter
static uint16 vc;						// Video counter
static uint16 vc_base;					// Video counter base
static uint16 x_scroll;					// X scroll value
static uint16 y_scroll;					// Y scroll value
static uint16 cia_vabase;				// CIA VA14/15 video base

static int cycle;						// Current cycle in line (1..63)

static int display_idx;					// Index of current display mode
static int ml_index;					// Index in matrix/color_line[]
static int skip_counter;				// Counter for frame-skipping

static uint16 mc[8];					// Sprite data counters
static uint16 mc_base[8];				// Sprite data counter bases

static uint8 spr_coll_buf[0x180];		// Buffer for sprite-sprite collisions and priorities
static uint8 fore_mask_buf[0x180/8];	// Foreground mask for sprite-graphics collisions and priorities

static bool display_state;				// true: Display state, false: Idle state
static bool border_on;					// Flag: Upper/lower border on
static bool frame_skipped;				// Flag: Frame is being skipped
static bool bad_lines_enabled;			// Flag: Bad Lines enabled for this frame
static bool lp_triggered;				// Flag: Lightpen was triggered in this frame
static bool is_bad_line;			 	// Flag: Current line is Bad Line
static bool draw_this_line;				// Flag: This line is drawn on the screen
static bool ud_border_on;				// Flag: Upper/lower border on
static bool vblanking;					// Flag: VBlank in next cycle

static bool border_on_sample[5];		// Samples of border state at different cycles (1, 17, 18, 56, 57)
static uint8 border_color_sample[DISPLAY_X/8];	// Samples of border color at each "displayed" cycle

static uint16 matrix_base;				// Video matrix base
static uint16 char_base;					// Character generator base
static uint16 bitmap_base;				// Bitmap base

static uint8 ref_cnt;					// Refresh counter
static uint8 spr_exp_y;					// 8 sprite y expansion flipflops
static uint8 spr_dma_on;				// 8 flags: Sprite DMA active
static uint8 spr_disp_on;				// 8 flags: Sprite display active
static uint8 spr_draw;					// 8 flags: Draw sprite in this line
static uint16 spr_ptr[8];				// Sprite data pointers

static uint8 gfx_data, char_data, color_data, last_char_data;
static uint8 spr_data[8][4];			// Sprite data read
static uint8 spr_draw_data[8][4];		// Sprite data for drawing

static uint32 first_ba_cycle;			// Cycle when BA first went low
#endif


/*
 *  Constructor: Initialize variables
 */

MOS6569::MOS6569(C64 *c64, C64Display *disp, MOS6510 *CPU, uint8 *RAM, uint8 *Char, uint8 *Color)
#ifndef GLOBAL_VARS
	: ram(RAM), char_rom(Char), color_ram(Color), the_c64(c64), the_display(disp), the_cpu(CPU)
#endif
{
	int i;

	// Set pointers
#ifdef GLOBAL_VARS
	the_vic = this;
	the_c64 = c64;
	the_display = disp;
	the_cpu = CPU;
	ram = RAM;
	char_rom = Char;
	color_ram = Color;
#endif
	matrix_base = 0;
	char_base = 0;
	bitmap_base = 0;

	// Get bitmap info
	chunky_ptr = chunky_line_start = disp->BitmapBase();
	xmod = disp->BitmapXMod();

	// Initialize VIC registers
	mx8 = 0;
	ctrl1 = ctrl2 = 0;
	lpx = lpy = 0;
	me = mxe = mye = mdp = mmc = 0;
	vbase = irq_flag = irq_mask = 0;
	clx_spr = clx_bgr = 0;
	cia_vabase = 0;
	ec = b0c = b1c = b2c = b3c = mm0 = mm1 = 0;
	for (i=0; i<8; i++) mx[i] = my[i] = sc[i] = 0;

	// Initialize other variables
	raster_y = TOTAL_RASTERS - 1;
	rc = 7;
	irq_raster = vc = vc_base = x_scroll = y_scroll = 0;
	dy_start = ROW24_YSTART;
	dy_stop = ROW24_YSTOP;
	ml_index = 0;

	cycle = 1;
	display_idx = 0;
	display_state = false;
	border_on = ud_border_on = vblanking = false;
	lp_triggered = draw_this_line = false;

	spr_dma_on = spr_disp_on = 0;
	for (i=0; i<8; i++) {
		mc[i] = 63;
		spr_ptr[i] = 0;
	}

	frame_skipped = false;
	skip_counter = 1;

	memset(spr_coll_buf, 0, 0x180);
	memset(fore_mask_buf, 0, 0x180/8);

	// Preset colors to black
	disp->InitColors(colors);
	ec_color = b0c_color = b1c_color = b2c_color = b3c_color = mm0_color = mm1_color = colors[0];
	for (i=0; i<8; i++) spr_color[i] = colors[0];
}


/*
 *  Reinitialize the colors table for when the palette has changed
 */

void MOS6569::ReInitColors(void)
{
	int i;

	// Build inverse color table.
	uint8 xlate_colors[256];
	memset(xlate_colors, 0, sizeof(xlate_colors));
	for (i=0; i<16; i++)
		xlate_colors[colors[i]] = i;

	// Get the new colors.
	the_display->InitColors(colors);

	// Build color translation table.
	for (i=0; i<256; i++)
		xlate_colors[i] = colors[xlate_colors[i]];

	// Translate all the old colors variables.
	ec_color = colors[ec];
	b0c_color = colors[b0c];
	b1c_color = colors[b1c];
	b2c_color = colors[b2c];
	b3c_color = colors[b3c];
	mm0_color = colors[mm0];
	mm1_color = colors[mm1];
	for (i=0; i<8; i++)
		spr_color[i] = colors[sc[i]];

	// Translate the border color sample buffer.
	for (unsigned x = 0; x < sizeof(border_color_sample); x++)
		border_color_sample[x] = xlate_colors[border_color_sample[x]];

	// Translate the chunky buffer.
	uint8 *scanline = the_display->BitmapBase();
	for (int y=0; y<DISPLAY_Y; y++) {
		for (int x=0; x<DISPLAY_X; x++)
			scanline[x] = xlate_colors[scanline[x]];
		scanline += xmod;
	}
}


/*
 *  Get VIC state
 */

void MOS6569::GetState(MOS6569State *vd)
{
	int i;

	vd->m0x = mx[0] & 0xff; vd->m0y = my[0];
	vd->m1x = mx[1] & 0xff; vd->m1y = my[1];
	vd->m2x = mx[2] & 0xff; vd->m2y = my[2];
	vd->m3x = mx[3] & 0xff; vd->m3y = my[3];
	vd->m4x = mx[4] & 0xff; vd->m4y = my[4];
	vd->m5x = mx[5] & 0xff; vd->m5y = my[5];
	vd->m6x = mx[6] & 0xff; vd->m6y = my[6];
	vd->m7x = mx[7] & 0xff; vd->m7y = my[7];
	vd->mx8 = mx8;

	vd->ctrl1 = (ctrl1 & 0x7f) | ((raster_y & 0x100) >> 1);
	vd->raster = raster_y & 0xff;
	vd->lpx = lpx; vd->lpy = lpy;
	vd->ctrl2 = ctrl2;
	vd->vbase = vbase;
	vd->irq_flag = irq_flag;
	vd->irq_mask = irq_mask;

	vd->me = me; vd->mxe = mxe; vd->mye = mye; vd->mdp = mdp; vd->mmc = mmc;
	vd->mm = clx_spr; vd->md = clx_bgr;

	vd->ec = ec;
	vd->b0c = b0c; vd->b1c = b1c; vd->b2c = b2c; vd->b3c = b3c;
	vd->mm0 = mm0; vd->mm1 = mm1;
	vd->m0c = sc[0];
	vd->m1c = sc[1];
	vd->m2c = sc[2];
	vd->m3c = sc[3];
	vd->m4c = sc[4];
	vd->m5c = sc[5];
	vd->m6c = sc[6];
	vd->m7c = sc[7];

	vd->pad0 = 0;
	vd->irq_raster = irq_raster;
	vd->vc = vc;
	vd->vc_base = vc_base;
	vd->rc = rc;
	vd->spr_dma = spr_dma_on;
	vd->spr_disp = spr_disp_on;
	for (i=0; i<8; i++) {
		vd->mc[i] = mc[i];
		vd->mc_base[i] = mc_base[i];
	}
	vd->display_state = display_state;
	vd->bad_line = raster_y >= FIRST_DMA_LINE && raster_y <= LAST_DMA_LINE && ((raster_y & 7) == y_scroll) && bad_lines_enabled;
	vd->bad_line_enable = bad_lines_enabled;
	vd->lp_triggered = lp_triggered;
	vd->border_on = border_on;

	vd->bank_base = cia_vabase;
	vd->matrix_base = ((vbase & 0xf0) << 6) | cia_vabase;
	vd->char_base = ((vbase & 0x0e) << 10) | cia_vabase;
	vd->bitmap_base = ((vbase & 0x08) << 10) | cia_vabase;
	for (i=0; i<8; i++)
		vd->sprite_base[i] = spr_ptr[i] | cia_vabase;

	vd->cycle = cycle;
	vd->raster_x = raster_x;
	vd->ml_index = ml_index;
	vd->ref_cnt = ref_cnt;
	vd->last_vic_byte = LastVICByte;
	vd->ud_border_on = ud_border_on;
}


/*
 *  Set VIC state (only works if in VBlank)
 */

void MOS6569::SetState(MOS6569State *vd)
{
	int i, j;

	mx[0] = vd->m0x; my[0] = vd->m0y;
	mx[1] = vd->m1x; my[1] = vd->m1y;
	mx[2] = vd->m2x; my[2] = vd->m2y;
	mx[3] = vd->m3x; my[3] = vd->m3y;
	mx[4] = vd->m4x; my[4] = vd->m4y;
	mx[5] = vd->m5x; my[5] = vd->m5y;
	mx[6] = vd->m6x; my[6] = vd->m6y;
	mx[7] = vd->m7x; my[7] = vd->m7y;
	mx8 = vd->mx8;
	for (i=0, j=1; i<8; i++, j<<=1) {
		if (mx8 & j)
			mx[i] |= 0x100;
		else
			mx[i] &= 0xff;
	}

	ctrl1 = vd->ctrl1;
	ctrl2 = vd->ctrl2;
	x_scroll = ctrl2 & 7;
	y_scroll = ctrl1 & 7;
	if (ctrl1 & 8) {
		dy_start = ROW25_YSTART;
		dy_stop = ROW25_YSTOP;
	} else {
		dy_start = ROW24_YSTART;
		dy_stop = ROW24_YSTOP;
	}
	display_idx = ((ctrl1 & 0x60) | (ctrl2 & 0x10)) >> 4;

	raster_y = 0;
	lpx = vd->lpx; lpy = vd->lpy;

	vbase = vd->vbase;
	cia_vabase = vd->bank_base;
	matrix_base = (vbase & 0xf0) << 6;
	char_base = (vbase & 0x0e) << 10;
	bitmap_base = (vbase & 0x08) << 10;

	irq_flag = vd->irq_flag;
	irq_mask = vd->irq_mask;

	me = vd->me; mxe = vd->mxe; mye = vd->mye; mdp = vd->mdp; mmc = vd->mmc;
	clx_spr = vd->mm; clx_bgr = vd->md;

	ec = vd->ec;
	ec_color = colors[ec];

	b0c = vd->b0c; b1c = vd->b1c; b2c = vd->b2c; b3c = vd->b3c;
	b0c_color = colors[b0c];
	b1c_color = colors[b1c];
	b2c_color = colors[b2c];
	b3c_color = colors[b3c];

	mm0 = vd->mm0; mm1 = vd->mm1;
	mm0_color = colors[mm0];
	mm1_color = colors[mm1];

	sc[0] = vd->m0c; sc[1] = vd->m1c;
	sc[2] = vd->m2c; sc[3] = vd->m3c;
	sc[4] = vd->m4c; sc[5] = vd->m5c;
	sc[6] = vd->m6c; sc[7] = vd->m7c;
	for (i=0; i<8; i++)
		spr_color[i] = colors[sc[i]];

	irq_raster = vd->irq_raster;
	vc = vd->vc;
	vc_base = vd->vc_base;
	rc = vd->rc;
	spr_dma_on = vd->spr_dma;
	spr_disp_on = vd->spr_disp;
	for (i=0; i<8; i++) {
		mc[i] = vd->mc[i];
		mc_base[i] = vd->mc_base[i];
		spr_ptr[i] = vd->sprite_base[i] & 0x3fff;
	}
	display_state = vd->display_state;
	bad_lines_enabled = vd->bad_line_enable;
	lp_triggered = vd->lp_triggered;
	border_on = vd->border_on;

	cycle = vd->cycle;
	raster_x = vd->raster_x;
	ml_index = vd->ml_index;
	ref_cnt = vd->ref_cnt;
	LastVICByte = vd->last_vic_byte;
	ud_border_on = vd->ud_border_on;
}


/*
 *  Trigger raster IRQ
 */

#ifdef GLOBAL_VARS
static inline void raster_irq(void)
#else
inline void MOS6569::raster_irq(void)
#endif
{
	irq_flag |= 0x01;
	if (irq_mask & 0x01) {
		irq_flag |= 0x80;
		the_cpu->TriggerVICIRQ();
	}
}


/*
 *  Read from VIC register
 */

uint8 MOS6569::ReadRegister(uint16 adr)
{
	switch (adr) {
		case 0x00: case 0x02: case 0x04: case 0x06:
		case 0x08: case 0x0a: case 0x0c: case 0x0e:
			return mx[adr >> 1];

		case 0x01: case 0x03: case 0x05: case 0x07:
		case 0x09: case 0x0b: case 0x0d: case 0x0f:
			return my[adr >> 1];

		case 0x10:	// Sprite X position MSB
			return mx8;

		case 0x11:	// Control register 1
			return (ctrl1 & 0x7f) | ((raster_y & 0x100) >> 1);

		case 0x12:	// Raster counter
			return raster_y;

		case 0x13:	// Light pen X
			return lpx;

		case 0x14:	// Light pen Y
			return lpy;

		case 0x15:	// Sprite enable
			return me;

		case 0x16:	// Control register 2
			return ctrl2 | 0xc0;

		case 0x17:	// Sprite Y expansion
			return mye;

		case 0x18:	// Memory pointers
			return vbase | 0x01;

		case 0x19:	// IRQ flags
			return irq_flag | 0x70;

		case 0x1a:	// IRQ mask
			return irq_mask | 0xf0;

		case 0x1b:	// Sprite data priority
			return mdp;

		case 0x1c:	// Sprite multicolor
			return mmc;

		case 0x1d:	// Sprite X expansion
			return mxe;

		case 0x1e:{	// Sprite-sprite collision
			uint8 ret = clx_spr;
			clx_spr = 0;	// Read and clear
			return ret;
		}

		case 0x1f:{	// Sprite-background collision
			uint8 ret = clx_bgr;
			clx_bgr = 0;	// Read and clear
			return ret;
		}

		case 0x20: return ec | 0xf0;
		case 0x21: return b0c | 0xf0;
		case 0x22: return b1c | 0xf0;
		case 0x23: return b2c | 0xf0;
		case 0x24: return b3c | 0xf0;
		case 0x25: return mm0 | 0xf0;
		case 0x26: return mm1 | 0xf0;

		case 0x27: case 0x28: case 0x29: case 0x2a:
		case 0x2b: case 0x2c: case 0x2d: case 0x2e:
			return sc[adr - 0x27] | 0xf0;

		default:
			return 0xff;
	}
}


/*
 *  Write to VIC register
 */

void MOS6569::WriteRegister(uint16 adr, uint8 byte)
{
	switch (adr) {
		case 0x00: case 0x02: case 0x04: case 0x06:
		case 0x08: case 0x0a: case 0x0c: case 0x0e:
			mx[adr >> 1] = (mx[adr >> 1] & 0xff00) | byte;
			break;

		case 0x10:{
			int i, j;
			mx8 = byte;
			for (i=0, j=1; i<8; i++, j<<=1) {
				if (mx8 & j)
					mx[i] |= 0x100;
				else
					mx[i] &= 0xff;
			}
			break;
		}

		case 0x01: case 0x03: case 0x05: case 0x07:
		case 0x09: case 0x0b: case 0x0d: case 0x0f:
			my[adr >> 1] = byte;
			break;

		case 0x11:{	// Control register 1
			ctrl1 = byte;
			y_scroll = byte & 7;

			uint16 new_irq_raster = (irq_raster & 0xff) | ((byte & 0x80) << 1);
			if (irq_raster != new_irq_raster && raster_y == new_irq_raster)
				raster_irq();
			irq_raster = new_irq_raster;

			if (byte & 8) {
				dy_start = ROW25_YSTART;
				dy_stop = ROW25_YSTOP;
			} else {
				dy_start = ROW24_YSTART;
				dy_stop = ROW24_YSTOP;
			}

			// In line $30, the DEN bit controls if Bad Lines can occur
			if (raster_y == 0x30 && byte & 0x10)
				bad_lines_enabled = true;

			// Bad Line condition?
			is_bad_line = (raster_y >= FIRST_DMA_LINE && raster_y <= LAST_DMA_LINE && ((raster_y & 7) == y_scroll) && bad_lines_enabled);

			display_idx = ((ctrl1 & 0x60) | (ctrl2 & 0x10)) >> 4;
			break;
		}

		case 0x12:{	// Raster counter
			uint16 new_irq_raster = (irq_raster & 0xff00) | byte;
			if (irq_raster != new_irq_raster && raster_y == new_irq_raster)
				raster_irq();
			irq_raster = new_irq_raster;
			break;
		}

		case 0x15:	// Sprite enable
			me = byte;
			break;

		case 0x16:	// Control register 2
			ctrl2 = byte;
			x_scroll = byte & 7;
			display_idx = ((ctrl1 & 0x60) | (ctrl2 & 0x10)) >> 4;
			break;

		case 0x17:	// Sprite Y expansion
			mye = byte;
			spr_exp_y |= ~byte;
			break;

		case 0x18:	// Memory pointers
			vbase = byte;
			matrix_base = (byte & 0xf0) << 6;
			char_base = (byte & 0x0e) << 10;
			bitmap_base = (byte & 0x08) << 10;
			break;

		case 0x19: // IRQ flags
			irq_flag = irq_flag & (~byte & 0x0f);
			if (irq_flag & irq_mask)	// Set master bit if allowed interrupt still pending
				irq_flag |= 0x80;
			else
				the_cpu->ClearVICIRQ();	// Else clear interrupt
			break;
		
		case 0x1a:	// IRQ mask
			irq_mask = byte & 0x0f;
			if (irq_flag & irq_mask) {	// Trigger interrupt if pending and now allowed
				irq_flag |= 0x80;
				the_cpu->TriggerVICIRQ();
			} else {
				irq_flag &= 0x7f;
				the_cpu->ClearVICIRQ();
			}
			break;

		case 0x1b:	// Sprite data priority
			mdp = byte;
			break;

		case 0x1c:	// Sprite multicolor
			mmc = byte;
			break;

		case 0x1d:	// Sprite X expansion
			mxe = byte;
			break;

		case 0x20: ec_color = colors[ec = byte]; break;
		case 0x21: b0c_color = colors[b0c = byte]; break;
		case 0x22: b1c_color = colors[b1c = byte]; break;
		case 0x23: b2c_color = colors[b2c = byte]; break;
		case 0x24: b3c_color = colors[b3c = byte]; break;
		case 0x25: mm0_color = colors[mm0 = byte]; break;
		case 0x26: mm1_color = colors[mm1 = byte]; break;

		case 0x27: case 0x28: case 0x29: case 0x2a:
		case 0x2b: case 0x2c: case 0x2d: case 0x2e:
			spr_color[adr - 0x27] = colors[sc[adr - 0x27] = byte];
			break;
	}
}


/*
 *  CIA VA14/15 has changed
 */

void MOS6569::ChangedVA(uint16 new_va)
{
	cia_vabase = new_va << 14;
	WriteRegister(0x18, vbase); // Force update of memory pointers
}


/*
 *  Trigger lightpen interrupt, latch lightpen coordinates
 */

void MOS6569::TriggerLightpen(void)
{
	if (!lp_triggered) {		// Lightpen triggers only once per frame
		lp_triggered = true;

		lpx = raster_x >> 1;	// Latch current coordinates
		lpy = raster_y;

		irq_flag |= 0x08;		// Trigger IRQ
		if (irq_mask & 0x08) {
			irq_flag |= 0x80;
			the_cpu->TriggerVICIRQ();
		}
	}
}


/*
 *  Read a byte from the VIC's address space
 */

#ifdef GLOBAL_VARS
static inline uint8 read_byte(uint16 adr)
#else
inline uint8 MOS6569::read_byte(uint16 adr)
#endif
{
	uint16 va = adr | cia_vabase;
	if ((va & 0x7000) == 0x1000)
#ifdef GLOBAL_VARS
		return the_vic->LastVICByte = char_rom[va & 0x0fff];
#else
		return LastVICByte = char_rom[va & 0x0fff];
#endif
	else
#ifdef GLOBAL_VARS
		return the_vic->LastVICByte = ram[va];
#else
		return LastVICByte = ram[va];
#endif
}


/*
 *  Quick memset of 8 bytes
 */

inline void memset8(uint8 *p, uint8 c)
{
	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = p[6] = p[7] = c;
}


/*
 *  Video matrix access
 */

#ifdef __i386
inline
#endif
#ifdef GLOBAL_VARS
static void matrix_access(void)
#else
void MOS6569::matrix_access(void)
#endif
{
	if (the_cpu->BALow) {
		if (the_c64->CycleCounter-first_ba_cycle < 3)
			matrix_line[ml_index] = color_line[ml_index] = 0xff;
		else {
			uint16 adr = (vc & 0x03ff) | matrix_base;
			matrix_line[ml_index] = read_byte(adr);
			color_line[ml_index] = color_ram[adr & 0x03ff];
		}
	}
}


/*
 *  Graphics data access
 */

#ifdef __i386
inline
#endif
#ifdef GLOBAL_VARS
static void graphics_access(void)
#else
void MOS6569::graphics_access(void)
#endif
{
	if (display_state) {

		uint16 adr;
		if (ctrl1 & 0x20)	// Bitmap
			adr = ((vc & 0x03ff) << 3) | bitmap_base | rc;
		else				// Text
			adr = (matrix_line[ml_index] << 3) | char_base | rc;
		if (ctrl1 & 0x40)	// ECM
			adr &= 0xf9ff;
		gfx_data = read_byte(adr);
		char_data = matrix_line[ml_index];
		color_data = color_line[ml_index];
		ml_index++;
		vc++;

	} else {

		// Display is off
		gfx_data = read_byte(ctrl1 & 0x40 ? 0x39ff : 0x3fff);
		char_data = color_data = 0;
	}
}


/*
 *  Background display (8 pixels)
 */

#ifdef GLOBAL_VARS
static void draw_background(void)
#else
void MOS6569::draw_background(void)
#endif
{
	uint8 *p = chunky_ptr;
	uint8 c;

	if (!draw_this_line)
		return;

	switch (display_idx) {
		case 0:		// Standard text
		case 1:		// Multicolor text
		case 3:		// Multicolor bitmap
			c = b0c_color;
			break;
		case 2:		// Standard bitmap
			c = colors[last_char_data];
			break;
		case 4:		// ECM text
			if (last_char_data & 0x80)
				if (last_char_data & 0x40)
					c = b3c_color;
				else
					c = b2c_color;
			else
				if (last_char_data & 0x40)
					c = b1c_color;
				else
					c = b0c_color;
			break;
		default:
			c = colors[0];
			break;
	}
	memset8(p, c);
	//LOGI("draw_background");
}


/*
 *  Graphics display (8 pixels)
 */

#ifdef __i386
inline
#endif
#ifdef GLOBAL_VARS
static void draw_graphics(void)
#else
void MOS6569::draw_graphics(void)
#endif
{
	uint8 *p = chunky_ptr + x_scroll;
	uint8 c[4], data;

	if (!draw_this_line)
		return;
	if (ud_border_on) {
		draw_background();
		return;
	}

	switch (display_idx) {

		case 0:		// Standard text
			c[0] = b0c_color;
			c[1] = colors[color_data];
			goto draw_std;

		case 1:		// Multicolor text
			if (color_data & 8) {
				c[0] = b0c_color;
				c[1] = b1c_color;
				c[2] = b2c_color;
				c[3] = colors[color_data & 7];
				goto draw_multi;
			} else {
				c[0] = b0c_color;
				c[1] = colors[color_data];
				goto draw_std;
			}

		case 2:		// Standard bitmap
			c[0] = colors[char_data];
			c[1] = colors[char_data >> 4];
			goto draw_std;

		case 3:		// Multicolor bitmap
			c[0]= b0c_color;
			c[1] = colors[char_data >> 4];
			c[2] = colors[char_data];
			c[3] = colors[color_data];
			goto draw_multi;

		case 4:		// ECM text
			if (char_data & 0x80)
				if (char_data & 0x40)
					c[0] = b3c_color;
				else
					c[0] = b2c_color;
			else
				if (char_data & 0x40)
					c[0] = b1c_color;
				else
					c[0] = b0c_color;
			c[1] = colors[color_data];
			goto draw_std;

		case 5:		// Invalid multicolor text
			memset8(p, colors[0]);
			if (color_data & 8) {
				fore_mask_ptr[0] |= ((gfx_data & 0xaa) | (gfx_data & 0xaa) >> 1) >> x_scroll;
				fore_mask_ptr[1] |= ((gfx_data & 0xaa) | (gfx_data & 0xaa) >> 1) << (8-x_scroll);
			} else {
				fore_mask_ptr[0] |= gfx_data >> x_scroll;
				fore_mask_ptr[1] |= gfx_data << (7-x_scroll);
			}
			return;

		case 6:		// Invalid standard bitmap
			memset8(p, colors[0]);
			fore_mask_ptr[0] |= gfx_data >> x_scroll;
			fore_mask_ptr[1] |= gfx_data << (7-x_scroll);
			return;

		case 7:		// Invalid multicolor bitmap
			memset8(p, colors[0]);
			fore_mask_ptr[0] |= ((gfx_data & 0xaa) | (gfx_data & 0xaa) >> 1) >> x_scroll;
			fore_mask_ptr[1] |= ((gfx_data & 0xaa) | (gfx_data & 0xaa) >> 1) << (8-x_scroll);
			return;

		default:	// Can't happen
			return;
	}

draw_std:

	fore_mask_ptr[0] |= gfx_data >> x_scroll;
	fore_mask_ptr[1] |= gfx_data << (7-x_scroll);

	data = gfx_data;
	p[7] = c[data & 1]; data >>= 1;
	p[6] = c[data & 1]; data >>= 1;
	p[5] = c[data & 1]; data >>= 1;
	p[4] = c[data & 1]; data >>= 1;
	p[3] = c[data & 1]; data >>= 1;
	p[2] = c[data & 1]; data >>= 1;
	p[1] = c[data & 1]; data >>= 1;
	p[0] = c[data];
	return;

draw_multi:

	fore_mask_ptr[0] |= ((gfx_data & 0xaa) | (gfx_data & 0xaa) >> 1) >> x_scroll;
	fore_mask_ptr[1] |= ((gfx_data & 0xaa) | (gfx_data & 0xaa) >> 1) << (8-x_scroll);

	data = gfx_data;
	p[7] = p[6] = c[data & 3]; data >>= 2;
	p[5] = p[4] = c[data & 3]; data >>= 2;
	p[3] = p[2] = c[data & 3]; data >>= 2;
	p[1] = p[0] = c[data];
	return;
}


/*
 *  Sprite display
 */

#ifdef GLOBAL_VARS
inline static void draw_sprites(void)
#else
inline void MOS6569::draw_sprites(void)
#endif
{
	int i;
	int snum, sbit;		// Sprite number/bit mask
	int spr_coll=0, gfx_coll=0;

	// Clear sprite collision buffer
	{
		uint32 *lp = (uint32 *)spr_coll_buf - 1;
		for (i=0; i<DISPLAY_X/4; i++)
			*++lp = 0;
	}

	// Loop for all sprites
	for (snum=0, sbit=1; snum<8; snum++, sbit<<=1) {

		// Is sprite visible?
		if ((spr_draw & sbit) && mx[snum] <= DISPLAY_X-32) {
#ifdef __POWERPC__
			uint8 *p = (uint8 *)chunky_tmp + mx[snum] + 8;
#else
			uint8 *p = chunky_line_start + mx[snum] + 8;
#endif
			uint8 *q = spr_coll_buf + mx[snum] + 8;
			uint8 color = spr_color[snum];

			// Fetch sprite data and mask
			uint32 sdata = (spr_draw_data[snum][0] << 24) | (spr_draw_data[snum][1] << 16) | (spr_draw_data[snum][2] << 8);

			int spr_mask_pos = mx[snum] + 8;	// Sprite bit position in fore_mask_buf
			
			uint8 *fmbp = fore_mask_buf + (spr_mask_pos / 8);
			int sshift = spr_mask_pos & 7;
			uint32 fore_mask = (((*(fmbp+0) << 24) | (*(fmbp+1) << 16) | (*(fmbp+2) << 8)
				  		    | (*(fmbp+3))) << sshift) | (*(fmbp+4) >> (8-sshift));

			if (mxe & sbit) {		// X-expanded
				if (mx[snum] > DISPLAY_X-56)
					continue;

				uint32 sdata_l = 0, sdata_r = 0, fore_mask_r;
				fore_mask_r = (((*(fmbp+4) << 24) | (*(fmbp+5) << 16) | (*(fmbp+6) << 8)
						| (*(fmbp+7))) << sshift) | (*(fmbp+8) >> (8-sshift));

				if (mmc & sbit) {	// Multicolor mode
					uint32 plane0_l, plane0_r, plane1_l, plane1_r;

					// Expand sprite data
					sdata_l = MultiExpTable[sdata >> 24 & 0xff] << 16 | MultiExpTable[sdata >> 16 & 0xff];
					sdata_r = MultiExpTable[sdata >> 8 & 0xff] << 16;

					// Convert sprite chunky pixels to bitplanes
					plane0_l = (sdata_l & 0x55555555) | (sdata_l & 0x55555555) << 1;
					plane1_l = (sdata_l & 0xaaaaaaaa) | (sdata_l & 0xaaaaaaaa) >> 1;
					plane0_r = (sdata_r & 0x55555555) | (sdata_r & 0x55555555) << 1;
					plane1_r = (sdata_r & 0xaaaaaaaa) | (sdata_r & 0xaaaaaaaa) >> 1;

					// Collision with graphics?
					if ((fore_mask & (plane0_l | plane1_l)) || (fore_mask_r & (plane0_r | plane1_r))) {
						gfx_coll |= sbit;
						if (mdp & sbit)	{
							plane0_l &= ~fore_mask;	// Mask sprite if in background
							plane1_l &= ~fore_mask;
							plane0_r &= ~fore_mask_r;
							plane1_r &= ~fore_mask_r;
						}
					}

					// Paint sprite
					for (i=0; i<32; i++, plane0_l<<=1, plane1_l<<=1) {
						uint8 col;
						if (plane1_l & 0x80000000) {
							if (plane0_l & 0x80000000)
								col = mm1_color;
							else
								col = color;
						} else {
							if (plane0_l & 0x80000000)
								col = mm0_color;
							else
								continue;
						}
						if (q[i])
							spr_coll |= q[i] | sbit;
						else {
							p[i] = col;
							q[i] = sbit;
						}
					}
					for (; i<48; i++, plane0_r<<=1, plane1_r<<=1) {
						uint8 col;
						if (plane1_r & 0x80000000) {
							if (plane0_r & 0x80000000)
								col = mm1_color;
							else
								col = color;
						} else {
							if (plane0_r & 0x80000000)
								col = mm0_color;
							else
								continue;
						}
						if (q[i])
							spr_coll |= q[i] | sbit;
						else {
							p[i] = col;
							q[i] = sbit;
						}
					}

				} else {			// Standard mode

					// Expand sprite data
					sdata_l = ExpTable[sdata >> 24 & 0xff] << 16 | ExpTable[sdata >> 16 & 0xff];
					sdata_r = ExpTable[sdata >> 8 & 0xff] << 16;

					// Collision with graphics?
					if ((fore_mask & sdata_l) || (fore_mask_r & sdata_r)) {
						gfx_coll |= sbit;
						if (mdp & sbit)	{
							sdata_l &= ~fore_mask;	// Mask sprite if in background
							sdata_r &= ~fore_mask_r;
						}
					}

					// Paint sprite
					for (i=0; i<32; i++, sdata_l<<=1)
						if (sdata_l & 0x80000000) {
							if (q[i])	// Collision with sprite?
								spr_coll |= q[i] | sbit;
							else {		// Draw pixel if no collision
								p[i] = color;
								q[i] = sbit;
							}
						}
					for (; i<48; i++, sdata_r<<=1)
						if (sdata_r & 0x80000000) {
							if (q[i]) 	// Collision with sprite?
								spr_coll |= q[i] | sbit;
							else {		// Draw pixel if no collision
								p[i] = color;
								q[i] = sbit;
							}
						}
				}

			} else {				// Unexpanded

				if (mmc & sbit) {	// Multicolor mode
					uint32 plane0, plane1;

					// Convert sprite chunky pixels to bitplanes
					plane0 = (sdata & 0x55555555) | (sdata & 0x55555555) << 1;
					plane1 = (sdata & 0xaaaaaaaa) | (sdata & 0xaaaaaaaa) >> 1;

					// Collision with graphics?
					if (fore_mask & (plane0 | plane1)) {
						gfx_coll |= sbit;
						if (mdp & sbit) {
							plane0 &= ~fore_mask;	// Mask sprite if in background
							plane1 &= ~fore_mask;
						}
					}

					// Paint sprite
					for (i=0; i<24; i++, plane0<<=1, plane1<<=1) {
						uint8 col;
						if (plane1 & 0x80000000) {
							if (plane0 & 0x80000000)
								col = mm1_color;
							else
								col = color;
						} else {
							if (plane0 & 0x80000000)
								col = mm0_color;
							else
								continue;
						}
						if (q[i])
							spr_coll |= q[i] | sbit;
						else {
							p[i] = col;
							q[i] = sbit;
						}
					}

				} else {			// Standard mode

					// Collision with graphics?
					if (fore_mask & sdata) {
						gfx_coll |= sbit;
						if (mdp & sbit)
							sdata &= ~fore_mask;	// Mask sprite if in background
					}
	
					// Paint sprite
					for (i=0; i<24; i++, sdata<<=1)
						if (sdata & 0x80000000) {
							if (q[i]) {	// Collision with sprite?
								spr_coll |= q[i] | sbit;
							} else {		// Draw pixel if no collision
								p[i] = color;
								q[i] = sbit;
							}
						}
				}
			}
		}
	}

	if (ThePrefs.SpriteCollisions) {

		// Check sprite-sprite collisions
		if (clx_spr)
			clx_spr |= spr_coll;
		else {
			clx_spr |= spr_coll;
			irq_flag |= 0x04;
			if (irq_mask & 0x04) {
				irq_flag |= 0x80;
				the_cpu->TriggerVICIRQ();
			}
		}

		// Check sprite-background collisions
		if (clx_bgr)
			clx_bgr |= gfx_coll;
		else {
			clx_bgr |= gfx_coll;
			irq_flag |= 0x02;
			if (irq_mask & 0x02) {
				irq_flag |= 0x80;
				the_cpu->TriggerVICIRQ();
			}
		}
	}
}


#ifdef __POWERPC__
static asm void fastcopy(register uchar *dst, register uchar *src);
static asm void fastcopy(register uchar *dst, register uchar *src)
{
	lfd		fp0,0(src)
	lfd		fp1,8(src)
	lfd		fp2,16(src)
	lfd		fp3,24(src)
	lfd		fp4,32(src)
	lfd		fp5,40(src)
	lfd		fp6,48(src)
	lfd		fp7,56(src)
	addi	src,src,64
	stfd	fp0,0(dst)
	stfd	fp1,8(dst)
	stfd	fp2,16(dst)
	stfd	fp3,24(dst)
	stfd	fp4,32(dst)
	stfd	fp5,40(dst)
	stfd	fp6,48(dst)
	stfd	fp7,56(dst)
	addi	dst,dst,64

	lfd		fp0,0(src)
	lfd		fp1,8(src)
	lfd		fp2,16(src)
	lfd		fp3,24(src)
	lfd		fp4,32(src)
	lfd		fp5,40(src)
	lfd		fp6,48(src)
	lfd		fp7,56(src)
	addi	src,src,64
	stfd	fp0,0(dst)
	stfd	fp1,8(dst)
	stfd	fp2,16(dst)
	stfd	fp3,24(dst)
	stfd	fp4,32(dst)
	stfd	fp5,40(dst)
	stfd	fp6,48(dst)
	stfd	fp7,56(dst)
	addi	dst,dst,64

	lfd		fp0,0(src)
	lfd		fp1,8(src)
	lfd		fp2,16(src)
	lfd		fp3,24(src)
	lfd		fp4,32(src)
	lfd		fp5,40(src)
	lfd		fp6,48(src)
	lfd		fp7,56(src)
	addi	src,src,64
	stfd	fp0,0(dst)
	stfd	fp1,8(dst)
	stfd	fp2,16(dst)
	stfd	fp3,24(dst)
	stfd	fp4,32(dst)
	stfd	fp5,40(dst)
	stfd	fp6,48(dst)
	stfd	fp7,56(dst)
	addi	dst,dst,64

	lfd		fp0,0(src)
	lfd		fp1,8(src)
	lfd		fp2,16(src)
	lfd		fp3,24(src)
	lfd		fp4,32(src)
	lfd		fp5,40(src)
	lfd		fp6,48(src)
	lfd		fp7,56(src)
	addi	src,src,64
	stfd	fp0,0(dst)
	stfd	fp1,8(dst)
	stfd	fp2,16(dst)
	stfd	fp3,24(dst)
	stfd	fp4,32(dst)
	stfd	fp5,40(dst)
	stfd	fp6,48(dst)
	stfd	fp7,56(dst)
	addi	dst,dst,64

	lfd		fp0,0(src)
	lfd		fp1,8(src)
	lfd		fp2,16(src)
	lfd		fp3,24(src)
	lfd		fp4,32(src)
	lfd		fp5,40(src)
	lfd		fp6,48(src)
	lfd		fp7,56(src)
	addi	src,src,64
	stfd	fp0,0(dst)
	stfd	fp1,8(dst)
	stfd	fp2,16(dst)
	stfd	fp3,24(dst)
	stfd	fp4,32(dst)
	stfd	fp5,40(dst)
	stfd	fp6,48(dst)
	stfd	fp7,56(dst)
	addi	dst,dst,64

	lfd		fp0,0(src)
	lfd		fp1,8(src)
	lfd		fp2,16(src)
	lfd		fp3,24(src)
	lfd		fp4,32(src)
	lfd		fp5,40(src)
	lfd		fp6,48(src)
	lfd		fp7,56(src)
	addi	src,src,64
	stfd	fp0,0(dst)
	stfd	fp1,8(dst)
	stfd	fp2,16(dst)
	stfd	fp3,24(dst)
	stfd	fp4,32(dst)
	stfd	fp5,40(dst)
	stfd	fp6,48(dst)
	stfd	fp7,56(dst)
	addi	dst,dst,64
	blr		
}
#endif


/*
 *  Emulate one clock cycle, returns true if new raster line has started
 */

// Set BA low
#define SetBALow \
	if (!the_cpu->BALow) { \
		first_ba_cycle = the_c64->CycleCounter; \
		the_cpu->BALow = true; \
	}

// Turn on display if Bad Line
#define DisplayIfBadLine \
	if (is_bad_line) \
		display_state = true;

// Turn on display and matrix access if Bad Line
#define FetchIfBadLine \
	if (is_bad_line) { \
		display_state = true; \
		SetBALow; \
	}

// Turn on display and matrix access and reset RC if Bad Line
#define RCIfBadLine \
	if (is_bad_line) { \
		display_state = true; \
		rc = 0; \
		SetBALow; \
	}

// Idle access
#define IdleAccess \
	read_byte(0x3fff)

// Refresh access
#define RefreshAccess \
	read_byte(0x3f00 | ref_cnt--)

// Turn on sprite DMA if necessary
#define CheckSpriteDMA \
	mask = 1; \
	for (i=0; i<8; i++, mask<<=1) \
		if ((me & mask) && (raster_y & 0xff) == my[i]) { \
			spr_dma_on |= mask; \
			mc_base[i] = 0; \
			if (mye & mask) \
				spr_exp_y &= ~mask; \
		}

// Fetch sprite data pointer
#define SprPtrAccess(num) \
	spr_ptr[num] = read_byte(matrix_base | 0x03f8 | num) << 6;

// Fetch sprite data, increment data counter
#define SprDataAccess(num, bytenum) \
	if (spr_dma_on & (1 << num)) { \
		spr_data[num][bytenum] = read_byte(mc[num] & 0x3f | spr_ptr[num]); \
		mc[num]++; \
	} else if (bytenum == 1) \
		IdleAccess;

// Sample border color and increment chunky_ptr and fore_mask_ptr
#define SampleBorder \
	if (draw_this_line) { \
		if (border_on) \
			border_color_sample[cycle-13] = ec_color; \
		chunky_ptr += 8; \
		fore_mask_ptr++; \
	}


bool MOS6569::EmulateCycle(void)
{
	uint8 mask;
	int i;

	switch (cycle) {

		// Fetch sprite pointer 3, increment raster counter, trigger raster IRQ,
		// test for Bad Line, reset BA if sprites 3 and 4 off, read data of sprite 3
		case 1:
			if (raster_y == TOTAL_RASTERS-1)

				// Trigger VBlank in cycle 2
				vblanking = true;

			else {

				// Increment raster counter
				raster_y++;

				// Trigger raster IRQ if IRQ line reached
				if (raster_y == irq_raster)
					raster_irq();

				// In line $30, the DEN bit controls if Bad Lines can occur
				if (raster_y == 0x30)
					bad_lines_enabled = ctrl1 & 0x10;

				// Bad Line condition?
				is_bad_line = (raster_y >= FIRST_DMA_LINE && raster_y <= LAST_DMA_LINE && ((raster_y & 7) == y_scroll) && bad_lines_enabled);

				// Don't draw all lines, hide some at the top and bottom
				draw_this_line = (raster_y >= FIRST_DISP_LINE && raster_y <= LAST_DISP_LINE && !frame_skipped);
			}

			// First sample of border state
			border_on_sample[0] = border_on;

			SprPtrAccess(3);
			SprDataAccess(3, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x18))
				the_cpu->BALow = false;
			break;

		// Set BA for sprite 5, read data of sprite 3
		case 2:
			if (vblanking) {

				// Vertical blank, reset counters
				raster_y = vc_base = 0;
				ref_cnt = 0xff;
				lp_triggered = vblanking = false;

				if (!(frame_skipped = --skip_counter))
					skip_counter = ThePrefs.SkipFrames;

				the_c64->VBlank(!frame_skipped);

				// Get bitmap pointer for next frame. This must be done
				// after calling the_c64->VBlank() because the preferences
				// and screen configuration may have been changed there
				chunky_line_start = the_display->BitmapBase();
				xmod = the_display->BitmapXMod();

				// Trigger raster IRQ if IRQ in line 0
				if (irq_raster == 0)
					raster_irq();

			}

			// Our output goes here
#ifdef __POWERPC__
			chunky_ptr = (uint8 *)chunky_tmp;
#else
			chunky_ptr = chunky_line_start;
#endif

			// Clear foreground mask
			memset(fore_mask_buf, 0, DISPLAY_X/8);
			fore_mask_ptr = fore_mask_buf;

			SprDataAccess(3,1);
			SprDataAccess(3,2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x20)
				SetBALow;
			break;

		// Fetch sprite pointer 4, reset BA is sprite 4 and 5 off
		case 3:
			SprPtrAccess(4);
			SprDataAccess(4, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x30))
				the_cpu->BALow = false;
			break;

		// Set BA for sprite 6, read data of sprite 4 
		case 4:
			SprDataAccess(4, 1);
			SprDataAccess(4, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x40)
				SetBALow;
			break;

		// Fetch sprite pointer 5, reset BA if sprite 5 and 6 off
		case 5:
			SprPtrAccess(5);
			SprDataAccess(5, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x60))
				the_cpu->BALow = false;
			break;

		// Set BA for sprite 7, read data of sprite 5
		case 6:
			SprDataAccess(5, 1);
			SprDataAccess(5, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x80)
				SetBALow;
			break;

		// Fetch sprite pointer 6, reset BA if sprite 6 and 7 off
		case 7:
			SprPtrAccess(6);
			SprDataAccess(6, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0xc0))
				the_cpu->BALow = false;
			break;

		// Read data of sprite 6
		case 8:
			SprDataAccess(6, 1);
			SprDataAccess(6, 2);
			DisplayIfBadLine;
			break;

		// Fetch sprite pointer 7, reset BA if sprite 7 off
		case 9:
			SprPtrAccess(7);
			SprDataAccess(7, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x80))
				the_cpu->BALow = false;
			break;

		// Read data of sprite 7
		case 10:
			SprDataAccess(7, 1);
			SprDataAccess(7, 2);
			DisplayIfBadLine;
			break;

		// Refresh, reset BA
		case 11:
			RefreshAccess;
			DisplayIfBadLine;
			the_cpu->BALow = false;
			break;

		// Refresh, turn on matrix access if Bad Line
		case 12:
			RefreshAccess;
			FetchIfBadLine;
			break;

		// Refresh, turn on matrix access if Bad Line, reset raster_x, graphics display starts here
		case 13:
			draw_background();
			SampleBorder;
			RefreshAccess;
			FetchIfBadLine;
			raster_x = 0xfffc;
			break;

		// Refresh, VCBASE->VCCOUNT, turn on matrix access and reset RC if Bad Line
		case 14:
			draw_background();
			SampleBorder;
			RefreshAccess;
			RCIfBadLine;
			vc = vc_base;
			break;

		// Refresh and matrix access, increment mc_base by 2 if y expansion flipflop is set
		case 15:
			draw_background();
			SampleBorder;
			RefreshAccess;
			FetchIfBadLine;

			for (i=0; i<8; i++)
				if (spr_exp_y & (1 << i))
					mc_base[i] += 2;

			ml_index = 0;
			matrix_access();
			break;

		// Graphics and matrix access, increment mc_base by 1 if y expansion flipflop is set
		// and check if sprite DMA can be turned off
		case 16:
			draw_background();
			SampleBorder;
			graphics_access();
			FetchIfBadLine;

			mask = 1;
			for (i=0; i<8; i++, mask<<=1) {
				if (spr_exp_y & mask)
					mc_base[i]++;
				if ((mc_base[i] & 0x3f) == 0x3f)
					spr_dma_on &= ~mask;
			}

			matrix_access();
			break;

		// Graphics and matrix access, turn off border in 40 column mode, display window starts here
		case 17:
			if (ctrl2 & 8) {
				if (raster_y == dy_stop)
					ud_border_on = true;
				else {
					if (ctrl1 & 0x10) {
						if (raster_y == dy_start)
							border_on = ud_border_on = false;
						else
							if (!ud_border_on)
								border_on = false;
					} else
						if (!ud_border_on)
							border_on = false;
				}
			}

			// Second sample of border state
			border_on_sample[1] = border_on;

			draw_background();
			draw_graphics();
			SampleBorder;
			graphics_access();
			FetchIfBadLine;
			matrix_access();
			break;

		// Turn off border in 38 column mode
		case 18:
			if (!(ctrl2 & 8)) {
				if (raster_y == dy_stop)
					ud_border_on = true;
				else {
					if (ctrl1 & 0x10) {
						if (raster_y == dy_start)
							border_on = ud_border_on = false;
						else
							if (!ud_border_on)
								border_on = false;
					} else
						if (!ud_border_on)
							border_on = false;
				}
			}

			// Third sample of border state
			border_on_sample[2] = border_on;

			// Falls through

		// Graphics and matrix access
		case 19: case 20: case 21: case 22: case 23: case 24:
		case 25: case 26: case 27: case 28: case 29: case 30:
		case 31: case 32: case 33: case 34: case 35: case 36:
		case 37: case 38: case 39: case 40: case 41: case 42:
		case 43: case 44: case 45: case 46: case 47: case 48:
		case 49: case 50: case 51: case 52: case 53: case 54:	// Gnagna...
			draw_graphics();
			SampleBorder;
			graphics_access();
			FetchIfBadLine;
			matrix_access();
			last_char_data = char_data;
			break;

		// Last graphics access, turn off matrix access, turn on sprite DMA if Y coordinate is
		// right and sprite is enabled, handle sprite y expansion, set BA for sprite 0
		case 55:
			draw_graphics();
			SampleBorder;
			graphics_access();
			DisplayIfBadLine;

			// Invert y expansion flipflop if bit in MYE is set
			mask = 1;
			for (i=0; i<8; i++, mask<<=1)
				if (mye & mask)
					spr_exp_y ^= mask;
			CheckSpriteDMA;

			if (spr_dma_on & 0x01) {	// Don't remove these braces!
				SetBALow;
			} else
				the_cpu->BALow = false;
			break;

		// Turn on border in 38 column mode, turn on sprite DMA if Y coordinate is right and
		// sprite is enabled, set BA for sprite 0, display window ends here
		case 56:
			if (!(ctrl2 & 8))
				border_on = true;

			// Fourth sample of border state
			border_on_sample[3] = border_on;

			draw_graphics();
			SampleBorder;
			IdleAccess;
			DisplayIfBadLine;
			CheckSpriteDMA;

			if (spr_dma_on & 0x01)
				SetBALow;
			break;

		// Turn on border in 40 column mode, set BA for sprite 1, paint sprites
		case 57:
			if (ctrl2 & 8)
				border_on = true;

			// Fifth sample of border state
			border_on_sample[4] = border_on;

			// Sample spr_disp_on and spr_data for sprite drawing
			if ((spr_draw = spr_disp_on) != 0)
				memcpy(spr_draw_data, spr_data, 8*4);

			// Turn off sprite display if DMA is off
			mask = 1;
			for (i=0; i<8; i++, mask<<=1)
				if ((spr_disp_on & mask) && !(spr_dma_on & mask))
					spr_disp_on &= ~mask;

			draw_background();
			SampleBorder;
			IdleAccess;
			DisplayIfBadLine;
			if (spr_dma_on & 0x02)
				SetBALow;
			break;

		// Fetch sprite pointer 0, mc_base->mc, turn on sprite display if necessary,
		// turn off display if RC=7, read data of sprite 0
		case 58:
			draw_background();
			SampleBorder;

			mask = 1;
			for (i=0; i<8; i++, mask<<=1) {
				mc[i] = mc_base[i];
				if ((spr_dma_on & mask) && (raster_y & 0xff) == my[i])
					spr_disp_on |= mask;
			}
			SprPtrAccess(0);
			SprDataAccess(0, 0);

			if (rc == 7) {
				vc_base = vc;
				display_state = false;
			}
			if (is_bad_line || display_state) {
				display_state = true;
				rc = (rc + 1) & 7;
			}
			break;

		// Set BA for sprite 2, read data of sprite 0
		case 59:
			draw_background();
			SampleBorder;
			SprDataAccess(0, 1);
			SprDataAccess(0, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x04)
				SetBALow;
			break;

		// Fetch sprite pointer 1, reset BA if sprite 1 and 2 off, graphics display ends here
		case 60:
			draw_background();
			SampleBorder;

			if (draw_this_line) {

				// Draw sprites
				if (spr_draw && ThePrefs.SpritesOn)
					draw_sprites();

				// Draw border
#ifdef __POWERPC__
				if (border_on_sample[0])
					for (i=0; i<4; i++)
						memset8((uint8 *)chunky_tmp+i*8, border_color_sample[i]);
				if (border_on_sample[1])
					memset8((uint8 *)chunky_tmp+4*8, border_color_sample[4]);
				if (border_on_sample[2])
					for (i=5; i<43; i++)
						memset8((uint8 *)chunky_tmp+i*8, border_color_sample[i]);
				if (border_on_sample[3])
					memset8((uint8 *)chunky_tmp+43*8, border_color_sample[43]);
				if (border_on_sample[4])
					for (i=44; i<DISPLAY_X/8; i++)
						memset8((uint8 *)chunky_tmp+i*8, border_color_sample[i]);
#else
				if (border_on_sample[0])
					for (i=0; i<4; i++)
						memset8(chunky_line_start+i*8, border_color_sample[i]);
				if (border_on_sample[1])
					memset8(chunky_line_start+4*8, border_color_sample[4]);
				if (border_on_sample[2])
					for (i=5; i<43; i++)
						memset8(chunky_line_start+i*8, border_color_sample[i]);
				if (border_on_sample[3])
					memset8(chunky_line_start+43*8, border_color_sample[43]);
				if (border_on_sample[4])
					for (i=44; i<DISPLAY_X/8; i++)
						memset8(chunky_line_start+i*8, border_color_sample[i]);
#endif

#ifdef __POWERPC__
				// Copy temporary buffer to bitmap
				fastcopy(chunky_line_start, (uint8 *)chunky_tmp);
#endif

				// Increment pointer in chunky buffer
				chunky_line_start += xmod;
			}

			SprPtrAccess(1);
			SprDataAccess(1, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x06))
				the_cpu->BALow = false;
			break;

		// Set BA for sprite 3, read data of sprite 1
		case 61:
			SprDataAccess(1, 1);
			SprDataAccess(1, 2);
			DisplayIfBadLine;
			if (spr_dma_on & 0x08)
				SetBALow;
			break;

		// Read sprite pointer 2, reset BA if sprite 2 and 3 off, read data of sprite 2
		case 62:
			SprPtrAccess(2);
			SprDataAccess(2, 0);
			DisplayIfBadLine;
			if (!(spr_dma_on & 0x0c))
				the_cpu->BALow = false;
			break;

		// Set BA for sprite 4, read data of sprite 2
		case 63:
			SprDataAccess(2, 1);
			SprDataAccess(2, 2);
			DisplayIfBadLine;

			if (raster_y == dy_stop)
				ud_border_on = true;
			else
				if (ctrl1 & 0x10 && raster_y == dy_start)
					ud_border_on = false;

			if (spr_dma_on & 0x10)
				SetBALow;

			// Last cycle
			raster_x += 8;
			cycle = 1;
			return true;
	}

	// Next cycle
	raster_x += 8;
	cycle++;
	return false;
}
