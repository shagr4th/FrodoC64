/*
 *  el_Acorn.h - Somewhat faster (on Acorn) versions of the el_-functions
 *               of VIC.cpp
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


#ifdef GLOBAL_VARS
static inline void el_std_text(uint8 *p, uint8 *q, uint8 *r)
#else
inline void MOS6569::el_std_text(uint8 *p, uint8 *q, uint8 *r)
#endif
{
	register uint32 *lp = (uint32 *)p, *t;
	uint8 *cp = color_line;
	uint8 *mp = matrix_line;
	uint32 *tab = (uint32*)&TextColorTable[0][b0c][0][0];

	// Loop for 40 characters
	for (int i=0; i<40; i++) {
		uint8 data = r[i] = q[mp[i] << 3];

		t = tab + ((cp[i] & 15) << 13) + (data << 1);
		*lp++ = *t++; *lp++ = *t++;
	}
}


#ifdef GLOBAL_VARS
static inline void el_mc_text(uint8 *p, uint8 *q, uint8 *r)
#else
inline void MOS6569::el_mc_text(uint8 *p, uint8 *q, uint8 *r)
#endif
{
	register uint32 *wp = (uint32 *)p, *t;
	uint32 *tab = (uint32*)&TextColorTable[0][b0c][0][0];
	uint8 *cp = color_line;
	uint8 *mp = matrix_line;
	uint8 *lookup = (uint8*)mc_color_lookup;

	// Loop for 40 characters
	for (int i=0; i<40; i++) {
		register uint32 color = cp[i];
		uint8 data = q[mp[i] << 3];

		if (color & 8) {
			r[i] = (data & 0xaa) | (data & 0xaa) >> 1;
			lookup[6] = colors[color & 7];
			color = lookup[(data & 0xc0) >> 5] | (lookup[(data & 0x30) >> 3] << 16);
			*wp++ = color | (color << 8);
			color = lookup[(data & 0x0c) >> 1] | (lookup[(data & 0x03) << 1] << 16);
			*wp++ = color | (color << 8);

		} else { // Standard mode in multicolor mode
			r[i] = data;
			color = cp[i]; t = tab + (color << 13) + (data << 1);
			*wp++ = *t++; *wp++ = *t++;
		}
	}
}


#ifdef GLOBAL_VARS
static inline void el_std_bitmap(uint8 *p, uint8 *q, uint8 *r)
#else
inline void MOS6569::el_std_bitmap(uint8 *p, uint8 *q, uint8 *r)
#endif
{
	register uint32 *lp = (uint32 *)p, *t;
	uint8 *mp = matrix_line;

	// Loop for 40 characters
	for (int i=0; i<40; i++, q+=8) {
		uint8 data = r[i] = *q;
		uint8 h = mp[i];

		t = (uint32*)&TextColorTable[h >> 4][h & 15][data][0];
		*lp++ = *t++; *lp++ = *t++;
	}
}


#ifdef GLOBAL_VARS
static inline void el_mc_bitmap(uint8 *p, uint8 *q, uint8 *r)
#else
inline void MOS6569::el_mc_bitmap(uint8 *p, uint8 *q, uint8 *r)
#endif
{
	uint8 lookup[4];
	register uint32 *wp = (uint32 *)p;
	uint8 *cp = color_line;
	uint8 *mp = matrix_line;

	lookup[0] = b0c_color;

	// Loop for 40 characters
	for (int i=0; i<40; i++, q+=8) {
		uint8 data = *q;
		register uint32 h = mp[i];

		lookup[1] = colors[h >> 4];
		lookup[2] = colors[h];
		lookup[3] = colors[cp[i]];

		r[i] = (data & 0xaa) | (data & 0xaa) >> 1;

		h = lookup[data >> 6] | (lookup[(data >> 4) & 3] << 16); *wp++ = h | (h << 8);
		h = lookup[(data >> 2) & 3] | (lookup[data & 3] << 16);  *wp++ = h | (h << 8);
	}
}


#ifdef GLOBAL_VARS
static inline void el_ecm_text(uint8 *p, uint8 *q, uint8 *r)
#else
inline void MOS6569::el_ecm_text(uint8 *p, uint8 *q, uint8 *r)
#endif
{
	register uint32 *lp = (uint32 *)p, *t;
	uint8 *cp = color_line;
	uint8 *mp = matrix_line;
	uint8 *bcp = &b0c;

	// Loop for 40 characters
	for (int i=0; i<40; i++) {
		uint8 data = r[i] = mp[i];

		t = (uint32*)&TextColorTable[cp[i]][bcp[(data >> 6) & 3]][q[(data & 0x3f) << 3]][0];
		*lp++ = *t++; *lp++ = *t++;
	}
}


#ifdef GLOBAL_VARS
static inline void el_std_idle(uint8 *p, uint8 *r)
#else
inline void MOS6569::el_std_idle(uint8 *p, uint8 *r)
#endif
{
	uint8 data = *get_physical(ctrl1 & 0x40 ? 0x39ff : 0x3fff);
	uint32 *lp = (uint32 *)p;
	uint32 conv0 = TextColorTable[0][b0c][data][0].b;
	uint32 conv1 = TextColorTable[0][b0c][data][1].b;

	for (int i=0; i<40; i++) {
		*lp++ = conv0;
		*lp++ = conv1;
		*r++ = data;
	}
}


#ifdef GLOBAL_VARS
static inline void el_mc_idle(uint8 *p, uint8 *r)
#else
inline void MOS6569::el_mc_idle(uint8 *p, uint8 *r)
#endif
{
	uint8 data = *get_physical(0x3fff);
	register uint8 c0 = b0c_color, c1 = colors[0];
	register uint32 *lp = (uint32 *)p;
        register uint32 conv0, conv1;

	conv0 = (((data & 0xc0) == 0) ? c0 : c1) | (((data & 0x30) == 0) ? c0<<16 : c1<<16);
	conv1 = (((data & 0x0c) == 0) ? c0 : c1) | (((data & 0x03) == 0) ? c0<<16 : c1<<16);
	conv0 |= (conv0 << 8); conv1 |= (conv1 << 8);

	for (int i=0; i<40; i++) {
		*lp++ = conv0;
		*lp++ = conv1;
		*r++ = data;
	}
}
