/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */

#include "defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(STATFS_USE_STATVFS)
#include <sys/statvfs.h>
#elif defined(STATFS_USE_VFS)
#include <sys/vfs.h>
#elif defined(STATFS_USE_MOUNT)
#include <sys/param.h>
#include <sys/mount.h>
#elif defined(STATFS_USE_STATFS_VMOUNT)
#include <sys/statfs.h>
#include <sys/vmount.h>
#endif
#include <fcntl.h>
#include <dirent.h>

#include "debug.h"
#include "main.h"
#include "utils.h"
#include "tosdefs.h"
#include "native.h"
#include "toserror.h"
#include "cartridge/cartridge.h"


#if 1
#define STATFUNC stat
#else
#define STATFUNC lstat
#endif

#define DEBUG_GEMDOS 0
#define DEFAULT_MODE 0644
#define DEFAULT_DIRMODE 0755

#define FSNEXT_JUSTSTAT 0x8000

/* some of this code is was stolen&modified from JET */
#define TOS_NAMELEN 13

/* Monitor support */
#if MONITOR
#include "monitor.h"
#endif

#if DEBUG_GEMDOS
#define DBG(_args...) fprintf( stderr, ## _args )
#else
#define DBG(_args...)
#endif

typedef struct
{
    char index[2];  /* FIXME - should use seekdir() instead of DIR * array */
    char magic[4];
#define SVALID 0x1234fedc
#define EVALID 0x5678ba90
    char dta_pat[TOS_NAMELEN+1];
    char dta_sattrib;
    char dta_attrib;
    char dta_time[2];
    char dta_date[2];
    char dta_size[4];
    char dta_name[TOS_NAMELEN];
} DTA;

typedef struct
{
    UL bp;
    int f,redirected;
} FINFO;

#define MAXDRIVES 26

extern int drive_bits, drive_fd[];
extern int redirect_cconws;
int gemdos_ok=0;
UL gemdos_drives = 0;
UW curdrv=0;
char curpath[MAXDRIVES][500];
#define MAXFILES 100
#define H_OFFSET 80 /* < 100 because of problems with PC/TC's assembler */
	/* FIXME: There should be one handle-pool only!  */
static FINFO file[MAXFILES];
static int redirect_stdh[6][MAXFILES];
void *act_pd;
#define MAXDIRS 40

DIR *fsdir[MAXDIRS];
char curspath[MAXDIRS][500];
static char *root[MAXDRIVES];
int cursdrv;

const char *get_gemdos_drive (char d)
{
    int drv=toupper(d&0xff)-'A';
    if (drv<0||drv>MAXDRIVES)
    {
	fprintf(stderr,"Invalid drive: `%c'\n",d);
	return NULL;
    }
    else if (!(drive_bits & (1<<drv)))
    {
	fprintf(stderr,"GEMDOS drive `%c' not defined!\n",d);
	return NULL;
    }
    return root[drv];
}

int add_gemdos_drive (char d, char *path)
{
    int drv=toupper(d&0xff)-'A';
    if (drv<0||drv>MAXDRIVES)
    {
	fprintf(stderr,"Invalid drive: `%c'\n",d);
	return 0;
    }
    else if (drive_bits & (1<<drv))
    {
	fprintf(stderr, "Drive `%c' is already used as a BIOS drive\n",d);
	return 0;
    }
    else if (gemdos_drives & (1<<drv))
    {
	fprintf(stderr, "Drive `%c' already specified as a GEMDOS drive\n",d);
	return 0;
    }
    gemdos_drives |= 1<<drv;
    drive_bits |= 1<<drv;
    drive_fd[drv] = -1;
    root[drv] = strdup (path);
    return 1;
}

static void kill_redirectionfrom(int stdh,int r)
{
    int i;
    if ( stdh >= 0 && stdh < 6 && r < MAXFILES )
    {
	for ( i = r; i < MAXFILES && redirect_stdh[stdh][i] != -1; i++ )
	    if ( file[redirect_stdh[stdh][i]].redirected > 0 )
		file[redirect_stdh[stdh][i]].redirected--;
	redirect_stdh[stdh][r] = -1;
    }
}

static int get_redirection(int handle)
{
    int r,i;
    if ( handle < 0 || handle > 5 || redirect_stdh[handle][0] == -1 )
	return -1;
    for (r=1;r<MAXFILES && redirect_stdh[handle][r]!=-1;r++);
    i=redirect_stdh[handle][--r];
    if ( file[i].f < 0 || file[i].redirected <= 0 )
    {
	fprintf(stderr,"Redirection %d --> %d already closed!\n",handle,i+H_OFFSET);
	kill_redirectionfrom(handle,r);
	return -1;
    }
    return i;
}

static void C_in(int handle)
{
    int i;
    char c;
    if ((i=get_redirection(handle))<0) return;
    read(file[i].f,&c,1);
    DREG(0)=c;
    SET_Z();
    return;
}
    
static void C_is(int handle)
{
    int i;
    off_t pos,endpos;
    if ((i=get_redirection(handle))<0) return;
    if ((pos=lseek(file[i].f,0,SEEK_CUR)) < 0) return;
    endpos=lseek(file[i].f,0,SEEK_END);
    lseek(file[i].f,pos,SEEK_SET);
    if (endpos > pos)
    {
	DREG(0)=-1;
    }
    else
    {
	DREG(0)=0;
    }
    SET_Z();
    return;
}

static void C_os(int handle)
{
    if (get_redirection(handle)<0) return;
    DREG(0)=-1;
    SET_Z();
    return;
}

static void C_out(int handle,char c)    
{
    int i,r;
    if ((i=get_redirection(handle))<0) return;
    if ((r=write(file[i].f,&c,1))<0)
	r=unix2toserrno(errno,TOS_EACCDN);
    DREG(0)=r; /* not documented! */
    SET_Z();
    return;
}

void Cconin(void)
{
    return C_in(0);
}

void Cconis(void)
{
    return C_is(0);
}

void Cconos(void)
{
    if ( get_redirection(1) < 0 && !redirect_cconws ) return;
    DREG(0)=1;
    SET_Z();
    return;
}
    
void Cconout(UW c)
{
    if ( get_redirection(1) >= 0 )
	C_out(1,c);
    else if ( redirect_cconws )
    {
	fputc(c,stdout);
	SET_Z();
    }
    return;
}

void Crawio(UW w)
{
    if ( (w & 0xFF) == 0xFF )
	Cconin();
    else 
	Cconout(w);
    return;
}

void Cconws (char *str)
{
    int r,i;
    if ( (i = get_redirection(1)) >= 0 )
    {
	if ( ( r = write( file[i].f, str, strlen(str) ) ) < 0 )
	    r = unix2toserrno( errno, TOS_EACCDN );
	DREG(0)=r; /* not documented! */
	SET_Z();
	return;
    }
    else if (!redirect_cconws) return;
    fputs(str,stdout);
    SET_Z();
}

void Dsetdrv (UW drv)
{
    curdrv = drv;
#if 0
    if (BIT(drv,drive_bits) == 0)
    {
	DREG(0) = TOS_EDRIVE;
	SET_Z();
    }
    else
    {
	curdrv = drv;
	DREG(0) = drive_bits;
	SET_Z();
    }
#endif
}

void Dgetdrv (void)
{
    DREG(0) = curdrv;
    SET_Z();
}

void Fsetdta (UL dtaptr)
{
    UL bp = LM_UL(act_pd);
#if 0
    SM_UL(MEM(bp+32),dtaptr);
    SET_Z();
#endif
}

void Fgetdta (void)
{
    UL bp = LM_UL(act_pd);
    DREG(0) = LM_UL(MEM(bp+32));
    SET_Z();
}

void fname2unix (char *stpath, char *unixpath)
{
    char *a,*b;
    char *n;
    a=stpath; b=unixpath;
    if (stpath[1]==':') n = stpath+2;
    else n=stpath;
    while(*n&&*n!=' ')
    {
	*unixpath++ = (*n == '\\' ? '/' : tolower(*n));
	n++;
    }
    *unixpath=0;
}

static char lastpath[500];

int st2unixpath (char *stpath, char *unixpath)
{
    int drv;
    char path[500];
    drv = (stpath[1] == ':' ? toupper(stpath[0])-'A' : curdrv);
    if (((gemdos_drives>>drv)&1) == 0)
    {
	*unixpath=0;
	return 0;
    }
    if (stpath[1] == ':')
	strcpy(lastpath,stpath);
    else if (stpath[0] == '\\')
    {
	sprintf (lastpath, "%c:%s", curdrv+'A',stpath);
    }
    else
    {
	sprintf (lastpath,"%c:%s\\%s",curdrv+'A',curpath[drv],stpath);
    }
    fname2unix(lastpath,path);
    sprintf(unixpath,"%s%s",root[drv],path);
    return 1;
}

int unix2fname (char *unixname, char *stname)
{
    /* FIXME */
    char *p=unixname;
    char *q=stname;
    if (strlen(unixname)>12) return 0;
    while (*p)
    {
	if (*p == '.')
	{
#if 0
	    if (dot) return 0;
	    else dot=1;
#endif
	    *stname++='.';
	}
	else *stname++ = toupper(*p);
	p++;
    }
    *stname=0;
    return 1;
}

static int match (char *pat, char *name)
{
    if (strcmp(pat,"*.*")==0) return 1;
    else if (strcasecmp(pat,name)==0) return 1;
    else
    {
	char *p=pat,*n=name;
	for(;*n;)
	{
	    if (*p=='*') {while (*n && *n != '.') n++;p++;}
	    else if (*p=='?' && *n) {n++;p++;}
	    else if (*p++ != *n++) return 0;
	}
	if (*p==0)
	{
	    return 1;
	}
    }
    return 0;
}

void compress_path (char *path)
{
    char *lastdir=NULL, lastchar=0, *d, *p=path;
    char *dirstack[MAXDIRS];
    int a=0, i;
    d = path;
    while (*p)
    {
	if (*p == '\\')
	{
	    if (strncmp(p,"\\..\\",4) == 0)
	    {
		while (--d>=path && *d != '\\');
		if (d<path)
		{
		    fprintf (stderr,
			     "FATAL error in compress_path: \\..\\\n");
		    exit(1);
		}
		p += 3;
	    }
	    else if (strncmp(p,"\\.\\",3) == 0)
	    {
		p += 2;
	    }
	    else
	    {
		while (*++p == '\\');
		*d++ = '\\';
	    }
	}
	else *d++=*p++;
    }
    if (d != path && *(d-1) == '\\') d--;
    *d=0;
}

int xopendir (char *p, int doopen)
{
    int i;
    DIR *d;
    for (i=0; i<MAXDIRS && fsdir[i]; i++);
    if (i==MAXDIRS)
    {
	DBG("FATAL error in xopendir - out of dirs\n");
	exit(1);
    }
    if (!doopen) {
	fsdir[i] = (DIR *)-1;
	return i | FSNEXT_JUSTSTAT;
    }
    d = opendir(p);
    if (d==NULL)
    {
	DBG("opendir(%s)  - no files\n",p);
	return -1;
    }
    fsdir[i] = d;
    return i;
}

void xclosedir (int i)
{
    if (i >= 0)
	closedir(fsdir[i]);
    fsdir[i&(FSNEXT_JUSTSTAT-1)] = NULL;
}

struct dirent *xreaddir (int i)
{
    if (i >= 0)
	return readdir (fsdir[i]);
    return NULL;
}


UW time2dos (time_t t)
{
    struct tm *x;
    x = localtime (&t);
    return (x->tm_sec>>1)|(x->tm_min<<5)|(x->tm_hour<<11);
}

UW date2dos (time_t t)
{
    struct tm *x;
    x = localtime (&t);
    return x->tm_mday|((x->tm_mon+1)<<5)|(MAX(x->tm_year-80,0)<<9);
}


void Fsnext (void)
{
    UL bp;
    struct stat s;
    struct dirent *e;
    struct
    {
        struct dirent etmpdirent;
        /* On some systems like Irix 5.3, dirent.d_name is only declared as
         * a "char d_name[1]", so we have to add some additional space after
         * our etmpdirent variable here for the file name: */
        char etmpname[256];
    } etmp;
    char stname[TOS_NAMELEN+1];
    char uname[500];
    UB attribs;
    DTA *dta;
    UW da;
    int rr = -1;

    if (((gemdos_drives >> cursdrv)&1)==0)  return;

    bp = LM_UL(act_pd);
    dta = (DTA *)MEM(LM_UL(MEM(bp+32)));
    attribs = LM_UB(&(dta->dta_sattrib));

 repeat:
    SM_UL((UL *)&(dta -> magic),EVALID);
    do
    {
	rr = (LM_W((W *)&(dta->index)));
	if (rr < 0) {
	    if (rr == -1) {
		DREG(0) = TOS_ENMFIL;
		SET_Z();
		return;
	    } else {
		SM_W((W *)&(dta->index),-1);
		xclosedir(rr);
		rr &= (FSNEXT_JUSTSTAT-1);
		e = &etmp.etmpdirent;
		strncpy (e->d_name,dta->dta_pat,TOS_NAMELEN);
		e->d_name[TOS_NAMELEN] = '\0';
		unix2fname(e->d_name,stname);
		fname2unix(stname,e->d_name);
		break;
	    }
	} else {
	    e = xreaddir (rr);
	    if (e == NULL)
	    {	
		DREG(0) = TOS_ENMFIL;
		SET_Z();
		xclosedir(LM_UW((UW *)&(dta->index)));
		return;
	    }
	}
    } while (!unix2fname(e->d_name,stname) || !match((char *)&(dta->dta_pat),stname));
    SM_UL((UL *)&(dta->magic),SVALID);
    strcpy(dta->dta_name, stname);
    sprintf (uname, "%s/%s", curspath[rr], e->d_name);
    if (STATFUNC (uname, &s) < 0)
	goto repeat;
    da = (s.st_mode & S_IFDIR)?0x10:0; 	/* FIXME */
    if (!(da == 0 ||
	  ((da != 0) && (attribs != 8)) || ((attribs | 0x21) & da)))
	goto repeat;
    SM_UB(&(dta->dta_attrib),da);
    SM_UW((UW *)&(dta->dta_time),time2dos(s.st_mtime));
    SM_UW((UW *)&(dta->dta_date),date2dos(s.st_mtime));
    SM_UL((UL *)&(dta->dta_size),0);
    if (LM_UB(&(dta->dta_attrib)) & 0x10)
    {
	DREG(0)=0;
	SET_Z();
	return;
    }
    SM_UL((UL *)&(dta->dta_size),s.st_size);
    DREG(0)=0;
    SET_Z();
    return;
}


void Dsetpath (char *pname)
{
    char upath[500];
    struct stat s;
    int drv;
    if (pname[1] == ':') drv=toupper(pname[0])-'A';
    else drv=curdrv;
    if (!st2unixpath(pname,upath))
    {
	DBG("Dsetpath(%s) not done\n",pname);
	return;
    }
    if (STATFUNC(upath,&s) < 0 && !S_ISDIR(s.st_mode))
    {
	DREG(0) = TOS_EPTHNF;
	SET_Z();
	return;
    }
    if (pname[1] == ':') strcpy(pname,&pname[2]); /* for Dsetdrv(C:\FOO) */
    if (pname[0] == '\\') strcpy(curpath[drv],pname);
    else
    {
	strcat(curpath[drv],"\\");
	strcat(curpath[drv],pname);
    }
#if 1
    compress_path(curpath[drv]);
#endif
    /* FIXME */
    DREG(0)=0;
    SET_Z();
}

void Fsfirst (char *fspec, UW attribs)
{
    int r;
    UL bp;
    DTA *dta;
    char *p, upath[500],tpath[500];
    if (!st2unixpath(fspec,upath))
    {
	DBG("Fsfirst(%s,%x) drive %d not done\n",fspec,attribs,curdrv);
	cursdrv=999;
	return;
    }
    if (fspec[1]==':') cursdrv=toupper(fspec[0])-'A';
    else cursdrv=curdrv;
    bp = LM_UL(act_pd);
    dta = (DTA *)MEM(LM_UL(MEM(bp+32)));
    SM_UL((UL *)&(dta -> magic),EVALID);
    p = strrchr (lastpath, '\\')+1;
    strcpy (dta -> dta_pat, p);
    *p=0;
#if 0
    strcpy(tpath,lastpath);
    Dsetpath(tpath);
#endif
    SM_UB(&(dta -> dta_sattrib),attribs);
#ifndef	FA_LABEL
#define	FA_LABEL 0x08
#endif
    if (attribs == FA_LABEL) {
	DREG(0) = TOS_EFILNF;
	SET_Z();
	return;
    }
    st2unixpath (lastpath, upath);
#if 1
    /* kill trailing /es */
    while (upath && *upath && *(p=upath+strlen(upath)-1) == '/')
	*p = '\0';
#endif
    if (!strchr (dta -> dta_pat, '?') &&
	!strchr (dta -> dta_pat, '*')) {
	r = xopendir (upath, 0);
    } else {
	r = xopendir (upath, 1);
	if (r < 0)
	{
	    DREG(0) = unix2toserrno(errno,TOS_EFILNF);
	    SET_Z();
	    return;
	}
    }
    SM_UW((UW *)&(dta->index),r);
    strcpy (curspath[r&(FSNEXT_JUSTSTAT-1)], upath);
    Fsnext ();
    if (DREG(0) == TOS_ENMFIL)
	DREG(0) = TOS_EFILNF;
    SET_Z();
}

int st2unixmode(UW mode)
{
    switch(mode)
    {
      case 0: return O_RDONLY;
      case 1: return O_WRONLY;	/* kludge to avoid files being created */
      case 2: return O_RDWR;
    }
    return 0;
}

void Fopen (char *fname, UW mode)
{
    int i;
    UL bp;
    int f;
    char upath[500];
    if (!st2unixpath(fname, upath))
    {
	DBG("Fopen(%s,%x) drive %d not done\n",fname,mode,curdrv);
	return;
    }
    bp = LM_UL(act_pd);
    DBG("Fopen(%s,%x) on drive %d -> %s\n",fname,mode,curdrv,upath);
    for (i=0; i<MAXFILES && file[i].bp != 0; i++)
	if (i >= MAXFILES)
	{
	    DREG(0) = TOS_ENMFIL;
	    SET_Z();
	    return;
	}
    
    f = open(upath,st2unixmode(mode));
    if (f < 0)
    {
	DBG("open(): %s\n",strerror(errno));
	DREG(0) = unix2toserrno(errno,TOS_EFILNF);
	SET_Z();
	return;
    }
    file[i].bp = bp;
    file[i].f = f;
    file[i].redirected = 0;
    DBG("\thandle is %d\n",i+H_OFFSET);
    DREG(0) = i+H_OFFSET;
    SET_Z();
}

/* Problem: if we pass a large `count', some systems give "Bad Address"
 * errors, because buf+count would be outside the process' address space (?)
 */
void Fread (W handle, UL count, char *buf)
{
#define FREAD_CHUNK 4096
    int r,i=handle-H_OFFSET,total=0,rd;
    if ( (r=get_redirection(handle)) < 0 && handle<H_OFFSET ) return;
    DBG( "Fread(%d=%d,%ld,%#lx)\n", (int)handle, (r<0)?i:r, (long)count, (long)buf );
    if (r>=0)
	i = r;
    else if (file[i].bp != LM_UL(act_pd) || file[i].f < 0)
    {
	DREG(0) = TOS_EIHNDL;
	SET_Z();
	return;
    }
    if (count > 0x1fffffff) count = 0x1fffffff;
    rd = FREAD_CHUNK;
    for(;count > 0;)
    {
	if (count < rd) rd = count;
	DBG( "\tread(%d,%#lx,%d)",i,(long)(buf+total),rd);
	r = read (file[i].f, buf+total, rd);
	DBG( "=%d\n",r);
	if (r < 0)
	{
	    DBG("read(): %s\n",strerror(errno));
	    DREG(0) = unix2toserrno(errno,-1);
	    SET_Z();
	    return;
	}
	count -= r;
	total += r;
	if (r < rd) break;
    }
    DREG(0) = (L)total;
    SET_Z();
    DBG( "\tReturns: %d\n", total );
    return;
}

void Fclose (W handle)
{
    int r,n,j,i=handle-H_OFFSET;
    DBG( "Fclose(%d)\n",handle );
    if (handle<H_OFFSET) return;
    if (file[i].bp != LM_UL(act_pd) || file[i].f < 0)
    {
	DBG( "\tfile cannot be closed!\n" );
	DREG(0) = TOS_EIHNDL;
	SET_Z();
	return;
    }
    close(file[i].f);
    file[i].bp = 0;
    file[i].f = -1;
    /* handle end of redirection (see Fforce) */
    for ( n = 0; file[i].redirected > 0 && n < 6; n++ )
	get_redirection(n); /* kills all redirections to i */
    DREG(0) = 0;
    SET_Z();
}

void Fcreate (char *fname, UW attribs)
{
    int i;
    UL bp;
    int f;
    char upath[500];
    if (!st2unixpath(fname, upath))
    {
	DBG("Fcreate(%s,%x) drive %d not done\n",fname,attribs,curdrv);
	return;
    }
    bp = LM_UL(act_pd);
    DBG("Fcreate(%s,%x) on drive %d -> %s\n",fname,attribs,curdrv,upath);
    if (attribs == FA_LABEL) {
	DBG("\tDrivelabels cannot be created!\n");
	DREG(0) = TOS_EFILNF;
	SET_Z();
	return;
    }
    for (i=0; i<MAXFILES && file[i].bp != 0; i++)
	if (i >= MAXFILES)
	{
	    DBG("\tMax. %d files can be handles!\n",(int)MAXFILES);
	    DREG(0) = TOS_ENMFIL;
	    SET_Z();
	    return;
	}
    
    f = creat(upath,DEFAULT_MODE);
    if (f < 0)
    {
	DBG("creat(): %s\n",strerror(errno));
	/* it would be nice if checking for ENOENT would work... */
	DREG(0) = unix2toserrno(errno,TOS_EFILNF);
	SET_Z();
	return;
    }
    file[i].bp = bp;
    file[i].f = f;
    file[i].redirected = 0;
    DBG("\tHandle = %d\n",i+H_OFFSET);
    DREG(0) = i+H_OFFSET;
    SET_Z();
}

void Fwrite (W handle, UL count, char *buf)
{
    int r,i=handle-H_OFFSET;
    if ((r=get_redirection(handle)) < 0 && handle<H_OFFSET) return;
    DBG("Fwrite(%d,%lu,%#lx)\n",handle,(unsigned long)count,(long)buf);
    if (r>=0)
	i = r;
    else if (file[i].bp != LM_UL(act_pd) || file[i].f < 0)
    {
	DREG(0) = TOS_EIHNDL;
	SET_Z();
	return;
    }
    r = write (file[i].f, buf, (size_t) count);
    if ( r < 0 )
	r = unix2toserrno( errno, TOS_EACCDN );
    DBG("\tReturnvalue for handle %d = %d\n",i+H_OFFSET,r);
    DREG(0) = (L)r;
    SET_Z();
    return;
}

void Fdelete (char *fname)
{
    char upath[500];
    if (!st2unixpath(fname,upath)) return;
    if (unlink(upath) < 0)
    {
	DREG(0) = unix2toserrno( errno, TOS_EACCDN );
	SET_Z();
	return;
    }
    DREG(0)=0;
    SET_Z();
    return;
}

void Dcreate (char *pname)
{
    char upath[500];
    if (!st2unixpath(pname,upath)) return;
    if (mkdir(upath,DEFAULT_DIRMODE) < 0)
    {
	DREG(0) = unix2toserrno(errno,TOS_EACCDN);
	SET_Z();
	return;
    }
    DREG(0)=0;
    SET_Z();
}

void Ddelete (char *pname)
{
    char upath[500];
    if (!st2unixpath(pname,upath)) return;
    if (rmdir(upath) < 0)
    {
	DREG(0) = unix2toserrno(errno,TOS_EACCDN);
	SET_Z();
	return;
    }
    DREG(0)=0;
    SET_Z();
}

void Dgetpath (char *buf, UW drv)
{
    /* FIXME */
    if (drv == 0)
	drv=curdrv;
    else
	--drv;
    if (!((gemdos_drives >> drv)&1))
    {
	return;
    }
    DREG(0) = 0;
    sprintf(buf,"%s",curpath[drv]);
    SET_Z();
}

void Pexec (UW mode, char *file, char *cmdlin, char *env)
{
    char x[500];
    DBG("Pexec(%x,%s,%s,%s)\n",
	mode, file?file:"NULL", cmdlin?cmdlin:"NULL", env?env:"NULL");
    if ((mode != 0 && mode != 3) || !st2unixpath(file,x))
    {	
	DBG("Not on unix filesystem\n");
    }
#if 1
    else
#endif
	sr |= MASK_CC_V;	/* Quick Pexec detection hack -> cartridge.S */
}

void Malloc (UL amount)
{
    UL bp = LM_UL(act_pd);
    DBG("Malloc(%ld) by BP %lx\n", (long)amount, (long)bp);
}

void Mfree (UL addr)
{
    UL bp = LM_UL(act_pd);
    DBG("Mfree(%lx) by BP %lx\n", (long)addr, (long)bp);
}

void Mshrink (UL addr, UL amount)
{
    UL bp = LM_UL(act_pd);
    DBG("Mshrink(%lx,%ld) by BP %lx\n", (long)addr, (long)amount, (long)bp);
}

void Fseek (L offset, W handle, UW mode)
{
    int r,i=handle-H_OFFSET;
    long dlen,dpos;
    if ((r = get_redirection(handle)) < 0 && handle<H_OFFSET) return;
    if (r>=0)
	i = r;
    else if (file[i].bp != LM_UL(act_pd) || file[i].f < 0)
    {
	DREG(0) = TOS_EIHNDL;
	SET_Z();
	return;
    }
    dpos=lseek(file[i].f,0,SEEK_CUR);
    dlen=lseek(file[i].f,0,SEEK_END);
    (void)lseek(file[i].f,dpos,SEEK_SET);
    dpos=lseek(file[i].f,offset,mode);
    if (dpos > dlen) DREG(0)=-64;	/* ERANGE */
    else DREG(0)=dpos;
    SET_Z();
    return;
}

void Frename (char *old, char *nw)
{
    char o[500],n[500];
    int r;
    if (!st2unixpath(old,o) || !st2unixpath(nw,n)) return;
    r = rename(o,n);
    if (r<0)
    {
	DBG( "rename(): %s\n",strerror(errno));
	DREG(0) = unix2toserrno(errno,TOS_EACCDN);
    }
    else
	DREG(0)=0;
    SET_Z();
}

void Fdatime (UW *t, UW *d, W handle, UW wflag)
{
    struct stat s;
    int i=handle-H_OFFSET;
    if (i<0) return;
    if (file[i].bp != LM_UL(act_pd) || file[i].f < 0)
    {
	DREG(0) = TOS_EIHNDL;
	SET_Z();
	return;
    }
    if (wflag == 0)
    {
	fstat (file[i].f, &s);
	SM_UW(t,s.st_mtime);
	SM_UW(d,s.st_mtime);
	DREG(0) = 0;
	SET_Z();
	return;
    }
    else if (wflag == 1)
    {
	DREG(0) = 0;
	SET_Z();
	return;
    }
    SET_Z();
    return;
}

void Dfree (UL *buf, UW drv)
{
#ifdef STATFS_USE_STATVFS
    struct statvfs s;
#else
    struct statfs s;
#endif
    struct stat st;
    if (drv == 0)
	drv=curdrv;
    else
	--drv;
    if (!((gemdos_drives >> drv)&1)) return;
#ifdef STATFS_USE_STATVFS
    statvfs (root[drv], &s);
#else
    statfs (root[drv], &s);
#endif
#ifdef STATFS_USE_MOUNT	/* XXX 512b blocks */
    SM_UL(buf,s.f_bavail);
    SM_UL(buf+1,s.f_blocks-(s.f_bfree-s.f_bavail));
    SM_UL(buf+2,s.f_bsize);
    SM_UL(buf+3,1);
#else
    stat (root[drv], &st);
    DBG("Dfree(%d) = %ld/%ld/%ld/%d\n",drv,s.f_bavail,s.f_blocks,
	st.st_blksize/2,2);
    SM_UL(buf,s.f_bavail);
    SM_UL(buf+1,s.f_blocks);
    SM_UL(buf+2,st.st_blksize/2);
    SM_UL(buf+3,2);
#endif
    DREG(0)=0;
    SET_Z();
}

void Fattrib (char *name, UW mode, UW attrib)
{
    char u[500];
    struct stat s;
    if (!st2unixpath(name,u)) return;
    DREG(0)=0;
    if (mode == 0)
    {
	if (STATFUNC (u, &s) < 0)
	{
	    DREG(0)=-33;
	    SET_Z();
	    return;
	}
	DREG(0) = (s.st_mode & S_IFDIR)?0x10:0; 	/* FIXME */
    }
    SET_Z();
}

void Fforce(W stdh, W nonstdh)
{
    int r,i=nonstdh-H_OFFSET;;
    
    /* BUG: Redirection of redirection cannot be handled here, 
     *      because I do not know the GEMDOS's duphandle of the
     *      standard handle.
     *      I hope, Fclose(nonstdh) will comes directly before
     *      or after Fforce(stdh,dsthd), so I handle this there. */
    if (stdh > 5 || nonstdh < H_OFFSET) return;
    if (file[i].bp != LM_UL(act_pd) || file[i].f < 0)
    {
	DREG(0)=TOS_EIHNDL;
	SET_Z();
	return;
    }
    for ( r = 0; r < MAXFILES && redirect_stdh[stdh][r] != -1; r++ );
    if (r >= MAXFILES )
    {
	fprintf(stderr,"Too much redirections at standard-handle %d\n", stdh );
	return;
    }
    redirect_stdh[stdh][r++] = i;
    file[i].redirected++;
    if ( r < MAXFILES )
	redirect_stdh[stdh][r] = -1;
    DREG(0)=0;
    SET_Z();
    return;
}

void Cauxin(void)
{
    C_in(2);
    return;
}

void Cauxis(void)
{
    C_is(2);
    return;
}

void Cauxos(void)
{
    C_os(2);
    return;
}

void Cauxout(UW c)
{
    C_out(2,c);
    return;
}

void Cprnos()
{
    C_os(3);
    return;
}

void Cprnout(UW c)
{
    C_out(3,c);
    return;
}

void Gemdos (UL as)
{
    W n;
    UW ssr;
    UL args;
    sr &= ~MASK_CC_V;
    ssr = LM_UW(MEM(as-4));
    if (BIT(13,ssr) == 0) args = AREG(8);
    else args = as+2;
    n = LM_W(MEM(args));

    #if MONITOR
        signal_monitor (GEMDOS,&n);
    #endif
    
    // DBG( "Gemdos %d\n", n);
    
    SET_N();
    switch (n)
    {
      case 3:
	  Cauxin();
	  return;
      case 18:
	  Cauxis();
	  return;
      case 19:
	  Cauxos();
	  return;
      case 4:
	  Cauxout(LM_UW(MEM(args+2)));
	  return;
      case 1:
      case 8:
      case 7:
	  Cconin();
	  return;
      case 11:
	  Cconis();
	  return;
      case 16:
	  Cconos();
	  return;
      case 2:
	  Cconout(LM_UW(MEM(args+2)));
	  return;
      case 9:
	  Cconws ((char *)MEM(LM_UL(MEM(args+2))));
	  return;
      case 6:
	  Crawio(LM_UW(MEM(args+2)));
	  return;
      case 17:
	  Cprnos();
	  return;
      case 5:
	  Cprnout(LM_UW(MEM(args+2)));
	  return;
      case 14:
	  Dsetdrv (LM_UW(MEM(args+2)));
	  return;
      case 25:
	  Dgetdrv ();
	  return;
      case 26:
	  Fsetdta (LM_UL(MEM(args+2)));
	  return;
      case 47:
	  Fgetdta ();
	  return;
      case 78:
	  Fsfirst ((char *)MEM(LM_UL(MEM(args+2))),LM_UW(MEM(args+6)));
	  return;
      case 79:
	  Fsnext ();
	  return;
      case 61:
	  Fopen ((char *)MEM(LM_UL(MEM(args+2))),LM_UW(MEM(args+6)));
	  return;
      case 63:
	  Fread (LM_UW(MEM(args+2)), LM_UL(MEM(args+4)),
		 (char *)MEM(LM_UL(MEM(args+8))));
	  return;
      case 64:
	  Fwrite (LM_UW(MEM(args+2)), LM_UL(MEM(args+4)),
		  (char *)MEM(LM_UL(MEM(args+8))));
	  return;
      case 62:
	  Fclose (LM_UW(MEM(args+2)));
	  return;
      case 60:
	  Fcreate ((char *)MEM(LM_UL(MEM(args+2))),LM_UW(MEM(args+6)));
	  return;
      case 65:
	  Fdelete ((char *)MEM(LM_UL(MEM(args+2))));
	  return;
      case 70:
	  Fforce (LM_W(MEM(args+2)),LM_W(MEM(args+4)));
	  return;
      case 57:
	  Dcreate ((char *)MEM(LM_UL(MEM(args+2))));
	  return;
      case 58:
	  Ddelete ((char *)MEM(LM_UL(MEM(args+2))));
	  return;
      case 71:
	  Dgetpath ((char *)MEM(LM_UL(MEM(args+2))), LM_UW(MEM(args+6)));
	  return;
      case 59:
	  Dsetpath ((char *)MEM(LM_UL(MEM(args+2))));
	  return;
      case 75:
	  Pexec (LM_UW(MEM(args+2)),
		 (char *)MEM(LM_UL(MEM(args+4))),
		 (char *)MEM(LM_UL(MEM(args+8))),
		 (char *)MEM(LM_UL(MEM(args+12))));
	  return;
      case 72:
	  Malloc (LM_UL(MEM(args+2)));
	  return;
      case 73:
	  Mfree (LM_UL(MEM(args+2)));
	  return;
      case 74:
	  Mshrink (LM_UL(MEM(args+4)), LM_UL(MEM(args+8)));
	  return;
      case 66:
	  Fseek (LM_L(MEM(args+2)), LM_UW(MEM(args+6)), LM_UW(MEM(args+8)));
	  return;
      case 86:
	  Frename ((char*)MEM(LM_UL(MEM(args+4))),
		   (char*)MEM(LM_UL(MEM(args+8))));
	  return;
      case 87:
	  Fdatime ((UW*)MEM(LM_UL(MEM(args+2))),
		   (UW*)MEM(LM_UL(MEM(args+2))+2),
		   LM_UW(MEM(args+6)),LM_UW(MEM(args+8)));
	  return;
      case 54:
	  Dfree ((UL *)MEM(LM_UL(MEM(args+2))), LM_UW(MEM(args+6)));
	  return;
      case 67:
	  Fattrib ((char *)MEM(LM_UL(MEM(args+2))), LM_UW(MEM(args+6)),
		   LM_UW(MEM(args+8)));
	  return;
      default:
	  break;
    }
}

static int gemdos_initialized=0;

void init_gemdos (void)
{
    UL o;
    int i;
    if (gemdos_initialized) return;
    gemdos_initialized=1;
    if ( verbose )
	fprintf(stderr,"Setting up native GEMDOS routines (pc=%x): ",pc);
    for (i=0; i<MAXFILES; i++) 
    {
	file[i].bp = 0;
	file[i].f = -1;
	file[i].redirected = 0;
    }
    for (i=0; i<6; i++ )
	redirect_stdh[i][0] = -1;

    /*
     * Revector the GEMDOS Trap to Cartridge
     * set up return address in old_gemdos variable
     * in cartridge (hope it doesn't move!)
     */

    SM_L(MEM(CART_OLD_GEMDOS), LM_L(MEM(0x84)));
    SM_L(MEM(0x84), CART_NEW_GEMDOS);
    o = LM_UL(MEM(8+LM_UL(MEM(0x4f2))));
    if (LM_UW(MEM(o+2)) < 0x102)
    {
	if ((LM_UW(MEM(o+0x1c)) >> 1) == 4) 
	    act_pd = MEM(0x873c);
	else 
	    act_pd = MEM(0x602c);
    }
    else 
	act_pd = MEM(LM_UL(MEM(o+0x28)));
    disk_init();
    if ( verbose )
	fprintf (stderr, 
		 "nflops=%d; boot_dev=%d cmdload=%d\n",
		 LM_W(ADDR(0x4a6)),boot_dev, LM_W(ADDR(0x482)));
}
