/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#include "defs.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include "mem.h"
#include "utils.h"
#include "version.h"

extern int show_profile;
extern void dump_profile(void);

#if PROFILE
void ui_handler (int sig)
{	
	show_profile=1;
	flags ^= F_PROFILE;
}
#endif

void init_ui (void)
{
#if PROFILE
	sigset_t set;
	struct sigaction a;
	sigemptyset(&set);
	a.sa_handler = ui_handler;
	a.sa_mask = set;
	a.sa_flags = 0;
	sigaction (SIGUSR1, &a, NULL);
	OnExit(dump_profile);
#endif
#if 0
	printf("Press `?' for help\n");
#endif
}

int read_cmd (void)
{
	char c=0;
	int d;
	fd_set x;
	struct timeval t;
	t.tv_sec=0; t.tv_usec=0;
	FD_ZERO(&x);
	FD_SET(0,&x);
	d = select(1,&x,NULL,NULL,&t);
	if (d==1)
		read(0,&c,1);
	return (int)c;
}

void check_ui (void)
{
	int x;
	return;
	x = read_cmd();
	switch(x)
	{
		case '?':
			printf("Commands:\n ? = Help\n\n");
			break;
	}
}
