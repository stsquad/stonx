/*
 * Copyright 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 *
 * See COPYING for details of legal notes.
 *
 * Modified by Chris Felsch <C.Felsch@gmx.de>.
 *
 * File : stonx_xfs.c
 *        Filesystem driver routines
 */

#include "stonx_xfs.h"

/*
 * interface to STonX (see callstonx.s)
 */
extern long     _cdecl stx_fs_native_init(struct kerinfo *k, short fs_devnum);

/*
 * assembler routines (see callstonx.s)
 */
extern long     _cdecl stx_fs_root	 (int drv, fcookie *fc);
extern long     _cdecl stx_fs_lookup	 (fcookie *dir, const char *name, 
					  fcookie *fc);
extern long     _cdecl stx_fs_creat      (fcookie *dir, const char *name,
					  unsigned mode, int attrib, 
					  fcookie *fc);
extern DEVDRV * _cdecl stx_fs_getdev	 (fcookie *fc, long *devspecial);
extern long     _cdecl stx_fs_getxattr	 (fcookie *file, XATTR *xattr);
extern long     _cdecl stx_fs_chattr	 (fcookie *file, int attr);
extern long     _cdecl stx_fs_chown	 (fcookie *file, int uid, int gid);
extern long     _cdecl stx_fs_chmode	 (fcookie *file, unsigned mode);
extern long     _cdecl stx_fs_mkdir	 (fcookie *dir, const char *name, 
					  unsigned mode);
extern long     _cdecl stx_fs_rmdir	 (fcookie *dir, const char *name);
extern long     _cdecl stx_fs_remove	 (fcookie *dir, const char *name);
extern long     _cdecl stx_fs_getname	 (fcookie *relto, fcookie *dir, 
					  char *pathname, int size);
extern long     _cdecl stx_fs_rename	 (fcookie *olddir, char *oldname,
					  fcookie *newdir,
					  const char *newname);
extern long     _cdecl stx_fs_opendir	 (DIR *dirh, int tosflag);
extern long     _cdecl stx_fs_readdir	 (DIR *dirh, char *name, int namelen,
					  fcookie *fc);
extern long     _cdecl stx_fs_rewinddir	 (DIR *dirh);
extern long     _cdecl stx_fs_closedir	 (DIR *dirh);
extern long     _cdecl stx_fs_pathconf	 (fcookie *dir, int which);
extern long     _cdecl stx_fs_dfree	 (fcookie *dir, long *buf);
extern long     _cdecl stx_fs_writelabel (fcookie *dir, const char *name);
extern long     _cdecl stx_fs_readlabel	 (fcookie *dir, char *name,
					  int namelen);
extern long     _cdecl stx_fs_symlink	 (fcookie *dir, const char *name,
					  const char *to);
extern long     _cdecl stx_fs_readlink	 (fcookie *dir, char *buf, int len);
extern long     _cdecl stx_fs_hardlink	 (fcookie *fromdir, 
					  const char *fromname,
					  fcookie *todir, const char *toname);
extern long     _cdecl stx_fs_fscntl	 (fcookie *dir, const char *name, 
					  int cmd, long arg);
extern long     _cdecl stx_fs_dskchng	 (int drv, int mode);
extern long     _cdecl stx_fs_release	 (fcookie *);
extern long     _cdecl stx_fs_dupcookie	 (fcookie *new, fcookie *old);
extern long     _cdecl stx_fs_sync	 (void);
extern long     _cdecl stx_fs_mknod	 (fcookie *dir, const char *name, 
					  ulong mode);
extern long     _cdecl stx_fs_unmount	 (int drv);

/*
 * filesystem driver map
 */
FILESYS stonx_filesys = 
{
    (struct filesys *)0,	/* next */
    /*
     * FS_KNOPARSE         kernel shouldn't do parsing
     * FS_CASESENSITIVE    file names are case sensitive
     * FS_NOXBIT           if a file can be read, it can be executed
     * FS_LONGPATH         file system understands "size" argument to "getname"
     * FS_NO_C_CACHE       don't cache cookies for this filesystem
     * FS_DO_SYNC          file system has a sync function
     * FS_OWN_MEDIACHANGE  filesystem control self media change (dskchng)
     * FS_REENTRANT_L1     fs is level 1 reentrant
     * FS_REENTRANT_L2     fs is level 2 reentrant
     * FS_EXT_1            extensions level 1 - mknod & unmount
     * FS_EXT_2            extensions level 2 - additional place at the end
     * FS_EXT_3            extensions level 3 - stat & native UTC timestamps
     */
    (FS_NOXBIT|FS_CASESENSITIVE|FS_EXT_1),
    stx_fs_root, stx_fs_lookup, stx_fs_creat, stx_fs_getdev, stx_fs_getxattr,
    stx_fs_chattr, stx_fs_chown, stx_fs_chmode, stx_fs_mkdir, stx_fs_rmdir,
    stx_fs_remove, stx_fs_getname, stx_fs_rename, stx_fs_opendir, 
    stx_fs_readdir, stx_fs_rewinddir, stx_fs_closedir, stx_fs_pathconf,
    stx_fs_dfree, stx_fs_writelabel, stx_fs_readlabel, stx_fs_symlink, 
    stx_fs_readlink, stx_fs_hardlink, stx_fs_fscntl, stx_fs_dskchng,
    stx_fs_release, stx_fs_dupcookie, stx_fs_sync, stx_fs_mknod, 
    stx_fs_unmount,
    0L, 				/* stat64() */
    0L, 0L, 0L,			        /* res1-3 */
    0L, 0L, 				/* lock, sleeperd */
    0L, 0L				/* block(), deblock() */
};

/*
 * filesystem basic description
 */
static struct fs_descr stonx_fs_descr = 
{
    &stonx_filesys,
    0, /* this is filled in by MiNT at FS_MOUNT */
    0, /* FIXME: what about flags? */
    {0,0,0,0}  /* reserved */
};


FILESYS *stonx_fs_init(void) {
    if ( stonx_cookie->flags & STNX_IS_XFS ) {
	long r;
	int succ = 0;

	/* Try to install */
	r = d_cntl (FS_INSTALL, "u:\\", (long) &stonx_fs_descr);
	if (r != (long)kernel)
	{
	    c_conws (MSG_PFAILURE("u:\\"MINT_FS_NAME,
				  "Dcntl(FS_INSTALL,...) failed"));
	    DEBUG(("Return value was %li", r));
	    return NULL; /* Nothing installed, so nothing to stay resident */
	} else {
	    succ |= 1;
	    /* mount */
	    r = d_cntl(FS_MOUNT, "u:\\"MINT_FS_NAME, (long) &stonx_fs_descr);
	    if (r != stonx_fs_descr.dev_no )
	    {
		c_conws (MSG_PFAILURE("u:\\"MINT_FS_NAME,
				      "Dcnt(FS_MOUNT,...) failed"));
		DEBUG(("Return value was %li", r));
	    } else {
		succ |= 2;
		/* init */
		r = stx_fs_native_init(KERNEL, stonx_fs_descr.dev_no);
		if ( r < 0 ) {
		    c_conws (MSG_PFAILURE("u:\\"MINT_FS_NAME,
					  "native init failed"));
		    DEBUG(("Return value was %li", r));
		} else {
		    return &stonx_filesys; /* We where successfull */
		}
	    }
	}

	/* Try to uninstall, if necessary */
	if ( succ & 2 ) {
	    /* unmount */
	    r = d_cntl(FS_UNMOUNT, "u:\\"MINT_FS_NAME, 
		       (long) &stonx_fs_descr);
	    DEBUG(("Dcntl(FS_UNMOUNT,...) = %li", r ));
	    if ( r < 0 ) {
		return (FILESYS *) 1; /* Can't uninstall, 
				       * because unmount failed */ 
	    }
	}
	if ( succ & 1 ) {
	    /* uninstall */
	    r = d_cntl(FS_UNINSTALL, "u:\\"MINT_FS_NAME, 
		       (long) &stonx_fs_descr);
	    DEBUG(("Dcntl(FS_UNINSTALL,...) = %li", r ));
	    if ( r < 0 ) {
		return (FILESYS *) 1; /* Can't say NULL,
				       * because uninstall failed */
	    }
	}
    } else {
	c_conws (MSG_PFAILURE("u:\\"MINT_FS_NAME,
			      "not activated at STonX"));
    }

    return NULL; /* Nothing installed, so nothing to stay resident */
}
