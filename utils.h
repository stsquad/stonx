/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef UTILS_H
#define UTILS_H

#include "defs.h"
#include <stdlib.h>

extern void error (char *fmt, ...);
extern long load_file (char *file, B *d);

#ifdef NO_BZERO
#define bzero(_x,_y) memset(_x,0,_y)
#endif

#ifdef NO_STRDUP
#define strdup(_x) strcpy(malloc(strlen(_x)+1),_x)
#endif

#if NO_CLEAN_EXIT
#define OnExit(_x)
#elif USE_ON_EXIT
#define OnExit(_x) (void)on_exit(_x,0)
#else
#define OnExit(_x) (void)atexit(_x)
#endif

extern void cleanup(int);

#endif /* UTILS_H */
