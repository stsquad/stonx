/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */

#include "defs.h"
#include "utils.h"
#include <stdio.h>
#include <stdarg.h>

extern int verbose;


long load_file (char *file, B *d)
{
	FILE *f;
	long length;

	f = fopen(file, "rb");
	if (f == NULL)
	{
		fprintf (stderr, "Error: File `%s' not found!\n", file);
		exit(3);
	}
	if (verbose)  fprintf (stderr, "Loading `%s'...\n", file);

	fseek(f, 0, SEEK_END);
	length = ftell(f);
	fseek(f, 0, SEEK_SET);

	return fread(d, 1, length, f);
}


void error (char *fmt, ...)
{
	va_list args;
	va_start(args,fmt);
	vfprintf(stderr, fmt, args);
   	va_end(args);
	exit(1);
}

int show_profile=0;

#if PROFILE
#include "proftabs.c"
void dump_profile (void)
{
	int i;
	if (!show_profile) return;
	for (i=0; i<NUM_OPCODES; i++)
	{
		printf("%20s %8d\n",prof_opname[i],prof_freq[i]);
	}
}

#else
void dump_profile(void)
{
}
#endif /* PROFILE */

void cleanup(int n)
{
	exit(n); /* sorry */
}

