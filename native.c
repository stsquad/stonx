/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
 
#include "defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "debug.h"
#include "main.h"
#include "tosdefs.h"
#include "toserror.h"
#include "gemdos.h"
#include "native.h"
#include "xlib_vdi.h"
#include "screen.h"
#include "utils.h"
#include "cartridge/cartridge.h"
#include "mint/mint_interface.h"

#ifndef MINT_STONXFS
# define MINT_STONXFS 1
#endif

#if STONXFS4MINT
#include "STonXfs4MiNT.h"
#endif

#define SERIAL_DEV "/dev/ttyS1"

#define FUNC_OPCODE 0xa0ff
#define BPB_OFF CART_FAKE_BPB

#define	FLG_RDONLY 	1
#define FLG_SPECIAL	2

int drive_bits;
int drive_fd[MAXDRIVES];
static int drive_flags[MAXDRIVES];
static int drive_sectsize[MAXDRIVES];

#define GRESULT T(("= %ld\n", DREG(0)))

static UL old_rw, old_bpb, old_init, old_boot, old_mediach;

typedef struct _FSE
{
    struct _FSE *next;       /* pointer to next fs_entry */
    int          dev;        /* TOS BIOS device number */
    char         letter;     /* TOS drive letter */
    char         path[1024]; /* UNIX directory */
} FS_ENTRY;

static FS_ENTRY *fs_entries = NULL;

/* Add a drive to the system; this must happen before TOS is booted */
int add_drive(char drive, char *fname)
{
	int fd;
	int r;
	struct stat st;
	FS_ENTRY *new;
	UB sectsiz[2];

	int d = toupper(drive & 0xff) - 'A';

	if (d < 0 || d >= MAXDRIVES)
	{
		(void) fprintf(stderr, "invalid drive specification `%c'\n", drive);
		return FALSE;
	}
	if (drive_bits & (1 << d))
	{
		(void) fprintf(stderr, "drive `%c' already specified\n", drive);
		return FALSE;
	}
	fd = open(fname, O_RDWR);

	if (fd < 0)
	{
		if (errno == EROFS)
		{
			fd = open(fname, O_RDONLY);
		} else if (errno == ENXIO)
		{
			/* maybe floppy without disk inserted */
			fprintf(stderr, "can't open file %s (%s), ignored\n", fname, strerror(errno));
			return TRUE;
		}
		if (fd > 0)
		{
			fprintf(stderr, "Warning: file %s is read-only\n", fname);
			drive_flags[d] |= FLG_RDONLY;
		} else
		{
			error("can't open file %s (%s)\n", fname, strerror(errno));
		}
	}
	r = fstat(fd, &st);
	if (r < 0)
	{
		fprintf(stderr, "Warning: can't stat file %s: %s\n", fname, strerror(errno));
	} else
	{
		if (!S_ISREG(st.st_mode))
		{
			fprintf(stderr, "Note: %s is a special file - it may or may not work as desired...\n", fname);
			drive_flags[d] |= FLG_SPECIAL;
#ifndef __linux__
			/* linux has no raw devices, so don't warn about that */
			if (S_ISBLK(st.st_mode))
			{
				fprintf(stderr, "Note: if you want to emulate a `real' floppy disk drive, you should use the\ncorresponding raw device instead!\n");
			}
#endif
		} else
		{
			if (st.st_size == 0)
			{
				fprintf(stderr, "Warning: file %s has size 0!\n", fname);
			}
		}
	}
	if ( lseek(fd,0xb,SEEK_SET) < 0 || read(fd,sectsiz,2) != 2 ) 
	{
	  fprintf(stderr, "Warning: Cannot read sectorsize from %s, using 512 bytes\n", fname );
	  drive_sectsize[d] = 512;
	} else
	{
	  drive_sectsize[d] = sectsiz[0] + 256 * sectsiz[1];
	}
	lseek(fd,0,SEEK_SET);
	drive_bits |= 1 << d;
	drive_fd[d] = fd;

	/* append to fs_entry list    */
	fprintf(stderr, "-------> add_disk: %d, %c, %s", d, drive, fname);
	new = (FS_ENTRY *)malloc(sizeof(FS_ENTRY));
	if (new != NULL)
	{
	    FS_ENTRY    *r;
        
	    new->dev = d;
	    new->letter = drive;
	    strncpy(new->path, fname, 256);
	    printf("-------> add_disk: %d, %c, %s\n",
		   new->dev, new->letter, new->path);
	    new->next=NULL;
	    r = fs_entries;
	    if (r == NULL)
		fs_entries = new;
	    else
	    {
		while (r->next != NULL)
		    r = r->next;
		r->next = new;
	    }
	}
	return 1;
}


int disk_rw(char *as)
{
	W rwflag;
	UL buf;
	unsigned int count, dev;
	unsigned long recno;
	
	rwflag = LM_W(as);
	buf = LM_UL((UL *)(as + 2));
	count = LM_UW(as + 6);
	recno = LM_UW(as + 8);
	dev = LM_UW(as + 10);
	if (recno == 0xffff)
		recno = LM_UL((UL *)(as+12));
#if 0
	fprintf(stderr,"Rwabs(%d,$%lx,Cnt:%d,Rec:%ld,%d), %x, %d:\n ", rwflag, (long)buf, count, recno, dev, drive_bits, boot_dev);
#endif
	if (dev < 2 || dev >= MAXDRIVES || !(drive_bits & (1 << dev)) || drive_fd[dev] < 0)
	{
		pc = old_rw;
		return FALSE;
	}
	if ((drive_flags[dev] & FLG_RDONLY) && (rwflag & 1))
	{
		DREG(0) = -13;
		GRESULT;
		return TRUE;
	}
	rwflag &= 1;						/* media change not yet implemented... */
	if (lseek(drive_fd[dev], recno * drive_sectsize[dev], SEEK_SET) < 0)
	{
		fprintf(stderr, "lseek() on drive %d failed: %s\n", dev, strerror(errno));
		DREG(0) = -6;					/* E_SEEK is probably inappropriate */
		GRESULT;
		return TRUE;
	}
	if (rwflag == 0)
	{
		if (read(drive_fd[dev], MEM(buf), count * drive_sectsize[dev]) < 0)
		{
			fprintf(stderr, "read() on drive %d failed (not 9/18 sectors/track? %s\n)", dev, strerror(errno));
			DREG(0) = -11;
			GRESULT;
			return TRUE;
		}
	} else
	{
		/* we should not lseek() if the following is true... */
		if (drive_flags[dev] & FLG_RDONLY)
		{
			DREG(0) = -13;
			return TRUE;
		}
		if (write(drive_fd[dev], MEM(buf), count * drive_sectsize[dev]) < 0)
		{
			fprintf(stderr, "write() on drive %d failed (not 9/18 sectors/track?) %s", dev, strerror(errno));
			DREG(0) = -10;
			GRESULT;
			return TRUE;
		}
	}
	DREG(0) = 0;
	GRESULT;
	return TRUE;
}

/* Is it illegal to return a BPB address which is read-only???
 */
int disk_bpb(int dev)
{
	UB bootsec[512], *d;
	UB *x = (UB *)MEM(BPB_OFF);
	UW *wx = (UW *) x;
	off_t old;

	T(("Getting BPB for drive %d: ", dev));

	if (dev < 2 || dev >= MAXDRIVES || !(drive_bits & (1 << dev)) || drive_fd[dev] < 0)
	{
		pc = old_bpb;
		return FALSE;
	}
	if ( (old = lseek(drive_fd[dev], 0, SEEK_CUR)) < 0 ||
	     lseek(drive_fd[dev], 0, SEEK_SET) < 0 ||
	     read(drive_fd[dev], bootsec, 512) != 512 ||
	     lseek(drive_fd[dev], old, SEEK_SET) < 0 )
	{
	  fprintf(stderr,"Warning: Cannot create BPB of drive %d\n",dev);
	  DREG(0)=0; /* error-return */
	  return TRUE;
	}
	d = bootsec;
	x[0] = d[0xc];
	x[1] = d[0xb];
	if ( drive_sectsize[dev] != d[0xb] + 256 * d[0xc] )
	{
	  fprintf(stderr,"Changing sectorsize of drive %d from %d to %d\n",dev,drive_sectsize[dev],d[0xb] + 256 * d[0xc]);
	  drive_sectsize[dev] = d[0xb] + 256 * d[0xc];
	}
	x[2] = 0;
	x[3] = d[0xd];
	SM_W(&wx[2], LM_W(&wx[1]) * LM_W(&wx[0]));
	SM_W(&wx[3], (d[0x11] + d[0x12] * 256) / 16);
	x[8] = d[0x17];
	x[9] = d[0x16];
	SM_W(&wx[5], LM_W(&wx[4]) + 1);
	SM_W(&wx[6], LM_W(&wx[5]) + LM_W(&wx[4]) + LM_W(&wx[3]));
	SM_W(&wx[7], ((d[0x13] + d[0x14] * 256) - LM_W(&wx[6])) / LM_W(&wx[1]));
	SM_W(&wx[8], (LM_W(&wx[7]) > 0xfef));	/* WRONG!? */
#define LM_IW(x) (((x)[1] << 8) | (x)[1])
	T(("secsize=%d, sectors/cluster=%d, bytes/cluster=%d, root dir len=%d, fatsize=%d, fat2=%d, data=%d, clusters=%d, bflags=%x ",
		LM_W(wx), LM_W(wx+1), LM_W(wx+2), LM_W(wx+3), LM_W(wx+4), LM_W(wx+5), LM_W(wx+6), LM_W(wx+7), LM_W(wx+8)));
	DREG(0) = BPB_OFF;
	T(("= $%lx\n", DREG(0)));
	return TRUE;
}

static void Initialize(void)
{
	old_rw = LM_UL(ADDR(0x476));
	old_bpb = LM_UL(ADDR(0x472));
	old_init = LM_UL(ADDR(0x46a));
	old_boot = LM_UL(ADDR(0x47a));
	old_mediach = LM_UL(ADDR(0x47e));
}

void disk_init(void)
{
	int b = boot_dev;
#if 1
	while (b < 32 && !(drive_bits & (1l << b)))
		b++;
	if (b >= 32)
	{
		b = 0;
		while (b < 32 && !(drive_bits & (1l << b)))
			b++;
	}
	boot_dev = b;
	SM_W(ADDR(0x446), boot_dev);
	SM_W(ADDR(0x4a6), (((drive_bits & 1)!=0))+((drive_bits & 2)!=0));
#if 0
	if ((drive_bits & 1))
		drive_bits |= 2;
#endif
	SM_L(ADDR(0x4c2), drive_bits);
	/* XXX set in init_mem() too??? */
	if (verbose) fprintf(stderr,"Will boot from drive %c\n", boot_dev+'A');
#endif
#if CLIPBOARD
	clip_init(boot_dev); /* FIXME: I don't think, this sould be here! (MJK) */
#endif
}

/* -------------- Serial Port functions ------------- */
/* A0 is the argument stack here... */
static void Bconin_AUX(UL as)
{
#if MODEM1
	int td;
	char c;

	fprintf(stderr, "Bconin_AUX\n");
	td = open(SERIAL_DEV, O_RDONLY);
	if (td < 0)
	{
		perror("open " SERIAL_DEV);
		DREG(0) = -1;
		return;
	}
	if (read(td, &c, 1) < 0)
	{
		perror("read " SERIAL_DEV);
		DREG(0) = -1;
		return;
	}
	close(td);
#else
	fprintf(stderr,"Bconin_AUX not implemented\n");
#endif
}

static void Bconout_AUX(UL as)
{
#if MODEM1
	int td;
	char c = LM_W(MEM(AREG(0) + 4));

	fprintf(stderr, "Bconout_AUX (%c = %d)\n", c, (int) c & 0xff);
	td = open(SERIAL_DEV, O_WRONLY);
	if (td < 0)
	{
		perror("open " SERIAL_DEV);
		DREG(0) = -1;
		return;
	}
	if (write(td, &c, 1) < 0)
	{
		perror("write " SERIAL_DEV);
		DREG(0) = -1;
		return;
	}
	close(td);
#else
	fprintf(stderr,"Bconout_AUX not implemented\n");
#endif
}

static void Bconstat_AUX(UL as)
{
	fprintf(stderr, "Bconstat_AUX not implemented\n");
}

static void Bcostat_AUX(UL as)
{
	fprintf(stderr, "Bcostat_AUX not implemented\n");
}

static void Rsconf(UL as)
{
	fprintf(stderr, "Rsconf\n");
}

void gemdos_post(void)
{
}


#if 0
/* *** Print string from emulated side *** */
/* This is normally not needed at all, but sometimes usefull if you */
/* want to debug a ST program... */
void write_native(char * addr)
{
    char buf[1024];
    int n;

    buf[0]=0;   /* to be save */
    buf[80]=0;   /* to be save */

    for(n=0;n<1024;n++) {       /* Fill string char by char */
        if ((buf[n]=LM_UB(MEM(addr++))) == 0) break;
    }
    fprintf(stderr,"%s", buf );
}
#endif


void call_native(UL as, UL func)
{
#if 0
	fprintf(stderr,"Calling native %d\n",func);
#endif
	switch (func)
	{
	case 0:
		/* write_native((char *)LM_UL(MEM(as))); */
		break;
	case 1:
		disk_rw((char *)MEM(as));
		break;
	case 2:
		disk_bpb(LM_W(MEM(as)));
		break;
	case 3:
		disk_init();
		break;
	case 4:
		/* was move_native */
		break;
	case 5:
		Gemdos(as);
		break;
	case 6:
		vdi_post();
		break;
	case 7:
		/* was linea_post */
		break;
	case 8:
		/* was Vdi */
		break;
	case 9:
		Initialize();
		break;
	case 10:
		Init_Linea();
		break;
	case 11:
		gemdos_post();
		break;
        case 0x00008000:
	        DREG(0) = unix2toserrno(stonx_shutdown((int)LM_W(MEM(as))),0);
		break;
	default:
#if MINT_STONXFS || MINT_COM || MINT_SERIAL
	        if ( ( func & 0xffff0000 ) == 0x00010000 )
		  { /* STonX-MiNT subfunction */
		    mint_call_native( as, func & 0xffff );
		  }
		else
#endif
		  {
#if DEBUG
		    error("native function #%d out of range (pc=%lx)\n", func, pc);
#endif
		  }
		break;
	}
}
