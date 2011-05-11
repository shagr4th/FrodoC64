/*
 *  CmdPipe.cpp
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

#include "CmdPipe.h"


extern "C" {
	#include <string.h>
	#include <unistd.h>
	#include <stdlib.h>
	#include <sys/wait.h>
	#include <sys/types.h>
	#include <string.h>
	#include <signal.h>

#if defined(__alpha__)
	#include <cma.h>
#endif

#if defined(AIX)
	#include <sys/select.h>
#else
	#include <unistd.h>
#endif

#if defined(__linux__)
	#include <sys/time.h>
#endif

	#include <time.h>
	#include <errno.h>
}

static void kaputt(const char * c1, const char * c2) {
	fprintf(stderr,"error: %s%s\n",c1,c2);
	exit(20);
}

Pipe::Pipe(void) : fail(true) {

	fds[0] = 0;
	fds[1] = 1;

	if (-1 == pipe(fds)) {
		kaputt("Pipe: ","unable to create pipe");
		return;
	}

	fail = false;
	return;
}

Pipe::~Pipe(void) {

	if (! fail) {
		close(fds[0]);
		 close(fds[1]);
	}
	return;
}

unsigned long Pipe::ewrite(const void * buf, unsigned long len) {

	unsigned long wsum = 0; 
	while (len) {
		long wlen;
                
		wlen = ::write(fds[1], buf, (long) len);
		if (wlen <= 0) {
			kaputt("Pipe::ewrite ","write-error");
		}
                
		len -= wlen;
		buf = (void*) ((char*) buf + wlen);
		wsum += wlen;
	}
	return wsum;
}

unsigned long Pipe::eread(void * buf, unsigned long len) {

	unsigned long rsum = 0;
	while (len) {
		long rlen;

		rlen = ::read(fds[0], buf, (long) len);

		if (rlen <= 0) {
			kaputt("Pipe::eread ","read-error");
		}
                
		len -= rlen;
		buf = (void*) ((char*) buf + rlen);
		rsum += rlen;
	}
	return rsum;
}

int Pipe::probe(void) const {
        
	fd_set set;
	FD_ZERO(&set);
	FD_SET(fds[0], &set);
        
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
        
	int res;
// Use the following commented line for HP-UX < 10.20
//	res = select(FD_SETSIZE, (int *)&set, (int *)0, (int *)0, &tv);
	res = select(FD_SETSIZE, &set, (fd_set *)0, (fd_set *)0, &tv);
        
	if (res > 0) return -1;
	return 0;

}

CmdPipe::CmdPipe(const char * command, const char * arg, int nicediff) : childpid(0), fail(true) {
        
	if (tocmd.fail || fromcmd.fail) {
		kaputt("CmdPipe: ","unable to initialize pipes");
		return;
	}
                
	childpid = fork();
        
	if (childpid == -1) {
		childpid = 0;
		kaputt("CmdPipe: ","unable to fork process");
		return;
	}
        
	if (childpid == 0) {
                
		if (nicediff) {
			if (-1 == nice(nicediff)) {
				fprintf(stderr,"CmdPipe: unable to change nice-level (non-fatal)");
			}
		}
                
		dup2(tocmd.get_read_fd(), STDIN_FILENO);
                
		dup2(fromcmd.get_write_fd(), STDOUT_FILENO);
		execlp(command, "Frodo_GUI", arg, (char *)0);
		kaputt("CmdPipe: unable to execute child process ",command);
		_exit(0); // exit (and do NOT call destructors etc..)
	}

	fail = false;
	return;
}

CmdPipe::~CmdPipe(void) {
        
	if (childpid) {
		int status;
		waitpid(childpid, &status, 0);
                                
		if (status != 0) {
			fprintf(stderr,"~CmdPipe child process returned error\n");
		}
	}
}
