/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef DEBUG_H
#define DEBUG_H

#include "options.h"
#include <stdio.h>

#define DEBUG_MEMACC 0
#define DEBUG_EXCEPT 0	
#define DEBUG_TRAPS	 0

#define D(_x) debug _x
#define T(_x) debug _x

#if 0
#if TRACE && !defined(LINT)
#define T(_x, _args...) do{fprintf(debugfile,"pc=%lx ",pc);fprintf(debugfile,_x, ## _args);} while(0)
extern FILE *debugfile;
#else
#define T(_x, _args...)
#endif
#endif

#define Warn(_x) fprintf(stderr, _x);

extern void init_debug(void);
extern void exit_debug(void);
void debug (char *fmt, ...);

#endif /* DEBUG_H */

