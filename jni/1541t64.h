/*
 *  1541t64.h - 1541 emulation in archive-type files (.t64/LYNX/.p00)
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

#ifndef _1541T64_H
#define _1541T64_H

#include "IEC.h"


/*
 *  Definitions
 */

// Archive types
enum {
	TYPE_T64,			// C64S tape file
	TYPE_LYNX,			// C64 LYNX archive
	TYPE_P00			// .p00 file
};

// Archive file drive class
class ArchDrive : public Drive {
public:
	ArchDrive(IEC *iec, const char *filepath);
	virtual ~ArchDrive();

	virtual uint8 Open(int channel, const uint8 *name, int name_len);
	virtual uint8 Close(int channel);
	virtual uint8 Read(int channel, uint8 &byte);
	virtual uint8 Write(int channel, uint8 byte, bool eoi);
	virtual void Reset(void);

private:
	bool change_arch(const char *path);

	uint8 open_file(int channel, const uint8 *name, int name_len);
	uint8 open_directory(int channel, const uint8 *pattern, int pattern_len);
	bool find_first_file(const uint8 *pattern, int pattern_len, int &num);
	void close_all_channels(void);

	virtual void rename_cmd(const uint8 *new_file, int new_file_len, const uint8 *old_file, int old_file_len);
	virtual void initialize_cmd(void);
	virtual void validate_cmd(void);

	FILE *the_file;			// File pointer for archive file
	int archive_type;		// File/archive type (see defines above)
	vector<c64_dir_entry> file_info;	// Vector of file information structs for all files in the archive

	char dir_title[16];		// Directory title
	FILE *file[16];			// File pointers for each of the 16 channels (all temporary files)

	uint8 read_char[16];	// Buffers for one-byte read-ahead
};


/*
 *  Functions
 */

// Check whether file with given header (64 bytes) and size looks like one
// of the file types supported by this module
extern bool IsArchFile(const char *path, const uint8 *header, long size);

// Read directory of archive file into (empty) c64_dir_entry vector
extern bool ReadArchDirectory(const char *path, vector<c64_dir_entry> &vec);

#endif
