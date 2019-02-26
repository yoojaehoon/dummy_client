# ------------------------------------------------
# This Makefile is created by kskim. 2008.12.23
# ------------------------------------------------

CC = g++
RANLIB = ranlib
MAKEDEPEND = makedepend
CFLAGS = -g -Wall -D_LINUX -D_THREAD_SAFE -D_REENTRANT 

OS  := $(notdir $(shell pwd))
PGM := ../lib/libinno-$(OS).a

CINCS = -I../inc
OFILES = bash.o condext.o debug.o exceptrpt.o file64.o fold64.o \
	icmpmgr4.o icmpmgr6.o icmpmgr.o ihuman.o inetsock.o \
	killall.o mutext.o mycalc.o mydefs.o rcscript.o snprintf.o \
	stat64.o svcapp.o tcpsock4.o tcpsock6.o tcpsock.o tcpssl.o \
	aqueue.o thread.o tilde.o urlutil.o which.o zentdd.o

all: lib

%.o: %.cpp
	$(CC) -O2 -c $(CFLAGS) $(CINCS) $<

lib: $(OFILES)
	ar rc $(PGM) $(OFILES)
	$(RANLIB) $(PGM)

clean : 
	rm -rf $(PGM) *.o 

depend : $(CFILES)
	$(MAKEDEPEND) $(CINCS) $(CFILES)


# DO NOT DELETE
