/*
 * Filename:    main.c
 * Version:     
 * Author:      Markus Kohm, Chris Felsch
 * Started:     1998
 * Target OS:   MiNT running on StonX (Linux)
 * Description: STonX-file-system for MiNT
 *              Most of this file system has to be integrated into STonX.
 *              The native functions in STonX uses the native 
 *              Linux/Unix-filesystem.
 *              The MiNT file system part only calls these native functions.
 * 
 * Note:        Please send suggestions, patches or bug reports to 
 *              Chris Felsch <C.Felsch@gmx.de>
 * 
 * Copying:     Copyright 1998 Markus Kohm
 * 
 * Copying:     Copyright 1998 Markus Kohm
 *              Copyright 2000 Chris Felsch (C.Felsch@gmx.de)
 * 
 * History:	2000-09-10 (CF)  fs_mknod(), fs_unmount() added
 *		2000-11-12 (CF)  ported to freemint 1.15.10b
 *				 stx_com added
 *              2001-02-02 (MJK) Read the cookie.
 *                               Redesign of initialization and more files
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

#include "global.h"
#include "stonxfsv.h"
#include "stonx_xfs.h"
#include "serial_dev.h"
#include "com_dev.h"

extern FILESYS * _cdecl init(struct kerinfo *k);

/*
 * global kerinfo structure
 */
struct kerinfo *KERNEL;

/*
 * global STonX cookie
 */
STONX_COOKIE *stonx_cookie;

#ifdef DEV_DEBUG
static void show_cookie(void) {
    int i;
    char name[10];
    DEBUG(("cookieadr   = %08lx", (unsigned long)stonx_cookie));
    DEBUG(("Magic       = %08lx (should be %08lx)",
	   stonx_cookie->magic, COOKIE_STon));
    DEBUG(("Major       = %04x", stonx_cookie->stonx_major ));
    DEBUG(("Minor       = %04x", stonx_cookie->stonx_minor ));
    DEBUG(("Patch Major = %04x", stonx_cookie->stonx_pmajor ));
    DEBUG(("Patch Minor = %04x", stonx_cookie->stonx_pminor ));
    for ( i = 0; i < 8 && stonx_cookie->screenname[i]; i++ )
	name[i] = stonx_cookie->screenname[i];
    name[i] = 0;
    DEBUG(("screenname  = \"%s\"", name));
    DEBUG(("Flags       = %08lx\n", stonx_cookie->flags ));
}
#endif

FILESYS * _cdecl init(struct kerinfo *k)
{
    long cval;
    FILESYS *RetVal = NULL;
    void *rval;

    KERNEL = k;
    
    c_conws (MSG_BOOT);
    c_conws (MSG_GREET);

#ifdef ALPHA
    c_conws (MSG_ALPHA);
#elif defined (BETA)
    c_conws (MSG_BETA);
#endif

    DEBUG(("Found MiNT %ld.%ld with kerinfo version %ld",
	   (long)MINT_MAJOR, (long)MINT_MINOR, (long)MINT_KVERSION));

    if ( get_toscookie( COOKIE_STon, &cval ) ) {
	c_conws( MSG_FAILURE("STon cookie not found") );
	return NULL;
    } else {
	stonx_cookie = (void *)cval;
#ifdef DEV_DEBUG
	show_cookie();
#endif
    }
    
    /* check for MiNT version */
#if 0
    if ( MINT_MAJOR < 1 || (MINT_MAJOR == 1 && MINT_MINOR < 15))
#else
    if ( MINT_MAJOR < 1 || (MINT_MAJOR == 1 && MINT_MINOR < 14))
#endif
    {
	c_conws (MSG_OLDMINT);
	c_conws (MSG_FAILURE("MiNT to old"));

	return NULL;
    }

    /* install filesystem */
    RetVal = stonx_fs_init();

    /* install serial device */
    rval = serial_init();
    if ( rval && !RetVal )
	RetVal = (FILESYS *) 1;

    /* install communication device */
    rval = com_init();
    if ( rval && !RetVal )
	RetVal = (FILESYS *) 1;
    
    return RetVal;
}

#if 0
#endif
