/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#include "defs.h"
#include "cpu.h"
#include "debug.h"
#include "tosdefs.h"
#include "audio.h"
#include "mem.h"
#include "main.h"
#include "screen.h"
#include "ikbd.h"
#include "midi.h"
#include "io.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>

#define DEBUG_MIDI 0
#define FDC_VERBOSE 0

static FILE *Parallel;
static int para_open = 0;
char *parallel_dev=NULL;

#if MODEM1

#define SERIAL_BUFSIZE 	1024
#define SERIAL_LO_WATER    1

char *serial_dev=NULL;
static int td = -1;
static B serial_buffer[SERIAL_BUFSIZE];
static int bp, bs;
static B tmpbuf[SERIAL_BUFSIZE];

#endif /* MODEM1 */

#define INTERRUPTS 1

/* main.c */
extern long tv_usec;
extern int hz200pt, vblpt, audio;
static struct timeval itv;

extern void show_screen (void);

static struct itimerval itval;
static struct itimerval itold;
static void (*oldsig)(int);
static int vblcount, systcount;

/*--------- Information copied from hardware regs at regular intervals: ------*/
volatile UL vbase;
UL old_vbase=0;
/*------------ Information directly accessed though hw regs  -----------------*/
/*                   (watch out for unaligned accesses!!!)                    */
UW *color = (UW *)MEM(M_COLORS);
/*----------------------------------------------------------------------------*/

/* This is where all hardware emulation takes place
 */
void hardware_jobs (int dummy)
{
#if JOYSTICK
        joystick_update();
#endif
	if (audio) audio_update();
#if MODEM1
	if (serial_dev != NULL && serial_stat())
	{
		SM_UB(MEM(0xfffa2b),LM_UB(MEM(0xfffa2b)) | 0x80);
		flags |= F_RCV_FULL;
	}
#endif
	/* VBL */
	if (--vblcount == 0)
	{
#if INTERRUPTS
		flags |= F_VBL;
#endif
		vblcount = vblpt;
		/* maybe later? -> don't call shifter in this case? Nah... */

		/* update some variables according to hw regs */
		if (old_vbase != vbase)
		{
#if 0
			fprintf(stderr,"New screen address is %lx\n", vbase);
#endif
			old_vbase = vbase;
		}
		/* call the shifter emulation */
		/* moved to cpu.m4c */
	}
#if INTERRUPTS
	if (--systcount == 0)
	{
		systcount = hz200pt;
		if (LM_UB(MEM(0xfffa09)) & 0x20) flags |= F_200Hz;
	}
#endif
}

void crash (int sig)
{
    fprintf (stderr, "Got a fatal signal %d [pc=%08lx]\n", sig, (long)pc);

    if ( machine.process_signal )
	machine.process_signal(sig);
}

void init_hardware (void)
{
	sigset_t set;
	struct sigaction a;
	sigemptyset(&set);
	a.sa_handler = hardware_jobs;
	a.sa_mask = set;
	a.sa_flags = 0;
	sigaction (realtime ? SIGALRM : SIGVTALRM, &a, NULL);
	itv.tv_sec = 0L;
	itv.tv_usec = tv_usec;
	itval.it_interval = itval.it_value = itv;
	setitimer (realtime ? ITIMER_REAL : ITIMER_VIRTUAL, &itval, &itold);
	if (verbose)
	{
		fprintf(stderr,"Starting %s timer.\n",
			realtime ? "real-time" : "virtual");
	}
	vblcount = vblpt;
	systcount = hz200pt;
	signal (SIGBUS, crash);
	signal (SIGSEGV, crash);
}

void kill_hardware (void)
{
	if (realtime)
	{
		setitimer (ITIMER_REAL, &itold, NULL);
		signal (SIGALRM, oldsig);
	}
	else
	{
		setitimer (ITIMER_VIRTUAL, &itold, NULL);
		signal (SIGVTALRM, oldsig);
	}
}

#if MODEM1
void init_serial (void)
{
	td = open (serial_dev, O_RDWR|O_NDELAY);
	if (td < 0)
	{
		char s[100];
		sprintf (s, "open %s", serial_dev);
		perror (s);
		exit(1);
	}
	bp=0, bs=0;
}

B read_serial(void)
{	
	int l, i;
	B c;
	if (td >= 0 && (l = SERIAL_BUFSIZE-bs) > SERIAL_BUFSIZE-SERIAL_LO_WATER)
	{
		if ((l = read (td, tmpbuf, l)) < 0)
		{
			char s[100];
			sprintf (s, "read %s", serial_dev);
			perror (s);
			DREG(0) = -1;
			return 0;
		}
		for (i=0; i<l; i++, bp = (bp+1)&(SERIAL_BUFSIZE-1))
			serial_buffer[bp] = tmpbuf[i];
		bs += l;
	}
	if (bs > 0)
	{	
		c = serial_buffer[(bp-bs)&(SERIAL_BUFSIZE-1)];
		bs--; 
		return c;
	}
	else
	{
		fprintf (stderr, "Serial buffer UNDERRUN!\n");
		return -1;
	}
}

void write_serial (B v)
{
	if (td >= 0 && write (td, &v, 1) < 0)
	{
		char s[100];
		sprintf (s, "write %s", serial_dev);
		perror (s);
		DREG(0) = -1;
		return;
	}
}

int serial_stat (void)
{
	fd_set x;
	int d;
	struct timeval t;
	if (td < 0) return 0;
	if (bs > 0) return 1;
	t.tv_sec=0; t.tv_usec=0;
	FD_ZERO(&x);
	FD_SET(td, &x);
	d = select (td+1, &x, NULL, NULL,&t);
	if (d < 0)
	{
		char s[100];
		sprintf (s, "select failed for %s", serial_dev);
		perror(s);
	}
#if 0
	fprintf (stderr, "[%d]",d);
#endif
	return d==1;
}
#endif /* MODEM1 */

/*
 *	Parallel Port Related stuff.	
 *	Martin GRIFFiths, 1995.
 *
 *	Last Updated : November 5th 1995. 
 */

int init_parallel(void)
{	if ((Parallel=fopen(parallel_dev,"w")) == NULL)	
		return 0;
	para_open = 1;
	return 1;
}

void write_parallel(unsigned char c)
{	if (para_open)	
	{	fwrite(&c,1,1,Parallel);
		fflush(Parallel);
	}
}

unsigned char read_parallel(unsigned char c)
{	if (para_open)
	{	unsigned char c;
		fread(&c,1,1,Parallel);
	}
	return 0;
}

void done_parallel(void)
{	if (para_open)	
	{	fclose(Parallel);
	
	}
}

int parallel_stat(void)
{
	return Parallel != NULL;
}

/* MFP stuff */

#define IGNORE_S(_addr) void STORE_B_ ## _addr (B v) {}
#define UNDEF_L(_addr) B LOAD_B_ ## _addr (void) { return 0xff; }
#define ZERO(_addr) B LOAD_B_ ## _addr (void) { return 0; }

volatile UW mfp_IERA, mfp_TADR=256, mfp_TACR, mfp_TADR_reload=256,
	mfp_IMRA, mfp_TCDCR;
static double timer_a_base;
static struct timeval last_time;
extern int shiftmod;

static int have_switched=0;

B LOAD_B_fffa01(void)
{
#if 0
	if (gr_mode == MODE_MONO)
#else
	/* allow switching between mono/color */
	if ((!have_switched && gr_mode == MODE_MONO) || shiftmod == 2)
#endif
	{
		have_switched = 1;
		return LM_B(MEM(0xfffa01)) & 0x7f;
	}
	return  LM_B(MEM(0xfffa01)) | 0x80;
}

B LOAD_B_fffa03(void)
{
	return /* TODO */ 1;
}

B LOAD_B_fffa05(void)
{
	return /* TODO */ 1;
}

B LOAD_B_fffa07(void)
{
	return mfp_IERA;
}

B LOAD_B_fffa09(void)
{
	return LM_B(MEM(0xfffa09));
}

B LOAD_B_fffa0b(void)
{
#if STE
	return 0x20;
#else
	static B ipra;
	ipra ^= 0xff;
	return ipra;
#endif
}

B LOAD_B_fffa0d(void)
{
	return /* TODO */ 1;
}

B LOAD_B_fffa0f(void)
{
	return /* TODO */ 1;
}

B LOAD_B_fffa11(void)
{
#if 0
	return LM_B(MEM(0xfffa01));
#else
	return LM_B(MEM(0xfffa11));
#endif
}

B LOAD_B_fffa13(void)
{
	return mfp_IMRA;
}

B LOAD_B_fffa15(void)
{
	return /* TODO */ 1;
}

B LOAD_B_fffa17(void)
{
	return /* TODO */ 1;
}

B LOAD_B_fffa19(void)
{
	return mfp_TACR;
}

B LOAD_B_fffa1b(void)
{
	return /* TODO */ 1;
}

B LOAD_B_fffa1d(void)
{
	return /* TODO */ 1;
}

B LOAD_B_fffa1f(void)
{
#if TIMER_A
	if (timer_a) return mfp_TADR;
	else
#endif
		mfp_TADR--;
	return mfp_TADR;
}

B LOAD_B_fffa21(void)
{ 
	return /* TODO */ 1;
}

B LOAD_B_fffa23(void)
{
	return rand() % 0xff;
}

B LOAD_B_fffa25(void)
{
	return /* TODO */ LM_B(MEM(0xfffa25));
}

B LOAD_B_fffa27(void)
{
	return /* TODO */ 1;
}

B LOAD_B_fffa29(void)
{
	return /* TODO */ 1;
}

B LOAD_B_fffa2b(void)
{
	return LM_B(MEM(0xfffa2b));
}

B LOAD_B_fffa2d(void)
{
	return 0x80|1;
}

B LOAD_B_fffa2f(void)
{
    B x;
#if MODEM1
    x = read_serial();
#if 0
    fprintf (stderr, "reading UDR -> %d\n", x&0xff);
#endif
    if (!serial_stat())
    {
	SM_UB(MEM(0xfffa2b),LM_UB(MEM(0xfffa2b)) & 0x7f);
    }
    else 
	flags |= F_RCV_FULL;
#else /* MODEM1 */
    x = 0;
#endif
    return x;
}

UNDEF_L(fffa31)
IGNORE_S(fffa31)

void STORE_B_fffa01(B v)
{
	/* TODO */
}

void STORE_B_fffa03(B v)
{
	/* TODO */
}

void STORE_B_fffa05(B v)
{
	/* TODO */
}

struct { int mode; int div; } mode_A_tab[]=
{{0,1},
 {2,4},
 {2,10},
 {2,16},
 {2,50},
 {2,64},
 {2,100},
 {2,200},
 {1,1},
 {3,4},
 {3,10},
 {3,16},
 {3,50},
 {3,64},
 {3,100},
 {3,200},
};

char *mode_strings[] = {"stopped","event","delay","pulse"};

void check_timer_a(void)
{
	float hz=2457600.0 / mode_A_tab[mfp_TACR&15].div;
#if 0
	fprintf(stderr,"Timer A Mode: %s, %f (%f) Hz\n",
		mode_strings[mode_A_tab[mfp_TACR&15].mode], hz, hz/mfp_TADR);
#endif
	if (mode_A_tab[mfp_TACR&15].mode == 2)
	{
		flags |= F_TIMERA_ON;
		timer_a_base = 1000000.0/hz;
		gettimeofday(&last_time,NULL);
	}
}

void STORE_B_fffa07(B v)
{
#if TIMER_A
	UB x=v^mfp_IERA;
	if (!timer_a) return;
	/* TODO */
	if (x & v & 0x20)
	{
		check_timer_a();
	}
	else if ((x & 0x20) && !(v & 0x20))
	{
#if 0
		fprintf(stderr,"Timer A switched off\n");
#endif
		flags &= ~F_TIMERA_ON;
	}
	mfp_IERA=v;
#endif
}

void STORE_B_fffa09(B v)
{
	SM_B(MEM(0xfffa09),v);
}

void STORE_B_fffa0b(B v)
{
	/* TODO */
}

void STORE_B_fffa0d(B v)
{
	/* TODO */
}

void STORE_B_fffa0f(B v)
{
	/* TODO */
}

void STORE_B_fffa11(B v)
{
	B x = LM_B(MEM(0xfffa11));
	x &= v;
	SM_B(MEM(0xfffa11),x);
}

void STORE_B_fffa13(B v)
{
	mfp_IMRA = v&0xff;
}

void STORE_B_fffa15(B v)
{
	/* TODO */
}

void STORE_B_fffa17(B v)
{
	/* TODO */
}

void STORE_B_fffa19(B v)
{
#if TIMER_A
	float hz;
	if (!timer_a) return;
#if 0
	fprintf(stderr,"TACR -> %x\n",v&0xff);
#endif
	mfp_TACR = v;
	check_timer_a();
#endif
}

void STORE_B_fffa1b(B v)
{
#if 0
	fprintf(stderr,"TBCR -> %x\n",v&0xff);
#endif
	/* TODO */
}

void STORE_B_fffa1d(B v)
{
#if 0
	fprintf(stderr,"TCD_CR -> %x\n",v&0xff);
#endif
}

void STORE_B_fffa1f(B v)
{
#if 0
	fprintf(stderr,"TADR -> %x\n",v&0xff);
#endif
#if TIMER_A
	if (!timer_a) return;
	mfp_TADR_reload = (v == 0? 256:v);
	if ((flags & F_TIMERA_ON) == 0)
		mfp_TADR = mfp_TADR_reload;
#endif
}

void STORE_B_fffa21(B v)
{
#if 0
	fprintf(stderr,"TBDR -> %x\n",v&0xff);
#endif
	/* TODO */
}

void STORE_B_fffa23(B v)
{
#if 0
	fprintf(stderr,"TCDR -> %x\n",v&0xff);
#endif
	SM_B(MEM(0xfffa23),v);
	/* TODO */
}

void STORE_B_fffa25(B v)
{
#if 0
	fprintf(stderr,"TDDR -> %x\n",v&0xff);
#endif
	SM_B(MEM(0xfffa25),v);
	/* TODO */
}

void STORE_B_fffa27(B v)
{
	/* TODO */
}

void STORE_B_fffa29(B v)
{
	/* TODO */
}

void STORE_B_fffa2b(B v)
{
	/* TODO */
}

void STORE_B_fffa2d(B v)
{
	/* TODO */
}

void STORE_B_fffa2f(B v)
{
#if MODEM1
    char c = v;
#if 0
    fprintf (stderr, "Writing UDR: %d\n", v&0xff);
#endif
    write_serial(v);
#endif /* MODEM1 */
}


#if TIMER_A
long mfp_get_count (void)
{
	long d;
	double decr;
	long r=0;
	struct timeval t;
	gettimeofday(&t,NULL);
	d = 1000000 * (t.tv_sec-last_time.tv_sec) + t.tv_usec - last_time.tv_usec;
#if 0
	fprintf(stderr,"Delay: %ld\n",d);
#endif
	last_time = t;
	if (d > timer_a_base)
	{
		decr = d/timer_a_base;
		if (decr > mfp_TADR)
		{
			r = 1;
			decr -= mfp_TADR;
			r += (long)(decr / mfp_TADR_reload);
			mfp_TADR = (long)decr % mfp_TADR_reload;
		}
		else mfp_TADR -=(UW) decr;
#if 0
	fprintf(stderr,"diff/base = %ld/%ld, decr = %d, %d interrupts\n",
			d,timer_a_base, decr, r);
#endif
	}
	return (mfp_IMRA & 0x20) ? r : 0;
}
#endif /* TIMER_A */


extern int dma_mode, dma_scr, dma_car, fdc_command, fdc_track, fdc_sector,
	fdc_data, fdc_int, fdc_status, dma_sr;
extern void fdc_exec_command(void);

#define BOOTSECTOR 0

#define IGNORE_S(_addr) void STORE_B_ ## _addr (B v) {}
#define UNDEF_L(_addr) B LOAD_B_ ## _addr (void) { return 0xff; }
#define ZERO(_addr) B LOAD_B_ ## _addr (void) { return 0; }
#define NIB(_x) (((_x) & 0x0f)|0xf0)
#if BOOTSECTOR
/* custom bootsector with some 'disk' driver routines */
#include "bootsector.c"
#endif

/* This file contains the definitions of all the 'special' LOAD_* and STORE_*
 * functions for I/O-registers. The prototypes are contained in "iotab[12].c"
 * as generated from "iotab[12].tab" by "mktabl.pl".
 * 
 * For multi-byte accesses, the order will be lowest->highest byte, so e.g.
 * when writing to MWDATA (not implemented), STORE_B_ff8923 should trigger
 * the desired action.
 */

/* DMA registers */
static UW DMAfifo=0;	/* write to $8606.w */
static UW DMAstatus=0;	/* read from $8606.w */
static UW DMAxor=0;
static UB DMAdiskctl=0;
static UB FDC_T=0, HDC_T=0;     /* Track register */
static UB FDC_S=0, HDC_S=0;     /* Sector register */
static UB FDC_D=0, HDC_D=0;     /* Data register */

extern volatile UL vbase;
int shiftmod;

/* generate a random Video Adress Counter value */
static UL gen_fake_vcount (void)
{
	return vbase + (rand() % 32000);
}

#define TG if (bank==1) return 0xff; time_gen();
/* save the time and date for later use */
static struct tm *tm;
static void time_gen (void)
{
	struct timeval tv;
	time_t tmp;
	gettimeofday (&tv, NULL);
	tmp=tv.tv_sec;
	tm = localtime (&tmp);
}

static void FDC (UB command)
{
	if ((command & -4) == 0)	/* RESTORE */
	{
		FDC_T = FDC_S = FDC_D = 0;
	}
}

static void HDC (UB command)
{
}

/* -------------------- ROUTINES FOR iotab1.tab -------------------- */

ZERO(ff8204)
ZERO(ff8206)
ZERO(ff8208)


B LOAD_B_ff8205(void)
{
	return (gen_fake_vcount() >> 16) & 0xff;
}

B LOAD_B_ff8207(void)
{
	return (gen_fake_vcount() >> 8) & 0xff;
}

B LOAD_B_ff8209(void)
{
	return gen_fake_vcount() & 0xff;
}

B LOAD_B_ff8604(void)
{
	if (dma_mode & 0x10)
	{
		return dma_scr>>8;
	}
	else
	{
		if (dma_mode & 8)
		{
			return dma_car>>8;
		}
		else
		{
			switch(dma_mode & 6)
			{
				case 0:
					if (!fdc_int)
					{
						SM_UB(MEM(0xfffa01),LM_UB(MEM(0xfffa01))|0x20);
					}
					return fdc_status>>8;
				case 2:
					return fdc_track>>8;
				case 4:
					return fdc_sector>>8;
				case 6:
					return fdc_data>>8;
				default:
					return 0;
			}
		}
	}
}

B LOAD_B_ff8605(void)
{
	if (dma_mode & 0x10)
	{
		return dma_scr&0xff;
	}
	else
	{
		if (dma_mode & 8)
		{
			return dma_car&0xff;
		}
		else
		{
			switch(dma_mode & 6)
			{
				case 0:
					if (!fdc_int)
					{
						SM_UB(MEM(0xfffa01),LM_UB(MEM(0xfffa01))|0x20);
					}
					return fdc_status&0xff;
				case 2:
					return fdc_track&0xff;
				case 4:
					return fdc_sector&0xff;
				case 6:
					return fdc_data&0xff;
				default:
					return 0;
			}
		}
	}
#if 0
	return 0x04;
#endif
}

B LOAD_B_ff8606(void)
{
	return dma_sr>>8;
}

B LOAD_B_ff8607(void)
{
	return dma_sr&0xff;
#if 0
	return /* TODO */ 0x04;
#endif
}

#ifdef NO_AUDIO
B LOAD_B_ff8800(void)
{
	return /* TODO */ 0;
}

B LOAD_B_ff8802(void)
{
	return /* TODO */ 0;
}

UNDEF_L(ff8801)
UNDEF_L(ff8803)
#endif

UNDEF_L(ff8200)
IGNORE_S(ff8200)
IGNORE_S(ff8261)
IGNORE_S(ff8262)
IGNORE_S(ff8263)

void STORE_B_ff8260(B v)
{
	shiftmod = v;
	SM_B(MEM(0xff8260),v);
#if 0
	fprintf(stderr,"Shiftmode set (pc=%lx)\n",(long)pc);
#endif
	machine.screen_shifter();
}

void set_vbase (UL n)
{
	SM_B(MEM(0xff8201),(n>>16)&0xff);
	SM_B(MEM(0xff8203),(n>>8)&0xff);
#if STE
	SM_B(MEM(0xff820d),n&0xff);
#endif
	vbase = n;
}

void STORE_B_ff8201(B v)
{
	SM_B(MEM(0xff8201),v);
	vbase &= 0xff00ffff;
	vbase |= (v & 0xff)<<16;
#if 0
	fprintf(stderr,"vbase modified from %lx (->%lx)\n",pc,vbase);
#endif
}

UNDEF_L(ff8202)
IGNORE_S(ff8202)

void STORE_B_ff8203(B v)
{
	SM_B(MEM(0xff8203),v);
	vbase &= 0xffff00ff;
	vbase |= (v & 0xff)<<8;
#if 0
	fprintf(stderr,"vbase modified from %lx (->%lx)\n",pc,vbase);
#endif
}

void STORE_B_ff820d(B v)
{
#if STE
	SM_B(MEM(0xff820d),v);
	vbase &= 0xffffff00;
	vbase |= (v & 0xff);
#endif
}

B LOAD_B_ff820d(void)
{
	return 0xff;
}

void STORE_B_ff820a(B v)
{
#if 0
	char *n;
	/* just for fun... */
	switch (v & 3)
	{
	case 0:	n = "STemu, 60Hz, internal sync."; break;
	case 1:	n = "STemu, 60Hz, external sync."; break;
	case 2:	n = "STemu, 50Hz, internal sync."; break;
	case 3:	n = "STemu, 50Hz, external sync."; break;
	}
	set_window_name (n);
#endif
}

void STORE_B_ff8604(B v)
{
	UB vv=v;
#if FDC_VERBOSE
	fprintf(stderr,"DMA car/scr hi <- %x (mode=%x)\n",vv,dma_mode);
#endif
	if (dma_mode&0x10)
	{
		dma_scr &= 0xff;
		dma_scr |= vv<<8;
	}
	else
	{
		if (dma_mode&8)
		{
			dma_car &= 0xff;
			dma_car |= vv<<8;
		}
		else
		{
			switch (dma_mode&6)
			{
				case 0:
					fdc_command &= 0xff;
					fdc_command |= vv<<8;
					break;
				case 2:
					fdc_track &= 0xff;
					fdc_track |= vv<<8;
					break;
				case 4:
					fdc_sector &= 0xff;
					fdc_sector |= vv<<8;
					break;
				case 6:
					fdc_data &= 0xff;
					fdc_data |= vv<<8;
					break;
			}
		}
	}
}

void STORE_B_ff8605(B v)
{
	UB vv=v;
#if FDC_VERBOSE
	fprintf(stderr,"DMA car/scr lo <- %x (mode=%x)\n",vv,dma_mode);
#endif
	if (dma_mode&0x10)
	{
		dma_scr &= 0xff00;
		dma_scr |= vv;
	}
	else
	{
		if (dma_mode&8)
		{
			dma_car &= 0xff00;
			dma_car |= vv;
		}
		else
		{
			switch (dma_mode&6)
			{
				case 0:
					fdc_command &= 0xff00;
					fdc_command |= vv;
					fdc_exec_command();
					break;
				case 2:
					fdc_track &= 0xff00;
					fdc_track |= vv;
					break;
				case 4:
					fdc_sector &= 0xff00;
					fdc_sector |= vv;
					break;
				case 6:
					fdc_data &= 0xff00;
					fdc_data |= vv;
					break;
			}
		}
	}
#if 0
	DMAdiskctl = vv & 0xff;
#endif
}

void STORE_B_ff8606(B v)
{
	UB vv=v;
	dma_mode &= 0xff;
	dma_mode |= vv<<8;
#if FDC_VERBOSE
	fprintf(stderr,"DMA mode <- %04x\n",dma_mode);
#endif
#if 0
	DMAfifo &= 0xff;
	DMAfifo |= vv<<8;
#endif
}

void STORE_B_ff8607(B v)
{
	UB vv=v;
	dma_mode &= 0xff00;
	dma_mode |= vv;
#if FDC_VERBOSE
	fprintf(stderr,"DMA mode <- %04x\n",dma_mode);
#endif
	return;
#if 0
	DMAfifo &= 0xff00;
	DMAfifo |= vv & 0xff;
	/* if the Sector-Control Register was selected, it's a 'Kippeln' (?) */
	if (DMAfifo & 0x10)
	{
		DMAxor ^= DMAfifo;
	}
	else
	{
		switch (DMAfifo & 14)
		{
		case 8: HDC(HDC_CS = DMAdiskctl); break;
		case 10: HDC_T = DMAdiskctl; break;
		case 12: HDC_S = DMAdiskctl; break;
		case 14: HDC_D = DMAdiskctl; break;
		case 0:	FDC(FDC_CS = DMAdiskctl); break;	
		case 2: FDC_T = DMAdiskctl; break;
		case 4: FDC_S = DMAdiskctl; break;
		case 6: FDC_D = DMAdiskctl; break;
		}
	}
#else
#if BOOTSECTOR
	if ((vv&0xff) == 0x88 && DMAdiskctl == 1)
	{
		UL x = (LM_UB(MEM(0xff8609))<<16)|(LM_UB(MEM(0xff860b))<<8)
				|LM_UB(MEM(0xff860d));
		bcopy(bootsec, MEM(x), 512);
	}
#endif
#endif
}

#ifdef NO_AUDIO
IGNORE_S(ff8800)
IGNORE_S(ff8801)
IGNORE_S(ff8802)
IGNORE_S(ff8803)
#endif

B LOAD_B_ff8920(void) {return 0;}
B LOAD_B_ff8910(void) {return 0;}
B LOAD_B_ff8900(void) {return 0;}
B LOAD_B_ff8921(void) {return 0;}
B LOAD_B_ff8911(void) {return 0;}
B LOAD_B_ff8901(void) {return 0;}
B LOAD_B_ff8922(void) {return 0;}
B LOAD_B_ff8912(void) {return 0;}
B LOAD_B_ff8902(void) {return 0;}
B LOAD_B_ff8923(void) {return 0;}
B LOAD_B_ff8913(void) {return 0;}
B LOAD_B_ff8903(void) {return 0;}
B LOAD_B_ff8924(void) {return 0;}
B LOAD_B_ff8904(void) {return 0;}
B LOAD_B_ff8925(void) {return 0;}
B LOAD_B_ff8905(void) {return 0;}
B LOAD_B_ff8906(void) {return 0;}
B LOAD_B_ff8907(void) {return 0;}
B LOAD_B_ff8908(void) {return 0;}
B LOAD_B_ff8909(void) {return 0;}
B LOAD_B_ff890a(void) {return 0;}
B LOAD_B_ff890b(void) {return 0;}
B LOAD_B_ff890c(void) {return 0;}
B LOAD_B_ff890d(void) {return 0;}
B LOAD_B_ff890e(void) {return 0;}
B LOAD_B_ff890f(void) {return 0;}
/* -------------------- ROUTINES FOR iotab2.tab -------------------- */
B LOAD_B_fffc00(void)
{
	return LM_B(MEM(0xfffc00)) | 2;
}

B LOAD_B_fffc02(void)
{
	return ikbd_readbuf();
}

B LOAD_B_fffc04(void)
{
#if DEBUG_MIDI
	fprintf(stderr,"Reading c04\n");
#endif
	return 2;
}

B LOAD_B_fffc06(void)
{
#if DEBUG_MIDI
	fprintf(stderr,"Reading c06\n");
#endif
	return /* TODO */ 1;
}
void STORE_B_fffc00(B v)
{
	/* TODO */
}

void STORE_B_fffc02(B v)
{
	UB x;
	/* send to IKBD (TODO) */
	x = LM_UB(MEM(0xfffc02));
	ikbd_send_byte(v);
	x |= 2;
	SM_UB(MEM(0xfffc02), x);
}

void STORE_B_fffc04(B v)
{
#if DEBUG_MIDI
	fprintf(stderr,"Writing %02x to c04\n",v&0xff);
#endif
}

void STORE_B_fffc06(B v)
{
#if DEBUG_MIDI
	fprintf(stderr,"Writing %02x to c06\n",v&0xff);
#endif
#if MIDI
	midi_send(v&0xff);
#endif
}

static int bank=0;
IGNORE_S(fffc20)
IGNORE_S(fffc22)
IGNORE_S(fffc24)
IGNORE_S(fffc26)
IGNORE_S(fffc28)
IGNORE_S(fffc2a)
IGNORE_S(fffc2c)
IGNORE_S(fffc2e)
IGNORE_S(fffc30)
IGNORE_S(fffc32)
IGNORE_S(fffc34)
IGNORE_S(fffc36)
IGNORE_S(fffc38)
IGNORE_S(fffc3a)
IGNORE_S(fffc3c)
IGNORE_S(fffc3e)
UNDEF_L(fffc20)
UNDEF_L(fffc22)
UNDEF_L(fffc24)
UNDEF_L(fffc26)
UNDEF_L(fffc28)
UNDEF_L(fffc2a)
UNDEF_L(fffc2c)
UNDEF_L(fffc2e)
UNDEF_L(fffc30)
UNDEF_L(fffc32)
UNDEF_L(fffc34)
UNDEF_L(fffc36)
UNDEF_L(fffc38)
UNDEF_L(fffc3a)
UNDEF_L(fffc3c)
UNDEF_L(fffc3e)

/* Normally, we would set the clock according to what's written to these
   addresses. But since we use the Unix system clock, we don't want (and,
   often, we can't) change it. */

IGNORE_S(fffc21)
IGNORE_S(fffc23)
/* c25, c27 below */
IGNORE_S(fffc29)
IGNORE_S(fffc2b)
IGNORE_S(fffc2d)
IGNORE_S(fffc2f)
IGNORE_S(fffc31)
IGNORE_S(fffc33)
IGNORE_S(fffc37)
IGNORE_S(fffc39)
/* c3b below */
IGNORE_S(fffc3d)
IGNORE_S(fffc3f)

static int mode24=1;
void STORE_B_fffc35 (B v)
{
	if (bank == 1) mode24 = v & 1;
}

static B fake_am, fake_amz;
void STORE_B_fffc25 (B v)
{
	/* TOS uses this... */
	if (bank == 1) fake_am = NIB(v);
	/* else ignore */
}
void STORE_B_fffc27 (B v)
{
	if (bank == 1) fake_amz = NIB(v);
}


static B c3b;
void STORE_B_fffc3b (B v)
{
	bank = v & 1;	/* the other 2 bits are ignored */	
	c3b = NIB(v);
}

#define TFUNC(_addr,_ret) B LOAD_B_ ## _addr (void) {TG return NIB(_ret);}
TFUNC(fffc21,tm->tm_sec % 10)
TFUNC(fffc23,tm->tm_sec / 10)
TFUNC(fffc29,tm->tm_hour % 10)
TFUNC(fffc2d,tm->tm_wday)
TFUNC(fffc2f,tm->tm_mday % 10)
TFUNC(fffc31,tm->tm_mday / 10)
TFUNC(fffc33,(tm->tm_mon+1) % 10)
TFUNC(fffc35,(tm->tm_mon+1) / 10)
TFUNC(fffc37,tm->tm_year % 10)
TFUNC(fffc39,(tm->tm_year-80) / 10)	

B LOAD_B_fffc3b (void)
{
	return c3b;
}

B LOAD_B_fffc2b(void)
{
	TG 
	if (mode24) return tm->tm_hour / 10;
	else
	{
		return (1<<(tm->tm_hour == 0 || tm->tm_hour >= 13))
				|((tm->tm_hour % 12)==0) || ((tm->tm_hour % 12)>=10);
	}
}

B LOAD_B_fffc25(void)
{
	if (bank == 1) return fake_am;
	time_gen();
	return NIB(tm->tm_min % 10);
}
B LOAD_B_fffc27(void)
{
	if (bank == 1) return fake_amz;
	time_gen();
	return NIB(tm->tm_min / 10);
}

B LOAD_B_fffc3d(void)
{
	return NIB(0);
}

UNDEF_L(fffc3f)
