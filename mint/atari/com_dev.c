/*
 * Copyright 1999, 2000 by Chris Felsch <C.Felsch@gmx.de>
 *
 * See COPYING for details of legal notes.
 *
 * Modified 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 *
 * File : com_dev.c
 *        communication device driver routines
 */

#include "com_dev.h"

#ifdef COM_DEBUG
#  define DEBUG_COM(x)      KERNEL_ALERT x
#else
#  define DEBUG_COM(x)
#endif

/*
 * routines (for extern see callstonx.s)
 */
extern long _cdecl stx_com_open	    (FILEPTR *f);
extern long _cdecl stx_com_write    (FILEPTR *f, const char *buf, long bytes);
extern long _cdecl stx_com_read	    (FILEPTR *f, char *buf, long bytes);
static long _cdecl stx_com_lseek    (FILEPTR *f, long where, int whence);
extern long _cdecl stx_com_ioctl    (FILEPTR *f, int mode, void *buf);
static long _cdecl stx_com_datime   (FILEPTR *f, ushort *timeptr, int rwflag);
extern long _cdecl stx_com_close    (FILEPTR *f, int pid);
static long _cdecl stx_com_select   (FILEPTR *f, long proc, int mode);
static void _cdecl stx_com_unselect (FILEPTR *f, long proc, int mode);


/*
 * communication device driver map
 */
static DEVDRV com_devdrv =
{
    stx_com_open, stx_com_write, stx_com_read, stx_com_lseek, stx_com_ioctl,
    stx_com_datime, stx_com_close, stx_com_select, stx_com_unselect, 
    NULL, NULL /* writeb, readb not needed */
};


/*
 * communication device basic description
 */
static struct dev_descr com_dev_descr =
{
    &com_devdrv,
    0,
    0,
    NULL,
    0,
    S_IFCHR |
    S_IRUSR |
    S_IWUSR |
    S_IRGRP |
    S_IWGRP |
    S_IROTH |
    S_IWOTH,
    NULL,
    0,
    0
};


DEVDRV * com_init(void)
{
    if ( stonx_cookie->flags & STNX_IS_COM ) {
	long r;
	
	r = d_cntl(DEV_INSTALL, "u:\\dev\\"MINT_COM_NAME, 
		   (long)&com_dev_descr);
	if ( r >= 0) {
	    return (DEVDRV *) &com_devdrv;
	} else {
	    c_conws( MSG_PFAILURE( "u:\\dev\\"MINT_COM_NAME, 
				   "Dcntl(DEV_INSTALL,...) failed" ) );
	    DEBUG(( "Return value was %li", r ));
	}
    } else {
	c_conws (MSG_PFAILURE("u:\\dev\\"MINT_COM_NAME,
			      "not activated at STonX"));
    }

    return NULL;
}


/*
 * communication device 
 * Only open(), close(), read(), write() and ioctl() will be 
 * redirected to STonX
 */
static long _cdecl stx_com_lseek(FILEPTR *f, long where, int whence)
{
    DEBUG_COM(("stx_com lseek"));
    
    UNUSED (f);
    UNUSED (whence);
    
    return (where == 0) ? 0 : EBADARG;
}


static long _cdecl stx_com_datime(FILEPTR *f, ushort *timeptr, int rwflag)
{
    DEBUG_COM(("stx_com datime"));
    
    UNUSED (f);
    
    if (rwflag)
	return EACCES;

    *timeptr++ = timestamp;
    *timeptr = datestamp;

    return E_OK;
}


static long _cdecl stx_com_select(FILEPTR *f, long proc, int mode)
{
    DEBUG_COM(("stx_com select: %x", mode));
    
    UNUSED (f);
    UNUSED (p);
    
    if ((mode == O_RDONLY) || (mode == O_WRONLY))
    {
	/* we're always ready to read/write */
	return 1;
    }
    
    /* other things we don't care about */
    return E_OK;
}

static void _cdecl stx_com_unselect(FILEPTR *f, long proc, int mode)
{
    DEBUG_COM(("stx_com unselect"));
    
    UNUSED (f); 
    UNUSED (p); 
    UNUSED (mode);
    /* nothing to do */
}
