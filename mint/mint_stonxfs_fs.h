/*
 * Part of STonX
 * Copyright (c) Markus Kohm, 1998 - 2001
 *
 * see mint_stonxfs_fs.c for details
 *
 */

#ifndef _mint_stonxfs_fs_h_

#include "../defs.h"
#include "mint_defs.h"

extern UL mint_fs_drv;       /* file system driver */
extern UW mint_fs_devnum;    /* unix device number */

extern UL mint_fs_root       ( W drv, MINT_FCOOKIE *fc );
extern UL mint_fs_lookup     ( MINT_FCOOKIE *dir, const char *name, 
			       MINT_FCOOKIE *fc );
extern UL mint_fs_creat      ( MINT_FCOOKIE *dir, const char *name, UW mode, 
			       W attrib, MINT_FCOOKIE *fc );
extern UL mint_fs_getdev     ( MINT_FCOOKIE *fc, L *devspecial );
extern UL mint_fs_getxattr   ( MINT_FCOOKIE *file, MINT_XATTR *xattr );
extern UL mint_fs_chattr     ( MINT_FCOOKIE *file, W attr );
extern UL mint_fs_chown      ( MINT_FCOOKIE *file, W uid, W gid );
extern UL mint_fs_chmode     ( MINT_FCOOKIE *file, UW mode );
extern UL mint_fs_mkdir      ( MINT_FCOOKIE *dir, const char *name, UW mode );
extern UL mint_fs_rmdir      ( MINT_FCOOKIE *dir, const char *name );
extern UL mint_fs_remove     ( MINT_FCOOKIE *dir, const char *name );
extern UL mint_fs_getname    ( MINT_FCOOKIE *relto, MINT_FCOOKIE *dir,
			       char *pathname, W size );
extern UL mint_fs_rename     ( MINT_FCOOKIE *olddir, char *oldname, 
			       MINT_FCOOKIE *newdir, const char *newname );
extern UL mint_fs_opendir    ( MINT_DIR *dirh, W tosflag );
extern UL mint_fs_readdir    ( MINT_DIR *dirh, char *name, W namelen,
			       MINT_FCOOKIE *fc );
extern UL mint_fs_rewinddir  ( MINT_DIR *dirh );
extern UL mint_fs_closedir   ( MINT_DIR *dirh );
extern UL mint_fs_pathconf   ( MINT_FCOOKIE *dir, W which );
extern UL mint_fs_dfree      ( MINT_FCOOKIE *dir, L *buf );
extern UL mint_fs_writelabel ( MINT_FCOOKIE *dir, const char *name);
extern UL mint_fs_readlabel  ( MINT_FCOOKIE *dir, char *name, W namelen );
extern UL mint_fs_symlink    ( MINT_FCOOKIE *dir, const char *name, 
			       const char *to );
extern UL mint_fs_readlink   ( MINT_FCOOKIE *dir, char *buf, W len );
extern UL mint_fs_hardlink   ( MINT_FCOOKIE *fromdir, const char *fromname,
			       MINT_FCOOKIE *todir, const char *toname );
extern UL mint_fs_fscntl     ( MINT_FCOOKIE *dir, const char *name, W cmd,
			       L arg );
extern UL mint_fs_dskchng    ( W drv );
extern UL mint_fs_release    ( MINT_FCOOKIE *fc );
extern UL mint_fs_dupcookie  ( MINT_FCOOKIE *new, MINT_FCOOKIE *old );
extern UL mint_fs_sync       ( void );
extern UL mint_fs_mknod      ( MINT_FCOOKIE *dir, const char *name, L mode );
extern UL mint_fs_unmount    ( W drv );

#endif /* _mint_stonxfs_fs_h_ */
