/*
 * Filename:    mint_comm.c
 * Version:     0.5
 * Author:      Chris Felsch
 * Started:     2000
 * Target O/S:  MiNT running on StonX (Linux)
 * Description: STonX communication device backend
 * 
 * Note:        Please send suggestions, patches or bug reports to 
 *              Chris Felsch <C.Felsch@gmx.de>
 * 
 * Copying:     Copyright 2000 Chris Felsch (C.Felsch@gmx.de)
 * 
 * History:     2000-11-19 (CF)  started
 *              2001-01-26 (MJK) merged and debugging changed
 *
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

#ifdef atarist
#error This file has to be part of STonX!
#endif

#include <errno.h>
#include <stdio.h>

#include "../main.h"
#include "../mem.h"
#include "../toserror.h"

#include "mint_defs.h"

#include "../options.h" /* see, if MINT_COM is set to 0 */

#ifndef MINT_COM
# define MiNT_COM 1
#endif

#if MINT_COM

#ifndef DEBUG_MINT_COM
# define DEBUG_MINT_COM 0
#endif

#if DEBUG_MINT_COM
#  define DBG( _args... ) fprintf(stderr, "mint_com_"## _args );
#  define DBG_NP( _args...) fprintf(stderr, ## _args );
#else
#  define DBG( _args... )
#  define DBG_NP( _args...)
#endif

static int in_use = 0;

UL mint_com_close(MINT_FILEPTR *f, W pid)
{
    DBG("close\n"
        "  in_use: %d\n", in_use);
    
    if (in_use)
        in_use = 0;
    else
        return unix2toserrno(errno, TOS_EIHNDL);
    
    return TOS_E_OK;
}


UL mint_com_ioctl(MINT_FILEPTR *f, W mode, void *buf)
{
    int    fd = LM_L(&(f->devinfo)), rv;
    DBG("ioctl\n"
	"  mode: 0x%04x\n", mode);

    switch (mode)
    {
      case 0x00:
	  if ( verbose )
	      fprintf(stderr, "HALT detected\n");
	  stonx_exit();
	  exit(0);
	  break;
	  
      case 0x01:
	  if ( verbose )
	      fprintf(stderr,"WARM boot detected\n");
	  break;
	  
      case 0x02:
	  if ( verbose )
	      fprintf(stderr,"COLD boot detected\n");
	  break;

      default:
	  if ( verbose )
	      fprintf( stderr, "Unknown MiNT-COM-IOCTL mode 0x%04x.\n", mode );
	  return TOS_EINVFN;
    }
    return TOS_E_OK;
}


UL mint_com_open(MINT_FILEPTR *f)
{
    DBG("open\n"
	"  in_use: %d\n", in_use);
    if (in_use)
        return unix2toserrno(errno, TOS_EACCDN);
    else
        in_use = 1;

    return TOS_E_OK;
}


UL mint_com_read(MINT_FILEPTR *f, char *buf, L bytes)
{
    int    fd = LM_L(&(f->devinfo));
    int    rv = 0;

   DBG("com_read\n"
       "  bytes: %ld\n", bytes);

#if 0
    rv = read(fd, buf, bytes);
    DBG_NP("  read(): rv= %d\n", rv);
#endif

    if (rv == -1)
        return unix2toserrno( errno, TOS_EACCDN );
    
    return rv;
}


UL mint_com_write(MINT_FILEPTR *f, const char *buf, L bytes)
{
    int    fd = LM_L(&(f->devinfo));
    int    rv = 0;

    DBG("write\n"
	"  bytes: %ld\n", bytes);

#if 0
    rv = write(fd, buf, bytes);
    DBG_NP("  write(): rv= %d\n", rv);
#endif

    if (rv == -1)
        return unix2toserrno( errno, TOS_EACCDN );

    return rv;
}

#endif /* MINT_COM */
