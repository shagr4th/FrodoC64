/*
 *  CmdPipe.h
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

#ifndef CmdPipe_h
#define CmdPipe_h

extern "C" {
	#include <stdio.h>
	#include <sys/types.h>
}

class Pipe {

protected:

	int fds[2];

public:
        
	bool fail;

	Pipe(void);
	Pipe(int fdin, int fdout) : fail(false) {
		fds[0] = fdin;
		fds[1] = fdout;
	}
	~Pipe(void);
        
	unsigned long ewrite(const void * buf, unsigned long len);
	unsigned long eread (void * buf, unsigned long len);
        
	int get_read_fd(void) const {
		return fds[0];
	}
        
	int get_write_fd(void) const {
		return fds[1];
	}
        
	int probe(void) const;
};

class CmdPipe {
        
protected:

	Pipe tocmd;
	Pipe fromcmd;
        
	int childpid;
        
public:
        
	bool fail;
        
	CmdPipe(const char * command, const char * arg, int nicediff = 0);
	~CmdPipe(void);
        
	unsigned long ewrite(const void * buf, unsigned long len) {
		return tocmd.ewrite(buf, len);
	}
        
	unsigned long eread (void * buf, unsigned long len) {
		return fromcmd.eread(buf, len);
	}
        
	int get_read_fd(void) const {
		return fromcmd.get_read_fd();
	}
        
	int get_write_fd(void) const {
		return tocmd.get_write_fd();
	}
        
	int probe(void) const {
		return fromcmd.probe();
	}

};

#endif // CmdPipe_h
