/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef GEMDOS_H
#define GEMDOS_H

#include "defs.h"
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define MAXDRIVES 26

extern int gemdos_initialized;
extern int redirect_cconws;
extern int drive_bits;
extern int drive_fd[MAXDRIVES];

#define SYSBASE() LM_UL(MEM(0x4f2))
#define OS_BEG()  LM_UL(MEM(8+SYSBASE()))
#define TOS_VERSION(os_beg) LM_UW(MEM((os_beg)+2))

extern int Gemdos(UL as);
extern int Bios(void);
extern int disk_bpb(int dev);
extern int disk_rw(char *args);
extern void gemdos_post(void);
extern void init_gemdos(void);
extern int add_gemdos_drive (char d, char *path);
extern const char *get_gemdos_drive (char d);
extern int add_drive (char drive, char *fname);
extern UL get_bp(void);

#endif /* GEMDOS_H */

