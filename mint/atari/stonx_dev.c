/*
 * Copyright 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 *
 * See COPYING for details of legal notes.
 *
 * Modified by Chris Felsch <C.Felsch@gmx.de>.
 *
 * File : stonx_dev.c
 *        Filesystem device driver routines
 */

#include "stonx_dev.h"

/*
 * assembler routines (see callstonx.s)
 */
extern long _cdecl stx_fs_dev_open     (FILEPTR *f);
extern long _cdecl stx_fs_dev_write    (FILEPTR *f, const char *buf, 
					long bytes);
extern long _cdecl stx_fs_dev_read     (FILEPTR *f, char *buf, long bytes);
extern long _cdecl stx_fs_dev_lseek    (FILEPTR *f, long where, int whence);
extern long _cdecl stx_fs_dev_ioctl    (FILEPTR *f, int mode, void *buf);
extern long _cdecl stx_fs_dev_datime   (FILEPTR *f, ushort *timeptr, 
					int rwflag);
extern long _cdecl stx_fs_dev_close    (FILEPTR *f, int pid);  
extern long _cdecl stx_fs_dev_select   (FILEPTR *f, long proc, int mode);
extern void _cdecl stx_fs_dev_unselect (FILEPTR *f, long proc, int mode);

DEVDRV stonx_fs_devdrv = 
{
    stx_fs_dev_open, stx_fs_dev_write, stx_fs_dev_read, stx_fs_dev_lseek, 
    stx_fs_dev_ioctl, stx_fs_dev_datime, stx_fs_dev_close, stx_fs_dev_select, 
    stx_fs_dev_unselect,
    NULL, NULL /* writeb, readb not needed */
};
