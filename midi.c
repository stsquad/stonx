/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#include "defs.h"
#include "utils.h"
#include "main.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>

#if MIDI
static char *device="/dev/sequencer";
static int pp[2],cp[2];
static int ppid, cpid;
extern long tv_usec;
extern int verbose;

static void midi_server()
{
	fd_set rd_fds,wr_fds,ex_fds;
	struct timeval timeout;
	unsigned char buf;
	int fd=open(device,O_RDWR);
	FD_ZERO(&rd_fds);
	FD_ZERO(&wr_fds);
	FD_ZERO(&ex_fds);
	for(;;)
	{
		FD_SET(pp[0],&rd_fds);
		timeout.tv_sec=0;
		timeout.tv_usec=tv_usec;
		if (select(FD_SETSIZE,&rd_fds,&wr_fds,&ex_fds,&timeout)<=0)
		{
			if (kill(ppid,0)<0)
			{
				if (verbose) fprintf(stderr,"MIDI server terminated\n");
				exit(0);
			}
		}
		if (FD_ISSET(pp[0],&rd_fds))
		{
			read(pp[0],&buf,1);
			write(fd,&buf,1);
		}
	}
}
#endif

void init_midi(void)
{
#if MIDI
	pipe(pp);
	pipe(cp);
	ppid=getpid();
	cpid=fork();
	switch(cpid)
	{
		case 0:
			midi_server();
		case -1:
			fprintf(stderr,"Failed to fork() MIDI server\n");
		default:
			break;
	}
#endif
}

void midi_send(unsigned char data)
{
#if MIDI
	if (midi) write(pp[1],&data,1);
#endif
}

