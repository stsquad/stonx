/*
 * Filename:    mint_stonxfs_dev.c
 * Version:     0.5
 * Author:      Markus Kohm, Chris Felsch
 * Started:     1998
 * Target O/S:  MiNT running on StonX (Linux)
 * Description: STonX file-system device backend
 * 
 * Note:        Please send suggestions, patches or bug reports to 
 *              Chris Felsch <C.Felsch@gmx.de>
 * 
 * Copying:     Copyright 1998 Markus Kohm
 *              Copyright 2000 Chris Felsch (C.Felsch@gmx.de)
 * 
 * History:     2000-11-18 (CF)  re-structured
 *              2001-01-26 (MJK) reintegrated and debugging changed
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

#ifdef __TOS__
#error This file has to be part of STonX!
#endif

#include <sys/time.h>
#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "../main.h"
#include "../mem.h"

#include "mint_misc.h"

#include "../options.h" /* see, if MINT_STONXFS is set to 0 */

#ifndef MINT_STONXFS
# define MINT_STONXFS 1
#endif

#if MINT_STONXFS

#if DEBUG_MINT_STONXFS
#  define DBG( _args... ) fprintf(stderr, "mint_stonxfs_dev_"## _args );
#  define DBG_NP( _args...) fprintf(stderr, ## _args );
#else
#  define DBG( _args... )
#  define DBG_NP( _args...)
#endif

extern UW time2dos (time_t t);   /* from gemdos.c */
extern UW date2dos (time_t t);   /* from gemdos.c */


UL mint_fs_devdrv; /* device driver */


UL mint_fs_dev_close( MINT_FILEPTR *f, W pid )
{
    /* FIXME: Handle internal and external locks.
     *        See mint_fs_dev_ioctl for more information
     */
    DBG( "dev_close"
	 "  f   = %#08lx\n"
	 "    links   = %d\n"
	 "    flags   = %#02x\n"
	 "    pos     = %ld\n"
	 "    devinfo = %ld\n"
	 "    fc      = %#08lx\n"
	 "      fs    = %#08lx\n"
	 "      dev   = %#04x\n"
	 "      aux   = %#04x\n"
	 "      index = %#08lx\n"
	 "        parent   = %#08lx\n"
	 "        name     = \"%s\"\n"
	 "        usecnt   = %d\n"
	 "        childcnt = %d\n"
	 "  pid = %d\n",
	 (long)f,
	 (int)LM_UW( &(f->links) ),
	 (int)LM_UW( &(f->flags) ),
	 (long)LM_UL( &(f->pos) ),
	 (long)LM_UL( &(f->devinfo) ),
	 (long)&(f->fc),
	 (long)LM_UL( &(f->fc.fs) ),
	 (int)LM_UW(&(f->fc.dev)),
	 (int)LM_UW(&(f->fc.aux)),
	 (long)f->fc.index,
	 (long)(f->fc.index->parent),
	 f->fc.index->name,
	 (int)f->fc.index->usecnt,
	 (int)f->fc.index->childcnt,
	 (int)pid );
    if ( LM_W( &(f->links) ) <= 0 )
    { /* close the file realy */
	if ( close( LM_L( &(f->pos) ) ) ) /* FIXME: see dev_open.c */
	{
	    if ( verbose )
		fprintf( stderr, "dev_close() at STonXFS failed %d!\n", errno );
	    return unix2toserrno( errno, TOS_EIHNDL );
	}
	/* FIXME: if we need an internal list, delete f from it, now */
    }
    else
	DBG( "close: not closed because of link count!\n" );
    return 0;
}


UL mint_fs_dev_datime( MINT_FILEPTR *f, W *timeptr, W rwflag )
{
    DBG( "datime (NOT TESTED!)\n" );
    if ( rwflag )
    { /* change access and modification time */
	struct utimbuf ut;
	ut.actime = mint_m2u_time( LM_UW( timeptr ), LM_UW( timeptr+1 ) );
	ut.modtime = ut.actime;
	if ( utime(mint_makefilename( f->fc.index, NULL, NULL ), &ut ) )
	    return unix2toserrno( errno, TOS_EFILNF );
	else
	    return 0;
    }
    else
    { /* get access time */
	struct stat st;
	if ( stat( mint_makefilename( f->fc.index, NULL, NULL ), &st ) )
	    return unix2toserrno( errno, TOS_EFILNF );
	else
	{
	    SM_UW( timeptr, time2dos( st.st_atime ) );
	    SM_UW( timeptr+1, date2dos( st.st_atime ) );
	    return 0;
	}
    }
}


UL mint_fs_dev_ioctl( MINT_FILEPTR *f, W mode, void *buf )
{
    struct stat st;
    int rv1, rv2;
    int fd = LM_L( &(f->pos) ); /* FIXME: see mint_fs_dev_open */
    DBG( "mint_fs_dev_ioctl (NOT TESTED!)\n" );
    switch (mode)
    {
      case TOS_FIONREAD:
	  /* FIXME: Aks for readability using select before */
	  if ( fstat( fd, &st ) )
	      return unix2toserrno( errno, TOS_EIHNDL );
	  if ( S_ISREG( st.st_mode ) || S_ISDIR( st.st_mode ) )
	  { /* regular file or directory, so use fseek-methode */
	      rv1 = lseek( fd, 0, SEEK_CUR );
	      if ( rv1 < 0 )
		  return unix2toserrno( errno, TOS_E_SEEK );
	      rv2 = lseek( fd, 0, SEEK_END );
	      lseek( fd, rv1, SEEK_SET );
	      if ( rv2 < 0 )
		  return unix2toserrno( errno, TOS_E_SEEK );
	      SM_L( buf, rv2-rv1 );
	      return 0;
	  }
	  else if ( S_ISCHR( st.st_mode ) || 
		    S_ISFIFO( st.st_mode ) ||
		    S_ISSOCK( st.st_mode ) )
	  {
	      SM_L( buf, 1 ); /* FIXME: Don't know, if can realy read something! */
	      return 0;
	  }
	  else if ( S_ISBLK( st.st_mode ) )
	  {
	      SM_L( buf, st.st_blksize ); /* FIXME: Don't know, if correct! */
	      return 0;
	  }
	  else
	      return TOS_EINVFN;
      case TOS_FIONWRITE:
	  /* FIXME: Ask for writability using select before! */
	  SM_L( buf, 1 ); /* FIXME: This is not correct! */
	  return 0;
	  /* FIXME: file locking would be nice! */
      default:
	  return TOS_EINVFN;
    }
}


UL mint_fs_dev_lseek( MINT_FILEPTR *f, L where, W whence )
{
    int fd = LM_L( &(f->pos) ); /* FIXME: see mint_fs_dev_open */
    int rv;
    DBG( "lseek (NOT TESTED!)\n" );
    /* FIXME: who says whence at MiNT is same like whence at unix? */
    if ( ( rv = lseek( fd, where, whence ) ) == -1 )
	return unix2toserrno( errno, TOS_EACCDN );
    else
	return rv;
}


UL mint_fs_dev_open( MINT_FILEPTR *f )
{
    int mode, rv;
    UW flags = LM_UW( &(f->flags) );
    DBG( "open\n"
	 "  f = %#08lx\n"
	 "    links   = %d\n"
	 "    flags   = %#02x\n"
	 "    pos     = %ld\n"
	 "    devinfo = %ld\n"
	 "    fc      = %#08lx\n"
	 "      fs    = %#08lx\n"
	 "      dev   = %#04x\n"
	 "      aux   = %#04x\n"
	 "      index = %#08lx\n"
	 "        parent   = %#08lx\n"
	 "        name     = \"%s\"\n"
	 "        usecnt   = %d\n"
	 "        childcnt = %d\n",
	 (long)f,
	 (int)LM_UW( &(f->links) ),
	 (int)LM_UW( &(f->flags) ),
	 (long)LM_UL( &(f->pos) ),
	 (long)LM_UL( &(f->devinfo) ),
	 (long)&(f->fc),
	 (long)LM_UL( &(f->fc.fs) ),
	 (int)LM_UW(&(f->fc.dev)),
	 (int)LM_UW(&(f->fc.aux)),
	 (long)f->fc.index,
	 (long)(f->fc.index->parent),
	 f->fc.index->name,
	 (int)f->fc.index->usecnt,
	 (int)f->fc.index->childcnt );
    mode = mint_m2u_openmode( flags );
    DBG( "open flags: %#04x\nmode: %#04x\n", f->fc.index->name, flags, mode);
    if ( (rv = open( mint_makefilename(f->fc.index,NULL,NULL), mode )) == -1 )
    {
	DBG( "open() failed: %d\n", errno );
	return unix2toserrno( errno, TOS_EPTHNF );
    }
    SM_L( &(f->pos), rv ); /* FIXME: store this at fc.index->somewhere */
    DBG( "open: f->pos = unixfilehandle = %ld\n", (long)LM_L( &(f->pos) ) );
    return 0;
}


UL mint_fs_dev_read( MINT_FILEPTR *f, char *buf, L bytes )
{
    int fd = LM_L( &(f->pos) ); /* FIXME: see mint_fs_dev_open */
    int rv;
    DBG( "read\n"
	 "  f     = %#08lx\n"
	 "    links   = %d\n"
	 "    flags   = %#02x\n"
	 "    pos     = %ld\n"
	 "    devinfo = %ld\n"
	 "    fc      = %#08lx\n"
	 "      fs    = %#08lx\n"
	 "      dev   = %#04x\n"
	 "      aux   = %#04x\n"
	 "      index = %#08lx\n"
	 "        parent   = %#08lx\n"
	 "        name     = \"%s\"\n"
	 "        usecnt   = %d\n"
	 "        childcnt = %d\n"
	 "  buf   = %08lx\n"
	 "  bytes = %ld\n",
	 (long)f,
	 (int)LM_UW( &(f->links) ),
	 (int)LM_UW( &(f->flags) ),
	 (long)LM_UL( &(f->pos) ),
	 (long)LM_UL( &(f->devinfo) ),
	 (long)&(f->fc),
	 (long)LM_UL( &(f->fc.fs) ),
	 (int)LM_UW(&(f->fc.dev)),
	 (int)LM_UW(&(f->fc.aux)),
	 (long)f->fc.index,
	 (long)(f->fc.index->parent),
	 f->fc.index->name,
	 (int)f->fc.index->usecnt,
	 (int)f->fc.index->childcnt,
	 (long)buf,
	 (long)bytes );
    if ( ( rv = read( fd, buf, bytes ) ) == -1 )
    {
	DBG( "read() failed: %d\n", errno );
	return unix2toserrno( errno, TOS_EACCDN );
    }
    DBG( "read result:\n"
	 "%d bytes read.\n", rv );
    return rv;
}


UL mint_fs_dev_select( MINT_FILEPTR *f, L proc, W mode )
{
    int fd = LM_L( &(f->pos) ); /* FIXME: see dev_open.c */
    fd_set fds;
    struct timeval tv;
    
    DBG( "select (NOT TESTED!)\n" );
    FD_ZERO( &fds );
    FD_SET( fd, &fds );
    tv.tv_sec = 0;  /* Don't wait */
    tv.tv_usec = 0;
    switch ( mode )
    { /* Only handle TOS_O_RDONLY and TOS_O_WRONLY */
      case TOS_O_RDONLY:
	  if ( select( fd+1, &fds, NULL, NULL, &tv ) )
	      return 1;
	  else
	  { /* FIXME: We should add fd to a global fd_set and proc to a global
	     *        wakeup list for fd. We should also have a global wakeup
	     *        test, which also does the wakeselect.
	     * BUG:   Because we have no such global wakeup test, we return 1.
	     */
	      DBG( "select bug: dev_select(%u) should be 0!\n", (unsigned)fd );
	      return 1;
	  }
	  break;
      case TOS_O_WRONLY:
	  if ( select( fd+1, NULL, &fds, NULL, &tv ) )
	      return 1;
	  else
	  { /* FIXME: We should add fd to a global fd_set and proc to a global
	     *        wakeup list for fd. We should also have a global wakeup
	     *        test, which also does the wakeselect.
	     * BUG:   Because we have no such global wakeup test, we return 1.
	     */
	      DBG( "select bug: dev_select(%u) should be 0!\n", (unsigned)fd );
	      return 1;
	  }
	  break;
      default:
	  return 0;
    }
}


UL mint_fs_dev_unselect( MINT_FILEPTR *f, L proc, W mode )
{
    /* FIXME: if f is under global select wakeup control, we have to delete
     *        this control.
     *        Because we have no global wakeup test, we simply have to do
     *        nothing.
     *        See dev_select.c for more information!
     */
    if ( verbose )
	fprintf( stderr, "dev_unselect not supported at STonXFS!\n" );
    return 0;
}


UL mint_fs_dev_write( MINT_FILEPTR *f, const char *buf, L bytes )
{
    int fd = LM_L( &(f->pos) ); /* FIXME: see mint_fs_dev_open */
    int rv;
    DBG( "write\n"
	 "  f     = %#08lx\n"
	 "    links   = %d\n"
	 "    flags   = %#02x\n"
	 "    pos     = %ld\n"
	 "    devinfo = %ld\n"
	 "    fc      = %#08lx\n"
	 "      fs    = %#08lx\n"
	 "      dev   = %#04x\n"
	 "      aux   = %#04x\n"
	 "      index = %#08lx\n"
	 "        parent   = %#08lx\n"
	 "        name     = \"%s\"\n"
	 "        usecnt   = %d\n"
	 "        childcnt = %d\n"
	 "  buf   = %08lx\n"
	 "  bytes = %ld\n",
	 (long)f,
	 (int)LM_UW( &(f->links) ),
	 (int)LM_UW( &(f->flags) ),
	 (long)LM_UL( &(f->pos) ),
	 (long)LM_UL( &(f->devinfo) ),
	 (long)&(f->fc),
	 (long)LM_UL( &(f->fc.fs) ),
	 (int)LM_UW(&(f->fc.dev)),
	 (int)LM_UW(&(f->fc.aux)),
	 (long)&(f->fc.index),
	 (long)(f->fc.index->parent),
	 f->fc.index->name,
	 (int)f->fc.index->usecnt,
	 (int)f->fc.index->childcnt,
	 (long)buf,
	 (long)bytes );
    if ( ( rv = write( fd, buf, bytes ) ) == -1 )
    {
	DBG( "write() failed: %d\n", rv );
	return unix2toserrno( errno, TOS_EACCDN );
    }
    DBG( "write result\n"
	 "%d bytes written.\n", rv );
    return rv;
}

#endif /* MINT_STONXFS */
