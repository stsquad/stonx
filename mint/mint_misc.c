/*
 * Part of STonX
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

#include "../defs.h"
#include "mint_misc.h"

#ifdef DEBUG_MINT_MISC
# define DBG(_args...) fprintf(stderr, ## _args)
#else
# define DBG(_args...)
#endif


/*
 * Convert a Dos style (time, date) pair into
 * a Unix time_t
 */
time_t mint_m2u_time(UW tostime, UW tosdate)
{
    struct tm t;

    t.tm_sec = (tostime & 31) << 1;
    t.tm_min = (tostime >> 5) & 63;
    t.tm_hour = (tostime >> 11) & 31;
    t.tm_mday = tosdate & 31;
    t.tm_mon = ((tosdate >> 5) & 15) - 1;
    t.tm_year = 80 + ((tosdate >> 9) & 255);
    t.tm_isdst = -1; /* FIXME: daylight saving time or not? */
    return mktime( &t );
}


/*
 * converting of file modes and attributes
*/
UW mint_u2m_mode(mode_t m)
{
    UW mintmode;

    mintmode = (m & 07777);
    if ( S_ISCHR( m ) )
    mintmode |= TOS_S_IFCHR;

    if ( S_ISDIR( m ) )
    mintmode |= TOS_S_IFDIR;

    if ( S_ISREG( m ) )
    mintmode |= TOS_S_IFREG;

    if ( S_ISFIFO( m ) )
    mintmode |= TOS_S_IFIFO;

    if ( S_ISLNK( m ) )
    mintmode |= TOS_S_IFLNK;

    return mintmode;
}


mode_t mint_m2u_openmode(UW m)
{
    mode_t unixmode;

    switch( m & TOS_O_RWMASK )
    { /* read/write/exec-mode */
      case TOS_O_RDONLY:
	  unixmode = O_RDONLY;
	  break;
	  
      case TOS_O_WRONLY:
	  unixmode = O_WRONLY;
	  break;
	  
      case TOS_O_RDWR:
	  unixmode = O_RDWR;
	  break;
	  
      case TOS_O_EXEC: /* special kernel mode for opening executables */
	  /* FIXME: while uid and gid isn't correct we should test x-flag 
	   *        first, but the real correct sollution is to change uid
	   *        and gid at MiNT-init.
	   */
	  /* don't say `break' here */
      default:
	  unixmode = O_RDONLY; /* open these readonly */
	  break;
    }
    
    unixmode |= O_NOCTTY; /* should never be controlling tty */
    
    if ( m & TOS_O_APPEND )
        unixmode |= O_APPEND;
    /* FIXME: TOS_O_SHMODE is ignored! */
    /* FIXME: TOS_O_NOINHERIT is ignored! */
    
    if ( m & TOS_O_NDELAY )
        unixmode |= O_NDELAY;
    
    if ( m & TOS_O_CREAT ) /* doc says: this will never happend */
        unixmode |= O_CREAT;
    
    if ( m & TOS_O_TRUNC )
        unixmode |= O_TRUNC;
    
    if ( m & TOS_O_EXCL )
        unixmode |= O_EXCL;
    
    return unixmode;
}


W mint_u2m_attrib(mode_t m , const char *filename)
{
    W rv;

    if ( S_ISDIR( m ) )
        rv = 0x10;
    else if ( ( m & ( S_IWUSR | S_IWGRP | S_IWOTH ) ) == 0 )
        rv = 0x01; /* READONLY */
    else
        rv = 0;
    if ( rv != 0x10 && *filename == '.' )
        rv |= 0x02; /* HIDDEN (why are directories never hidden at tos?) */
    return rv;
}


/*
 * build a complete linux filepath
 * --> fs    something like a handle to a known file
 *     name  a postfix to the path of fs or NULL, if no postfix
 *     buf   buffer for the complete filepath or NULL if static one
 *           should be used.
 * <-- complete filepath or NULL if error 
 */
char *mint_makefilename(MINT_FSFILE *fs, const char *name, char *buf)
{
    static char sbuf[256]; /* FIXME: size should told by unix */

    if (!buf)
        buf = sbuf;
    if (!fs)
    {
        /* we are at root */
        if (!name)
            return NULL;
        else
            strcpy(buf, name);
    }
    else
    {
        char *h;
        if (!mint_makefilename(fs->parent, fs->name, buf))
            return NULL;
        if (name && *name)
        {
            if ((h = strchr(buf, '\0'))[-1] != '/')
                *h++ = '/';
            strcpy( h, name );
        }
    }
    return buf;
}


/*
 * converting linux to tos filenames and vice versa
*/
#define IS_TOSFILENAMECHAR(c)     (islower(c) || isdigit(c) ||\
                                   strchr("_!@#$%^&()+-=`~;\"\",<>|[]{}", c))
#define IS_WEAKTOSFILENAMECHAR(c) (isalpha(c) || IS_TOSFILENAMECHAR(c))


/* Is ext a extension of a 8+3-name? */
static int is_tosextend(const char *ext)
{
    int i;
    
    if (!*ext++)
        return 1;
    for (i = 0; i < 3 && *ext && IS_WEAKTOSFILENAMECHAR(*ext); i++, ext++ )
        ;
    return *ext == 0;
}


/* May this be a created 8+3 name? */
static int maybe_createdtosname(const char *filename)
{
    int i;

    for (i = 0; i < 8 && IS_WEAKTOSFILENAMECHAR(filename[i]); i++ )
        ;
    return (i && isxdigit( filename[i-1]) && 
        (isdigit(filename[i-1]) || isupper(filename[i-1])) &&
        is_tosextend(filename + i));
}


/* Copy tos allowed chars of last extension of filename to extbuf. */
static void add_lasttosextend( char *extbuf, const char *filename )
{
    int i;

    if ((filename = strrchr(filename, '.')) != NULL)
    {
        DBG( "%c", *filename );
        *extbuf++ = *filename++;
        for (i = 0; i < 3 && *filename; filename++)
            if (IS_WEAKTOSFILENAMECHAR(*filename))
            {
                extbuf[i++] = *filename;
            }
        extbuf[i] = '\0';
        DBG("%s", extbuf);
    }
}


/*
 * Make 8+3-Name, if it isn't one.
 * Never the less, names are casesensitiv, if they are!
 * <-- NULL, if the name is not possible!
 *     static char *tosname, otherwise.
 */
const char *mint_maketosname(const char *filename, off_t dirpos)
{
    static char tosname[14];    /* we need only 8+1+3+1 = 13 */
    char        numstr[18];     /* we need only 8+1 */
    int         i, l, j;

    DBG( "\"%s\" --> \"", filename );
    for (i = 0; 
	 i < 8 && *filename && IS_WEAKTOSFILENAMECHAR(*filename); 
	 filename++)
        tosname[i++] = *filename;
    if ((!i && *filename == '.' && 
        (!filename[1] || (filename[1] == '.' && !filename[2] ) ) ) ||
        is_tosextend(filename))
    {
        strcpy( tosname + i, filename );
        DBG( "%s\"\n", tosname );
    }
    else
    {
        /* We have to create a 8+3-name: */
        sprintf( numstr, "%lX", (long)dirpos );
        if ( ( l = strlen( numstr ) ) > (8 - i) ) {
            if ( l > 8 ) {
                return NULL;
            } else {
                i = 8 - l;
	    }
	}
        strcpy( tosname+i, numstr );
        i += l;
        DBG( "%s", tosname );
        add_lasttosextend( tosname+i, filename );
        DBG( "\"\n" );
    }
    return tosname;
}


/*
 * Test, if filename at dirname is a created name and return the
 * unixname of the file!
 * <-- NULL, if there is no unixname for this filename
 *     malloced unixname, otherwise.
 */
char *mint_searchfortosname(const char *dirname, const char *filename)
{
    DIR        *dir;
    struct        dirent *de;
    const char    *tosname;
    char        *buffer;
    
    DBG("mint_searchfortosname\n"
        "-->\n"
        "dirname  = \"%s\"\n"
        "filename = \"%s\"\n",
        dirname, filename );
    
    if (maybe_createdtosname(filename) && (dir = opendir(dirname)) != NULL)
    {
        while ((de = readdir(dir)) != NULL)
        {
            if ((tosname = mint_maketosname(de->d_name, telldir(dir))) != NULL
		&& !strcmp(tosname, filename))
                {
		    if ((buffer = strdup(de->d_name)) == NULL)
			break;
		    DBG( "<-- \"%s\"", buffer );
		    return buffer;
		}
        }
        closedir( dir );
    }
    errno = ENOENT;
    DBG( "<-- failed!\n" );
    return NULL;
}


/*
 * build a complete filepath for a 8+3 name
 * --> fs    something like a handle to a known file
 *     name  a 8+3 postfix to the path of fs or NULL, if no postfix
 *     buf   buffer for the complete filepath or NULL if static one
 *           should be used.
 * <-- complete filepath or NULL if error 
 */
char *mint_maketosfilename(MINT_FSFILE *fs, const char *name, char *buf)
{
    char *unixname, *rv = NULL;

    unixname = mint_searchfortosname(mint_makefilename(fs, NULL, buf), name);
    if (unixname != NULL)
    {
        rv = mint_makefilename(fs, unixname, buf);
        free(unixname);
    }
    return rv;
}


