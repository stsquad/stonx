/*
 * Filename:    mint_defs.h
 * Version:     0.5
 * Author:      Markus Kohm, Chris Felsch
 * Started:     1998
 * Target O/S:  MiNT running on StonX (Linux)
 * Description: MiNT/TOS definitions for STonX (taken from mint/dcntl.h)
 * 
 * Note:        Please send suggestions, patches or bug reports to 
 *              Chris Felsch <C.Felsch@gmx.de>
 * 
 * Copying:     Copyright 1998 Markus Kohm
 *              Copyright 2000 Chris Felsch (C.Felsch@gmx.de)
 * 
 * Known Bugs:  A lot of stuff at this file will only work at 32bit systems.
 *              A lot of stuff may not work, if alignment changes. This may
 *              cause problems at almost every structure type at this file.
 *              All structures should work with even alignment. But if the
 *              compiler uses 4-byte-alignment for 2- or 4-byte integer or
 *              pointers, this may result in problems with some of the
 *              structures.
 *
 * Remember:    Objects at atari memory are in 68000er-bitorder. Use mem.h
 *              to read or write those.
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
#ifndef _mint_defs_h_
#define _mint_defs_h_

#include <dirent.h>   /* FIXME: what about HAVE_DIRENT_H? */

#include "../defs.h"
#include "../toserror.h"

typedef struct devdrv MINT_DEVDRV;

typedef struct filesys MINT_FILESYS;

typedef struct fsfile 
{ 
    struct fsfile *parent;
    UL             usecnt;      /* free this struct only if usecnt == 0 */
    UL             childcnt;    /* free this struct only if childcnt == 0 */
    char          *name;        /* namepart of this file or directory */
} MINT_FSFILE;


typedef struct f_cookie 
{
    MINT_FILESYS *fs;
    UW            dev;
    UW            aux;
    MINT_FSFILE  *index;   /* FIXME: we can do this only at 32bit systems! */
} MINT_FCOOKIE;


typedef struct fileptr 
{
    W               links;    /* number of copies of this descriptor */
    UW              flags;    /* file open mode and other file flags */
    L               pos;      /* position in file */
    L               devinfo;  /* device driver specific info */
    MINT_FCOOKIE    fc;       /* file system cookie for this file */
    MINT_DEVDRV    *dev;  /* device driver that knows how to deal with this */
    struct fileptr *next;     /* link to next fileptr for this file */
} MINT_FILEPTR;


typedef struct xattr 
{
    UW    mode;
    UW    index[2];        /* to avoid alignment-problems */
    UW    dev;
    UW    rdev;
    UW    nlink;
    UW    uid;
    UW    gid;
    W     size[2];
    W     blksize[2];
    W     nblocks[2];
    W     mtime, mdate;
    W     atime, adate;
    W     ctime, cdate;
    W     attr;
    W     reserved2;
    L     reserved3[2];
} MINT_XATTR;


#define TOS_SEARCH 0x01
typedef struct dir 
{
    MINT_FCOOKIE fc;         /* cookie for this directory */
    UW           index;      /* index of the current entry */
    UW           flags;      /* flags (e. g. tos or not) */
    DIR         *dir;        /* used DIR */
    char         reserved[60 - sizeof(DIR*)];
    struct dir  *next;  /* linked together so we can close them to process term */
} MINT_DIR;


#define TOS_F_RDLCK    0
#define TOS_F_WRLCK    1
#define TOS_F_UNLCK    3
struct mflock 
{
    W    l_type;         /* type of lock */
    W    l_whence;       /* SEEK_SET, SEEK_CUR, SEEK_END */
    L    l_start;        /* start of locked region */
    L    l_len;          /* length of locked region */
    W    l_pid;          /* pid of locking process (F_GETLK only) */
};


/* structure for internal kernel locks */
typedef struct ilock 
{
    struct mflock    l;            /* the actual lock */
    struct ilock     *next;        /* next lock in the list */
    L                 reserved[4]; /* reserved for future expansion */
} MINT_LOCK;


typedef struct kerinfo
{
    W    maj_version;       /* kernel version number */
    W    min_version;       /* minor kernel version number */
    UW   default_perm;      /* default file permissions */
    W    reserved1;         /* room for expansion */

    /* OS functions */
    /* FIXME: We can do this only on 32bit systems */
    UL  *bios_tab;          /* pointer to the BIOS entry points */
    UL  *dos_tab;           /* pointer to the GEMDOS entry points */

    /* media change vector */
    UL   drvchng;           /* P_((unsigned)); */

    /* Debugging stuff */
    UL   trace;             /* P_((const char *, ...)); */
    UL   debug;             /* P_((const char *, ...)); */
    UL   alert;             /* P_((const char *, ...)); */
    UL   fatal;             /* P_((const char *, ...)); */

    /* memory allocation functions */
    UL   kmalloc;           /* P_((long)); */
    UL   kfree;             /* P_((void *)); */
    UL   umalloc;           /* P_((long)); */
    UL   ufree;             /* P_((void *)); */

    /* utility functions for string manipulation */
    UL   strnicmp;          /* P_((const char *, const char *, int)); */
    UL   stricmp;           /* P_((const char *, const char *)); */
    UL   strlwr;            /* P_((char *)); */
    UL   strupr;            /* P_((char *)); */
    UL   sprintf;           /* P_((char *, const char *, ...)); */

    /* utility functions for manipulating time */
    UL   millis_time;       /* P_((unsigned long, short *)); */
    UL   unixtim;           /* P_((unsigned, unsigned)); */
    UL   dostim;            /* P_((long)); */

    /* utility functions for dealing with pauses, or for putting processes to sleep */
    UL   nap;               /* P_((unsigned)); */
    UL   sleep;             /* P_((int que, long cond)); */
    UL   wake;              /* P_((int que, long cond)); */
    UL   wakeselect;        /* P_((long param)); */

    /* file system utility functions */
    UL   denyshare;         /* P_((FILEPTR *, FILEPTR *)); */
    UL   denylock;          /* P_((LOCK *, LOCK *)); */

    /* functions for adding/cancelling timeouts */
    UL   addtimeout;        /* P_((long, void (*)())); */
    UL   canceltimeout;     /* P_((struct timeout *)); */
    UL   addroottimeout;    /* P_((long, void (*)(), short)); */
    UL   cancelroottimeout; /* P_((struct timeout *)); */

    /* miscellaneous other things */
    UL   ikill;             /* P_((int, int)); */
    UL   iwake;             /* P_((int que, long cond, short pid)); */

    /* reserved for future use */
    L    res2[3];
} MINT_KERINFO;


#define MX_KER_XFSNAME    (('m'<< 8) | 5)

#define FS_INFO           0xf100
#define FS_USAGE          0xf101

#define _MAJOR_STONX      (11L << 16)
#define FS_STONX          (_MAJOR_STONX)

struct fs_info
{
    char    name[32];
    UW      version[2];
    UW      type[2];
    char    type_asc[32];
};


struct fs_usage
{
    UW     blocksize[2];        /* 32bit: size in bytes of a block */
    UW     blocks[4];           /* 64bit: number of blocks */
    UW     free_blocks[4];      /* 64bit: number of free blocks */
    UW     inodes[4];           /* 64bit: number of inodes */
    UW     free_inodes[4];      /* 64bit: number of free inodes */ 
};


#define TOS_S_IFMT  0170000
#define TOS_S_IFCHR 0020000
#define TOS_S_IFDIR 0040000
#define TOS_S_IFREG 0100000
#define TOS_S_IFIFO 0120000
#define TOS_S_IFMEM 0140000
#define TOS_S_IFLNK 0160000

#define TOS_DP_IOPEN        0
#define TOS_DP_MAXLINKS     1
#define TOS_DP_PATHMAX      2
#define TOS_DP_NAMEMAX      3
#define TOS_DP_ATOMIC       4
#define TOS_DP_TRUNC        5
# define TOS_DP_NOTRUNC      0
# define TOS_DP_AUTOTRUNC    1
# define TOS_DP_DOSTRUNC     2
#define TOS_DP_CASE         6
# define TOS_DP_CASESENS     0
# define TOS_DP_CASECONV     1
# define TOS_DP_CASEINSENS   2
#define TOS_DP_MODEATTR     7
# define TOS_DP_ATTRBITS     0x000000ffL
# define TOS_DP_MODEBITS     0x000fff00L
# define TOS_DP_FILETYPS     0xfff00000L
# define TOS_DP_FT_DIR       0x00100000L
# define TOS_DP_FT_CHR       0x00200000L
# define TOS_DP_FT_BLK       0x00400000L
# define TOS_DP_FT_REG       0x00800000L
# define TOS_DP_FT_LNK       0x01000000L
# define TOS_DP_FT_SOCK      0x02000000L
# define TOS_DP_FT_FIFO      0x04000000L
# define TOS_DP_FT_MEM       0x08000000L
#define TOS_DP_XATTRFIELDS  8
# define TOS_DP_INDEX        0x0001
# define TOS_DP_DEV          0x0002
# define TOS_DP_RDEV         0x0004
# define TOS_DP_NLINK        0x0008
# define TOS_DP_UID          0x0010
# define TOS_DP_GID          0x0020
# define TOS_DP_BLKSIZE      0x0040
# define TOS_DP_SIZE         0x0080
# define TOS_DP_NBLOCKS      0x0100
# define TOS_DP_ATIME        0x0200
# define TOS_DP_CTIME        0x0400
# define TOS_DP_MTIME        0x0800
#define TOS_DP_MAXREQ       8

#define TOS_UNLIMITED 0x7fffffffL

#define TOS_FSTAT       (('F' * 256) | 0)
#define TOS_FIONREAD    (('F' * 256) | 1)
#define TOS_FIONWRITE   (('F' * 256) | 2)
#define TOS_FUTIME      (('F' * 256) | 3)

#define TOS_O_RDONLY    0x00
#define TOS_O_WRONLY    0x01
#define TOS_O_RDWR      0x02
#define TOS_O_RWMASK    (TOS_O_WRONLY|TOS_O_RDWR) /* isolate read/write mode */
#define TOS_O_EXEC      0x03

#define TOS_O_APPEND    0x08

#define TOS_O_SHMODE    0x70
# define TOS_O_COMPAT    0x00
# define TOS_O_DENYRW    0x10
# define TOS_O_DENYW     0x20
# define TOS_O_DENYR     0x30
# define TOS_O_DENYNOME  0x40

#define TOS_O_NOINHERIT 0x80

#define TOS_O_NDELAY    0x100
#define TOS_O_CREAT     0x200
#define TOS_O_TRUNC     0x400
#define TOS_O_EXCL      0x800

#define TOS_O_USER      0xfff

#define TOS_O_GLOBAL    0x1000

#define TOS_O_TTY       0x2000
#define TOS_O_HEAD      0x4000
#define TOS_O_LOCK      0x8000

/*
 * MiNT tty defines
*/
#define TOS_TIOCGPGRP   (('T' * 256) | 6)
#define TOS_TIOCIBAUD   (('T' * 256) | 18)
#define TOS_TIOCOBAUD   (('T' * 256) | 19)
#define TOS_TIOCGFLAGS  (('T' * 256) | 22)
#define TOS_TIOCSFLAGS  (('T' * 256) | 23)

#define TOS_TIOCBUFFER  (('T' * 256) | 128)
#define TOS_TIOCCTLMAP  (('T' * 256) | 129)
#define TOS_TIOCCTLGET  (('T' * 256) | 130)
#define TOS_TIOCCTLSET  (('T' * 256) | 131)

#define TOS_B57600      16
#define TOS_B115200     17
#define TOS_B230400     18


/* number of  stop bits */
#define TOS_TF_1STOP     0x0001 /* 1 */
#define TOS_TF_15STOP    0x0002 /* 1.5  */
#define TOS_TF_2STOP     0x0003 /* 2  */

/* number of bits per char */
#define TOS_TF_8BIT      0x0 /* 8 Bit */
#define TOS_TF_7BIT      0x4
#define TOS_TF_6BIT      0x8
#define TOS_TF_5BIT      0xC /* 5 Bit */

/* handshake and parity */
#define TOS_TF_FLAG      0xF000
# define TOS_T_TANDEM     0x1000 /* XON/XOFF (=^Q/^S) active */
# define TOS_T_RTSCTS     0x2000 /* RTS/CTS active */
# define TOS_T_EVENP      0x4000 /* even parity */
# define TOS_T_ODDP       0x8000 /* odd parity */

#endif
