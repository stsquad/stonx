/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef NATIVE_H
#define NATIVE_H

#include "defs.h"

extern int add_drive (char drive, char *fname);
extern void call_native (UL as, UL func);
extern int Bios(void);
extern void disk_init(void);

#endif /* NATIVE_H */
