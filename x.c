/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */

/* TODO in the next few releases:
 * - allocate colors on demand (not all 4096 immediately)
 * - clean up code, remove dead flags and code
 * - test on more displays
 */

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/times.h>
#include <signal.h>
#include <time.h>

#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include "config.h"
#include "defs.h"
#include "tosdefs.h"
#include "debug.h"
#include "ikbd.h"
#include "main.h"
#include "screen.h"
#include "utils.h"
#include "xlib_vdi.h"


#define KB_DEBUG 0
#define DEBUG_X 0

#ifndef CLK_TCK
#define CLK_TCK HZ
#endif

#define GCURX 0x25a
#define GCURY 0x258

UW stcolor_buf[320*200];

#ifndef GRABMODE
# define GRABMODE 0
#endif

#ifndef SHOW_X_CURSOR
# define SHOW_X_CURSOR 1
#endif

#define BENCH_REFRESH 0
#define NUM_AVG	100

#if DEBUG_X
#define DBG(_args...) fprintf( stderr, ## _args )
#else
#define DBG(_args...)
#endif

#ifdef SH_MEM
# include <sys/types.h>
# include <sys/ipc.h>
# include <sys/shm.h>
# include <X11/extensions/XShm.h>
#include "x.h"

static XShmSegmentInfo shminfo;
static int shm_used;
#endif /* SH_MEM */

static char txt[256];
Cursor cursors[3];     /* 2001-01-25 (MJK): Chris needs 3 not 2 cursors */
static int cursidx=0;
#if BENCH_REFRESH
static int cref=0;
static clock_t reftime;
static int chunks_seen=0,chunks_drawn=0;
#endif
static Pixmap empty;
#if GRABMODE
static int grabbed=1;
#endif
static Window in_which=None;
static XVisualInfo vi;
static int tc_cols[16];

/* store X11 mouse cursor position */
int    xmouse_x=-1, xmouse_y=-1;

/* new stuff */
#define STE_COLORS 4096
static int mono_mode;
static int bits_per_pixel;

int indexed_color;
static unsigned long pixel_value[STE_COLORS];
unsigned long pixel_value_by_index[16];
static unsigned char pixel_index[16];
static int is_allocated[STE_COLORS];
SCRDEF scr_def[3];
static int scr_x, scr_y; /* x,y possition of STonX Main Window */

/* some interesting events: */
#define E_MASK (KeyPressMask|KeyReleaseMask|ButtonPressMask	\
		|ButtonReleaseMask|PointerMotionMask|ColormapChangeMask|\
		 EnterWindowMask|LeaveWindowMask|ExposureMask)

static int old_shiftmod=-1;	/* last known shift mode, used to detect changes */
static UW old_color[16];	/* last known color palette */
static int shm_mono=1;
static char **ksyms;
static int redraw_flag=0;
static char win_name[64];
static Visual *visual = NULL;
static Status status;
static unsigned long wp, bp;		/* pixel values for black & white */
static int screen, nplanes, ncolors;
static int Max_x, Max_y;
static XEvent event;
static XSetWindowAttributes attrib;
static GC gc;
static XGCValues gc_val;
static XSizeHints sizehints;
Window imagewin;		/* window ID for image */
static XImage *image = NULL;
static Colormap hlcmap;
static char keybuf[64];
static int refcount=0;
static int lastmx=0,lastmy=0;
static unsigned char *buf;
static int shm_ct;
extern int fullscreen;

Display *display;
int kmin,kmax;
int shmflag = 1;
Colormap cmap=0;

char *key_filename = NULL; /* Keyboard-Translation-File */

extern int x_process_arg(char *optname, int argc, char *argv[]);
extern void x_show_arghelp(FILE *f);
extern void x_process_signal(int signal);
extern void x_process_events(void);

#if MONITOR
#include "monitor.h"
#endif

static void show_screen(void);
static void x_fullscreen(int *x, int *y);

Machine_Specific machine = {
    "X",
    x_process_arg,
    x_show_arghelp,
    x_process_signal,
    x_screen_open,
    x_screen_close,
    x_screen_shifter,
    x_init_keys,
    x_fullscreen
};

#define BUF(a,b) buf[(viewy * (b)) + (a)]
#undef SHM_WAIT

/*
  Process a special X machine option

  argv[0] is the first argument of the option
  argc    is the max. number of arguments

  Return:
      -1  it's not a special X machine option
       0  option without argument processed
    else  number of arguments processed
*/
int x_process_arg(char *optname, int argc, char *argv[]) {
    if ( !strcmp(optname, "noshm") ) {
	shmflag = 0;
	return 0;
    }
    else if ( !strcmp(optname,"kmap") ) {
	if (argc < 1)
	    error("kmap needs a file name as argument!\n");
	key_filename = strdup(argv[0]);
	return 1;
    }
    else if ( !strcmp(optname,"wpos") ) {
	if ( argc < 1 )
	    error("wpos needs an argument of the form <xpos>:<ypos>\n");
	if ( sscanf(argv[0],"%d:%d",&scr_x,&scr_y) != 2
	     || scr_x < 0 || scr_y < 0 )
	    error("invalid argument to -wpos\n");
	return 1;
    }
    /* It's not a special X machine option */
    return -1;
}

/*
  Show useage/help of all special X machine options 
*/
void x_show_arghelp(FILE *f) {
    fprintf( f, 
	     " -noshm                    Disable XShm extension\n"
	     " -kmap <file>              Load the Keyboard mappings from <file>\n"
	     " -wpos <xpos>:<ypos>       Set upper left corner of \"STonX Main Window\"\n"
	);
}

static int get_bpp(Display *display)
{
    static int count, i;
    static XPixmapFormatValues *x;
    x=XListPixmapFormats(display,&count);
    for (i=0; i<count; i++)
        if (x[i].depth == DefaultDepth(display,screen))
	    return x[i].bits_per_pixel;
    return 0;
}

/* Destroy the current emulator window
 */
static void destroy_image (void)
{
    if (image == NULL) 
	return;
    DBG("destroying XImage\n");
    XDestroyImage (image);
#ifdef SH_MEM
    if (shmflag && shm_used)
    {
	DBG("Cleaning up the shared memory mess\n");
	XShmDetach (display, &shminfo);
	shmdt (shminfo.shmaddr);
	shmctl (shminfo.shmid, IPC_RMID, 0);
	shm_used = 0;
    }
#endif
}

void x_screen_close (void)
{
  if ( !display ) { /* 2001-02-20 (MJK) */
    /* This may happen, because of OnExit at x_sceen_open */
    DBG( "Display not known at x_screen_close (already closed)!\n" );
    return;
  }
  XAutoRepeatOn(display);
  XFlush(display);
  destroy_image();

  if(fullscreen) {
    if((visual->class != TrueColor ) && (visual->class != DirectColor))
      XUninstallColormap(display, cmap);
    
    XUngrabServer(display);
  }

  XCloseDisplay(display);
  display = NULL;
  DBG("Screen closed.\n");
}

void set_window_name (Window id, char *name)
{
	XStoreName (display, id, name);
}

/* Create a new image for the emulation window, using shared memoy if possible
 */
static void create_image (int w, int h, int format)
{
  int planes;
  planes = (format == XYBitmap ? 1 : DefaultDepth(display,screen));
	Max_x = w-1;
	Max_y = h-1;
#ifdef SH_MEM
    if (shmflag && (planes != 1 || shm_mono))
    {
		DBG("Creating a shared memory XImage\n");
		image = XShmCreateImage (display, DefaultVisual(display,screen),
								(format == XYBitmap) ? 1 : DisplayPlanes(display,screen),
				 				format, NULL, &shminfo, w, h);
		if (image == NULL)
		{
			fprintf(stderr,"Error allocating a SHM-Image\n");
			XSync(display,False);
			exit(6);
		}
		shminfo.shmid = shmget (IPC_PRIVATE, image->bytes_per_line
								* image->height, IPC_CREAT | 0777);
		if (shminfo.shmid < 0)
		{
			fprintf(stderr,
			"Unable to allocate shared memory segment of size %ld bytes.\n"
			"Please increase the max. size of a segment (now %ld).\n",
			(long)image->bytes_per_line * image->height,(long) SHMMAX);
		}
		shminfo.shmaddr = image->data = shmat (shminfo.shmid, 0, 0);
		if (indexed_color || format == XYBitmap)
		{
			buf = shminfo.shmaddr;
		}
		else
		{
			DBG("Allocating separate buffer for conversion\n");
			buf = (unsigned char *)malloc(image->height * image->width);
			DBG("buf=%lx, shm=%lx\n",
				(long)buf,(long)shminfo.shmaddr);
		}

		shminfo.readOnly = False;
		XShmAttach (display, &shminfo);
		XSync(display, False);
		shmctl(shminfo.shmid, IPC_RMID, 0);
		shm_used = 1;
    }
    else
#endif
	{
		unsigned char *bb;
		DBG("Creating a plain XImage\n");
		/* 8 because that's what the conversion routines return at the moment*/
    	buf = (unsigned char *)malloc (w * h / (format == XYBitmap ? 8 : 1));

		/* we can write to the image directly in these cases: */
		if (indexed_color || format == XYBitmap)
		{
			bb=buf;
		}
		else
		{
			/* need a buffer since display != 8bpp */
			bb=(unsigned char *)malloc(w*h*BitmapUnit(display)/8);
			DBG("Allocating separate buffer for conversion (size=%ld)\n",(long)w*h*BitmapUnit(display)/8);
			DBG("buf=%lx, shm=%lx\n",(long)buf,(long)bb);
		}
    	image = XCreateImage (display, DefaultVisual(display,screen),
								format == XYBitmap ?
								1 : DisplayPlanes(display,screen), format,
								0, bb, w, h, 16, 0);
		DBG("Image->data=%lx, bb=%lx, buf=%lx\n",
			(long)image->data,(long)bb,(long)buf);
		if (image == NULL)
		{
			fprintf(stderr,"Error allocating an XImage\n");
			XSync(display,False);
			exit(6);
		}
#ifdef SH_MEM
		shm_used=0;
#endif
	}
	if (planes == 1)
	{
		image -> bitmap_bit_order = MSBFirst;
		image -> byte_order = MSBFirst;
	}
	XSync(display,False);
}

static void allocate_color(UW stcolor)
{
	if (!is_allocated[stcolor])
	{
		XColor col;
		int c=stcolor;
		col.flags = DoRed | DoGreen | DoBlue;
		col.red 	= (((c>>8)&7)<<13)|((c>>11)<<12);
		col.green	= (((c>>4)&7)<<13)|(((c>>7)&1)<<12);
		col.blue	= ((c&7)<<13)|(((c>>3)&1)<<12);
		if (!XAllocColor(display,cmap,&col))
		{
			fprintf(stderr,
				"Color allocation failed, exiting. This should"
				" never happen, please notify the author!\n");
			exit(4);
		}
		pixel_value[c]=col.pixel;
		is_allocated[c]=1;
		DBG("Allocating %04x -> %08lx\n",stcolor,col.pixel);
	}
}

/* Change the window colormap according to the contents of old_color[].
 * The colors are converted from ST/STe format (least-significant 9/12 bits
 * of a 16 bit integer) to the 16 bit per RGB component format used in X11.
 */
static int allocatedcols[COLS];
static void change_colors (void)
{
	XColor colors[COLS];
	UW c;
	int i;
	int q=0;
	unsigned long pixels[COLS];
	int j,k;
	if (shiftmod == 2)
	{
		for (i=0; i<COLS; i++)
		{
			colors[i].pixel = i;
			colors[i].flags = DoRed | DoGreen | DoBlue;
			colors[i].red = colors[i].green = colors[i].blue = 0;
			DBG("[%d] %03x\n",i,old_color[i]);
		}
		if (old_color[0] == 0) {
			/* mono show_screen does just memcpy; XPutImage... */
			colors[1].red = colors[1].green = colors[1].blue = 0xffff;
			XSetForeground (display, gc, wp);
			XSetBackground (display, gc, bp);
		} else {
			DBG("Colors are black on white background\n");
			colors[0].red = colors[0].green = colors[0].blue = 0xffff;
			XSetForeground (display, gc, bp);
			XSetBackground (display, gc, wp);
		}
	}
	else
	{
		if (indexed_color)
		{
			for (i=0; i<COLS; i++)
			{
				c = (i < (shiftmod == 0 ? 16 : 4) ? old_color[i] : 0);
#if !STE
				c &= 0x777;
#endif
				colors[i].pixel = i;
				colors[i].flags = DoRed | DoGreen | DoBlue;
				colors[i].red 	= (((c>>8)&7)<<13)|((c>>11)<<12);
				colors[i].green	= (((c>>4)&7)<<13)|(((c>>7)&1)<<12);
				colors[i].blue	= ((c&7)<<13)|(((c>>3)&1)<<12);
			}
		}
		else
		{	
			/* saves one indirection in the conversion routines... */
			for (i=0; i<COLS; i++)
			{
				if (!is_allocated[old_color[i]])
				{
					XColor col;
					int c=old_color[i];
					col.flags = DoRed | DoGreen | DoBlue;
					col.red 	= (((c>>8)&7)<<13)|((c>>11)<<12);
					col.green	= (((c>>4)&7)<<13)|(((c>>7)&1)<<12);
					col.blue	= ((c&7)<<13)|(((c>>3)&1)<<12);
					if (!XAllocColor(display,cmap,&col))
					{
						fprintf(stderr,
							"Color allocation failed, exiting. This should"
							" never happen, please notify the author!\n");
						exit(4);
					}
					DBG("Allocating %3x to pixel value %06lx\n",
						c,col.pixel);
					pixel_value[c]=col.pixel;
					is_allocated[c]=1;
				}
				pixel_value_by_index[i]=pixel_value[old_color[i]];
			}
		}
	}

	if (indexed_color)
	{
		if (!priv_cmap)
		{
			for (i=0; i<COLS; i++)
			{
				if (allocatedcols[i]>=0)
				{
					pixels[q++]=allocatedcols[i];
				}
			}
			if (q>0) XFreeColors(display, cmap, pixels, q, 0);
			for (i=0; i<COLS; i++)
			{
				XAllocColor (display, cmap, &colors[i]);
				mapcol[i]=colors[i].pixel;
			}
		}
		else
		{
			XStoreColors (display, cmap, colors, COLS);
		}
	}
	else
	{
		for (i=0; i<scr_height/CHUNK_LINES; i++)
			draw_chunk[i]|=1;
	}

#if 0
	XMapRaised (display, imagewin);
	XFlush(display);
#endif
#if 0
	XUnmapWindow (display, imagewin);
	XMapWindow (display, imagewin);
#endif
}

/* Do something appropriate when the 'shiftmod' hardware register has changed:
 * 1. The window is resized
 * 2. A new image is created
 * 3. A new conversion function is used every VBL
 */
static void change_mode (void)
{
	scr_width = scr_def[shiftmod].w;
	scr_height = scr_def[shiftmod].h;
	scr_planes=4>>shiftmod;
	DBG("Screen is now %dx%d, shift mode %d\n",scr_width,scr_height,shiftmod);
	switch (shiftmod)
	{
		case 0:
			XResizeWindow (display, imagewin, scr_width, scr_height);
			destroy_image();
			create_image (scr_width, scr_height, ZPixmap);
			change_colors();
			break;

		case 1:
			XResizeWindow (display, imagewin, scr_width, scr_height*2);
			destroy_image();
			create_image (scr_width, scr_height*2, ZPixmap);
			change_colors();
			break;

		case 2:
			XResizeWindow (display, imagewin, scr_width, scr_height);
			destroy_image();
			create_image (scr_width, scr_height, XYBitmap);
			change_colors();
			break;
	}
}

/*
  AltGr-Remap from Christian Felsch
*/
static void remap_key(XKeyEvent *ev, int mode)
{
    int state = ev->state;
    int keycode = ev->keycode;

#if KB_DEBUG
    if (mode == 1)
	DBG("keycode 0x%x, state 0x%x\n", keycode, state);
#endif

    /*
     * Belegung von state, abhängig von Modifier-Tasten
     *                        -   CAPSLOCK NUMLOCK CL+NL
     * Shift                 0x1    0x3    0x11    0x13
     * Ctrl                  0x4    0x6    0x14    0x16
     * Shift-Ctrl            0x5    0x7    0x15    0x17
     * Alt                   0x8    0xa    0x18    0x1a
     * Shift-Alt             0x9    0xb    0x19    0x1b
     * Ctrl-Alt              0xc    0xe    0x1c    0x1e
     * Shift-Ctrl-Alt        0xd    0xf    0x1d    0x1f
     * AltGr                 0x20    0x22    0x30    0x32
     * Shift-AltGr           0x21    0x23    0x31    0x33
     * Ctrl-AltGr            0x24    0x26    0x34    0x36
     * Shift-Ctrl-AltGr      0x25    0x27    0x35    0x37
     * Alt-AltGr             0x28    0x2a    0x38    0x3a
     * Shift-Alt-AltGr       0x29    0x2b    0x39    0x3b
     * Ctrl-Alt-AltGr        0x2c    0x2e    0x3c    0x3e
     * Shift-Ctrl-Alt-AltGr  0x2d    0x2f    0x3d    0x3f
     */

    /*
     * Das Drücken der AltGr-Taste ignorieren. Wird eine Taste mit
     * AltGr kombiniert, erkennt man das an state = 0x2000 (s.u.)
     */
    if (keycode == 0x71)
	return;

    /*
     * Ummappen der AltGr-Codes der deutschen Tatstatur zu ATARI Sonderzeichen
     *    PC         Atari
     *  AltGr-Q		@
     *  AltGr-7		{
     *  AltGr-8		[
     *  AltGr-9		]
     *  AltGr-0		}
     *  AltGr-ß		\
     *  AltGr-<		|
     *  AltGr-+		~
     */
    if (state & 0x2000)			/* AltGr + Taste umappen */
    {
#if KB_DEBUG
        DBG( "Remap.\n" );
#endif
	switch (keycode)
	{
	  case 0x5e:	/* < */
	      ikbd_key(0x32, mode);	/* Shift-^ erzeugt | */
	      ikbd_key(0x31, mode);
	      break;
	      
	  case 0x23:	/* + */
	      ikbd_key(0x31, mode);	/* ^ erzeugt ~ */
	      break;
	      
	  case 0x12:	/* 9 */
	      ikbd_key(0x40, mode);	/* Alt-ä erzeugt ] */
	      ikbd_key(0x30, mode);
	      break;
	      
	  case 0x11:	/* 8 */
	      ikbd_key(0x40, mode);	/* Alt-ö erzeugt [ */
	      ikbd_key(0x2f, mode);
	      break;
	      
	  case 0x18:	/* q */
	      ikbd_key(0x40, mode);	/* Alt-ü erzeugt @ */
	      ikbd_key(0x22, mode);
	      break;
	      
	  case 0x13:	/* 0 */
	      ikbd_key(0x32, mode);	/* Shift-Alt-ä erzeugt } */
	      ikbd_key(0x40, mode);
	      ikbd_key(0x30, mode);
	      break;
	      
	  case 0x10:	/* 7 */
	      ikbd_key(0x32, mode);	/* Shift-Alt-ö erzeugt { */
	      ikbd_key(0x40, mode);
	      ikbd_key(0x2f, mode);
	      break;
	      
	  case 0x14:	/* ß */
	      ikbd_key(0x32, mode);	/* Shift-Alt-ü erzeugt \ */
	      ikbd_key(0x40, mode);
	      ikbd_key(0x22, mode);
	      break;
	      
	  default:
	      DBG("AltGr-%#03x unknown\n", keycode);
	}
    }
    else
#if KB_DEBUG
        DBG("No remap.\n");
#endif
	ikbd_key (keycode, mode);
}

#if GRABMODE
/* un-grab mouse/keyboard, also called by x_process_signal(SIGSEGV) */
static void un_grab(void)
{
    XUngrabKeyboard(display, CurrentTime);
    XUngrabPointer(display, CurrentTime);
    XAutoRepeatOn(display);
    XUndefineCursor(display,imagewin);
    XDefineCursor (display, imagewin, cursors[2]);
    grabbed=0;
}

static void x_wait_for_grab(void)
{
    XEvent ev;
    
    while (!grabbed)
    {
        XPeekEvent(display, &ev);
        x_process_events();
        if (ev.type == Expose && (!vdi_mode || xw == None))
            show_screen();
    }
}
#endif /* GRABMODE */

void x_process_signal(int sig) {
    if ( sig == SIGSEGV ) {
#if GRABMODE
	un_grab();
#endif
	exit(1);
    }
}

/* Process all events that occured since we last came here...
 */
void x_process_events(void)
{
    int tx,ty;
    XEvent e;
    XExposeEvent *x;
    while (XCheckMaskEvent (display, E_MASK, &e))
    {
	switch (e.type)
	{
	  case EnterNotify:
	      DBG("Entering STonX window\n");
	      
	      if (vdi && xw != None) 
		  vdi_mode = (e.xcrossing.window == xw);
	      
	      in_which = e.xcrossing.window;
	      
	      if (in_which == imagewin)
	      {
#if GRABMODE
		  if (grabbed) {
#endif
		      XAutoRepeatOff(display);

		      XDefineCursor (display, imagewin, cursors[cursidx]);

		      /* save X11 mouse position */
		      xmouse_x = (((XCrossingEvent *)&e)->x);
		      xmouse_y = (((XCrossingEvent *)&e)->y);
#if GRABMODE
		  }
#endif /* GRABMODE */
	      }
	      break;

	  case LeaveNotify:
	      DBG("Leaving STonX window\n");
#if GRABMODE
	      if (grabbed) 
		  XWarpPointer(display,None,imagewin,0,0,0,0,lastmx,lastmy);
#else /* GRABMODE */
	      XAutoRepeatOn(display); /* BUG: should restore previous value */
	      if (e.xcrossing.window == imagewin) 
		  XUndefineCursor(display,imagewin);
	      in_which = None;
#endif /* GRABMODE */
	      break;

	  case KeyPress:
	      if (XLookupKeysym ((XKeyEvent *)&e,0) == XK_Pause)
	      {
		  if(((XKeyEvent *)&e)->state & ShiftMask) {
#if SHIFT_PAUSE_EXITS
		      fprintf(stderr,"Shift-Pause detected\n");
		      #if MONITOR
		      signal_monitor(BREAK,NULL);
		      #else
		      stonx_exit();
		      exit(0);
                      #endif
#endif /* SHIFT_PAUSE_EXITS */
		  }
#if GRABMODE
		  else if (grabbed)
		  {
		      DBG("remove mouse/keyboard grab\n");
#if CLIPBOARD
		      clip_TOS_to_X();
#endif
		      un_grab();
		      x_wait_for_grab();
		  }
#elif SHOW_X_CURSOR /* GRABMODE */
		  else {
		      cursidx = cursidx ? 0 : 1;
		      XDefineCursor( display, imagewin, cursors[cursidx]);
		  }
#endif /* GRABMODE */
	      }
	      else
#if GRABMODE
		  if(grabbed)
#endif /* GRABMODE */
#if ALTGR_HARDCODED
		      remap_key(((XKeyEvent *)&e), 1);
#else 
	              ikbd_key (((XKeyEvent *)&e)->keycode, 1);
#endif
	      break;

	  case KeyRelease:
#if GRABMODE
	      if (grabbed)
#endif
		  if (XLookupKeysym ((XKeyEvent *)&e,0) != XK_Pause)
#if ALTGR_HARDCODED
		      remap_key(((XKeyEvent *)&e), 0);
#else
	              ikbd_key (((XKeyEvent *)&e)->keycode, 0);
#endif
	      break;

	  case ButtonPress:
	      DBG("ButtonPress: %d\n",((XButtonEvent *)&e)->button);

#ifdef EXIT_ON_CTRL_RIGHTMOUSE
	      if (((XButtonEvent *)&e)->button == 3
		  && (((XButtonEvent *)&e)->state & ControlMask)) {
		stonx_exit(); /* Exit stonx clean */
		exit(0);
	      }
#endif

#if GRABMODE
	      if (!grabbed)
	      {
#if CLIPBOARD
		  clip_X_to_TOS();
#endif
		  XGrabPointer(display,imagewin,True,PointerMotionMask
			       |ButtonPressMask|ButtonReleaseMask,
			       GrabModeAsync, GrabModeAsync, None, None,
			       CurrentTime);
		  XGrabKeyboard(display,imagewin,True,GrabModeAsync,
				GrabModeAsync,CurrentTime);

		  if (in_which != xw) {
		      xmouse_x = ((XButtonEvent *)&e)->x;
		      xmouse_y = ((XButtonEvent *)&e)->y;
		      XDefineCursor(display,imagewin,cursors[cursidx]);
		  }
	      }
#endif /* GRABMODE */
	      switch(((XButtonEvent *)&e)->button)
	      {
		case 3:
		    ikbd_button(2,1); /* map button 3 to Atari button 2 */
		    break;
		case 2:
#if SIMULATE_DOUBLE_CLICK
		    ikbd_button(1,1);
		    ikbd_button(1,0);
		    ikbd_button(1,1);
		    /* second release is normal release of button 3 */
#elif GRABMODE && MOUSE2_UNGRABS /* SIMULATE_DOUBLE_CLICK */
		    if (grabbed)
		    {
			DBG("remove mouse/keyboard grab\n");
#if CLIPBOARD
			clip_TOS_to_X();
#endif
			un_grab();
			x_wait_for_grab();
		    }
#elif SHOW_X_CURSOR
		    cursidx = cursidx ? 0 : 1;
		    XDefineCursor( display, imagewin, cursors[cursidx]);
#endif /* SIMULATE_DOUBLE_CLICK ... SHOW_X_CURSOR */
		    break;
		case 1:
		    ikbd_button(1,1);
		    break;
	      }
#if GRABMODE
	      if ( !grabbed ) {
		  grabbed = 1;
		  DBG( "mouse/keyboard grabbed\n" );
	      }
#endif
	      break;

	  case ButtonRelease:
	      DBG("Buttonrelease: %d\n", ((XButtonEvent *)&e)->button);
#if GRABMODE
	      if (grabbed)
	      {
#endif /* GRABMODE */
		  switch(((XButtonEvent *)&e)->button)
		  {
		    case 3:
			ikbd_button(2,0); /* map button 3 to button 2 */
			break;
		    case 2:
#if SIMULATE_DOUBLE_CLICK
			ikbd_button(1,0); /* map simulated double click */
#endif
			break;
		    case 1:
			ikbd_button(1,0);
			break;
		  }
#if GRABMODE
	      }
#endif /* GRABMODE */
	      break;

	  case Expose:
	      x = (XExposeEvent *)&e;
#if XBUFFER
	      if (xw == x->window)
		  vdi_redraw(x->x,x->y,x->width,x->height);
#endif
	      {
		  int i;
		  for (i=0; i<scr_height/CHUNK_LINES; i++)
		      draw_chunk[i]|=1;
	      }
	      break;

	  case MotionNotify:
#if GRABMODE
	      if (grabbed)
	      {
#endif
		  xmouse_x = (((XCrossingEvent *)&e)->x);
		  xmouse_y = (((XCrossingEvent *)&e)->y);
#if GRABMODE
	      }
#endif /* GRABMODE */
	      break;
	}

	/* frequently check the TOS mouse position and */
	/* update it if the x mouse has moved and the ikbd */
	/* buffer is empty, this is done here, because the */
	/* buffer might have been full when the event occured */
	if (abase != 0) {
	  extern int ikbd_inbuf;
      
	  /* get TOS mouse position */
	  tx = LM_W(MEM(abase-GCURX));
	  ty = LM_W(MEM(abase-GCURY));

	  /* compensate zoom in st mid */
	  if(old_shiftmod == 1) ty *= 2;
	  
	  /* match X and TOS mouse */
	  if((ikbd_inbuf == 0)&&
	     ((xmouse_x != tx)||(xmouse_y != ty))&&
	     (xmouse_x != -1)) {
	    
	    if(old_shiftmod == 1) 
	      ikbd_adjust (xmouse_x-tx, (xmouse_y-ty)/2);
	    else
	      ikbd_adjust (xmouse_x-tx, xmouse_y-ty);
	  }
	}
    }
}

void x_fullscreen(int *x, int *y) {

  if ((display = XOpenDisplay (NULL)) == NULL) {
    error ("Could not open display.\n");
  }

  *x = XScreenOfDisplay(display, screen)->width;
  *y = XScreenOfDisplay(display, screen)->height;

  XCloseDisplay(display);
}

void x_screen_open (void)
{
    int i, j;
    int shm_major, shm_minor;
    Bool shm_pixmaps;
    XSizeHints h;
    XFontStruct *font_info;
    XImage *t;
    XColor ccols[2];
    XSetWindowAttributes xa;
    unsigned long attrvaluemask;

    scr_width = scr_def[shiftmod].w;
    scr_height = scr_def[shiftmod].h;

    strcpy(win_name, "STonX Main Window");

    if ((display = XOpenDisplay (NULL)) == NULL)
    {
	fprintf (stderr,"Could not open display.\n");
	exit (1);
    }

    fprintf (stderr,"Obtaining Keysym mappings for display...\n");
    XDisplayKeycodes (display, &kmin, &kmax);
    fprintf (stderr,"Keycode range = %d..%d\n", kmin, kmax);
    ksyms = (char **)malloc(sizeof(char *)*(kmax-kmin+1));
    for (i=kmin; i<=kmax; i++)
    {
	char *s = XKeysymToString(XKeycodeToKeysym(display,i,0));
	ksyms[i-kmin] = (s == NULL ? s : strdup(s));
    }

#ifdef SH_MEM
    if (shmflag)
    {
	if (!XShmQueryVersion (display, &shm_major, &shm_minor, &shm_pixmaps))
	{
	    fprintf(stderr,
		    "MIT Shared Memory Extension not supported.\n");
	    shmflag = 0;
	}
	else
	{
	    int foo;
	    if ( verbose )
		fprintf (stderr,"Using MIT Shared Memory Extension %d.%d," \
			 " %s shared pixmaps.\n", shm_major, shm_minor,
			 (shm_pixmaps ? "with" : "without"));
	    foo = XShmPixmapFormat (display);
	    if ( verbose ) {
		fprintf (stderr,"XShm Pixmap format: %s\n", 
			 ((foo == XYBitmap ? "XYBitmap" 
			   : (foo == XYPixmap ? "XYPixmap" : "ZPixmap"))));
		fprintf (stderr,
			 "If you get a BadAccess error, try the -noshm option!\n");
	    }
	}
    }
#endif
	
    screen = XDefaultScreen(display);
    depth = DefaultDepth(display, screen);
    if (depth == 1)
    {
	SM_B(MEM(0xff8260),2);
	old_shiftmod = 2;
    }
    visual = DefaultVisual(display, screen);
    nplanes = XDisplayPlanes(display, screen);

    /* See if we are on a local little-endian display - in that case,
       XShmPutImage will refuse to swap bits most of the time! */
    if (shmflag)
    {
	char *c=(char *)malloc(64);
	t = XCreateImage(display, visual, 1, XYBitmap, 0, c, 32, 16, 8, 0);
	if (t->bitmap_bit_order == LSBFirst)
	{
	    fprintf(stderr,"NOTE: Because XShmPutImage apparently can't swap bits, shared memory support\nwas turned off for monochrome mode!\n");
	    shm_mono=0;
	}
	XDestroyImage(t);
    }

    h.width = scr_width;
    h.height = scr_height;
    h.x = scr_x;
    h.y = scr_y;

    h.flags = PSize | PPosition;
    bp = BlackPixel(display, screen);
    wp = WhitePixel(display, screen);
    DBG("Black=%ld, White=%ld\n",bp,wp);

    if (XMatchVisualInfo(display,screen,8,PseudoColor, &vi))
    {
	if ( verbose )
	    fprintf(stderr,"Using 8 bpp PseudoColor visual\n");
	indexed_color = 1;
	bits_per_pixel = 8;
    }
    else
    {
	indexed_color = 0;
	bits_per_pixel = get_bpp(display);
	priv_cmap = 1; /* temp kludge for screen XXX */
    }
    if ( verbose )
	fprintf(stderr,
		"Indexed=%d, bits per pixel=%d\n",
		indexed_color,bits_per_pixel);
    
    DBG("Trying to create a (%d,%d,%d,%d) window\n",
	h.x,h.y,h.width,h.height);
    imagewin = XCreateWindow(display, RootWindow (display, screen),
			     h.x, h.y, h.width, h.height,
			     0, DefaultDepth(display,screen),
			     InputOutput, visual,
			     0, &xa);

    if(fullscreen) {
      xa.override_redirect = True;
      attrvaluemask = CWOverrideRedirect;      
      XChangeWindowAttributes(display, imagewin, attrvaluemask, &xa);
    }    

    XSelectInput(display, imagewin, StructureNotifyMask|E_MASK);
    XSetStandardProperties(display, imagewin, win_name, win_name, None,
			   NULL, 0, &h);
    XMapWindow(display, imagewin);

    for (;;)
    {
	XEvent e;
	XNextEvent(display, &e);
	if (e.type == MapNotify && e.xmap.event == imagewin) 
	    break;
    }
    gc_val.background = 0;
    gc_val.foreground = 1;
    gc = XCreateGC (display, imagewin, GCForeground | GCBackground, &gc_val);
#ifdef SH_MEM
    shm_ct = XShmGetEventBase (display) + ShmCompletion;
#endif
    if(fullscreen) {
       XGrabServer(display);
       XSetInputFocus(display, imagewin, RevertToPointerRoot, CurrentTime);
    }

    if (indexed_color)
    {
	if (priv_cmap)
	    cmap = XCreateColormap (display, RootWindow(display,screen),
				    visual, AllocAll);
	else
	{
	    cmap = DefaultColormap (display, screen);
	    for (i=0; i<COLS; i++) allocatedcols[i]=-1;
	}
	XSetWindowColormap (display, imagewin, cmap);
    }
    else
    {
	int colors=512;
	int pr=0,pr0=4096/10;
	if ( verbose )
	    fprintf(stderr,"Allocating %d colors",colors);
	cmap = XCreateColormap (display, RootWindow(display,screen),
				visual, AllocNone);
	for (i=0; i<4096; i++)
	{
	    XColor col;
	    if ((i & 0x777) == i)
	    {
		col.flags = DoRed | DoGreen | DoBlue;
		col.red 	= (((i>>8)&7)<<13)|((i>>11)<<12);
		col.green	= (((i>>4)&7)<<13)|(((i>>7)&1)<<12);
		col.blue	= ((i&7)<<13)|(((i>>3)&1)<<12);
		if (!XAllocColor(display,cmap,&col))
		{
		    fprintf(stderr,"Color allocation failed, exiting.\n");
		    exit(4);
		}
		pixel_value[i]=col.pixel;
	    }
	    else pixel_value[i]=pixel_value[i&0x777];
	    
	    if (++pr == pr0)
	    {
		if ( verbose )
		    putc('.',stderr);
		pr=0;
	    }
	}
	if ( verbose )
	    fprintf(stderr,"\n");
    }
    XClearWindow(display, imagewin);
    XSelectInput(display, imagewin, E_MASK);

    if (depth == 1) 
	create_image(scr_width,scr_height,XYBitmap);
    empty = XCreatePixmapFromBitmapData(display, imagewin, 
					(char*)calloc(32,1),
					16, 16,0,0,1);
    ccols[0].pixel = 0;
    ccols[1].pixel = 0;
    cursors[0] = XCreatePixmapCursor(display, empty, empty,
				     &ccols[0], &ccols[1],
				     0,0);
    cursors[1] = XCreateFontCursor(display, XC_tcross);
    cursors[2] = XCreateFontCursor(display, XC_X_cursor);
#if GRABMODE
    XGrabPointer(display, imagewin, True, 
		 PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
		 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    XGrabKeyboard(display, imagewin,True, GrabModeAsync,
		  GrabModeAsync, CurrentTime);
    grabbed = 1;
    printf("The X pointer and keyboard have been grabbed by STonX,\n"
	   "press PAUSE to release/grab again.\n");
#endif
    OnExit(x_screen_close);
}

/*
 * What about unaligned buffers here?
 */
static void stcolor_to_image (UW *b, unsigned char *p, int y0, int nlines)
{
    int n,i,cv;
    
    b += image->width * y0 * 2;
    p += (image->width * y0 * bits_per_pixel)/8;	/* XXX for <8 bpp... */
    n = nlines * image->width;						/* pixels to convert */
    
    switch(bits_per_pixel)
    {
      case 32:
	  for (i=0; i<n; i++)
	  {
	      UW col;
	      col=b[i];

	      *(int *)p=pixel_value[col];
	      p+=4;
	  }
	  break;
      case 24:	/* UNTESTED */
	  for (i=0; i<n; i++)
	  {
	      cv=pixel_value[b[i]];
	      *p++ = cv>>16;
	      *p++ = (cv>>8)&0xff;
	      *p++ = cv&0xff;
	  }
	  break;
      case 16: 	/* UNTESTED */
	  for (i=0; i<n; i++)
	  {
	      
	      *(UW *)p=pixel_value[b[i]];
	      p+=2;
	  }
	  break;
      case 8:		/* UNTESTED */
	  for (i=0; i<n; i++)
	  {
	      *p++ = pixel_value[b[i]];
	  }
	  break;
      default:
	  fprintf(stderr,"Weird screen format, exiting.\n");
	  exit(5);
    }
}

static void buffer_to_image (unsigned char *b, unsigned char *p,
							int y0, int nlines)
{
    int n,i,cv;
    
    if (indexed_color || shiftmod == 2) return;
    
    b += image->width * y0;
    p += (image->width * y0 * bits_per_pixel)/8;	/* XXX for <8 bpp... */
    n = nlines * image->width;				/* pixels to convert */
    
    switch(bits_per_pixel)
    {
      case 32:
	  for (i=0; i<n; i++)
	  {
	      *(int *)p=pixel_value_by_index[b[i]];
	      p+=4;
	  }
	  break;
      case 24:	/* UNTESTED */
	  for (i=0; i<n; i++)
	  {
	      cv=pixel_value_by_index[b[i]];
	      *p++ = cv>>16;
	      *p++ = (cv>>8)&0xff;
	      *p++ = cv&0xff;
	  }
	  break;
      case 16: 	/* UNTESTED */
	  for (i=0; i<n; i++)
	  {
	      *(UW *)p=pixel_value_by_index[b[i]];
	      p+=2;
	  }
	  break;
      case 8:		/* UNTESTED */
	  for (i=0; i<n; i++)
	      *p++ = pixel_value_by_index[b[i]];
	  break;
      default:
	  fprintf(stderr,"Weird screen format, exiting.\n");
	  exit(5);
    }
}

/* Update the X window using the shifter mode known when change_mode was last
 * called...
 */
static void show_screen (void)
{
    int w, h;
    int xzoom,yzoom=1;
    int i,n;
    UB *p;
    long cv;
    
    w=scr_width;
    h=scr_height;
    switch (old_shiftmod)
    {
      case 0:
	  st16c_to_z ((unsigned char*)MEM(vbase), buf);
	  break;
      case 1:
	  yzoom=2;
	  st4c_to_z ((unsigned char *)MEM(vbase), buf); 
	  break;
      case 2:
	  stmono_to_xy(buf, (unsigned char *)MEM(vbase));
	  break;
    }
#ifdef SH_MEM
    if (shm_used) /* shmflag && (old_shiftmod != 2 || shm_mono)) */
    {
	XEvent xev;
	if (chunky)
	{
	    int i;
	    for (i=0; i<h/CHUNK_LINES; i++)
		if (draw_chunk[i])
		{	
		    buffer_to_image(buf,image->data,yzoom*i*CHUNK_LINES,
				    yzoom*CHUNK_LINES);
		    XShmPutImage (display, imagewin, gc, image,
				  0, yzoom*i*CHUNK_LINES,
				  0, yzoom*i*CHUNK_LINES, w,
				  yzoom*CHUNK_LINES,
				  False);
		    draw_chunk[i]=0;
#if BENCH_REFRESH
		    chunks_drawn++;
#endif
		}
#if BENCH_REFRESH
	    chunks_seen+=h/CHUNK_LINES;
#endif
	}
	else
	{
	    buffer_to_image(buf,image->data,0,h*yzoom);
	    XShmPutImage (display, imagewin, gc, image, 0, 0, 0, 0, w, h*yzoom,
			  False);
	}
#if 1
	XSync(display,False);
#endif
#ifdef SHM_WAIT
	do
	{
	    XNextEvent (display, &xev);
	}
	while (xev.type != shm_ct);
#endif
    }
    else
#endif
    {
	if (chunky)
	{
	    int i;
	    for (i=0; i<h/CHUNK_LINES; i++)
		if (draw_chunk[i])
		{
		    buffer_to_image(buf,image->data,0,h*yzoom);
		    XPutImage (display, imagewin, gc, image,
			       0, yzoom*i*CHUNK_LINES,
			       0, yzoom*i*CHUNK_LINES, w,
			       yzoom*CHUNK_LINES);
		    draw_chunk[i]=0;
#if BENCH_REFRESH
		    chunks_drawn++;
#endif
		}
#if BENCH_REFRESH
	    chunks_seen+=h/CHUNK_LINES;
#endif
	}
	else
	{
	    buffer_to_image(buf,image->data,0,h*yzoom);
	    XPutImage (display, imagewin, gc, image, 0, 0, 0, 0, w, h*yzoom);
	}
	XFlush (display);
    }
#if 1
    XSync (display, False);
#endif
}

static void manage_window (void)
{
    XKeyEvent *keypress;
    KeySym keysym;
    XComposeStatus cstat;
    
    keypress = (XKeyEvent *) & event;
    
    if (XPending (display))
    {
	XNextEvent (display, &event);
	switch ((int) event.type)
	{
	  case KeyPress:
	      if (XLookupString (keypress, keybuf, sizeof (keybuf),
				 &keysym, &cstat) != 0)
	      {
		  /*		update_IKBD_BUFFER (); */
	      }
	      break;
	  default:
	      break;
	}
    }
}

/* This function is called periodically (from a signal handler) to check what's
 * new with the shifter mode, colors, and update ("shift") the screen. It
 * should probably rewritten to check whether it's necessary to update the
 * screen, since for color modes this is a very costly operation...
 *
 * The Shifter status information is changed in hw.c, where the hardware regs
 * are read.
 */
void x_screen_shifter (void)
{
    int i, w, pal = 0;
    
    x_process_events();
    
    if (++refcount < refresh) return;
    refcount=0;
    
    for (i=0; i<16; i++)
    {
	if ((w = LM_UW(&color[i])) != old_color[i])
	{
	    pal = 1;	/* palette has changed */
	    old_color[i] = w & 0xfff;
	}
    }
    
    if (pal) change_colors();	/* update the window colormap */
    
    if (depth > 1 && old_shiftmod != shiftmod)
    {
	old_shiftmod = shiftmod;
	change_mode();
    }
    
    if (!vdi_mode || xw == None)
    {
#if BENCH_REFRESH
	struct tms ta;
	clock_t rt;
	times(&ta);
	rt=ta.tms_utime+ta.tms_stime;
	show_screen();
	times(&ta);
	reftime += ta.tms_utime+ta.tms_stime-rt;
	cref++;
	if (cref==NUM_AVG)
	{
	    fprintf(stderr, "Refresh time (%d samples): %lf ms\n", NUM_AVG,
		    1000.0*(double)reftime/(NUM_AVG*CLK_TCK));
	    if (chunky) DBG("Chunks drawn/seen = %d/%d\n",chunks_drawn,chunks_seen);
	    reftime=0;
	    cref=0;
	}
#else
	show_screen();
#endif
    }
#if 0
    check_ui();
#endif
}

struct
{
    int c;
    char *s;
} st_keysyms[] =
{
    {ST_1, "ST_1"},
    {ST_2, "ST_2"},
    {ST_3, "ST_3"},
    {ST_4, "ST_4"},
    {ST_5, "ST_5"},
    {ST_6, "ST_6"},
    {ST_7, "ST_7"},
    {ST_8, "ST_8"},
    {ST_9, "ST_9"},
    {ST_0, "ST_0"},
    {ST_F1, "ST_F1"},
    {ST_F2, "ST_F2"},
    {ST_F3, "ST_F3"},
    {ST_F4, "ST_F4"},
    {ST_F5, "ST_F5"},
    {ST_F6, "ST_F6"},
    {ST_F7, "ST_F7"},
    {ST_F8, "ST_F8"},
    {ST_F9, "ST_F9"},
    {ST_F10, "ST_F10"},
    {ST_A, "ST_A"},
    {ST_B, "ST_B"},
    {ST_C, "ST_C"},
    {ST_D, "ST_D"},
    {ST_E, "ST_E"},
    {ST_F, "ST_F"},
    {ST_G, "ST_G"},
    {ST_H, "ST_H"},
    {ST_I, "ST_I"},
    {ST_J, "ST_J"},
    {ST_K, "ST_K"},
    {ST_L, "ST_L"},
    {ST_M, "ST_M"},
    {ST_N, "ST_N"},
    {ST_O, "ST_O"},
    {ST_P, "ST_P"},
    {ST_Q, "ST_Q"},
    {ST_R, "ST_R"},
    {ST_S, "ST_S"},
    {ST_T, "ST_T"},
    {ST_U, "ST_U"},
    {ST_V, "ST_V"},
    {ST_W, "ST_W"},
    {ST_X, "ST_X"},
    {ST_Y, "ST_Y"},
    {ST_Z, "ST_Z"},
    {ST_SPACE, "ST_SPACE"},
    {ST_ALT, "ST_ALT"},
    {ST_ESC, "ST_ESC"},
    {ST_MINUS, "ST_MINUS"},
    {ST_EQUAL, "ST_EQUAL"},
    {ST_GRAVE, "ST_GRAVE"},
    {ST_BS, "ST_BS"},
    {ST_DELETE, "ST_DELETE"},
    {ST_ISO, "ST_ISO"},
    {ST_INSERT, "ST_INSERT"},
    {ST_TAB, "ST_TAB"},
    {ST_SQ_OPEN, "ST_SQ_OPEN"},
    {ST_SQ_CLOSE, "ST_SQ_CLOSE"},
    {ST_UP, "ST_UP"},
    {ST_LEFT, "ST_LEFT"},
    {ST_RIGHT, "ST_RIGHT"},
    {ST_DOWN, "ST_DOWN"},
    {ST_CONTROL, "ST_CONTROL"},
    {ST_LSH, "ST_LSH"},
    {ST_RSH, "ST_RSH"},
    {ST_CAPSLOCK, "ST_CAPSLOCK"},
    {ST_SEMIC, "ST_SEMIC"},
    {ST_APOST, "ST_APOST"},
    {ST_BACKSL, "ST_BACKSL"},
    {ST_COMMA, "ST_COMMA"},
    {ST_DOT, "ST_DOT"},
    {ST_SLASH, "ST_SLASH"},
    {ST_RETURN, "ST_RETURN"},
    {ST_KP_ENTER, "ST_KP_ENTER"},
    {ST_KP_MINUS, "ST_KP_MINUS"},
    {ST_KP_PLUS, "ST_KP_PLUS"},
    {ST_HELP, "ST_HELP"},
    {ST_UNDO, "ST_UNDO"},
    {ST_KP_OPEN, "ST_KP_OPEN"},
    {ST_KP_CLOSE, "ST_KP_CLOSE"},
    {ST_KP_DIV, "ST_KP_DIV"},
    {ST_KP_MULT, "ST_KP_MULT"},
    {ST_KP_DOT, "ST_KP_DOT"},
    {ST_KP_0, "ST_KP_0"},
    {ST_KP_1, "ST_KP_1"},
    {ST_KP_2, "ST_KP_2"},
    {ST_KP_3, "ST_KP_3"},
    {ST_KP_4, "ST_KP_4"},
    {ST_KP_5, "ST_KP_5"},
    {ST_KP_6, "ST_KP_6"},
    {ST_KP_7, "ST_KP_7"},
    {ST_KP_8, "ST_KP_8"},
    {ST_KP_9, "ST_KP_9"},
    {ST_HOME, "ST_HOME"},
    {ST_END, "ST_END"},
    {ST_PG_UP, "ST_PG_UP"},
    {ST_PG_DN, "ST_PG_DN"}
};

#define NUM_STK (sizeof(st_keysyms)/sizeof(st_keysyms[0]))
static char mapped[NUM_STK];
extern char **ksyms;

void x_init_keys(void)
{
    int i, l=0, j;
    FILE *f = NULL;
    char b[1000], x[1000], y[1000];
    char kdefsfile[512];
    keycodes = (int *)malloc(sizeof(int)*(kmax+1));
    for (i=0; i<=kmax; i++)
	keycodes[i] = ST_UNDEF;
    if ( key_filename ) {
	strncpy( kdefsfile, key_filename , 512);	/* Important: Use strNcpy to prevent buffer overflows! */
	f = fopen( kdefsfile, "r" );
    } 
    else 
    {
	char *home=getenv("HOME");
	if ( home )
	{
	    strncpy( kdefsfile, home , 512-16);
	    strcat( kdefsfile, "/.stonx.keysyms" );
	    f = fopen( kdefsfile, "r" );
	}
	if ( !f )
	{
	    strcpy( kdefsfile, STONXETC);
	    strcat( kdefsfile, "/keysyms" );
	    f = fopen( kdefsfile, "r" );
	}
    }
    fprintf(stderr,"Reading keycode mappings from `%s'...\n", kdefsfile);
    if (f == NULL)
    {
	fprintf(stderr,"FATAL error: File `%s' not found - exiting...\n",
		kdefsfile);
	exit(1);
    }
    while (fgets(b, 1000, f) != NULL)
    {
	l++;
	if (b[0] == '#') continue;
	if (sscanf(b, "%s %s", x, y) < 2)
	{
	    fprintf (stderr,"Malformed line %d in file %s, ignoring it\n",
		     l, kdefsfile);
	}
	else 
	{
	    int q;
	    int ok=0, u;
	    for (j=kmin; j<=kmax; j++)
	    {
#if KB_DEBUG
		if (*x == 'F')
		{
		    DBG("x=<%s>, ksyms[%d]=<%s>, cmp=%d\n",
			x,j-kmin,ksyms[j-kmin],(ksyms[j-kmin]==NULL?999:strcasecmp(x,ksyms[j-kmin])));
		}
#endif
  	        if (ksyms[j-kmin] != NULL && strcasecmp (x, ksyms[j-kmin]) == 0) {
		  q = /*XKeysymToKeycode(display,
			XStringToKeysym(ksyms[*/ j /* -kmin ]))*/; 
		  for (u=0; u<sizeof(st_keysyms)/sizeof(st_keysyms[0]); u++) {
		    if (strcasecmp (y, st_keysyms[u].s) == 0) {
		      keycodes[q] = st_keysyms[u].c;
		      mapped[u] = 1;
		      break;
		    }
		  }
		  
		  if (u == sizeof(st_keysyms)/sizeof(st_keysyms[0])) {
		    
		    if(( sscanf(y, "%d", &keycodes[q]) == 1) ||
		       ( sscanf(y, "0x%x", &keycodes[q]) == 1)) {
		      for (u=0; u<sizeof(st_keysyms)/sizeof(st_keysyms[0]); u++) {
			if(st_keysyms[u].c == keycodes[q])
			  mapped[u] = 1;
		      }
		    } else {		
		      fprintf(stderr,
			      "Error: Illegal ST-Keysym in %s, line %d `%s'\n",
			      kdefsfile, l, y);
		      exit(1);
		    }
		  }
		  ok=1;
		}
	    }
	    if (!ok)
	    {
		fprintf (stderr,
			 "Warning: unknown KeySym in %s, line %d `%s'\n",
			 kdefsfile, l, x);
		continue;
	    }
	}
    }
    for (i=0; i<NUM_STK; i++)
    {
	if (!mapped[i] && verbose)
	    fprintf (stderr,"Warning: %s not mapped!\n", st_keysyms[i].s);
    }
    SM_UB(MEM(0xfffc00),0x0e);
}
