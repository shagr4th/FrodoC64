/*
 *  1541fs.h - 1541 emulation in host file system
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

#ifndef _1541FS_H
#define _1541FS_H

#include "IEC.h"


class FSDrive : public Drive {
public:
	FSDrive(IEC *iec, const char *path);
	virtual ~FSDrive();

	virtual uint8 Open(int channel, const uint8 *name, int name_len);
	virtual uint8 Close(int channel);
	virtual uint8 Read(int channel, uint8 &byte);
	virtual uint8 Write(int channel, uint8 byte, bool eoi);
	virtual void Reset(void);

private:
	bool change_dir(char *dirpath);

	uint8 open_file(int channel, const uint8 *name, int name_len);
	uint8 open_directory(int channel, const uint8 *pattern, int pattern_len);
	void find_first_file(char *pattern);
	void close_all_channels(void);

	virtual void initialize_cmd(void);
	virtual void validate_cmd(void);

	char dir_path[256];		// Path to directory
	char orig_dir_path[256]; // Original directory path
	char dir_title[16];		// Directory title
	FILE *file[16];			// File pointers for each of the 16 channels

	uint8 read_char[16];	// Buffers for one-byte read-ahead
};

#endif
