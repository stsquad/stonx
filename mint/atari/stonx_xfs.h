/*
 * Copyright 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 *
 * See COPYING for details of legal notes.
 *
 * Modified by Chris Felsch <C.Felsch@gmx.de>.
 *
 * File : stonx_xfs.h
 *        Filesystem driver routines
 */

#ifndef _stonx_xfs_h_
#define _stonx_xfs_h_

#include "global.h"

extern FILESYS stonx_filesys;        /* needed by callstonx.s */

extern FILESYS *stonx_fs_init(void); /* init filesystem driver */

#endif _stonx_xfs_h
