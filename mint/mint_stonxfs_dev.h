/*
 * Part of STonX
 * Copyright (c) Markus Kohm, 1998 - 2001
 *
 * see mint_stonxfs_dev.c for details
 *
 */

#ifndef _mint_stonxfs_dev_h_

#include "../defs.h"
#include "mint_defs.h"

extern UL mint_fs_devdrv; /* device driver */

extern UL mint_fs_dev_open    ( MINT_FILEPTR *f );
extern UL mint_fs_dev_write   ( MINT_FILEPTR *f, const char *buf, L bytes );
extern UL mint_fs_dev_read    ( MINT_FILEPTR *f, char *buf, L bytes );
extern UL mint_fs_dev_lseek   ( MINT_FILEPTR *f, L where, W whence );
extern UL mint_fs_dev_ioctl   ( MINT_FILEPTR *f, W mode, void *buf );
extern UL mint_fs_dev_datime  ( MINT_FILEPTR *f, W *timeptr, W rwflag );
extern UL mint_fs_dev_close   ( MINT_FILEPTR *f, W pid );
extern UL mint_fs_dev_select  ( MINT_FILEPTR *f, L proc, W mode );
extern UL mint_fs_dev_unselect( MINT_FILEPTR *f, L proc, W mode );

#endif /* _mint_stonxfs_dev_h */
