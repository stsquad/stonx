/*
 * Filename:    mint_stonxfs_fs.c
 * Version:     0.5
 * Author:      Markus Kohm, Chris Felsch
 * Started:     1998
 * Target O/S:  MiNT running on StonX (Linux)
 * Description: STonX file-system backend
 * 
 * Note:        Please send suggestions, patches or bug reports to 
 *              Chris Felsch <C.Felsch@gmx.de>
 * 
 * Copying:     Copyright 1998 Markus Kohm
 *              Copyright 2000 Chris Felsch (C.Felsch@gmx.de)
 * 
 * History:     2000-09-10 (CF)  fs_mknod(), fs_unmount() added
 *              2000-11-18 (CF)  re-structured
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

#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "../main.h"
#include "../mem.h"
#include "mint_stonxfs_fs.h"
#include "mint_stonxfs_dev.h"
#include "mint_misc.h"
#include "stonxfsv.h"

#include "../options.h" /* see, if MINT_STONXFS is set to 0 */

#ifndef MINT_STONXFS
# define MINT_STONXFS 1
#endif

#if MINT_STONXFS

#if DEBUG_MINT_STONXFS
#  define DBG( _args... ) fprintf(stderr, "mint_stonxfs_"## _args );
#  define DBG_NP( _args...) fprintf(stderr, ## _args );
#else
#  define DBG( _args... )
#  define DBG_NP( _args...)
#endif

extern const char *fs_completename(MINT_FSFILE *dir,const char *name);
extern UW time2dos (time_t t);   /* from gemdos.c */
extern UW date2dos (time_t t);   /* from gemdos.c */


UL mint_fs_drv;           /* file system driver */
UW mint_fs_devnum;        /* unix device number */


UL mint_fs_chattr( MINT_FCOOKIE *file, W attr )
{
    struct stat st;
    const char *dirpath;
    mode_t newmode;
    DBG( "chattr (NOT TESTED!)\n" );
    dirpath = mint_makefilename( file->index, NULL, NULL );
    if ( lstat( dirpath, &st ) )
	return unix2toserrno( errno, TOS_EACCDN );
    if ( attr & 0x01 ) /* FA_RDONLY */
	newmode = st.st_mode & ~( S_IWUSR | S_IWGRP | S_IWOTH );
    else
	newmode = st.st_mode | ( S_IWUSR | S_IWGRP | S_IWOTH );
    if ( newmode != st.st_mode &&
	 chmod( dirpath, newmode ) )
	return unix2toserrno( errno, TOS_EACCDN );
    else
	return 0;
}


UL mint_fs_chmode( MINT_FCOOKIE *file, UW mode )
{
    /* FIXME: STonX has to run at root and uid and gid have to */
    /* FIXME: be same at unix and MiNT! */
    DBG( "chmode (NOT TESTED)\n"
	 "  CANNOT WORK CORRECTLY UNTIL uid AND gid AT MiNT ARE SAME LIKE AT UNIX!)\n" );
    if ( chmod( mint_makefilename( file->index, NULL, NULL ), mode ) )
	return unix2toserrno( errno, TOS_EACCDN );
    else {
	if ( verbose )
	    fprintf( stderr, "chmode at STonXFS ignored\n" );
	return 0;
    }
}


UL mint_fs_chown( MINT_FCOOKIE *file, W uid, W gid )
{
    /* FIXME: STonX has to run at root and uid and gid have to */
    /* FIXME: be same at unix and MiNT! */
    DBG( "chown (NOT TESTED)\n"
	 "  CANNOT WORK CORRECTLY UNTIL uid AND gid AT MiNT ARE SAME LIKE AT UNIX!)\n" );
    if ( chown( mint_makefilename( file->index, NULL, NULL ), uid, gid ) )
	return unix2toserrno( errno, TOS_EINVFN );
    else {
	if (verbose)
	    fprintf( stderr, "chown at STonXFS ignored\n" );
	return 0;
    }
}


UL mint_fs_closedir( MINT_DIR *dirh )
{
    DBG( "closedir:\n" 
	 "  dirh    = %#08lx\n"
	 "    fc    = %#08lx\n"
	 "      fs    = %#08lx\n"
	 "      dev   = %#04x\n"
	 "      aux   = %#04x\n"
	 "      index = %#08lx\n"
	 "        parent   = %#08lx\n"
	 "        name     = \"%s\"\n"
	 "        usecnt   = %d\n"
	 "        childcnt = %d\n",
	 (long)dirh,
	 (long)&(dirh->fc),
	 (long)LM_UL((UL *)&(dirh->fc.fs)),
	 (int)LM_UW(&(dirh->fc.dev)),
	 (int)LM_UW(&(dirh->fc.aux)),
	 (long)dirh->fc.index,
	 (long)(dirh->fc.index->parent),
	 dirh->fc.index->name,
	 (int)dirh->fc.index->usecnt,
	 (int)dirh->fc.index->childcnt );
    if ( closedir( dirh->dir ) )
    {
	DBG( "closedir() failed %d!\n", errno );
	return unix2toserrno( errno, TOS_EIHNDL );
    }
    return 0;
}


UL mint_fs_creat( MINT_FCOOKIE *dir, const char *name, UW mode, W attrib,
          MINT_FCOOKIE *fc )
{
    /* REMEMBER: dir and fc are in atari memory and bitorder! */
    const char *pathname = mint_makefilename(dir->index,name,NULL);
    char *unixname = NULL;
    MINT_FSFILE *new;
    int handle;
    DBG( "mint_fs_creat\n"
	 "  dir    = %#08lx\n"
	 "    fs    = %#08lx\n"
	 "    dev   = %#04x\n"
	 "    aux   = %#04x\n"
	 "    index = %#08lx\n"
	 "      parent   = %#08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n"
	 "  name   = \"\%s\"\n"
	 "  mode   = %#04x\n"
	 "  attrib = %#04x\n",
	 (long)dir,
	 (long)LM_UL((UL *)&(dir->fs)),
	 (int)LM_UW(&(dir->dev)),
	 (int)LM_UW(&(dir->aux)),
	 (long)dir->index,
	 (long)(dir->index->parent),
	 dir->index->name,
	 (int)dir->index->usecnt,
	 (int)dir->index->childcnt,
	 name,
	 (int)mode,
	 (int)attrib );
    if ( access( pathname, F_OK ) && errno == ENOENT )
    {
	if ( (unixname=mint_searchfortosname(mint_makefilename( dir->index,
								NULL,
								NULL ),
					     name ) ) != NULL )
	{
	    /* There is no unixfile with name but a unixfile with the same
	     * 8+3-filename, so we have to use this filename!
	     */
	    DBG( "creat: There is no file \"%s\" but a file \"%s\".\n",
		 name, unixname );
	    name = unixname;
	}
	pathname = mint_makefilename(dir->index,name,NULL);
    }
    DBG( "creat(\"%s\",%#08x): ", pathname, mode );
    if ( (handle = creat(pathname,mode)) == -1)
    {
	DBG_NP( "\n" );
	DBG( "creat() failed: %d\n", errno );
	if ( unixname )
	    free( unixname );
	return unix2toserrno( errno, TOS_EACCDN );
    }
    DBG_NP( "%ld\n", (long)handle );
    close(handle);
    if ( (new = malloc( sizeof( MINT_FSFILE ) )) == NULL )
    {
	DBG( "creat: malloc() failed!\n" );
	free( unixname );
	return TOS_ENSMEM;
    }
    new->parent = dir->index;
    if ( (new->name = strdup(name)) == NULL )
    {
	DBG( "creat: strdup() failed!\n" );
	free( new );
	free( unixname );
	return TOS_ENSMEM;
    }
    new->usecnt = 1;
    dir->index->childcnt++;
    memcpy( fc, dir, sizeof( MINT_FCOOKIE ) );
    fc->index = new;
    if ( unixname )
	free( unixname );
    return 0;
}


UL mint_fs_dfree( MINT_FCOOKIE *dir, L *buf ) {
    struct statfs sfs;
    DBG( "dfree\n" );
    if ( statfs( mint_makefilename( dir->index, NULL, NULL ), &sfs ) )
	return unix2toserrno( errno, TOS_EPTHNF );
    SM_L( buf  , sfs.f_bfree );     /* cluster = block */
    SM_L( buf+1, sfs.f_blocks );
    SM_L( buf+2, sfs.f_bsize );     /* FIXME: is this correct? yes (CF) */
    SM_L( buf+3, 1 );               /* sector = cluster = block */
    return 0;
}


UL mint_fs_dskchng( W drv )
{
    if ( verbose )
	fprintf( stderr, 
		 "dskchng at STonXFS not supported!\n" );
    return 0; /* FIXME: what about GEMDOS-buffers after umount/mount? */
}


UL mint_fs_dupcookie( MINT_FCOOKIE *new, MINT_FCOOKIE *old )
{
    MINT_FSFILE *fs;
    DBG( "dupcookie:\n"
	 "  old  = %#08lx\n"
	 "    fs    = %#08lx\n"
	 "    dev   = %#04x\n"
	 "    aux   = %#04x\n"
	 "    index = %#08lx\n"
	 "      parent   = %#08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n"
	 "  new  = %#08lx\n",
	 (long)old,
	 (long)LM_UL((UL *)&(old->fs)),
	 (int)LM_UW(&(old->dev)),
	 (int)LM_UW(&(old->aux)),
	 (long)old->index,
	 (long)(old->index->parent),
	 old->index->name,
	 (int)old->index->usecnt,
	 (int)old->index->childcnt,
	 (long)new );
    if ( (fs = malloc( sizeof( MINT_FSFILE ) )) == NULL )
    {
	DBG( "dupcookie: malloc() failed!\n" );
	return TOS_ENSMEM;
    }
    if ( ( fs->parent = old->index->parent ) != NULL )
    {
	if ( (fs->name = strdup(old->index->name)) == NULL )
	{
	    DBG( "dupcookie: strdup() failed!\n" );
	    free( fs );
	    return TOS_ENSMEM;
	}
	fs->parent->childcnt++;
    }
    else
	fs->name = "/";
    fs->usecnt = 1;
    fs->childcnt = 0; /* don't heritate childs! */
    memcpy( new, old, sizeof( MINT_FCOOKIE ) );
    new->index = fs;
    DBG( "dupcookie result:\n"
	 "  new  = %#08lx\n"
	 "    fs    = %#08lx\n"
	 "    dev   = %#04x\n"
	 "    aux   = %#04x\n"
	 "    index = %#08lx\n"
	 "      parent   = %#08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n",
	 (long)new,
	 (long)LM_UL((UL *)&(new->fs)),
	 (int)LM_UW(&(new->dev)),
	 (int)LM_UW(&(new->aux)),
	 (long)new->index,
	 (long)(new->index->parent),
	 new->index->name,
	 (int)new->index->usecnt,
	 (int)new->index->childcnt );
    return 0;
}


UL mint_fs_fscntl(MINT_FCOOKIE *dir, const char *name, W cmd, L arg)
{
    struct statfs fs;

    DBG("fscntl: \n"
	"  name= %s, cmd= 0x%x\n", name, (UW)cmd);
 
    switch ((UW)cmd)
    {
      case 0xf1ff:
      {
	  long    *f = MEM(arg);
	  char    *name;
	  
	  if (f)
	  {
#define FS_CASESENSITIVE 0x00000002  /* file names are case sensitive */
#define FS_NOXBIT        0x00000004  /* if a file can be read, it can be exec */
	      name = mint_makefilename(dir->index, NULL, NULL);
#ifdef __linux__
	      /* Try to find out, if FS_NOXBIT has to be used */
	      if ( statfs( name, &fs ) == 0 && fs.f_type == 0x4d44 ) /* MSDOS */
		  SM_UL(f, (UL)(FS_CASESENSITIVE|FS_NOXBIT));
	      else
#endif
	      /* Do a heuristic test */
	      if (strncasecmp(name, "/win/", 5) == 0 ||
		  strncasecmp(name, "/windows/", 9) == 0 ||
		  strncasecmp(name, "/win95/", 7) == 0 ||
		  strncasecmp(name, "/win98/", 7) == 0 ||
		  strncasecmp(name, "/windows95/", 11) == 0 ||
		  strncasecmp(name, "/windows98/", 11) == 0)
		  SM_UL(f, (UL)(FS_CASESENSITIVE|FS_NOXBIT)); 
	      else
		  SM_UL(f, (UL)(FS_CASESENSITIVE));
	      DBG("fscntl: %s, *f= 0x%lx\n", name, *f);
	  }
	  return TOS_E_OK;
      }
        
      case MX_KER_XFSNAME:
      {
	  char    *name = MEM(arg);
	  strcpy(name, MINT_FS_NAME);
	  
	  return TOS_E_OK;
      }
      
      case FS_INFO:
      {
	  struct fs_info *info = MEM(arg);
	  
	  if (info)
	  {
	      strcpy(info->name, MINT_FS_NAME);
	      strcpy(info->type_asc, "STonX");
	      SM_UW(info->version, MINT_FS_V_MAJOR);
	      SM_UW(info->version+1, MINT_FS_V_MINOR);
	      SM_UL(info->type, (UL)FS_STONX);
	  }
	  return TOS_E_OK;
      }
      
      case FS_USAGE:
      {
	  struct fs_usage *usage = MEM(arg);
	  
	  if (usage)
	  {
	      struct statfs st;
	      
	      if (statfs(mint_makefilename(dir->index, NULL, NULL), &st) == 0)
	      {
		  DBG("fscntl  (FS_USAGE) statfs():\n"
		      "   bsize:  0x%08lx%08lx\n"
		      "   blocks: 0x%08lx%08lx\n"
		      "   bfree:  0x%08lx%08lx\n"
		      "   files:  0x%08lx%08lx\n"
		      "   ffree:  0x%08lx%08lx\n",
		      (long)(st.f_bsize >> 8), (long)(st.f_bsize) 
		      (long)(st.f_blocks >> 8), (long)(st.f_blocks) 
		      (long)(st.f_bfree >> 8), (long)(st.f_bfree) 
		      (long)(st.f_files >> 8), (long)(st.f_files) 
		      (long)(st.f_ffree >> 8), (long)(st.f_ffree) 
		      );
		  /* May loose the main size, but could this realy happen? */
		  SM_UL(usage->blocksize, st.f_bsize);

		  /* MiNT uses longlong and Linux maybe, too! */
		  SM_UL(usage->blocks, (UL)(st.f_blocks >> 8));
		  SM_UL(usage->blocks+2, (UL)st.f_blocks);

		  SM_UL(usage->free_blocks, (UL)(st.f_bfree >> 8));
		  SM_UL(usage->free_blocks+2, (UL)st.f_bfree);

		  SM_UL(usage->inodes, (UL)(st.f_files >> 8));
		  SM_UL(usage->inodes+2, st.f_files);
                    
		  SM_UL(usage->free_inodes, (UL)(st.f_ffree >> 8));
		  SM_UL(usage->free_inodes+2, st.f_ffree);

		  return TOS_E_OK;
	      }
	      else
	      {
		  DBG("fcntl (FS_USAGE) statfs failed %d\n", errno);
		  return unix2toserrno(errno, TOS_EPTHNF);
	      }
	  }
      }
    }
    
    DBG("fcntl: unknown cmd!\n");
    return TOS_EINVFN;
}


UL mint_fs_getdev( MINT_FCOOKIE *fc, L *devspecial )
{
    DBG( "getdev:\n"
	 "  fc  = %#08lx\n"
	 "    fs    = %#08lx\n"
	 "    dev   = %#04x\n"
	 "    aux   = %#04x\n"
	 "    index = %#08lx\n"
	 "      parent   = %#08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n"
	 "  &devspecial = %#08lx\n",
	 (long)fc,
	 (long)LM_UL((UL *)&(fc->fs)),
	 (int)LM_UW(&(fc->dev)),
	 (int)LM_UW(&(fc->aux)),
	 (long)fc->index,
	 (long)(fc->index->parent),
	 fc->index->name,
	 (int)fc->index->usecnt,
	 (int)fc->index->childcnt,
	 (long)devspecial );
    SM_UL(devspecial,0); /* reserved */
    DBG( "getdev result\n"
	 "  devspecial = %#08lx\n",
	 (long)LM_UL(devspecial) );
    return (UL)mint_fs_devdrv;
}


UL mint_fs_getname( MINT_FCOOKIE *relto, MINT_FCOOKIE *dir, char *pathname, 
		    W size )
{
    char *base;
    const char *dirpath;
#if DEBUG_MINT_STONXFS
    char *savepath = pathname;
#endif
    size_t baselength, dirlength;
    DBG( "getname\n" 
	 "  relto  = %#08lx\n"
	 "    fs    = %#08lx\n"
	 "    dev   = %#04x\n"
	 "    aux   = %#04x\n"
	 "    index = %#08lx\n"
	 "      parent   = %#08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n"
	 "  dir    = %#08lx\n"
	 "    fs    = %#08lx\n"
	 "    dev   = %#04x\n"
	 "    aux   = %#04x\n"
	 "    index = %#08lx\n"
	 "      parent   = %#08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n",
	 (long)relto,
	 (long)LM_UL((UL *)&(relto->fs)),
	 (int)LM_UW(&(relto->dev)),
	 (int)LM_UW(&(relto->aux)),
	 (long)relto->index,
	 (long)(relto->index->parent),
	 relto->index->name,
	 (int)relto->index->usecnt,
	 (int)relto->index->childcnt,
	 (long)dir,
	 (long)LM_UL((UL *)&(dir->fs)),
	 (int)LM_UW(&(dir->dev)),
	 (int)LM_UW(&(dir->aux)),
	 (long)dir->index,
	 (long)(dir->index->parent),
	 dir->index->name,
	 (int)dir->index->usecnt,
	 (int)dir->index->childcnt );
    if ( (base = strdup(mint_makefilename(relto->index,NULL,NULL))) == NULL )
    {
	DBG( "getname: strdup() failed!\n" );
	return unix2toserrno( errno, TOS_ENSMEM );
    }
    DBG( "getname: relto = \"%s\"\n", base );
    baselength = strlen(base);
    if ( baselength && base[baselength-1] == '/' )
    {
	baselength--;
	base[baselength] = '\0';
	DBG( "getname: fixed relto = \"%s\"\n", base );
    }
    dirpath = mint_makefilename( dir->index, NULL, NULL );
    DBG( "getname: dir = \"%s\"\n", dirpath );
    dirlength = strlen(dirpath);
    if ( dirlength < baselength ||
	 strncmp( dirpath, base, baselength ) ) 
    {
	/* dir is not a sub...directory of relto, so use absolute */
	/* FIXME: try to use a relative path, if relativ is smaller */
	DBG( "getname: dir not relativ to relto!\n" );
    }
    else 
    {
	/* delete "same"-Part */
	dirpath += baselength;
    }
    free(base);
    DBG( "getname: relativ dir to relto = \"%s\"\n", dirpath );
    /* copy and unix2dosname */ 
    for ( ; *dirpath && size > 0; size--, dirpath++ )
	if ( *dirpath == '/' )
	    *pathname++ = '\\';
	else
	    *pathname++ = *dirpath;
    if ( !size ) 
    {
	DBG( "getname: Relative dir is to long for buffer!\n" );
	return TOS_ERANGE;
    }
    else
    {
	*pathname = '\0';
#if DEBUG_MINT_STONXFS
	DBG( "getname result = \"%s\"\n", savepath );
#endif
	return 0;
    }
}


UL mint_fs_getxattr( MINT_FCOOKIE *file, MINT_XATTR *xattr ) 
{
    /* REMEMBER: file and xattr are in atari memory and bitorder! */
    MINT_FSFILE *fs = (MINT_FSFILE *)(file->index);
    struct stat st;
    
    DBG( "getxattr:\n"
	 "  file  = %08lx\n"
	 "    fs    = %08lx\n"
	 "    dev   = %04x\n"
	 "    aux   = %04x\n"
	 "    index = %08lx\n"
	 "      parent   = %08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n"
	 "  xattr = %08lx\n",
	 (long)file,
	 (long)LM_UL((UL *)&(file->fs)),
	 (int)LM_UW(&(file->dev)),
	 (int)LM_UW(&(file->aux)),
	 (long)file->index,
	 (long)(file->index->parent),
	 file->index->name,
	 file->index->usecnt,
	 file->index->childcnt,
	 (long)xattr );
    if ( lstat( mint_makefilename( fs, NULL, NULL ), &st ) )
    {
	DBG( "getxattr: lstat() failed: %d\n", errno );
	return unix2toserrno( errno, TOS_EACCDN );
    }
    SM_UW( &(xattr->mode), mint_u2m_mode( st.st_mode ) );
    SM_UL( &(xattr->index), st.st_ino );
    xattr->dev = file->dev;
    SM_UW( &(xattr->rdev), 0 ); /* FIXME: Don't know, if it's correct */
    SM_UW( &(xattr->nlink), st.st_nlink );
    SM_UW( &(xattr->uid), st.st_uid ); /* FIXME: MiNT doesn't know this! */
    SM_UW( &(xattr->gid), st.st_gid ); /* FIXME: MiNT doesn't know this! */
    SM_L( &(xattr->size), st.st_size );
    SM_L( &(xattr->blksize), st.st_blksize );
    SM_L( &(xattr->nblocks), st.st_blocks );
    SM_W( &(xattr->mtime), time2dos( st.st_mtime ) );
    SM_W( &(xattr->mdate), date2dos( st.st_mtime ) );
    SM_W( &(xattr->atime), time2dos( st.st_atime ) );
    SM_W( &(xattr->adate), date2dos( st.st_atime ) );
    SM_W( &(xattr->ctime), time2dos( st.st_ctime ) );
    SM_W( &(xattr->cdate), date2dos( st.st_ctime ) );
    SM_W( &(xattr->attr), mint_u2m_attrib( st.st_mode, file->index->name ) );
    SM_W( &(xattr->reserved2), 0 );
    SM_L( &(xattr->reserved3[0]), 0 );
    SM_L( &(xattr->reserved3[1]), 0 );
    DBG( "getxattr result\n"
	 "  mode    = %04x\n"
	 "  index   = %08lx\n"
	 "  dev     = %04x\n"
	 "  rdev    = %04x\n"
	 "  nlink   = %d\n"
	 "  uid     = %d\n"
	 "  gid     = %d\n"
	 "  size    = %ld\n"
	 "  blksize = %ld\n"
	 "  nblocks = %ld\n"
	 "  mtime   = %04x\n"
	 "  mdate   = %04x\n"
	 "  atime   = %04x\n"
	 "  adate   = %04x\n"
	 "  ctime   = %04x\n"
	 "  cdate   = %04x\n"
	 "  attr    = %04x\n",
	 (int)LM_UW( &(xattr->mode) ),
	 (long)LM_L( &(xattr->index) ),
	 (int)LM_UW( &(xattr->dev) ),
	 (int)LM_UW( &(xattr->rdev) ),
	 (int)LM_UW( &(xattr->nlink) ),
	 (int)LM_UW( &(xattr->uid) ),
	 (int)LM_UW( &(xattr->gid) ), 
	 (long)LM_L( &(xattr->size) ),
	 (long)LM_L( &(xattr->blksize) ),
	 (long)LM_L( &(xattr->nblocks) ),
	 (int)LM_W( &(xattr->mtime) ),
	 (int)LM_W( &(xattr->mdate) ),
	 (int)LM_W( &(xattr->atime) ),
	 (int)LM_W( &(xattr->adate) ),
	 (int)LM_W( &(xattr->ctime) ),
	 (int)LM_W( &(xattr->cdate) ),
	 (int)LM_W( &(xattr->attr) ) );
    return 0;
}


UL mint_fs_hardlink( MINT_FCOOKIE *fromdir, const char *fromname,
             MINT_FCOOKIE *todir, const char *toname )
{
    char *from, *unixname;
    int rv;
    DBG( "hardlink (NOT TESTED!)\n" );
    if ( (from = strdup( mint_makefilename( fromdir->index,
					    fromname,
					    NULL ) )) == NULL )
    {
	DBG( "hardlink: strdup() failed!\n" );
	return unix2toserrno( errno, TOS_ENSMEM );
    }
    rv = link( from, mint_makefilename( todir->index, toname, NULL) );
    free(from);
    /* REMEMBER: Creation of hardlinks is only possible with extended
     *           filesystems. So programms, using this, should use long
     *           filenames. So we need no handling of 8+3 names here!
     */
    if ( rv )
    {
	DBG( "hardlink: link() failed!\n" );
	return unix2toserrno( errno, TOS_ENSAME );
    }
    return 0;
}


UL mint_fs_lookup( MINT_FCOOKIE *dir, const char *name, MINT_FCOOKIE *fc )
{
    MINT_FSFILE *new;
    struct stat st;
    DBG( "lookup:\n"
	 "  dir  = %#08lx\n"
	 "    fs    = %#08lx\n"
	 "    dev   = %#04x\n"
	 "    aux   = %#04x\n"
	 "    index = %#08lx\n"
	 "      parent   = %#08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n"
	 "  name = \"%s\"\n",
	 (long)dir,
	 (long)LM_UL((UL *)&(dir->fs)),
	 (int)LM_UW(&(dir->dev)),
	 (int)LM_UW(&(dir->aux)),
	 (long)dir->index,
	 (long)(dir->index->parent),
	 dir->index->name,
	 (int)dir->index->usecnt,
	 (int)dir->index->childcnt,
	 name );
    if ( !name || !*name || (*name == '.' && !name[1]) )
    {
	new = dir->index;
	new->usecnt++;
    }
    else if ( *name == '.' && name[1] == '.' && !name[2] )
    {
	if ( !dir->index->parent )
	{
	    DBG( "lookup to \"..\" at root\n" );
	    return TOS_EMOUNT;
	}
	new = dir->index->parent;
	new->usecnt++;
    }
    else 
    {
	if ( (new = malloc( sizeof(MINT_FSFILE) )) == NULL )
	{
	    DBG( "lookup: malloc() failed!\n" );
	    return TOS_ENSMEM;
	}
	new->parent = dir->index;
	if ( lstat( mint_makefilename(dir->index,name,NULL), &st ) )
	{
	    /* If the file/dir doesn't exist, it also could be 8+3-name,
	     * made by readdir. So we have to search for this, too!
	     */
	    if (errno==ENOENT &&
		(new->name=mint_searchfortosname(mint_makefilename(dir->index,
								   NULL,NULL),
						 name)) != NULL)
	    {
		/* we try is again */
		if ( lstat(mint_makefilename(dir->index,new->name,NULL),&st) )
		{
		    DBG( "lookup: lstat(\"%s\") after successfull stonx_searchfortosname() failed: %d!\n",
			 new->name, errno );
		    free( new->name );
		    free( new );
		    return unix2toserrno( errno, TOS_EACCDN );
		}
	    }
	    else
	    {
		DBG( "lookup: lstat() and/or mint_searchfortosname() failed!\n" );
		free(new);
		return unix2toserrno( errno, TOS_EFILNF );
	    }
	}
	else if ( (new->name = strdup(name)) == NULL ) 
	{
	    DBG( "lookup: strdup() failed!\n" );
	    free( new );
	    return TOS_ENSMEM;
	}
	new->usecnt = 1;
	new->childcnt = 0;
	dir->index->childcnt++; /* same as: new->parent->childcnt++ */
    }
    memcpy( fc, dir, sizeof( MINT_FCOOKIE ) );
    fc->index = new;
    DBG( "lookup result\n"
	 "  fc = %#08lx\n"
	 "    fs    = %#08lx\n"
	 "    dev   = %#04x\n"
	 "    aux   = %#04x\n"
	 "    index = %#08lx\n"
	 "      parent   = %#08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n",
	 (long)fc,
	 (long)LM_UL((UL *)&(fc->fs)),
	 (int)LM_UW(&(fc->dev)),
	 (int)LM_UW(&(fc->aux)),
	 (long)fc->index,
	 (long)(fc->index->parent),
	 fc->index->name,
	 (int)fc->index->usecnt,
	 (int)fc->index->childcnt );
    return 0;
}


UL mint_fs_mkdir( MINT_FCOOKIE *dir, const char *name, UW mode )
{
    DBG( "mkdir (NOT TESTED!)\n" );
    if ( mkdir( mint_makefilename( dir->index, name, NULL ), mode ) )
	return unix2toserrno( errno, TOS_EACCDN );
    else
	return 0;
}


UL mint_fs_mknod(MINT_FCOOKIE *dir, const char *name, L mode)
{
    if ( verbose )
	fprintf(stderr,"mknod() not yet supported at STonXFS!\n");
    return TOS_ENOSYS;
}


UL mint_fs_opendir( MINT_DIR *dirh, W tosflag )
{
    DBG( "mint_fs_opendir:\n"
	 "  dirh    = %#08lx\n"
	 "    fc    = %#08lx\n"
	 "      fs    = %#08lx\n"
	 "      dev   = %#04x\n"
	 "      aux   = %#04x\n"
	 "      index = %#08lx\n"
	 "        parent   = %#08lx\n"
	 "        name     = \"%s\"\n"
	 "        usecnt   = %d\n"
	 "        childcnt = %d\n"
	 "  tosflag = %d\n",
	 (long)dirh,
	 (long)&(dirh->fc),
	 (long)LM_UL((UL *)&(dirh->fc.fs)),
	 (int)LM_UW(&(dirh->fc.dev)),
	 (int)LM_UW(&(dirh->fc.aux)),
	 (long)dirh->fc.index,
	 (long)(dirh->fc.index->parent),
	 dirh->fc.index->name,
	 (int)dirh->fc.index->usecnt,
	 (int)dirh->fc.index->childcnt,
	 (int)tosflag );
    if ((dirh->dir=opendir(mint_makefilename(dirh->fc.index,NULL,NULL)))==NULL)
    {
	DBG( "opendir() failed: %d!\n", errno );
	return unix2toserrno( errno, TOS_EACCDN );
    }
    SM_UW( &(dirh->index), 0 );
    DBG( "opendir result\n"
	 "  dirh->dir   = %#08lx\n"
	 "  dirh->index = %d\n",
	 (long)dirh->dir,
	 (int)LM_UW( &(dirh->index) ) );
    return 0;
}


#define SELFNEEDOPEN  2 /* FIXME: Maybe should be many more! */
UL mint_fs_pathconf( MINT_FCOOKIE *dir, W which )
{
    struct statfs sfs;
    const char *dirname;
    long retval;
    DBG( "pathconf:\n"
	 "  dir   = %#08lx\n"
	 "    fs    = %#08lx\n"
	 "    dev   = %#04x\n"
	 "    aux   = %#04x\n"
	 "    index = %#08lx\n"
	 "      parent   = %#08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n"
	 "  which = %d\n",
	 (long)dir,
	 (long)LM_UL((UL *)&(dir->fs)),
	 (int)LM_UW(&(dir->dev)),
	 (int)LM_UW(&(dir->aux)),
	 (long)dir->index,
	 (long)(dir->index->parent),
	 dir->index->name,
	 dir->index->usecnt,
	 dir->index->childcnt,
	 (int)which );
    switch ( which )
    {
      case -1:
	  return TOS_DP_MAXREQ;
      case TOS_DP_IOPEN:
	  retval = sysconf( _SC_OPEN_MAX ) - SELFNEEDOPEN;
	  break;
      case TOS_DP_MAXLINKS:
	  retval = pathconf(mint_makefilename(dir->index,NULL,NULL),
			    _PC_LINK_MAX);
	  break;
      case TOS_DP_PATHMAX:
	  retval = pathconf( "/", _PC_PATH_MAX );
	  break;
      case TOS_DP_NAMEMAX:
	  dirname = mint_makefilename( dir->index, NULL, NULL );
	  if ( (retval = pathconf( dirname, _PC_NAME_MAX )) < 0 &&
	       !statfs( dirname, &sfs ) && sfs.f_namelen > 0 )
	      retval = sfs.f_namelen;
	  break;
      case TOS_DP_ATOMIC:
	  if ( statfs( mint_makefilename( dir->index, NULL, NULL ), &sfs ) ||
	       sfs.f_bsize <= 0 )
	      return TOS_UNLIMITED; /* FIXME: this is not correct but allmost true */
	  else
	      retval = (UL)sfs.f_bsize;
	  break;
      case TOS_DP_TRUNC:
	  if ( (retval=pathconf(mint_makefilename(dir->index,NULL,NULL),
				_PC_NO_TRUNC)) != 0 &&
	       retval != -1 )
	      return TOS_DP_NOTRUNC; /* FIXME: sometimes we trunc by ourself */
	  else
	      return TOS_DP_AUTOTRUNC; /* FIXME: this is not allways true, we
					*        TOS_DP_DOSTRUNC by ourself */
      case TOS_DP_CASE:
	  return TOS_DP_CASESENS;    /* FIXME: not allways true (MSDOS_FS) */
      case TOS_DP_MODEATTR:
	  return TOS_DP_FT_REG|TOS_DP_FT_LNK| /* FIXME: not true (MSDOS_FS) */
	      TOS_DP_FT_DIR|(0777L<<8)|       /* FIXME: -´´- */
	      0x01;                           /* ReadOnly */
      case TOS_DP_XATTRFIELDS:
	  return TOS_DP_INDEX|TOS_DP_DEV|TOS_DP_NLINK|
	      TOS_DP_UID|TOS_DP_GID| /* FIXME: it's uid and gid at Unix! */
	      TOS_DP_BLKSIZE|TOS_DP_SIZE|TOS_DP_NBLOCKS|TOS_DP_ATIME|
	      TOS_DP_CTIME|TOS_DP_MTIME;
      default:
	  return TOS_EINVFN;
    }
    DBG( "pathconf result: %ld\n", (long)retval );
    if ( retval < 0 )
	return unix2toserrno( errno, TOS_EPTHNF );
    else
	return retval;
}


UL mint_fs_readdir( MINT_DIR *dirh, char *name, W namelen, MINT_FCOOKIE *fc )
{
    struct dirent *de;
    const char *tosname;
    int i, j;
    char c;
    struct stat st;
    MINT_FSFILE *new;
    
    DBG( "readdir:\n" 
	 "  dirh    = %#08lx\n"
	 "    fc    = %#08lx\n"
	 "      fs    = %#08lx\n"
	 "      dev   = %#04x\n"
	 "      aux   = %#04x\n"
	 "      index = %#08lx\n"
	 "        parent   = %#08lx\n"
	 "        name     = \"%s\"\n"
	 "        usecnt   = %d\n"
	 "        childcnt = %d\n"
	 "  name    = %#08lx\n"
	 "  namelen = %ld\n"
	 "  fc    = %#08lx\n",
	 (long)dirh,
	 (long)&(dirh->fc),
	 (long)LM_UL((UL *)&(dirh->fc.fs)),
	 (int)LM_UW(&(dirh->fc.dev)),
	 (int)LM_UW(&(dirh->fc.aux)),
	 (long)dirh->fc.index,
	 (long)(dirh->fc.index->parent),
	 dirh->fc.index->name,
	 (int)dirh->fc.index->usecnt,
	 (int)dirh->fc.index->childcnt,
	 (long)name,
	 (long)namelen,
	 (long)fc );
    do {
	if ( (de = readdir( dirh->dir )) == NULL )
	{
	    DBG( "readdir: no more files? (%d)\n", errno );
	    return TOS_ENMFIL; /* FIXME: could be an error, too */
	}
    } while ( !dirh->fc.index->parent &&
	      ( de->d_name[0] == '.' &&
		( !de->d_name[1] ||
		  ( de->d_name[1] == '.' && !de->d_name[2] ) ) ) );
    
    if ( (new = malloc(sizeof(MINT_FSFILE))) == NULL )
    {
	DBG( "readdir: malloc() failed!\n" );
	return TOS_ENSMEM;
    }
    else if ( (new->name = strdup( de->d_name )) == NULL )
    {
	DBG( "readdir: failed!\n" );
	free( new );
	return TOS_ENSMEM;
    }
    
    lstat( mint_makefilename( dirh->fc.index, de->d_name, NULL ), &st );
    new->parent = dirh->fc.index;
    dirh->fc.index->childcnt++;
    new->usecnt = 1;
    new->childcnt = 0;
    SM_UL( (UL *)&(fc->fs), mint_fs_drv);
    fc->dev = dirh->fc.dev;
    fc->index = new;
    SM_UW( &(dirh->index), LM_UW( &(dirh->index) ) + 1 );
    
    if ( LM_UW( &(dirh->flags) ) & TOS_SEARCH )
    { /* We have to generate 8+3-names! */
	DBG( "readdir: create 8+3 name:\n" );
	if ( ( tosname = mint_maketosname( de->d_name, 
					   telldir( dirh->dir ) ) ) == NULL )
	{
	    DBG_NP( "readdir: mint_maketosname failed!\n" );
	    return TOS_ERANGE;
	}
	DBG( "readdir: \"%s\" --> \"%s\"\n", de->d_name, tosname );
	if ( namelen < strlen( tosname ) + 1 )
	{
	    DBG( "readdir: namebuffer is to small for a tosname!\n" );
	    return TOS_ERANGE;
	}
	strcpy( name, tosname );
    }
    else
    { /* we can handle long names */
	if ( namelen < 4 )
	{
	    DBG( "readdir: namebuffer smaller than 4 byte!\n" );
	    return TOS_ERANGE;
	}
	SM_L( (L *)name, (L)st.st_ino );
	name += 4;
	namelen -= 4;
	if ( strlen( de->d_name ) >= namelen )
	{
	    DBG( "readdir: namebuffer to small!\n" );
	    strncpy( name, de->d_name, namelen );
	    return TOS_ERANGE;
	}
	else
	    strcpy( name, de->d_name );
    }
    DBG( "readdir result\n"
	 "  name = \"%s\"\n"
	 "  fc    = %#08lx\n"
	 "    fs    = %#08lx\n"
	 "    dev   = %#04x\n"
	 "    aux   = %#04x\n"
	 "    index = %#08lx\n"
	 "      parent   = %#08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n",
	 name,
	 (long)fc,
	 (long)LM_UL((UL *)&(fc->fs)),
	 (int)LM_UW(&(fc->dev)),
	 (int)LM_UW(&(fc->aux)),
	 (long)fc->index,
	 (long)(fc->index->parent),
	 fc->index->name,
	 (int)fc->index->usecnt,
	 (int)fc->index->childcnt );
    return 0;
}


UL mint_fs_readlabel( MINT_FCOOKIE *dir, char *name, W namelen )
{
    if ( verbose )
	fprintf( stderr, "readlabel() not provided at STonXFS!\n" );
    return TOS_EINVFN;
}


UL mint_fs_readlink( MINT_FCOOKIE *dir, char *buf, W len )
{
    int rv, l;
    char *h;
    DBG( "readlink:\n"
	 "  dir    = %#08lx\n"
	 "    fs    = %#08lx\n"
	 "    dev   = %#04x\n"
	 "    aux   = %#04x\n"
	 "    index = %#08lx\n"
	 "      parent   = %#08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n"
	 "  buffer = %#08lx\n"
	 "  len    = %ld\n",
	 (long)dir,
	 (long)LM_UL((UL *)&(dir->fs)),
	 (int)LM_UW(&(dir->dev)),
	 (int)LM_UW(&(dir->aux)),
	 (long)dir->index,
	 (long)(dir->index->parent),
	 dir->index->name,
	 dir->index->usecnt,
	 dir->index->childcnt,
	 (long)buf,
	 (long)len );
    if ((rv=readlink(mint_makefilename(dir->index,NULL,NULL),buf,len-1)) < 0)
    {
	DBG( "readlink() failed: %d!\n", errno );
	return unix2toserrno( errno, TOS_EFILNF );
    }
    buf[rv++] = '\0';
    if ( *buf == '/' )
    { /* absolut symlink to stonxfs */
	const char *mname = "u:/"MINT_FS_NAME;
	size_t lmname = strlen( mname );
	
	if ( rv + lmname >= len )
	{
	    DBG( "readling: buffer to small!\n" );
	    return TOS_ERANGE;
	}
	for ( l = rv+1, h = buf + l;
	      l >= 0;
	      l--, h-- )
	    h[lmname] = *h;
	memcpy( buf, mname, lmname );
    }
    for ( h = strchr( buf, '/' ); h; h = strchr( h, '/' ) )
	*h++ = '\\';
    DBG( "readling result\n"
	 "  buffer = \"%s\"\n", buf );
    return 0;
}


static void freefs( MINT_FSFILE *fs )
{
    DBG( "freefs:\n"
	 "  fs = %08lx\n"
	 "    parent   = %08lx\n"
	 "    name     = \"%s\"\n"
	 "    usecnt   = %d\n"
	 "    childcnt = %d\n",
	 (long)fs,
	 (long)(fs->parent),
	 fs->name,
	 fs->usecnt,
	 fs->childcnt );
    if ( !fs->usecnt && !fs->childcnt )
    {
	DBG( "freefs: realfree\n" );
	if ( fs->parent )
	{
	    fs->parent->childcnt--;
	    freefs( fs->parent );
	    free( fs->name );
	}
	free( fs );
    }
    else
    {
	DBG( "freefs: notfree\n" );
    }
}


UL mint_fs_release( MINT_FCOOKIE *fc )
{
    DBG( "release():\n"
	 "  fc = %08lx\n"
	 "    fs    = %08lx\n"
	 "    dev   = %04x\n"
	 "    aux   = %04x\n"
	 "    index = %08lx\n"
	 "      parent   = %08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n",
	 (long)fc,
	 (long)LM_UL((UL *)&(fc->fs)),
	 (int)LM_UW(&(fc->dev)),
	 (int)LM_UW(&(fc->aux)),
	 (long)fc->index,
	 (long)(fc->index->parent),
	 fc->index->name,
	 fc->index->usecnt,
	 fc->index->childcnt );

    if ( !fc->index->usecnt )
    {
	DBG( "release: RELEASE OF UNUSED FILECOOKIE!\n" );
	return TOS_EACCDN;
    }
    fc->index->usecnt--;
    freefs( fc->index );
    return 0;
}


UL mint_fs_remove( MINT_FCOOKIE *dir, const char *name )
{
    char *unixname = NULL;
    const char *filename = mint_makefilename( dir->index, name, NULL );
    DBG( "remove\n" );
    if ( unlink( filename ) &&
	 ( errno != ENOENT ||
	   (filename = mint_maketosfilename( dir->index, 
					     name, 
					     NULL )) == NULL ||
	   unlink( filename ) ) )
    {
	DBG( "remove: unlink() and mint_searchfortosname() or unlink() failed: %d.\n",
	     errno );
	return unix2toserrno( errno, TOS_EACCDN );
    }
    else
	return 0;
}


UL mint_fs_rename( MINT_FCOOKIE *olddir, char *oldname, 
           MINT_FCOOKIE *newdir, const char *newname )
{
    char *olddirpath, *unixname;
    int rv;
    DBG( "mint_fs_rename:\n"
	 "  olddir  = %#08lx\n"
	 "    fs    = %#08lx\n"
	 "    dev   = %#04x\n"
	 "    aux   = %#04x\n"
	 "    index = %#08lx\n"
	 "      parent   = %#08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n"
	 "  oldname = \"%s\"\n"
	 "  newdir  = %#08lx\n"
	 "    fs    = %#08lx\n"
	 "    dev   = %#04x\n"
	 "    aux   = %#04x\n"
	 "    index = %#08lx\n"
	 "      parent   = %#08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n"
	 "  newname = \"%s\"\n",
	 (long)olddir,
	 (long)LM_UL((UL *)&(olddir->fs)),
	 (int)LM_UW(&(olddir->dev)),
	 (int)LM_UW(&(olddir->aux)),
	 (long)olddir->index,
	 (long)(olddir->index->parent),
	 olddir->index->name,
	 (int)olddir->index->usecnt,
	 (int)olddir->index->childcnt,
	 oldname,
	 (long)newdir,
	 (long)LM_UL((UL *)&(newdir->fs)),
	 (int)LM_UW(&(newdir->dev)),
	 (int)LM_UW(&(newdir->aux)),
	 (long)newdir->index,
	 (long)(newdir->index->parent),
	 newdir->index->name,
	 (int)newdir->index->usecnt,
	 (int)newdir->index->childcnt,
	 newname );
    if ( (olddirpath = strdup( mint_makefilename( olddir->index,
						  oldname,
						  NULL ) )) == NULL)
    {
	DBG( "rename: strdup() failed!\n" );
	return unix2toserrno( errno, TOS_ENSMEM );
    }
    rv = rename( olddirpath, mint_makefilename( newdir->index, 
						newname, 
						NULL) );
    free( olddirpath );
    if ( rv &&
	 errno == ENOENT &&
	 (unixname = mint_maketosfilename( olddir->index,
					   oldname,
					   NULL )) != NULL &&
	 (olddirpath = strdup( unixname )) != NULL )
    {
	rv = rename( olddirpath, mint_makefilename( newdir->index, 
						    newname, 
						    NULL) );
	free( olddirpath );
    }
    if ( rv )
    {
	DBG( "rename() failed: %d.\n", errno );
	return unix2toserrno( errno, TOS_EPTHNF );
    }
    return 0;
}


UL mint_fs_rewinddir( MINT_DIR *dirh )
{
    DBG( "rewinddir (NOT TESTED!)\n" );
    rewinddir( dirh->dir );
    SM_UW( &(dirh->index), 0 );
    return 0;
}


static int xrmdir( const char *dirpath )
{
    struct stat st;
    return ( rmdir( dirpath ) &&
	     ( errno != ENOTDIR ||
	       lstat( dirpath, &st ) ||
	       !S_ISLNK( st.st_mode ) ||
	       stat( dirpath, &st ) ||
	       !S_ISDIR( st.st_mode ) ||
	       unlink( dirpath ) ) );
}


UL mint_fs_rmdir( MINT_FCOOKIE *dir, const char *name )
{
    const char *dirpath = mint_makefilename( dir->index, name, NULL );
    DBG( "mint_fs_rmdir:\n"
	 "  dir  = %#08lx\n"
	 "    fs    = %08lx\n"
	 "    dev   = %04x\n"
	 "    aux   = %04x\n"
	 "    index = %08lx\n"
	 "      parent   = %08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n"
	 "  name = \"%s\"\n",
	 (long)dir,
	 (long)LM_UL((UL *)&(dir->fs)),
	 (int)LM_UW(&(dir->dev)),
	 (int)LM_UW(&(dir->aux)),
	 (long)dir->index,
	 (long)(dir->index->parent),
	 dir->index->name,
	 (int)dir->index->usecnt,
	 (int)dir->index->childcnt,
	 name );
    if ( xrmdir( dirpath ) && 
	 ( errno != ENOENT ||
	   (dirpath = mint_maketosfilename(dir->index, name, NULL )) == NULL ||
	   xrmdir( dirpath ) ) )
    {
	DBG( "rmdir() and lstat() or stat() or unlink() failed: %d\n",
	     errno );
	return unix2toserrno( errno, TOS_EACCDN );
    }
    return 0;
}


UL mint_fs_root( W drv, MINT_FCOOKIE *fc )
{
    /* REMEMBER: fc is in atari memory and bitorder! */
    MINT_FSFILE *new;
    
    DBG( "root:\n"
	 "  drv = %#04x\n"
	 "  fc  = %#08lx\n",
	 (int)drv,
	 (long)fc );
    
    if ( drv != mint_fs_devnum)
	return TOS_EDRIVE;
    
    if ( (new = malloc( sizeof( MINT_FSFILE ) )) == NULL )
    {
	DBG( "root: malloc() failed!\n" );
	return TOS_ENSMEM;
    }
    
    new->parent = NULL; /* root directory has no parent */
    new->name = "/";    /* root directory name doesn't need memory for name */
    new->usecnt = 1;    /* first use time */
    new->childcnt = 0;  /* since this is the first time there are no childs */
    
    SM_UL((UL *)&(fc->fs), mint_fs_drv);
    SM_UW(&(fc->dev),drv);
    SM_UW(&(fc->aux),0); /* not used */
    fc->index = new;    /* atari sees only a magic so we can set directly */
    DBG( "root result:\n"
	 "  fs    = %08lx\n"
	 "  dev   = %04x\n"
	 "  aux   = %04x\n"
	 "  index = %08lx\n"
	 "    parent   = %08lx\n"
	 "    name     = \"%s\"\n"
	 "    usecnt   = %d\n"
	 "    childcnt = %d\n",
	 (long)LM_UL((UL *)&(fc->fs)),
	 (int)LM_UW(&(fc->dev)),
	 (int)LM_UW(&(fc->aux)),
	 (long)fc->index,
	 (long)(fc->index->parent),
	 fc->index->name,
	 fc->index->usecnt,
	 fc->index->childcnt );
    return 0;
}


static const char *isstonxfs( const char *name )
{
    const char *mname = MINT_FS_NAME"\\";
    size_t lmname = strlen( mname ),
	lname = strlen( name );
    
    DBG( "symlink (NOT TESTED!)\n" );
    if ( ( lname == 4 && name[3] == ':' ) || /* CON: or something like this */
	 ( lname > 1 && tolower( *name ) != 'u' &&
	   name[1] == ':' ) ||            /* absolut symlink to another dev */
	 ( lname > 2 && tolower( *name ) == 'u' &&
	   name[1] == ':' && name[2] == '\\' &&
	   strncasecmp( name + 3, mname, lmname ) ) /* absolut symlink to 
						       another file system */
	)
	return NULL;
    /* It could be:
     *  "u:\\stonxfs\\..." (absolute to STonX file system) 
     *                     --> delete "u:\\stonxfs"
     *  "u:..."            (relative to STonX file system)
     *                     --> delete "u:"
     *  "\\stonxfs\\..."   (absolute to STonX file system)
     *                     --> delete "\\stonxfs"
     *  "\\..."            (absolute to STonX file sytem)
     *                     --> delete nothing
     *  "..."              (relative to STonX file system)
     *                     --> delete nothing
     */
    /* FIXME: "u:..." maybe not relativ to STonX file system */
    if ( tolower( *name ) == 'u' && name[1] == ':' )
	name += 2;
    if ( *name == '\\' && !strncasecmp( name + 1, mname, lmname ) )
	name += lmname;
    return name;
}


UL mint_fs_symlink( MINT_FCOOKIE *dir, const char *name, const char *to )
{
    char *toname, *h;
    const char *ch;
    int freetoname = 0, rv;
    DBG( "symlink:\n"
	 "  dir  = %#08lx\n"
	 "    fs    = %#08lx\n"
	 "    dev   = %#04x\n"
	 "    aux   = %#04x\n"
	 "    index = %#08lx\n"
	 "      parent   = %#08lx\n"
	 "      name     = \"%s\"\n"
	 "      usecnt   = %d\n"
	 "      childcnt = %d\n"
	 "  name = \"%s\"\n"
	 "  to   = \"%s\"\n",
	 (long)dir,
	 (long)LM_UL((UL *)&(dir->fs)),
	 (int)LM_UW(&(dir->dev)),
	 (int)LM_UW(&(dir->aux)),
	 (long)dir->index,
	 (long)(dir->index->parent),
	 dir->index->name,
	 dir->index->usecnt,
	 dir->index->childcnt,
	 name,
	 to );
    if ( (ch = isstonxfs( to )) == NULL )
    {
	/* Absolut link to a specific drive, which is not the unix-drive.
	 * We have to do:
	 *  - mark as special symlink (is already done with to[1] == ':')
	 *  - create symlink (see below)
	 */
	toname = (char *)to;
    }
    else
    {
	/* Absolut or relative link to current drive. */
	to = ch;
	/* Still an absolut or relative link to current drive.
	 * We have to do:
	 *  - converter name to unixname
	 *  - create symlink (see below)
	 */
	/* FIXME: could be a created 8+3-name! */
	if ( (toname = strdup( to )) == NULL )
	    return TOS_ENSMEM;
	freetoname = 1;
	for ( h = strchr( toname, '\\' ); h; h = strchr( h, '\\' ) )
	    *h++ = '/';
    }
    ch = mint_makefilename( dir->index, name, NULL );
    rv = symlink( toname, ch );
    if ( rv )
    {
	DBG( "symlink( \"%s\", \"%s\" ) --> %d\n",
	     toname,
	     ch,
	     errno );
	rv = unix2toserrno( errno, TOS_EPTHNF );
    }
    else
    {
	DBG( "symlink: made \"%s\" --> \"%s\"", toname, ch );
    }
    if ( freetoname )
	free( toname );
    return (UL)rv;
}


UL mint_fs_sync( void )
{
    DBG("sync()\n");
    sync();
    return TOS_E_OK;
}


UL mint_fs_unmount(W drv)
{
    if ( verbose )
	fprintf(stderr, "unmount() at STonXFS not yet supported!\n");
    return TOS_ENOSYS;
}


UL mint_fs_writelabel( MINT_FCOOKIE *dir, const char *name )
{
    if ( verbose )
	fprintf(stderr,"writelabel() at STonXFS not supported!\n");
    return TOS_EINVFN;
}

#endif /* MINT_STONXFS */
