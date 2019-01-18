ALL: default

CC           = gcc-4.8
CLINKER      = $(CC)
OPTFLAGS     = -O0


SHELL = /bin/sh

CFLAGS  =  -DREENTRANT -Wunused -Wall -g
CCFLAGS = $(CFLAGS)
LIBS =  -lpthread

EXECS = common.o handling_socket.o dsmexec dsmwrap truc exemple
default: $(EXECS)

dsmexec: dsmexec.o common.o handling_socket.o
	$(CLINKER) $(OPTFLAGS) -o dsmexec dsmexec.o  common.o handling_socket.o $(LIBS)
	mv dsmexec ./bin

dsmwrap: dsmwrap.o common.o handling_socket.o
	$(CLINKER) $(OPTFLAGS) -o dsmwrap dsmwrap.o  common.o handling_socket.o $(LIBS)
	mv dsmwrap ./bin

exemple: exemple.o dsm.o common.o handling_socket.o
	$(CLINKER) $(OPTFLAGS) -o exemple exemple.o common.o dsm.o handling_socket.o $(LIBS) 
		mv exemple ./bin

clean:
	@-/bin/rm -f *.o *~ PI* $(EXECS) *.out core
.c:
	$(CC) $(CFLAGS) -o $* $< $(LIBS)
.c.o:
	$(CC) $(CFLAGS) -c $<
.o:
	${CLINKER} $(OPTFLAGS) -o $* $*.o $(LIBS)
