/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#include "options.h"
#include "debug.h"
#include <stdio.h>
#include <stdarg.h>

FILE *debugfile;

void init_debug(void)
{
#if DEBUG
	debugfile = fopen("DEBUGFILE", "w");
#if 1
	setlinebuf(debugfile);
#endif
#endif
}

void exit_debug(void)
{
#if DEBUG
	fclose (debugfile);
#endif
}

void debug (char *fmt, ...)
{
#if DEBUG
	va_list args;
	va_start(args,fmt);
	vfprintf(debugfile, fmt, args);
   	va_end(args);
#endif
}
