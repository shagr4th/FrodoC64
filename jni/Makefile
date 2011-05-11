# Makefile.in for Frodo (generic Unix/X11)

## Version information
VERSION  = 4
REVISION = 2

## System specific configuration

SHELL = /bin/sh

prefix = /opt/local/frodo
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
datadir = ${prefix}/share

DESTDIR =

CXX = g++
CFLAGS = -g -O2 -fomit-frame-pointer -fno-exceptions -I/usr/include/SDL -D_REENTRANT -DHAVE_SDL -DXTHREADS -D_REENTRANT -DXUSE_MTSAFE_API -DORBIT2=1 -pthread -D__sdlui__ -D__zipfile__ -I/opt/local/libzip/include # -D__gp2x__
DEFS = -DHAVE_CONFIG_H -DDATADIR=\"$(datadir)/frodo/\" -DBINDIR=\"$(bindir)/\"
LDFLAGS =
LIBS =  -L/usr/lib -Wl,-rpath,/usr/lib -lSDL -lpthread -Wl,--export-dynamic -pthread -L/opt/local/libzip/lib -lzip -lz
INSTALL = /bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

## Files
OBJS    = main.o Display.o Prefs.o SID.o REU.o IEC.o 1541fs.o \
          1541d64.o 1541t64.o 1541job.o SAM.o
SLOBJS  = $(OBJS) C64.o CPUC64.o VIC.o CIA.o CPU1541.o
SLFLAGS = -DPRECISE_CPU_CYCLES=1 -DPRECISE_CIA_CYCLES=1 -DPC_IS_POINTER=0
SCOBJS  = $(OBJS) C64_SC.o CPUC64_SC.o VIC_SC.o CIA_SC.o CPU1541_SC.o CPU_common.o
SCFLAGS = -DFRODO_SC
SRCS    = main.cpp Display.cpp Prefs.cpp SID.cpp REU.cpp IEC.cpp 1541fs.cpp \
          1541d64.cpp 1541t64.cpp 1541job.cpp SAM.cpp C64.cpp \
          C64_SC.cpp CPUC64.cpp CPUC64_SC.cpp VIC.cpp VIC_SC.cpp CIA.cpp \
          CIA_SC.cpp CPU1541.cpp CPU1541_PC.cpp CPU1541_SC.cpp CPU_common.cpp

## Rules
.PHONY: install installdirs uninstall clean distclean depend dep
.SUFFIXES:
.SUFFIXES: .o .cpp .h

all: Frodo FrodoSC

Frodo: $(SLOBJS)
	$(CXX) -o Frodo $(LDFLAGS) $(SLOBJS) $(LIBS)

FrodoSC: $(SCOBJS)
	$(CXX) -o FrodoSC $(LDFLAGS) $(SCOBJS) $(LIBS)

install: Frodo FrodoSC installdirs
	$(INSTALL_PROGRAM) Frodo $(DESTDIR)$(bindir)/Frodo
	$(INSTALL_PROGRAM) FrodoSC $(DESTDIR)$(bindir)/FrodoSC
	$(INSTALL_PROGRAM) Frodo_GUI.tcl $(DESTDIR)$(bindir)/Frodo_GUI.tcl
	$(INSTALL_DATA) ../Kernal\ ROM $(DESTDIR)$(datadir)/frodo/Kernal\ ROM
	$(INSTALL_DATA) glade/Frodo.glade $(DESTDIR)$(datadir)/frodo/Frodo.glade

installdirs:
	$(SHELL) mkinstalldirs $(DESTDIR)$(bindir) $(DESTDIR)$(datadir)/frodo

uninstall:
	rm -f $(DESTDIR)$(bindir)/Frodo
	rm -f $(DESTDIR)$(bindir)/FrodoSC
	rm -f $(DESTDIR)$(bindir)/Frodo_GUI.tcl
	rm -f $(DESTDIR)$(datadir)/frodo/Kernal\ ROM
	rmdir $(DESTDIR)$(datadir)/frodo

clean:
	rm -f $(SLOBJS) $(SCOBJS)
	rm -f Frodo FrodoSC
	rm -f core* *.core *~ *.bak

distclean: clean
	rm -rf autom4te.cache
	rm -f Makefile sysconfig.h
	rm -f config.cache config.log config.status config.h

depend dep:
	makedepend $(CPPFLAGS) -Y. $(SRCS) 2>/dev/null

.cpp.o:
	$(CXX) $(DEFS) $(CFLAGS) -o $@ -c $*.cpp

C64.o: C64.cpp
	$(CXX) $(DEFS) $(CFLAGS) $(SLFLAGS) -o $@ -c $*.cpp

C64_SC.o: C64_SC.cpp
	$(CXX) $(DEFS) $(CFLAGS) $(SCFLAGS) -o $@ -c $*.cpp

CPUC64.o: CPUC64.cpp
	$(CXX) $(DEFS) $(CFLAGS) $(SLFLAGS) -o $@ -c $*.cpp

CPUC64_SC.o: CPUC64_SC.cpp
	$(CXX) $(DEFS) $(CFLAGS) $(SCFLAGS) -o $@ -c $*.cpp

CPU1541.o: CPU1541.cpp
	$(CXX) $(DEFS) $(CFLAGS) $(SLFLAGS) -o $@ -c $*.cpp

CPU1541_SC.o: CPU1541_SC.cpp
	$(CXX) $(DEFS) $(CFLAGS) $(SCFLAGS) -o $@ -c $*.cpp

VIC_SC.o: VIC_SC.cpp
	$(CXX) $(DEFS) $(CFLAGS) $(SCFLAGS) -o $@ -c $*.cpp

CIA_SC.o: CIA_SC.cpp
	$(CXX) $(DEFS) $(CFLAGS) $(SCFLAGS) -o $@ -c $*.cpp


#-------------------------------------------------------------------------
# DO NOT DELETE THIS LINE -- make depend depends on it.

main.o: sysdeps.h sysconfig.h main.h C64.h Display.h Prefs.h SAM.h
main.o: Basic_ROM.h Kernal_ROM.h Char_ROM.h 1541_ROM.h
Display.o: sysdeps.h sysconfig.h Display.h main.h Prefs.h Display_SDL.h
Prefs.o: sysdeps.h sysconfig.h Prefs.h Display.h C64.h main.h
SID.o: sysdeps.h sysconfig.h SID.h Prefs.h SID_SDL.h
REU.o: sysdeps.h sysconfig.h REU.h CPUC64.h C64.h Prefs.h
IEC.o: sysdeps.h sysconfig.h IEC.h 1541fs.h 1541d64.h 1541t64.h Prefs.h
IEC.o: Display.h main.h
1541fs.o: sysdeps.h sysconfig.h 1541fs.h IEC.h main.h Prefs.h
1541d64.o: sysdeps.h sysconfig.h 1541d64.h IEC.h Prefs.h C64.h main.h debug.h
1541t64.o: sysdeps.h sysconfig.h 1541t64.h IEC.h Prefs.h debug.h
1541job.o: sysdeps.h sysconfig.h 1541job.h CPU1541.h CIA.h Prefs.h C64.h
SAM.o: sysdeps.h sysconfig.h SAM.h C64.h CPUC64.h CPU1541.h CIA.h Prefs.h
SAM.o: VIC.h SID.h
C64.o: sysdeps.h sysconfig.h C64.h CPUC64.h CPU1541.h CIA.h Prefs.h VIC.h C64_x.h
C64.o: SID.h REU.h IEC.h 1541job.h Display.h
C64_SC.o: C64.cpp sysdeps.h sysconfig.h C64.h CPUC64.h CPU1541.h CIA.h
C64_SC.o: Prefs.h VIC.h SID.h REU.h IEC.h 1541job.h Display.h
CPUC64.o: sysdeps.h sysconfig.h CPUC64.h C64.h VIC.h SID.h CIA.h Prefs.h
CPUC64.o: REU.h IEC.h Display.h Version.h CPU_emulline.h
CPUC64_SC.o: sysdeps.h sysconfig.h CPUC64.h C64.h CPU_common.h VIC.h SID.h
CPUC64_SC.o: CIA.h Prefs.h REU.h IEC.h Display.h Version.h CPU_emulcycle.h
VIC.o: sysdeps.h sysconfig.h VIC.h C64.h CPUC64.h Display.h Prefs.h
VIC_SC.o: sysdeps.h sysconfig.h VIC.h C64.h CPUC64.h Display.h Prefs.h
CIA.o: sysdeps.h sysconfig.h CIA.h Prefs.h CPUC64.h C64.h CPU1541.h VIC.h
CIA_SC.o: sysdeps.h sysconfig.h CIA.h Prefs.h CPUC64.h C64.h CPU1541.h VIC.h
CPU1541.o: sysdeps.h sysconfig.h CPU1541.h CIA.h Prefs.h C64.h 1541job.h
CPU1541.o: Display.h CPU_emulline.h
CPU1541_SC.o: sysdeps.h sysconfig.h CPU1541.h CIA.h Prefs.h C64.h
CPU1541_SC.o: CPU_common.h 1541job.h Display.h CPU_emulcycle.h
CPU_common.o: sysdeps.h sysconfig.h CPU_common.h
