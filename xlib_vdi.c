/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 *
 * BUGS:
 * - vro_cpyfm probabyl doesn't work properly when fancy modes are used for
 *   screen->memory copying
 * - user-defined fill patterns are global (?)
 *
 * - need to 1) debug and 2) optimize the raster ops (in that order)
 *
 * - should use own vqt_attributes() if custom screen size looks better with
 *   larger font (really? maybe I just need to patch the appropriate variables)
 */
#include "defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "mem.h"
#include "utils.h"
#include "screen.h"
#include "xlib_vdi.h"
#include "debug.h"
#include "main.h"

extern Display *display;

extern int indexed_color;
extern unsigned long pixel_value_by_index[COLS];

#ifndef TRACE_VDI
#define TRACE_VDI 0
#endif
#define UPDATE_PHYS 0
#define DUAL_MODE 1
#define CALL_ROM_VDI !(UPDATE_PHYS || DUAL_MODE)

#if TRACE_VDI
#define debug(_args...) fprintf( stderr, _args )
#define V(_x) debug _x 
static UL last_pblock;
#else
#define V(_x) 
#endif


#if XBUFFER
#define XBUF(_x) _x
#else
#define XBUF(_x)
#endif



/* Dual mode: must execute all attribute functions etc. in TOS too!


 */

#define MAX_VWK 100
#define MAX_POINTS 1000
#define MAX_VDI_COLS	256

#define FIX_COLOR(_c) (priv_cmap ? vdi_maptab[(_c)&(MAX_VDI_COLS-1)]\
					: mapcol[vdi_maptab[(_c)&(MAX_VDI_COLS-1)]])

#define LAST_SCREEN_DEVICE 10

#define CHAR_HEIGHT() ((vdi && scr_height >= 400) || scr_planes==1 ? 16 : 8)


#define DEV_LOCATOR 1
#define DEV_VALUATOR 2
#define DEV_CHOICE 3
#define DEV_STRING 4
#define MODE_UNDEFINED 0
#define MODE_REQUEST 1
#define MODE_SAMPLE 2

#define WHITE 0
#define BLACK 1

typedef struct
{
	GC gc;
	int phys_wk;

	int wrmode;
	int clipping;
	XRectangle clipr;
	int mode[4];

	int line_color;
	int line_type;
	int line_pattern;
	int line_width;
	int line_ends;
	char ud_linepat[16];
	int ud_dashlen;
	int ud_dashoff;
	
	int text_color;
	int text_rotation;
	int font_index;
	int font_yoff;
	int font_ver;
	int font_hor;
	int font_eff;

	int fill_color;
	int fill_interior;
	int fill_style;
	int fill_perimeter;
	Pixmap ud_fill_pattern;
	
	int marker_color;
	int marker_type;
	int marker_height;
} VWK;

static int vdi_maptab16[] = { 0, 15, 1, 2, 4, 6, 3, 5, 7, 8, 9, 10, 12, 14, 11, 13 };

static int vdi_maptab[MAX_VDI_COLS];
extern char mapcol[];
unsigned int black_pixel, white_pixel;
extern Window imagewin;
extern Colormap cmap;
extern Cursor cursors[2];

UL abase;
UL fontbase;
Window xw = None;

static int vdi_planes;
static VWK *vwk[MAX_VWK];
static XImage *xi;
static XImage *cxi;
static XImage *subimage;
static char *subimage_data;
static GC cgc;
static GC rgc;
static Pixmap tmpx, cmask, cdata, xbuffer;
static int screen;
static Visual *visual;

#define SYSFONTS 3
static XFontStruct *sysfont[SYSFONTS];
static char *sysfontname[SYSFONTS] =
{
#if 0
	"-*-lucidatypewriter-medium-r-*-*-10-*-*-*-m-*-*-*",
	"-*-lucidatypewriter-medium-r-*-*-12-*-*-*-m-*-*-*",
	"-*-lucidatypewriter-medium-r-*-*-14-*-*-*-m-*-*-*"
#else
	"6x6 system font-8-1",
	"8x8 system font-9-1",
	"8x16 system font-10-1"
#endif
};

typedef struct
{
	UB height;
	UB cellheight;
	UB top;
	UB ascent;
	UB half;
	UB descent;
	UB bottom;
	UB width;
	UB cellwidth;
	UB left_offset;
	UB right_offset;
	UB point;
} sysfontinfo;

static sysfontinfo sfinfo[3] =
{
	{4, 6, 4, 4, 3, 1, 1, 5, 6, 0, 0, 8},
	{6, 8, 6, 6, 4, 1, 1, 7, 8, 0, 0, 9},
	{13, 16, 13, 11, 8, 2, 2, 7, 8, 0, 0, 10},
};

#define IGNORE(_x) static int _x(void){V(("%s[%d] ignored\n", #_x, V_HANDLE )); return FALSE; }
#define NYI(_x) static int _x(void){V(("%s[%d] NOT YET IMPLEMENTED\n", #_x, V_HANDLE )); return FALSE; }


#define vdi_w scr_width
#define vdi_h scr_height

static Window root;
static int phys_handle = -1;

#include "fill.xbm"
static Pixmap *fillpat[5];
static int patnum[] = {1, 1, 24, 12, 1};

/*
    TODO:

Opcode      VDI Name                   Function

    1       v_opnwk                    Open Workstation
    2       v_clswk                    Close Workstation
    3       v_clrwk                    Clear Workstation
    4       v_updwk                    Update Workstation
    5       v_escape                   Escape Functions
        1   vq_chcells                 Inquire addressable character cells
        2   v_exit_cur                 Exit alpha mode
        3   v_enter_cur                Enter alpha mode
        4   v_curup                    Cursor up
        5   v_curdown                  Cursor down
        6   v_curright                 Cursor right
        7   v_curleft                  Cursor left
        8   v_curhome                  Home Cursor
        9   v_eeos                     Erase to end of screen
       10   v_eeol                     Erase to end of line
       11   vs_curaddress              Direct cursor address
       12   v_curtext                  Output cursor addressable text
       13   v_rvon                     Start reverse video
       14   v_rvoff                    End reverse video
       15   vq_curaddress              Inquire current alpha cursor address
       16   vq_tabstatus
       17   v_hardcopy
       18   v_dspcur                   Place graphic cursor
       19   v_rmcur                    Remove last graphic cursor
       20   v_form_adv                 Eject page
       21   v_output_window
       22   v_clear_disp_list
       23   v_bit_image
       61   v_sound
       62   vs_mute
       77   vq_calibrate
       81   vt_resolution
       82   vt_axis
       83   vt_origin
       84   vq_tdimensions
       85   vt_alignment
       98   v_meta_extents
       99   v_write_meta
       99,0 vm_pagesize
       99,1 vm_coords
       100  vm_filename
       101  v_offset
       102  v_fontinit
       2000 v_escape2000
    6       v_pline                    Polyline
    7       v_pmarker                  Polymarker
    8       v_gtext                    Text
    9       v_fillarea                 Filled area
   11                                  GDP
        1   v_bar                      Bar
        2   v_arc                      Arc
        3   v_pieslice                 Pie
        4   v_circle                   Circle
        5   v_ellipse                  Ellipse
        6   v_ellarc                   Elliptical arc
        7   v_ellpie                   Elliptical pie
        8   v_rbox                     Rounded rectangle
        9   v_rfbox                    Filled rounded rectangle
       10   v_justified                Justified graphics text
    12      vst_height                 Set character height, absolute mode
    13      vst_rotation               Set text rotation
    14      vs_color                   Set color representation
    15      vsl_type                   Set polyline linetype
    16      vsl_width                  Set polyline width
    17      vsl_color                  Set polyline color index
    18      vsm_type                   Set polymarker type
    19      vsm_height                 Set polymarker height
    20      vsm_color                  Set polymarker color index
    21      vst_font                   Set text face
    22      vst_color                  Set graphics text color index
    23      vsf_interior               Set fill interior style
    24      vsf_style                  Set fill style index
    25      vsf_color                  Set fill color index
    26      vq_color                   Inquire color representation
    27      vq_cellarray               Inquire color lookup table
    28      vrq_locator,vsm_locator    Input locator
    29      vrq_valuator,vsm_valuator  
    30      vrq_choice,vsm_choice
    31      vrq_string,vsm_string      Input string
    32      vswr_mode                  Set writing mode
    33      vsin_mode                  Set input mode
    34
    35      vql_attribues              Inquire current polyline attributes
    36      vqm_attributes             Inquire current polymarker attributes
    37      vqf_attributes             Inquire current fill area attributes
    38      vqt_attributes             Inquire current graphic text attributes
    39      vst_alignment              Set graphic text alignment
   100      v_opnvwk                   Open virtual screen workstation
   101      v_clsvwk                   Close virtual screen workstation
   102      vq_extnd                   Extended inquire function
   103      v_contourfill
   104      vsf_perimeter              Set fill perimeter visibility
   105      v_get_pixel
   106      vst_effects                Set graphic text special effects
   107      vst_point                  Set character cell height, points mode
   108      vsl_ends                   Set polyline end styles
   109      vro_cpyfm                  Copy raster, opaque
   110      vr_trnfm                   Transform form
   111      vsc_form                   Set mouse form
   112      vsf_udpat                  Set user-defined fill pattern
   113      vsl_udsty                  Set user-defined linestyle
   114      vr_recfl                   Fill rectangle
   115      vqin_mode                  Inquire input mode
   116      vqt_extent                 Inquire text extent
   117      vqt_width                  Inquire character cell width
   118      vex_timv                   Exchange timer interrupt vector
   119      vst_load_fonts
   120      vst_unload_fonts
   121      vrt_cpyfm                  Copy raster, transparent
   122      v_show_c                   Show cursor
   123      v_hide_c                   Hide cursor
   124      vq_mouse                   Sample mouse button state
   125      vex_butv                   Exchange button change vector
   126      vex_motv                   Exchange mouse movement vector
   127      vex_curv                   Exchange cursor change vector
   128      vq_key_s                   Sample keyboard state information
   129      vs_clip                    Set clipping rectangle
   130      vqt_name                   Inquire face name and index
   131      vqt_fontinfo               Inquire current face information

*/

static UW work_out_buf[128];

static UL control, intin, ptsin, intout, ptsout;

#define CONTRL(_x) MEM(control+2*(_x))
#define INTIN(_x) MEM(intin+2*(_x))
#define PTSIN(_x) MEM(ptsin+2*(_x))
#define INTOUT(_x) MEM(intout+2*(_x))
#define PTSOUT(_x) MEM(ptsout+2*(_x))

#define V_OPCODE     LM_W(CONTRL(0))
#define V_NPTSIN     LM_W(CONTRL(1))
#define V_NPTSOUT(x) SM_W(CONTRL(2),x)
#define V_NINTIN     LM_W(CONTRL(3))
#define V_NINTOUT(x) SM_W(CONTRL(4),x)
#define V_SUBCODE    LM_W(CONTRL(5))
#define V_HANDLE     LM_W(CONTRL(6))

#define CHECK_V_HANDLE(h) \
	if (h < 0 || h >= MAX_VWK || vwk[h] == NULL) \
	{ \
		fprintf(stderr, "invalid vdi handle %d\n", h); \
		V(("invalid vdi handle %d\n", h)); \
		return FALSE; \
	} \
	if (vwk[h]->phys_wk != phys_handle) /* not a screen device: NYI */ \
		return FALSE;
	

#define MD_REPLACE 1
#define MD_TRANS   2
#define MD_XOR     3
#define MD_ERASE   4

#define FIS_HOLLOW  0
#define FIS_SOLID   1
#define FIS_PATTERN 2
#define FIS_HATCH   3
#define FIS_USER    4

#if TRACE_VDI
static char *interior_name[5] = { "HOLLOW", "SOLID", "PATTERN", "HATCH", "USER" };
static char *wrmode_name[5] = { "", "REPLACE", "TRANS", "XOR", "ERASE" };
static char *function_name[16] = {
	"GXclear", "GXand", "GXandReverse", "GXcopy",
	"GXandInverted", "GXnoop", "GXxor", "GXor",
	"GXnor", "GXequiv", "GXinvert", "GXorReverse",
	"GXcopyInverted", "GXorInverted", "GXnand", "GXset"
};
#endif

NYI(nothing)

static int get_wrmode(int mode, int fg)
{
	switch (mode)
	{
	case MD_REPLACE:
		return GXcopy;
	case MD_TRANS:
#if TRACE_VDI
	    V(("FIX_COLOR(%d) = %d, mode = %s\n",fg,FIX_COLOR(fg),FIX_COLOR(fg) == 0 ? "GXand" : "GXor"));
#endif
		return (FIX_COLOR(fg) == 0 ? GXand : GXor);
	case MD_XOR:
		return GXxor;
	case MD_ERASE:
	default:
		return GXandInverted;
	}
}

void vdi_redraw(int x, int y, int w, int h)
{
	V(("Redrawing (%d,%d,%d,%d)\n",x,y,w,h));
	XCopyArea(display, xbuffer, xw, rgc, x, y, w, h, x, y);
}


static void catchsig(int sig)
{
	int x, y;
	long data;
	XImage *image;
	FILE *f;
	Pixmap pixmap;
	GC gc;
	XGCValues gv;
	
	f = fopen("/tmp/STonx.debug", "w");
	if  (f)
	{
		image = XCreateImage(display, visual, depth, ZPixmap, 0, (char *)&data, 1, 1, 32, 1);
		pixmap = XCreatePixmap(display, xw, 1, 1, depth);
		gv.foreground = black_pixel;
		gv.background = white_pixel;
		gv.function = GXcopy;
		gv.plane_mask = AllPlanes;
		gv.graphics_exposures = False;
		gc = XCreateGC(display, xw, GCPlaneMask|GCGraphicsExposures|GCFunction|GCForeground|GCBackground, &gv);
		
		for (y = 0; y < 19; y++)
		{
			for (x = 0; x < 200; x++)
			{
				fprintf(f, "%4d %4d: ", y, x);

				data = 0;
				XGetSubImage(display, xw, x, y, 1, 1, AllPlanes, ZPixmap, image, 0, 0);
				fprintf(f, "%ld ", data);

				data = 0;
				XGetSubImage(display, xbuffer, x, y, 1, 1, AllPlanes, ZPixmap, image, 0, 0);
				fprintf(f, "%ld ", data);

#if 0
				data = 0;
				XCopyArea(display, xw, pixmap, gc, x, y, 1, 1, 0, 0);
				XGetSubImage(display, pixmap, x, y, 1, 1, AllPlanes, ZPixmap, image, 0, 0);
				fprintf(f, "%ld ", data);

				data = 0;
				XCopyArea(display, xbuffer, pixmap, gc, x, y, 1, 1, 0, 0);
				XGetSubImage(display, pixmap, x, y, 1, 1, AllPlanes, ZPixmap, image, 0, 0);
				fprintf(f, "%ld ", data);
#endif
				
				fprintf(f, "\n");
			}
		}
		image->data = NULL;
		XDestroyImage(image);
		XFreePixmap(display, pixmap);
		XFreeGC(display, gc);
		fclose(f);
	}
	signal(sig, catchsig);
}


static unsigned long get_pixel(int x, int y)
{
	long data;
	XImage *image;
	
	image = XCreateImage(display, visual, depth, ZPixmap, 0, (char *)&data, 1, 1, 32, 1);
	data = 0;
	XGetSubImage(display, xw, x, y, 1, 1, AllPlanes, ZPixmap, image, 0, 0);
	image->data = NULL;
	XDestroyImage(image);
	return data;
}


static void check_pixel(void)
{
	if (get_pixel(25, 3) == 5)
	{
		V(("PIXEL == 5\n"));
	}
}


static void print_pixel(void)
{
	V(("pixel = %ld\n", get_pixel(25, 3)));
}


void vdi_init(void)
{
	int i, j, k = 0;
	XGCValues gv;
	Window id;
	
	signal(SIGUSR2, catchsig);
	{
		char **path, **new_path;
		int num_path;
		
		path = XGetFontPath(display, &num_path);
		for (i = 0; i < num_path; i++)
		        if (strcmp(path[i], STONXDIR) == 0) /* FIXME: STONXDIR */
				break;
		if (i >= num_path)
		{
			new_path =(char **) malloc((num_path + 1) * sizeof(char *));
			for (i = 0; i < num_path; i++)
				new_path[i] = path[i];
			new_path[i] = STONXDIR;
			XSetFontPath(display, new_path, num_path + 1);
			free(new_path);
		}
		XFreeFontPath(path);
	}
	
	id = xw ? xw : imagewin;
	for (i = 0; i < 5; i++)
	{
		fillpat[i] = (Pixmap *)malloc(sizeof(Pixmap) * patnum[i]);
		for (j = 0; j < patnum[i]; j++)
		{
			fillpat[i][j] = XCreateBitmapFromData(display, id, &fill_bits[32 * k], 16, 16);
			k++;
		}
	}
	for (i = 0; i < SYSFONTS; i++)
	{
		sysfont[i] = XLoadQueryFont(display, sysfontname[i]);
		if (sysfont[i] == 0)
			cleanup(2);
	}
	
	cxi = XCreateImage(display, visual, 1, XYBitmap, 0, NULL, 16, 16, 16, 2);
	cmask = XCreatePixmap(display, id, 16, 16, 1);
	cdata = XCreatePixmap(display, id, 16, 16, 1);
	gv.foreground = FIX_COLOR(BLACK);
#if TRACE_VDI
	    V(("FIX_COLOR(BLACK=%d) = %d; ",BLACK,FIX_COLOR(BLACK)));
#endif
	gv.background = FIX_COLOR(WHITE);
#if TRACE_VDI
	    V(("FIX_COLOR(WHITE=%d) = %d\n",WHITE,FIX_COLOR(WHITE)));
#endif
	cgc = XCreateGC(display, cmask, GCForeground | GCBackground, &gv);

	white_pixel = WhitePixel(display,XDefaultScreen(display)); /* 2001-02-27 (Thothy): These variables were not set, so vsc_form did not work */
	black_pixel = BlackPixel(display,XDefaultScreen(display)); /* Perhaps there is a better solution by replacing them with FIX_COLOR one day */
}


void vdi_exit(void)
{
	int i, j;
	
	if (xw != None)
	{
		XDestroyWindow(display, xw);
		if (imagewin == xw)
			imagewin = None;
		xw = None;
	}
	if (cxi)
	{
		cxi->data = NULL;
		XDestroyImage(cxi);
		cxi = NULL;
	}
	if (cmask != None)
	{
		XFreePixmap(display, cmask);
		cmask = None;
	}
	if (cdata != None)
	{
		XFreePixmap(display, cdata);
		cdata = None;
	}
	if (cgc != None)
	{
		XFreeGC(display, cgc);
		cgc = None;
	}
	for (i = 0; i < 5; i++)
	{
		if (fillpat[i])
		{
			for (j = 0; j < patnum[i]; j++)
				if (fillpat[i][j])
					XFreePixmap(display, fillpat[i][j]);
			free(fillpat[i]);
			fillpat[i] = NULL;
		}
	}
	for (i = 0; i < SYSFONTS; i++)
	{
		if (sysfont[i])
		{
			XFreeFont(display, sysfont[i]);
			sysfont[i] = NULL;
		}
	}
}


static void destroy_image(void)
{
	if (xi != None)
	{
		xi->data = NULL;
		XDestroyImage(xi);
		xi = None;
	}
	if (subimage_data != NULL)
	{
		free(subimage_data);
		subimage_data = NULL;
	}
	if (subimage != None)
	{
		subimage->data = NULL;
		XDestroyImage(subimage);
		subimage = None;
	}
	
	if (rgc != None)
	{
		XFreeGC(display, rgc);
		rgc = None;
	}

	if (xbuffer != None)
	{
		XFreePixmap(display, xbuffer);
		xbuffer = None;
	}
	
	if (tmpx != None)
	{
		XFreePixmap(display, tmpx);
		tmpx = None;
	}
}


static void change_mode(void)
{
	XResizeWindow(display, xw, vdi_w, vdi_h);

	destroy_image();
	
	xi = XCreateImage(display, visual, 1, XYBitmap, 0, NULL,
		vdi_w, vdi_h, 16, vdi_w / 8);
	xi->bitmap_bit_order = MSBFirst;
	xi->byte_order = MSBFirst;
	
	subimage_data = (char *)malloc((vdi_w * vdi_h * depth) / 8);
	subimage = XCreateImage(display, visual, depth,
		depth == 1 ? XYBitmap : ZPixmap, 0, subimage_data,
		vdi_w, vdi_h, 32, (vdi_w * depth) / 8);

	rgc = XCreateGC(display, xw, 0, NULL);
#if XBUFFER
	xbuffer = XCreatePixmap(display, xw, vdi_w, vdi_h, depth);
	XFillRectangle(display, xbuffer, rgc, 0, 0, vdi_w, vdi_h);
	XCopyArea(display, xw, xbuffer, rgc, 0, 0, vdi_w, vdi_h, 0, 0);
	V(("xbuffer: %lx\n", xbuffer));
#endif
	
	if (vdi_planes == 1)
		tmpx = XCreatePixmap(display, xw, vdi_w, vdi_h, 1);
	else
		tmpx = XCreatePixmap(display, xw, vdi_w, vdi_h, depth);
	XResizeWindow(display, xw, vdi_w, vdi_h);
}


static void clear_window(void)
{
	XSetForeground(display, rgc, FIX_COLOR(WHITE));
#if TRACE_VDI
	    V(("FIX_COLOR(WHITE=%d) = %d; ",WHITE,FIX_COLOR(WHITE)));
#endif
	XFillRectangle(display, xw, rgc, 0, 0, vdi_w, vdi_h);
	XBUF(XFillRectangle(display, xbuffer, rgc, 0, 0, vdi_w, vdi_h);)
	XSetForeground(display, rgc, FIX_COLOR(BLACK));
#if TRACE_VDI
	    V(("FIX_COLOR(BLACK=%d) = %d\n",BLACK,FIX_COLOR(BLACK)));
#endif
}


static void init_window(void)
{
	screen = DefaultScreen(display);
	root = RootWindow(display, screen);
	visual = DefaultVisual(display, screen);

	if (xw == None)
	{
#if UPDATE_PHYS
		xw = XCreateSimpleWindow(display, root, 0, 0, vdi_w, vdi_h, 0, black_pixel, white_pixel);
#else
		xw = imagewin;
#endif
	}
	change_mode();

	if (imagewin != xw)
	{
		V(("xw: %lx\n", xw));
		XSelectInput(display, xw, KeyPressMask | KeyReleaseMask | ButtonPressMask \
			|ButtonReleaseMask | PointerMotionMask | ColormapChangeMask | \
			EnterWindowMask | LeaveWindowMask | ExposureMask | StructureNotifyMask);
		XMapWindow(display, xw);
#if 1
		if (cmap)
			XSetWindowColormap(display, xw, cmap);
#endif
#if 0
		{
			XEvent e;
	
			XWindowEvent(display, xw, StructureNotifyMask, &e);
		}
#endif
	}
	set_window_name(xw, "STonX VDI Window");
	clear_window();
}

static void init_wk(W * w_in, W * w_out1, W * w_out2)
{
	int i;

	SM_W(w_out1 + 0, vdi_w - 1);			/* maximum horizontal position */
	SM_W(w_out1 + 1, vdi_h - 1);			/* maximum vertical position */
	SM_W(w_out1 + 2, 0);					/* scaling flag */
	SM_W(w_out1 + 3, 372);					/* pixel width */
	SM_W(w_out1 + 4, 372);					/* pixel height */
	SM_W(w_out1 + 5, 3);					/* number of font sizes */
	SM_W(w_out1 + 6, 7);					/* number of line types */
	SM_W(w_out1 + 7, 0);					/* number of line widths */
	SM_W(w_out1 + 8, 6);					/* number of marker types */
	SM_W(w_out1 + 9, 8);					/* number of marker sizes */
	SM_W(w_out1 + 10, 1);					/* number of fonts */
	SM_W(w_out1 + 11, 24);					/* number of patterns */
	SM_W(w_out1 + 12, 12);					/* number of shapes */
	SM_W(w_out1 + 13, 1 << vdi_planes);		/* number of colors XXX TODO -> Get default visual */
	SM_W(w_out1 + 14, 10);					/* GDPs */
	for (i = 1; i <= 10; i++)
		SM_W(w_out1 + 14 + i, i);
	SM_W(w_out1 + 25, 3);					/* GDP attributes */
	SM_W(w_out1 + 26, 0);
	SM_W(w_out1 + 27, 3);
	SM_W(w_out1 + 28, 3);
	SM_W(w_out1 + 29, 3);
	SM_W(w_out1 + 30, 0);
	SM_W(w_out1 + 31, 3);
	SM_W(w_out1 + 32, 0);
	SM_W(w_out1 + 33, 3);
	SM_W(w_out1 + 34, 2);
	SM_W(w_out1 + 35, vdi_planes > 1);		/* colors available */
	SM_W(w_out1 + 36, 1);					/* text rotation available */
	SM_W(w_out1 + 37, 1);					/* fill area available */
	SM_W(w_out1 + 38, 0);					/* CELLARRAY available */
	SM_W(w_out1 + 39, vdi_planes == 1 ? 2 : 0);					/* number of available colors */
	SM_W(w_out1 + 40, 2);					/* cursor control */
	SM_W(w_out1 + 41, 1);					/* valuator control */
	SM_W(w_out1 + 42, 1);					/* choice control */
	SM_W(w_out1 + 43, 1);					/* string control */
	SM_W(w_out1 + 44, 2);					/* device type */
	SM_W(w_out2 + 0, 5);					/* XXX min. char width */
	SM_W(w_out2 + 1, 4);					/* XXX min. baseline-topline */
	SM_W(w_out2 + 2, 7);					/* XXX max. char width */
	SM_W(w_out2 + 3, 13);					/* XXX max. baseline-topline */
	SM_W(w_out2 + 4, 1);					/* XXX min. line width */
	SM_W(w_out2 + 5, 0);
	SM_W(w_out2 + 6, 40);					/* XXX max. line width */
	SM_W(w_out2 + 7, 0);
	SM_W(w_out2 + 8, 15);					/* XXX min. marker width */
	SM_W(w_out2 + 9, 11);					/* XXX min. marker height */
	SM_W(w_out2 + 10, 120);					/* XXX max. marker width */
	SM_W(w_out2 + 11, 88);					/* XXX max. marker height */
}

static void make_rectangle(int x1, int y1, int x2, int y2, XRectangle * r)
{
	if (x1 < x2)
	{
		r->x = x1;
		r->width = x2 - x1 + 1;
	} else
	{
		r->x = x2;
		r->width = x1 - x2 + 1;
	}
	if (y1 < y2)
	{
		r->y = y1;
		r->height = y2 - y1 + 1;
	} else
	{
		r->y = y2;
		r->height = y1 - y2 + 1;
	}
}

static void set_clipping(VWK * v)
{
	if (v->clipping)
		XSetClipRectangles(display, v->gc, 0, 0, &(v->clipr), 1, Unsorted);
	else
		XSetClipMask(display, v->gc, None);
}

static void done_raster(VWK * v, int oldmode)
{
	XSetFunction(display, v->gc, oldmode);
	set_clipping(v);
}

static void set_fontparms(int h)
{
	switch (vwk[h]->font_ver)
	{
	case 0:
		vwk[h]->font_yoff = 0;
		break;
	case 1:
		vwk[h]->font_yoff = sfinfo[vwk[h]->font_index].half;
		break;
	case 2:
		vwk[h]->font_yoff = sfinfo[vwk[h]->font_index].ascent;
		break;
	case 3:
		vwk[h]->font_yoff = -sfinfo[vwk[h]->font_index].descent;
		break;
	case 4:
		vwk[h]->font_yoff = -sfinfo[vwk[h]->font_index].bottom;
		break;
	case 5:
		vwk[h]->font_yoff = sfinfo[vwk[h]->font_index].top;
		break;
	}
}

static void change_font(int h, int i)
{
	XSetFont(display, vwk[h]->gc, sysfont[i]->fid);
	vwk[h]->font_index = i;
	set_fontparms(h);
}

static void init_font(VWK * v)
{
	XGCValues gv;

	gv.function = get_wrmode(v->wrmode, v->text_color);
	gv.foreground = FIX_COLOR(v->text_color);
#if TRACE_VDI
	    V(("FIX_COLOR(%d) = %d; ",v->text_color,FIX_COLOR(v->text_color)));
#endif
	gv.background = FIX_COLOR(WHITE);
#if TRACE_VDI
	    V(("FIX_COLOR(WHITE=%d) = %d\n",WHITE,FIX_COLOR(WHITE)));
#endif
	gv.fill_style = FillSolid;
	V(("  init_font: function = %s, fg=%lx, bg=%lx\n", function_name[gv.function], gv.foreground, gv.background));
	XChangeGC(display, v->gc,
		GCBackground | GCForeground | GCFillStyle | GCFunction, &gv);
}

static void draw_text(VWK * v, int x, int y,
	XChar2b * items, int n)
{
#if 0
	XSetFunction(display, v->gc, GXcopy);
#endif
	print_pixel();
	init_font(v);
	if (v->wrmode != MD_TRANS)
	{
		XDrawImageString16(display, xw, v->gc, x, y, items, n);
		XBUF(XDrawImageString16(display, xbuffer, v->gc, x, y, items, n);)
	} else
	{
		XDrawString16(display, xw, v->gc, x, y, items, n);
		XBUF(XDrawString16(display, xbuffer, v->gc, x, y, items, n);)
	}
	check_pixel();
	if (v->font_eff & 1)
	{									/* dodgy */
		XDrawString16(display, xw, v->gc, x + 1, y, items, n);
		XBUF(XDrawString16(display, xbuffer, v->gc, x + 1, y, items, n);)
	}
	if (v->font_eff & 8)
	{
		int len, ly;

		len = n * sfinfo[v->font_index].cellwidth;
		ly = y + sfinfo[v->font_index].descent - 1;
		XDrawLine(display, xw, v->gc, x, ly, x + len, ly);
		XBUF(XDrawLine(display, xbuffer, v->gc, x, ly, x + len, ly);)
	}
}

static void draw_jtext(VWK * v, int x, int y,
	XTextItem16 * items, int n)
{
	XDrawText16(display, xw, v->gc, x, y, items, n);
	XBUF(XDrawText16(display, xbuffer, v->gc, x, y, items, n);)
	if (v->font_eff & 1)
	{									/* dodgy */
		XDrawText16(display, xw, v->gc, x + 1, y, items, n);
		XBUF(XDrawText16(display, xbuffer, v->gc, x + 1, y, items, n);)
	}
	if (v->font_eff & 8)
	{
		int len, ly;

		len = n * sfinfo[v->font_index].cellwidth;
		ly = y + sfinfo[v->font_index].descent - 1;
		XDrawLine(display, xw, v->gc, x, ly, x + len, ly);
		XBUF(XDrawLine(display, xbuffer, v->gc, x, ly, x + len, ly);)
	}
}


static char user_defined_style[16];

static char *dashes[] =
{
	"", "\14\4", "\3\5\3\5", "\7\3\3\3", "\10\10", "\4\3\2\2\2\3",
	user_defined_style
};
static int dashlen[] = {0, 2, 4, 4, 2, 6, 16};
static int dashoff[] = {0, 0, 0, 0, 0, 0, 0};
static int capstyle[] =
{
	CapButt, CapButt, CapRound,
	CapButt, CapButt, CapRound,
	CapRound, CapRound, CapRound
};

static void init_line(VWK * v)
{
	XGCValues gv;
	int type = v->line_type;
	int dashl, dasho;
	char *dashs;

	gv.function = get_wrmode(v->wrmode, v->line_color);
	gv.foreground = FIX_COLOR(v->line_color);
#if TRACE_VDI
	    V(("FIX_COLOR(%d) = %d; ",v->line_color,FIX_COLOR(v->line_color)));
#endif
	gv.background = FIX_COLOR(WHITE);
#if TRACE_VDI
	    V(("FIX_COLOR(WHITE=%d) = %d\n",WHITE,FIX_COLOR(WHITE)));
#endif
	gv.fill_style = FillSolid;
	gv.line_style = LineSolid;
	XChangeGC(display, v->gc,
		GCBackground | GCForeground | GCFillStyle | GCLineStyle | GCFunction, &gv);

	if (type == 6)
	{
		dashl = v->ud_dashlen;
		dasho = v->ud_dashoff;
		dashs = v->ud_linepat;
	} else
	{
		dashl = dashlen[type];
		dasho = dashoff[type];
		dashs = dashes[type];
	}

	if (v->line_width > 1)
	{
		XSetLineAttributes(display, v->gc, v->line_width, LineSolid,
			capstyle[v->line_ends], JoinRound);
	} else if (dashl == 0)
	{
		XSetLineAttributes(display, v->gc, 0, LineSolid,
			capstyle[v->line_ends], JoinRound);
	} else
	{
		XSetLineAttributes(display, v->gc, 0, LineOnOffDash,
			capstyle[v->line_ends], JoinRound);
		XSetDashes(display, v->gc, dasho,
			dashs, dashl);
	}
}

static void init_perimeter(VWK * v)
{
	XGCValues gv;

	gv.function = get_wrmode(v->wrmode, v->fill_color);
	gv.foreground = FIX_COLOR(v->fill_color);
#if TRACE_VDI
	    V(("FIX_COLOR(%d) = %d; ",v->fill_color,FIX_COLOR(v->fill_color)));
#endif
	gv.background = FIX_COLOR(WHITE);
#if TRACE_VDI
	    V(("FIX_COLOR(WHITE=%d) = %d\n",WHITE,FIX_COLOR(WHITE)));
#endif
	gv.fill_style = FillSolid;
	gv.line_style = LineSolid;
	gv.line_width = 0;
	XChangeGC(display, v->gc, GCLineWidth |
		GCBackground | GCForeground | GCFillStyle | GCLineStyle | GCFunction, &gv);
}

static void init_filled(VWK * v)
{
	XGCValues gv;

	V(("  init_filled: interior: %s, style: %d, color: %d, mode: %s\n",
		interior_name[v->fill_interior],
		v->fill_style, v->fill_color,
		wrmode_name[v->wrmode]));
	gv.fill_style = FillOpaqueStippled;
	gv.function = get_wrmode(v->wrmode, v->fill_color);
	if (v->wrmode == MD_XOR)
	{
		gv.foreground = priv_cmap ? FIX_COLOR(1) : white_pixel;
#if TRACE_VDI
	    V(("FIX_COLOR(1) = %d; ",FIX_COLOR(1)));
#endif
		gv.background = priv_cmap ? 0 : FIX_COLOR(WHITE);
#if TRACE_VDI
	    V(("FIX_COLOR(WHITE=%d) = %d\n",WHITE,FIX_COLOR(WHITE)));
	    V(("init_filled: fg = %ld, bg = %ld\n", gv.foreground, gv.background));
#endif
	} else
	{
		if (v->wrmode == MD_TRANS && v->fill_color == WHITE)
		{
			gv.foreground = FIX_COLOR(BLACK);
#if TRACE_VDI
	    V(("FIX_COLOR(BLACK=%d) = %d; ",BLACK,FIX_COLOR(BLACK)));
#endif
			gv.background = FIX_COLOR(WHITE);
#if TRACE_VDI
	    V(("FIX_COLOR(WHITE=%d) = %d\n",WHITE,FIX_COLOR(WHITE)));
#endif
		} else
		{
			gv.foreground = FIX_COLOR(v->fill_interior == FIS_HOLLOW ? WHITE : v->fill_color);
#if TRACE_VDI
	    V(("FIX_COLOR(%d) = %d; ",v->fill_interior == FIS_HOLLOW ? WHITE : v->fill_color,FIX_COLOR(v->fill_interior == FIS_HOLLOW ? WHITE : v->fill_color)));
#endif
			gv.background = FIX_COLOR(WHITE);
#if TRACE_VDI
	    V(("FIX_COLOR(WHITE=%d) = %d\n",WHITE,FIX_COLOR(WHITE)));
#endif
		}
	}
	if (v->fill_interior == FIS_USER)
		gv.stipple = v->ud_fill_pattern;
	else
		gv.stipple = fillpat[v->fill_interior][v->fill_style - 1];
	XChangeGC(display, v->gc,
		GCBackground | GCForeground | GCFillStyle | GCStipple | GCFunction, &gv);
}

static void init_marker(VWK * v)
{
	XGCValues gv;

	gv.function = get_wrmode(v->wrmode, v->marker_color);
	gv.foreground = FIX_COLOR(v->marker_color);
#if TRACE_VDI
	    V(("FIX_COLOR(%d) = %d; ",v->marker_color,FIX_COLOR(v->marker_color)));
#endif
	gv.background = FIX_COLOR(WHITE);
#if TRACE_VDI
	    V(("FIX_COLOR(WHITE=%d) = %d\n",WHITE,FIX_COLOR(WHITE)));
#endif
	gv.fill_style = FillSolid;
	gv.line_style = LineSolid;
	gv.line_width = 0;
	XChangeGC(display, v->gc, GCLineWidth |
		GCBackground | GCForeground | GCFillStyle | GCLineStyle | GCFunction, &gv);
}

static void init_vwk(int i, int phys_handle)
{
	XGCValues gv;
	VWK *v;
	
	if ((v = vwk[i]) == NULL)
		v = vwk[i] = (VWK *)malloc(sizeof(VWK));

#define VMASK (GCPlaneMask|GCGraphicsExposures)

	v->phys_wk = phys_handle;
	
	gv.plane_mask = AllPlanes;			/* 1 */
	gv.graphics_exposures = True;
	
	v->gc = XCreateGC(display, xw, VMASK, &gv);
	v->wrmode = MD_REPLACE;
	v->clipping = FALSE;
	set_clipping(v);
	memset((char *) vwk[i]->mode, sizeof(vwk[i]->mode),1);

	v->line_color = BLACK;
	v->line_type = 0;
	v->line_pattern = 0;
	v->line_width = 1;
	v->line_ends = 0;
	v->ud_dashlen = 16;
	v->ud_dashoff = 0;
	init_line(v);

	v->text_color = BLACK;
	v->text_rotation = 0;
	v->font_ver = 0;
	v->font_hor = 0;
	v->font_eff = 0;
	change_font(i, CHAR_HEIGHT() == 16 ? 2 : 1);
	init_font(v);
	
	v->fill_color = BLACK;
	v->fill_interior = FIS_SOLID;
	v->fill_style = 1;
	v->fill_perimeter = FALSE;
	v->ud_fill_pattern = XCreateBitmapFromData(display, xw, &fill_bits[32 * 38], 16, 16);
	init_filled(v);
	
	v->marker_color = BLACK;
	v->marker_type = 1;
	v->marker_height = 1;
	init_marker(v);
}

static void do_clswk(int h)
{
	VWK *v;
	
	if ((v = vwk[h]) != NULL)
	{
		if (v->ud_fill_pattern != None)
			XFreePixmap(display, v->ud_fill_pattern);
		if (v->gc != None)
			XFreeGC(display, v->gc);
		free(v);
		vwk[h] = NULL;
	}
}

static void close_all_wk(void)
{
	int i;

	for (i = 0; i < MAX_VWK; i++)
	{
		do_clswk(i);
	}
}

/********************************************************************/

static int v_opnwk(void)
{
	V(("v_opnwk[%d]: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		V_HANDLE,
		LM_W(INTIN(0)),
		LM_W(INTIN(1)),
		LM_W(INTIN(2)),
		LM_W(INTIN(3)),
		LM_W(INTIN(4)),
		LM_W(INTIN(5)),
		LM_W(INTIN(6)),
		LM_W(INTIN(7)),
		LM_W(INTIN(8)),
		LM_W(INTIN(9)),
		LM_W(INTIN(10)) ));

	if (LM_W(INTIN(0)) <= LAST_SCREEN_DEVICE)
	{
		init_window();

		init_wk((W*)NULL, (W*)INTOUT(0), (W*)PTSOUT(0));
		V_NINTOUT(45);					/* XXX handle */
		V_NPTSOUT(6);
		close_all_wk();
	}
	return FALSE;
}

/********************************************************************/

static int v_clrwk(void)
{
	int h = V_HANDLE;
	
	V(("v_clrwk[%d]\n", h));
	CHECK_V_HANDLE(h);
	if (h == phys_handle)
	{
		clear_window();
	}
	return !UPDATE_PHYS;
}

/********************************************************************/

static int v_updwk(void)
{
	V(("v_updwk[%d]\n", V_HANDLE));
	CHECK_V_HANDLE(V_HANDLE);
	return CALL_ROM_VDI;
}

/********************************************************************/

static int v_opnvwk(void)
{
	V(("v_opnvwk[%d]\n", V_HANDLE));
	CHECK_V_HANDLE(V_HANDLE);
	return CALL_ROM_VDI;
}

/********************************************************************/

static int v_pline(void)
{
	int i;
	int n = V_NPTSIN;
	int h = V_HANDLE;
	XPoint points[MAX_POINTS];		/* TODO: resizable array */

	V(("v_pline[%d]: %d: ", h, n));
	CHECK_V_HANDLE(h);
	init_line(vwk[h]);

	for (i = 0; i < n; i++)
	{
		points[i].x = LM_W(PTSIN(2 * i));
		points[i].y = LM_W(PTSIN(2 * i + 1));
		if (i != 0)
			V((" -> "));
		V(("(%d,%d)", points[i].x, points[i].y));
	}
	V(("\n"));
	XDrawLines(display, xw, vwk[h]->gc, points, n, CoordModeOrigin);
	XBUF(XDrawLines(display, xbuffer, vwk[h]->gc, points, n, CoordModeOrigin);)
	return !UPDATE_PHYS;
}

/********************************************************************/

static int v_pmarker(void)
{
	int i;
	int n = V_NPTSIN;
	int h = V_HANDLE;
	XPoint points[MAX_POINTS];		/* TODO: resizable array */

	V(("v_pmarker[%d]: %d: ", h, n));
	CHECK_V_HANDLE(h);
	init_marker(vwk[h]);

	for (i = 0; i < n; i++)
	{
		points[i].x = LM_W(PTSIN(2 * i));
		points[i].y = LM_W(PTSIN(2 * i + 1));
		if (i != 0)
			V((" -> "));
		V(("(%d,%d)", points[i].x, points[i].y));
	}
	V(("\n"));
	switch (vwk[h]->marker_type)
	{
	case 1: /* dot */
		XDrawPoints(display, xw, vwk[h]->gc, points, n, CoordModeOrigin);
		XBUF(XDrawPoints(display, xw, vwk[h]->gc, points, n, CoordModeOrigin);)
		break;
	}
	return !UPDATE_PHYS;
}

/********************************************************************/

static int vsl_udsty(void)
{
	int i, j = 0, k = -1;
	int h = V_HANDLE;
	int m = LM_UW(INTIN(0));
	char *pat;

	V(("vsl_udsty[%d]\n", h));
	CHECK_V_HANDLE(h);
	pat = vwk[h]->ud_linepat;
	vwk[h]->line_pattern = m;
	/* create pattern in X format */

	vwk[h]->ud_dashoff = 0;

	while ((m & 1) && (m != 0x0FFFF))
	{
		vwk[h]->ud_dashoff++;
		m = (m >> 1) + 0x8000;
	}

	for (i = 0; i < 15; i++)
	{
		switch (m & 0x0C000)
		{
		case 0x8000:
			pat[j++] = i - k;
			k = i;
			m = m + m + 1;
			break;
		case 0x4000:
			pat[j++] = i - k;
			k = i;
		default:
			m = m + m;
		}
	}
	if (j & 1)
		pat[j++] = 15 - k;
	vwk[h]->ud_dashlen = j;
#if 0
	fprintf(stderr, "User defined pattern:");
	for (i = 0; i < j; i++)
		fprintf(stderr, " %02x", pat[i]);
	fprintf(stderr, "\n");
#endif
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vsl_type(void)
{
	int h = V_HANDLE;
	int m = LM_W(INTIN(0));

	V(("vsl_type[%d]: %d\n", h, m));
	CHECK_V_HANDLE(h);
	SM_W(INTOUT(0), (vwk[h]->line_type =
			(unsigned short) (m - 1) < 7 ? (m - 1) : 0) + 1);
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vswr_mode(void)
{
	int h = V_HANDLE;
	int m = LM_W(INTIN(0));

	V(("vswr_mode[%d]: %d\n", h, m));
	CHECK_V_HANDLE(h);
	vwk[h]->wrmode = m;
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vsl_width(void)
{
	XGCValues v;
	int h = V_HANDLE;
	int w = LM_W(PTSIN(0));

	V(("vsl_width[%d]: %d\n", h, w));
	CHECK_V_HANDLE(h);
	vwk[h]->line_width = w;
	v.line_width = w;
	XChangeGC(display, vwk[h]->gc, GCLineWidth, &v);
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vsl_color(void)
{
	int h = V_HANDLE;
	int c = LM_W(INTIN(0));

	V(("vsl_color[%d]: %d\n", h, c));
	CHECK_V_HANDLE(h);
	vwk[h]->line_color = c;
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vsl_ends(void)
{
	int h = V_HANDLE;
	int beg = LM_W(INTIN(0));
	int end = LM_W(INTIN(1));

	V(("vsl_ends[%d]: %d %d\n", h, beg, end));
	CHECK_V_HANDLE(h);
	vwk[h]->line_ends = ((((unsigned int) beg < 3) ? beg : 0) * 3 +	(((unsigned int) end < 3) ? end : 0));
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vsf_color(void)
{
	int h = V_HANDLE;
	int c = LM_W(INTIN(0));

	V(("vsf_color[%d]: %d\n", h, c));
	CHECK_V_HANDLE(h);
	vwk[h]->fill_color = c;
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vsf_interior(void)
{
	int h = V_HANDLE;
	int i = LM_W(INTIN(0));

	V(("vsf_interior[%d]: %d\n", h, i));
	CHECK_V_HANDLE(h);
	SM_W(INTOUT(0), vwk[h]->fill_interior = (unsigned int) (i) < 5 ? i : FIS_HOLLOW);
	vwk[h]->fill_style = 1;
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vsf_style(void)
{
	int h = V_HANDLE;
	int s = LM_W(INTIN(0));

	V(("vsf_style[%d]: %d\n", h, s));
	CHECK_V_HANDLE(h);
	SM_W(INTOUT(0), vwk[h]->fill_style = (unsigned int) (s - 1) < patnum[vwk[h]->fill_interior] ? s : 1);
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vsf_perimeter(void)
{
	int h = V_HANDLE;
	int p = LM_W(INTIN(0));

	V(("vsf_perimeter[%d]: %d\n", h, p));
	CHECK_V_HANDLE(h);
	vwk[h]->fill_perimeter = p;
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vsf_udpat(void)
{
	int h = V_HANDLE;
	char p[32];
	int i;

	V(("vsf_udpat[%d]\n", h));
	CHECK_V_HANDLE(h);
	for (i = 0; i < 16; i++)
	{
		p[2 * i] = LM_B(INTIN(i));
		p[2 * i + 1] = LM_B((char *)INTIN(i) + 1);
	}

	XFreePixmap(display, vwk[h]->ud_fill_pattern);
	vwk[h]->ud_fill_pattern = XCreateBitmapFromData(display, xw, p, 16, 16);
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vsm_type(void)
{
	int h = V_HANDLE;
	int m = LM_W(INTIN(0));
	
	V(("vsm_type[%d]: %d\n", h, m));
	CHECK_V_HANDLE(h);
	if (m < 1 || m > 6)
		m = 3;
	vwk[h]->marker_type = m;
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vsm_color(void)
{
	int h = V_HANDLE;
	int c = LM_W(INTIN(0));
	
	V(("vsm_color[%d]: %d\n", h, c));
	CHECK_V_HANDLE(h);
	vwk[h]->marker_color = c;
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vsm_height(void)
{
    int h = V_HANDLE;
    int height = LM_W(PTSIN(1));
    
    V(("vsm_height[%d]: %d\n", h, height));
    CHECK_V_HANDLE(h);
    if (vwk[h]->marker_type != 1) {
	if (height < 11) {
	    height = 11;
	} else if (height > 88) {
	    height = 88;
	}
    }
    vwk[h]->marker_height = height;
    SM_W(PTSOUT(0), height);
    SM_W(PTSOUT(1), height);
    return CALL_ROM_VDI;
}

/********************************************************************/

static int v_fillarea(void)
{
	int h = V_HANDLE;
	int n = V_NPTSIN;
	int i;
	XPoint points[MAX_POINTS];		/* TODO: resizable array */

	V(("v_fillarea[%d]: %d\n", h, n));
	CHECK_V_HANDLE(h);
	for (i = 0; i < n; i++)
	{
		points[i].x = LM_W(PTSIN(2 * i));
		points[i].y = LM_W(PTSIN(2 * i + 1));
	}
	init_filled(vwk[h]);
	XFillPolygon(display, xw, vwk[h]->gc, points, n, Complex, CoordModeOrigin);
	XBUF(XFillPolygon(display, xbuffer,	vwk[h]->gc, points, n, Complex, CoordModeOrigin);)
	return !UPDATE_PHYS;
}

/********************************************************************/

static int vr_recfl(void)
{
	XRectangle r;
	int h = V_HANDLE;

	make_rectangle(LM_W(PTSIN(0)), LM_W(PTSIN(1)), LM_W(PTSIN(2)), LM_W(PTSIN(3)), &r);
	V(("vr_recfl[%d]: %d,%d %d,%d\n", h, r.x, r.y, r.width, r.height));
	CHECK_V_HANDLE(h);
	init_filled(vwk[h]);
	XFillRectangle(display, xw, vwk[h]->gc, r.x, r.y, r.width, r.height);
	XBUF(XFillRectangle(display, xbuffer, vwk[h]->gc, r.x, r.y, r.width, r.height);)
	check_pixel();
	return !UPDATE_PHYS;
}

/********************************************************************/

#if TRACE_VDI

static unsigned char atari_to_iso[256] = {
#define _NA_ 0x80
	0x00, 0x7e, _NA_, _NA_, _NA_, _NA_, _NA_, 0x01, _NA_, _NA_, _NA_, _NA_, 0x04, 0x05, _NA_, _NA_,
	_NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
	0xc7, 0xfc, 0xe9, 0xe2, 0xe4, 0xe0, 0xe5, 0xe7, 0xea, 0xeb, 0xe8, 0xef, 0xee, 0xec, 0xc4, 0xc5,
	0xc9, 0xe6, 0xc6, 0xf4, 0xf6, 0xf2, 0xfb, 0xf9, 0xff, 0xd6, 0xdc, 0xa2, 0xa3, 0xa5, 0xdf, _NA_,
	0xe1, 0xed, 0xf3, 0xfa, 0xf1, 0xd1, _NA_, _NA_, 0xbf, _NA_, 0xac, 0xbd, 0xbc, 0xa1, 0xab, 0xbb,
	0xe3, 0xf5, 0xd8, 0xf8, _NA_, _NA_, 0xc0, 0xc3, 0xd5, 0xa8, 0xb4, _NA_, 0xb6, 0xa9, 0xae, _NA_,
	_NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_,
	_NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, 0xa7, _NA_, _NA_,
	_NA_, 0xdf, _NA_, 0x1c, _NA_, _NA_, 0xb5, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_, _NA_,
	_NA_, 0xb1, 0x1b, 0x1a, _NA_, _NA_, 0xf7, _NA_, 0xb0, _NA_, _NA_, _NA_, _NA_, 0xb2, 0xb3, 0xaf
#undef _NA_
};

static char *ascii_text(int nchars, W *string)
{
	int i;
	static char buf[256];

	for (i = 0; i < nchars; i++, string++)
		buf[i] = atari_to_iso[LM_W(string) & 0xff];
	buf[i] = 0;
	return buf;
}
#endif

static int vdi_gdp(void)
{
	XRectangle r;
	int h = V_HANDLE;
	int op = V_SUBCODE;

	V(("vdi_gdp[%d]: %d(", h, op));
	CHECK_V_HANDLE(h);
	switch (op)
	{
	case 1:							/* Bar */
		make_rectangle(LM_W(PTSIN(0)), LM_W(PTSIN(1)),
			LM_W(PTSIN(2)), LM_W(PTSIN(3)), &r);
		V(("v_bar[%d]: %d,%d, %d,%d\n", h, r.x, r.y, r.width, r.height));
		init_filled(vwk[h]);
		XFillRectangle(display, xw, vwk[h]->gc, r.x, r.y, r.width, r.height);
		XBUF(XFillRectangle(display, xbuffer, vwk[h]->gc, r.x, r.y, r.width, r.height);)
		if (vwk[h]->fill_perimeter)
		{
			init_perimeter(vwk[h]);
			XDrawRectangle(display, xw, vwk[h]->gc, r.x, r.y, r.width, r.height);
			XBUF(XDrawRectangle(display, xbuffer, vwk[h]->gc, r.x, r.y, r.width, r.height);)
		}
		break;

	case 2:							/* Arc */
		{
			int x, y, radius, beg_angle, end_angle;
			
			x = LM_W(PTSIN(0));
			y = LM_W(PTSIN(1));
			radius = LM_W(PTSIN(6));
			beg_angle = LM_W(INTIN(0)) * 64 / 10;
			end_angle = LM_W(INTIN(1)) * 64 / 10 - beg_angle;
			V(("v_arc[%d]: %d, %d, %d, %d, %d)\n", h, x, y, radius, LM_W(INTIN(0)), LM_W(INTIN(1)) ));
			init_line(vwk[h]);
			XDrawArc(display, xw, vwk[h]->gc,
				x - radius, y - radius,
				radius * 2, radius * 2,
				beg_angle, end_angle);
			XBUF(XDrawArc(display, xbuffer, vwk[h]->gc,
				x - radius, y - radius,
				radius * 2, radius * 2,
				beg_angle, end_angle);)
		}
		break;

	case 3:							/* Pieslice */
		{
			int radius;
			int x, y;
			int beg_angle, end_angle;
			
			init_filled(vwk[h]);
			x = LM_W(PTSIN(0));
			y = LM_W(PTSIN(1));
			radius = LM_W(PTSIN(6));
			beg_angle = LM_W(INTIN(0)) * 64 / 10;
			end_angle = LM_W(INTIN(1)) * 64 / 10 - beg_angle;
			V(("v_piesclice[%d]: %d, %d, %d, %d, %d)\n", h, x, y, radius, LM_W(INTIN(0)), LM_W(INTIN(1)) ));
			XFillArc(display, xw, vwk[h]->gc,
				x - radius, y - radius,
				2 * radius, 2 * radius,
				beg_angle, end_angle);
			XBUF(XFillArc(display, xbuffer, vwk[h]->gc,
				x - radius, y - radius,
				2 * radius, 2 * radius,
				beg_angle, end_angle);)
			if (vwk[h]->fill_perimeter)
			{
				init_perimeter(vwk[h]);
				XFillArc(display, xw, vwk[h]->gc,
					x - radius, y - radius,
					2 * radius, 2 * radius,
					beg_angle, end_angle);
				XBUF(XFillArc(display, xbuffer, vwk[h]->gc,
					x - radius, y - radius,
					2 * radius, 2 * radius,
					beg_angle, end_angle);)
			}
		}
		break;

	case 4:							/* Circle */
		{
			int radius;
			int x, y;
			
			init_filled(vwk[h]);
			x = LM_W(PTSIN(0));
			y = LM_W(PTSIN(1));
			radius = LM_W(PTSIN(4));
			V(("v_circle[%d]: %d, %d, %d\n", h, x, y, radius));
			XFillArc(display, xw, vwk[h]->gc,
				x - radius, y - radius,
				radius * 2, radius * 2,
				0, 360 * 64);
			XBUF(XFillArc(display, xbuffer, vwk[h]->gc,
				x - radius, y - radius,
				radius * 2, radius * 2,
				0, 360 * 64);)
			if (vwk[h]->fill_perimeter)
			{
				init_perimeter(vwk[h]);
				XDrawArc(display, xw, vwk[h]->gc,
					x - radius, y - radius,
					radius * 2, radius * 2,
					0, 360 * 64);
				XBUF(XDrawArc(display, xbuffer, vwk[h]->gc,
					x - radius, y - radius,
					radius * 2, radius * 2,
					0, 360 * 64);)
			}
		}
		break;

	case 5:							/* Ellipse */
		{
			int xradius, yradius;
			int x, y;
			
			init_filled(vwk[h]);
			x = LM_W(PTSIN(0));
			y = LM_W(PTSIN(1));
			xradius = LM_W(PTSIN(2));
			yradius = LM_W(PTSIN(3));
			V(("v_circle[%d]: %d, %d, %d, %d\n", h, x, y, xradius, yradius));
			XFillArc(display, xw, vwk[h]->gc,
				x - xradius, y - yradius,
				2 * xradius, 2 * yradius, 0, 360 * 64);
			XBUF(XFillArc(display, xbuffer, vwk[h]->gc,
				x - xradius, y - yradius, 2 * xradius, 2 * yradius,
				0, 360 * 64);)
			if (vwk[h]->fill_perimeter)
			{
				init_perimeter(vwk[h]);
				XFillArc(display, xw, vwk[h]->gc,
					x - xradius, y - yradius,
					2 * xradius, 2 * yradius,
					0, 360 * 64);
				XBUF(XFillArc(display, xbuffer, vwk[h]->gc,
					x - xradius, y - yradius,
					2 * xradius, 2 * yradius,
					0, 360 * 64);)
			}
		}
		break;

	case 6:							/* Elliptical Arc */
		{
			int x, y, xradius, yradius, beg_angle, end_angle;
			
			x = LM_W(PTSIN(0));
			y = LM_W(PTSIN(1));
			xradius = LM_W(PTSIN(2));
			yradius = LM_W(PTSIN(3));
			beg_angle = LM_W(INTIN(0)) * 64 / 10;
			end_angle = LM_W(INTIN(1)) * 64 / 10 - beg_angle;
			V(("v_ellarc[%d]: %d, %d, %d, %d, %d, %d\n", h, x, y, xradius, yradius, LM_W(INTIN(0)), LM_W(INTIN(1)) ));
			init_line(vwk[h]);
			XDrawArc(display, xw, vwk[h]->gc,
				x - xradius, y - yradius,
				xradius * 2, yradius * 2,
				beg_angle, end_angle);
			XBUF(XDrawArc(display, xbuffer, vwk[h]->gc,
				x - xradius, y - yradius,
				xradius * 2, yradius * 2,
				beg_angle, end_angle);)
		}
		break;

	case 7:							/* Elliptical Pie */
		{
			int x, y;
			int xradius, yradius;
			int beg_angle, end_angle;
			
			init_filled(vwk[h]);
			x = LM_W(PTSIN(0));
			y = LM_W(PTSIN(1));
			xradius = LM_W(PTSIN(2));
			yradius = LM_W(PTSIN(3));
			beg_angle = LM_W(INTIN(0)) * 64 / 10;
			end_angle = LM_W(INTIN(1)) * 64 / 10 - beg_angle;
			V(("v_ellpie[%d]: %d, %d, %d, %d, %d, %d)\n", h, x, y, xradius, yradius, LM_W(INTIN(0)), LM_W(INTIN(1)) ));
			XFillArc(display, xw, vwk[h]->gc,
				x - xradius, y - yradius,
				2 * xradius, 2 * yradius,
				beg_angle, end_angle);
			XBUF(XFillArc(display, xbuffer, vwk[h]->gc,
				x - xradius, y - yradius,
				2 * xradius, 2 * yradius,
				beg_angle, end_angle);)
			if (vwk[h]->fill_perimeter)
			{
				init_perimeter(vwk[h]);
				XFillArc(display, xw, vwk[h]->gc,
					x - xradius, y - yradius,
					2 * xradius, 2 * yradius,
					beg_angle, end_angle);
				XBUF(XFillArc(display, xbuffer, vwk[h]->gc,
					x - xradius, y - yradius,
					2 * xradius, 2 * yradius,
					beg_angle, end_angle);)
			}
		}
		break;

	case 8:							/* Rounded Rectangle */
		V(("v_rbox[%d]: NYI\n", h));
		break;

	case 9:							/* Filled Rounded Rectangle */
		V(("v_rfbox[%d]: NYI\n", h));
		break;

	case 10:						/* Justified Graphics Text */
		{
			int len, word_space, char_space, x, y, n, i, delta;
			XChar2b *t;
			double s, off = 0.0;
			XTextItem16 text16[1000];		/* dodgy */

			x = LM_W(PTSIN(0));
			y = LM_W(PTSIN(1));
			len = LM_W(PTSIN(2));
			word_space = LM_W(INTIN(0));
			char_space = LM_W(INTIN(1));
			n = V_NINTIN - 2;
			t = (XChar2b *) INTIN(2);

			V(("v_justified[%d]: (%d,%d), '%s', %d,%d,%d\n", h, x, y, ascii_text(n, INTIN(2)), len, word_space, char_space ));
			x -= len / 2;
			len -= n * sfinfo[vwk[h]->font_index].cellwidth;
			s = (double) len / (n - 1);
			delta = 0;
			init_font(vwk[h]);
			for (i = 0; i < n; i++)
			{
				text16[i].chars = &t[i];
				text16[i].nchars = 1;
				text16[i].delta = delta;
				text16[i].font = None;
				delta = ((int) (off + s)) - (int) off;
				off += s;
			}
			draw_jtext(vwk[h], x, y, text16, n);
		}
		break;
	
	default:
		V(("vdi_gdp[%d]\n", op));
		return CALL_ROM_VDI;
	}
	return !UPDATE_PHYS;
}

/********************************************************************/

static int vs_clip(void)
{
	XRectangle *r;
	int h = V_HANDLE;

	r = &(vwk[h]->clipr);

	V(("vs_clip[%d]: ", h));
	if (LM_W(INTIN(0)) == 0)
	{
		V(("off\n"));
		vwk[h]->clipping = FALSE;
	} else
	{
		vwk[h]->clipping = TRUE;
		make_rectangle(LM_W(PTSIN(0)), LM_W(PTSIN(1)), LM_W(PTSIN(2)),
			LM_W(PTSIN(3)), r);
		V(("Clipping to (%d,%d) +(%d,%d)\n", r->x, r->y, r->width, r->height));
	}
	set_clipping(vwk[h]);
	return CALL_ROM_VDI;
}

/********************************************************************/

static int v_string(void)
{
#if 0
	int h = V_HANDLE;
	int maxl = LM_W(INTIN(0));
	int echo_mode = LM_W(INTIN(1));
	int echo_x = LM_W(PTSIN(0));
	int echo_y = LM_W(PTSIN(1));

#endif
	int h = V_HANDLE;
		
#if 0
	V(("v_string[%d]: ", h));
#endif
	CHECK_V_HANDLE(h);
#if 0
	V(("%s (%d,%d) [%d,%d]\n",
		vwk[h]->mode[DEV_STRING] == MODE_REQUEST ? "rq" : "sm",
		LM_W(INTIN(0)),
		LM_W(INTIN(1)),
		LM_W(PTSIN(0)), LM_W(PTSIN(1))));
#endif
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vq_mouse(void)
{
	int dummy, x, y;
	unsigned int mask;
	Window dwin, c;
	W s = 0;

	V(("vq_mouse[%d]: ", V_HANDLE));
	CHECK_V_HANDLE(V_HANDLE);
	XQueryPointer(display, vdi_mode ? xw : imagewin, &dwin, &c, &dummy, &dummy, &x, &y, &mask);
	if (mask & Button1Mask)
		s |= 1;
	if (mask & Button3Mask)
		s |= 2;
	V(("-> %d,%d,%d\n", x, y, s));
	SM_W(INTOUT(0), s);
	SM_W(PTSOUT(0), x);
	SM_W(PTSOUT(1), y);
	return TRUE;
}

/********************************************************************/

static int vq_key_s(void)
{
#if 0
	V(("vq_key_s[%d]\n", V_HANDLE));
#endif
	CHECK_V_HANDLE(V_HANDLE);
	/* TODO */
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vsin_mode(void)
{
	int h = V_HANDLE;
	int d = LM_W(INTIN(0));
	int m = LM_W(INTIN(1));

#if 0
	V(("vsin_mode[%d]: %d(%s) %d(%s)\n",
		h,
		d, d==DEV_LOCATOR?"locator":d==DEV_VALUATOR?"valuator":d==DEV_CHOICE?"choice":"string",
		m, m == MODE_REQUEST ? "request" : "sample"));
#endif
	CHECK_V_HANDLE(h);
	d = (d - 1) & 3;
	if (vwk[h]->mode[d] == m)
		return TRUE;
	vwk[h]->mode[d] = m;
	return CALL_ROM_VDI;
}

/********************************************************************/

static int v_locator(void)
{
	V(("v_locator[%d]: ", V_HANDLE));
	CHECK_V_HANDLE(V_HANDLE);
	V((" %s (%d,%d)\n",
		vwk[V_HANDLE]->mode[DEV_LOCATOR] == MODE_REQUEST ? "rq" : "sm",
		LM_W(PTSIN(0)), LM_W(PTSIN(1))));
	/* TODO */
	return CALL_ROM_VDI;
}

/********************************************************************/

static int v_choice(void)
{
	V(("v_choice[%d]: ", V_HANDLE));
	CHECK_V_HANDLE(V_HANDLE);
	V(("%s %d\n",
		vwk[V_HANDLE]->mode[DEV_CHOICE] == MODE_REQUEST ? "rq" : "sm",
		LM_W(INTIN(0))));
	/* TODO */
	return CALL_ROM_VDI;
}

/********************************************************************/

static int v_valuator(void)
{
	V(("v_valuator[%d]: ", V_HANDLE));
	CHECK_V_HANDLE(V_HANDLE);
	V(("%s %d\n",
		vwk[V_HANDLE]->mode[DEV_VALUATOR] == MODE_REQUEST ? "rq" : "sm",
		LM_W(INTIN(0))));
	/* TODO */
	return CALL_ROM_VDI;
}

/********************************************************************/

/* yoff+1 is a kludge XXX */
static int v_gtext(void)
{
	int h = V_HANDLE;
	int x = LM_W(PTSIN(0));
	int y = LM_W(PTSIN(1));

	V(("v_gtext[%d]: ", h));
	CHECK_V_HANDLE(h);
	V(("%d,%d,'%s', i=%d, y_off=%d\n", x, y, ascii_text(V_NINTIN, INTIN(0)), vwk[h]->font_index, vwk[h]->font_yoff));
	draw_text(vwk[h], x, y + vwk[h]->font_yoff + 1, (XChar2b *) INTIN(0), V_NINTIN);

	return !UPDATE_PHYS;
}

/********************************************************************/

static int vst_font(void)
{
	V(("vst_font[%d]: %d\n", V_HANDLE, LM_W(INTIN(0))));
	CHECK_V_HANDLE(V_HANDLE);
	/* TODO */
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vst_height(void)
{
	int i;
	int h = V_HANDLE;
	int height = LM_W(PTSIN(1));

	V(("vst_height[%d]: %d\n", h, height));
	CHECK_V_HANDLE(h);

	i = 0;
	do {
		if (sfinfo[i+1].top > height)
			break;
	} while (++i<SYSFONTS-1);
	change_font(h, i);
	SM_W(PTSOUT(0), sfinfo[i].width);
	SM_W(PTSOUT(1), sfinfo[i].height);
	SM_W(PTSOUT(2), sfinfo[i].cellwidth);
	SM_W(PTSOUT(3), sfinfo[i].cellheight);
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vst_alignment(void)
{
	int h = V_HANDLE;
	int hor = LM_W(INTIN(0));
	int ver = LM_W(INTIN(1));

	V(("vst_alignment[%d]: %d,%d\n", h, hor, ver));
	CHECK_V_HANDLE(h);

	SM_W(INTOUT(0), hor);
	SM_W(INTOUT(1), ver);
	vwk[h]->font_hor = hor;
	vwk[h]->font_ver = ver;
	set_fontparms(h);
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vst_color(void)
{
	int h = V_HANDLE;
	int c = LM_W(INTIN(0));

	V(("vst_color[%d]: %d\n", h, c));
	CHECK_V_HANDLE(h);
	vwk[h]->text_color = c;
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vst_rotation(void)
{
	int h = V_HANDLE;
	int angle = LM_W(INTIN(0));

	V(("vst_rotation[%d]: %d\n", h, angle));
	CHECK_V_HANDLE(h);
	vwk[h]->text_rotation = angle;
	return CALL_ROM_VDI;
}

/********************************************************************/

static int init_traster(VWK * v, int mode, int f, int b)
{
	XGCValues gv;
	int oldmode;
	
	XGetGCValues(display, v->gc, GCFunction, &gv);
	oldmode = gv.function;
	gv.function = mode;
	gv.fill_style = FillSolid;
	gv.foreground = FIX_COLOR(f);
#if TRACE_VDI
	    V(("FIX_COLOR(%d) = %d; ",f,FIX_COLOR(f)));
#endif
	gv.background = FIX_COLOR(b);
#if TRACE_VDI
	    V(("FIX_COLOR(%d) = %d\n",b,FIX_COLOR(b)));
#endif
	XChangeGC(display, v->gc, GCFunction | GCFillStyle | GCForeground | GCBackground, &gv);
	return oldmode;
}

static int vrt_cpyfm(void)
{
	XRectangle r;
	int h = V_HANDLE;
	UL s = LM_UL(CONTRL(7));
	UL sa = LM_UL(MEM(s));
	UL d = LM_UL(CONTRL(9));
	UL da = LM_UL(MEM(d));
	int sx1, sy1, sx2, sy2;
	int dx1, dy1, dx2, dy2;
	int to_s, fr_s, c1, c2, m;
	int oldmode;
	
	sx1 = LM_W(PTSIN(0));
	sy1 = LM_W(PTSIN(1));
	sx2 = LM_W(PTSIN(2));
	sy2 = LM_W(PTSIN(3));
	dx1 = LM_W(PTSIN(4));
	dy1 = LM_W(PTSIN(5));
	dx2 = LM_W(PTSIN(6));
	dy2 = LM_W(PTSIN(7));
	c1 = LM_W(INTIN(1));
	m = get_wrmode(LM_W(INTIN(0)), c1);
	c2 = LM_W(INTIN(2));
	fr_s = (sa == 0 || sa == vbase);
	to_s = (da == 0 || da == vbase);
	V(("vrt_cpyfm[%d]: (%d/%d %d/%d %d) S: 0x%lx (0x%lx[%d],%d,%d,%d,%d), "
			"D: 0x%lx (0x%lx[%d], %d,%d,%d,%d)\n", h, fr_s, to_s, c1, c2, m,
			(long)s, (long)sa, LM_W(MEM(s + 10)), sx1, sy1, sx2, sy2, (long)d, (long)da, LM_W(MEM(d + 10)), dx1, dy1, dx2, dy2));
	CHECK_V_HANDLE(h);

	oldmode = init_traster(vwk[h], m, c1, c2);
	if (!fr_s && to_s)
	{
		make_rectangle(sx1, sy1, sx2, sy2, &r);
		xi->width = LM_W(MEM(s + 4));
		xi->height = LM_W(MEM(s + 6));
		xi->bytes_per_line = LM_W(MEM(s + 8)) * 2;
		xi->data = (char *) MEM(sa);
		XPutImage(display, xw, vwk[h]->gc, xi, r.x, r.y, dx1, dy1, r.width, r.height);
		XBUF(XPutImage(display, xbuffer, vwk[h]->gc, xi, r.x, r.y, dx1, dy1, r.width, r.height);)
		done_raster(vwk[h], oldmode);
		return !UPDATE_PHYS;
	}
	done_raster(vwk[h], oldmode);
	return CALL_ROM_VDI;
}

/********************************************************************/

static int init_oraster(VWK * v, int mode)
{
	XGCValues gv;
	int oldmode;
	
	XGetGCValues(display, v->gc, GCFunction, &gv);
	oldmode = gv.function;
	gv.function = mode;
	gv.fill_style = FillSolid;
	gv.foreground = FIX_COLOR(BLACK);
#if TRACE_VDI
	    V(("FIX_COLOR(BLACK=%d) = %d; ",BLACK,FIX_COLOR(BLACK)));
#endif
	gv.background = FIX_COLOR(WHITE);
#if TRACE_VDI
	    V(("FIX_COLOR(WHITE=%d) = %d\n",WHITE,FIX_COLOR(WHITE)));
#endif
	gv.clip_mask = None;
	XChangeGC(display, v->gc, GCClipMask | GCFunction | GCFillStyle | GCForeground | GCBackground, &gv);
	return oldmode;
}

/* copy raster from screen to MFDB d in device dependent format */
/* doesn't work without -private at the moment FIXME */
static void copy_screen_to_st_dd(GC gc, int x, int y, int w, int h, UL s,
	UL d, int dx, int dy)
{
	UL da = LM_UL(MEM(d));

	if (depth == 1 || vdi_planes == 1)
	{
#if XBUFFER
		XCopyPlane(display, xbuffer, tmpx, gc, x, y, w, h, x, y, 1);
#else
		XCopyPlane(display, xw, tmpx, gc, x, y, w, h, x, y, 1);
#endif
		V(("  copy_screen_to_dd (1/1, [%d,%d,%d,%d]->[%d,%d]\n",
			x, y, w, h, dx, dy));
		xi->width = LM_W(MEM(d + 4));
		xi->height = LM_W(MEM(d + 6));
		xi->bytes_per_line = LM_W(MEM(d + 8)) * 2;
		xi->data = (char *) MEM(da);
		XGetSubImage(display, tmpx, x, y, w, h,	1, XYPixmap, xi, dx, dy);
	} else
		/* assume depth == 8 FIXME */
	{
		int words = LM_W(MEM(d + 8));
		int height = LM_W(MEM(d + 6));
		int bpp = words * 2 * height;
		UB *dp = (UB *)MEM(da)
			+ words * 2 * dy + ((dx + 15) & -16) / 8;	/*FIXME */
		UB *sp = (UB *)&(subimage->data[dx + dy * vdi_w]);
		int planes = LM_W(MEM(d + 12));
		int i, j, k, l;

#if XBUFFER
		XCopyArea(display, xbuffer, tmpx, gc, x, y, w, h, x, y);
#else
		XCopyArea(display, xw, tmpx, gc, x, y, w, h, x, y);
#endif
		XGetSubImage(display, tmpx, x, y, w, h, AllPlanes,
			ZPixmap, subimage, dx, dy);
#if 0
		XPutImage(display, xw, gc, subimage, dx, dy, 320, 300, w, h);
#endif
		V(("  copy_screen_to_dd (8/4, [%d,%d,%d,%d]->[%d,%d] MFDB[%d,%d,%d]\n",
			x, y, w, h, dx, dy, planes, words, height));
		for (i = 0; i < h; i++)
		{
			for (j = 0; j < words * 2; j++)
			{
				for (l = 0; l < planes; l++)
				{
					UB p = 0;

					for (k = 0; k < 8; k++)
					{
						p |= ((sp[j * 8 + k + i * vdi_w] >> l) & 1) << (7 - k);
					}
					dp[bpp * l + j + i * words * 2] = p;
				}
			}
		}
	}
}

static void copy_st_dd_to_screen(GC gc, int x, int y, int w, int h, UL s,
	int dx, int dy)
{
	UL sa = LM_UL(MEM(s));

	if (depth == 1 || vdi_planes == 1)
	{
		V(("  copy_dd_to_screen (1/1, [%d,%d,%d,%d]->[%d,%d]\n",
			x, y, w, h, dx, dy));
		xi->width = LM_W(MEM(s + 4));
		xi->height = LM_W(MEM(s + 6));
		xi->bytes_per_line = LM_W(MEM(s + 8)) * 2;
		xi->data = (char *) MEM(sa);
		XBUF(XPutImage(display, xbuffer, gc, xi, x, y, dx, dy, w, h);)
		XPutImage(display, xw, gc, xi, x, y, dx, dy, w, h);
	} else
	{
		UB *sp = (UB *)MEM(sa)
			+ LM_W(MEM(s + 8)) * 2 * y + ((x + 15) & -16) / 8;		/* FIXME */
		UB *dp = (UB *)&(subimage->data[dx + dy * vdi_w]);
		int planes = LM_W(MEM(s + 12));
		int words = LM_W(MEM(s + 8));
		int height = LM_W(MEM(s + 6));
		int bpp = words * 2 * height;
		int i, j, k, l;

		V(("  copy_dd_to_screen (8/%d, [%d,%d,%d,%d]->[%d,%d] MFDB[%d,%d,%d]\n", LM_W(MEM(s + 12)),
			x, y, w, h, dx, dy, planes, words, height));
		for (i = 0; i < h; i++)
		{
			for (j = 0; j < words * 2; j++)
			{
				for (k = 0; k < 8; k++)
				{
					UB p = 0;

					for (l = 0; l < planes; l++)
					{
						p |= ((sp[i * 2 * words + j + l * bpp] >> (7 - k)) & 1) << l;
					}
					dp[j * 8 + i * vdi_w + k] = p;
				}
			}
		}
		XBUF(XPutImage(display, xbuffer, gc, subimage, dx, dy, dx, dy, w, h);)
		XPutImage(display, xw, gc, subimage, dx, dy, dx, dy, w, h);
	}
}

static int vro_cpyfm(void)
{
	XRectangle r;
	int h = V_HANDLE;
	UL s = LM_UL(CONTRL(7));
	UL sa = LM_UL(MEM(s));
	UL d = LM_UL(CONTRL(9));
	UL da = LM_UL(MEM(d));
	int sf = LM_W(MEM(s + 10));
	int df = LM_W(MEM(d + 10));
	int sx1, sy1, sx2, sy2;
	int dx1, dy1, dx2, dy2;
	int to_s, fr_s, mode;
	int oldmode;
	
	mode = LM_W(INTIN(0));
	sx1 = LM_W(PTSIN(0));
	sy1 = LM_W(PTSIN(1));
	sx2 = LM_W(PTSIN(2));
	sy2 = LM_W(PTSIN(3));
	dx1 = LM_W(PTSIN(4));
	dy1 = LM_W(PTSIN(5));
	dx2 = LM_W(PTSIN(6));
	dy2 = LM_W(PTSIN(7));
	fr_s = (sa == 0 || sa == vbase);
	to_s = (da == 0 || da == vbase);
	V(("vro_cpyfm[%d]: (%d/%d)[%d] S: 0x%lx (0x%lx[%d],%d,%d,%d,%d), D: 0x%lx (0x%lx[%d], %d,%d,%d,%d)\n",
		h,
		fr_s, to_s, mode,
		(long)s, (long)sa, sf, sx1, sy1, sx2, sy2, (long)d, (long)da, df, dx1, dy1, dx2, dy2));
	CHECK_V_HANDLE(h);

	if (sa == 0)
		sf = 0;
	if (da == 0)
		df = 0;
	if ((sf == 1 || df == 1) && LM_W(MEM(s+12)) != 1)
	{
		V(("Warning: vro_cpyfm() with illegal args (rasters in standard format)\n"));
	}

	oldmode = init_oraster(vwk[h], mode);
	make_rectangle(sx1, sy1, sx2, sy2, &r);
	if (fr_s)
	{
		if (to_s)
		{
			XCopyArea(display, xw, xw, vwk[h]->gc, r.x, r.y, r.width, r.height,	dx1, dy1);
			XBUF(XCopyArea(display, xbuffer, xbuffer, vwk[h]->gc, r.x, r.y, r.width, r.height, dx1, dy1);)
			done_raster(vwk[h], oldmode);
			return !UPDATE_PHYS;
		} else
		{
			copy_screen_to_st_dd(vwk[h]->gc, r.x, r.y, r.width, r.height, s, d,	dx1, dy1);
			done_raster(vwk[h], oldmode);
			return !UPDATE_PHYS;
		}
	} else
	{
		if (to_s)
		{
			copy_st_dd_to_screen(vwk[h]->gc, r.x, r.y, r.width, r.height, s, dx1, dy1);
			done_raster(vwk[h], oldmode);
			return !UPDATE_PHYS;
		} else
		{
			done_raster(vwk[h], oldmode);
			return CALL_ROM_VDI;
		}
	}
}

/********************************************************************/

static int vsc_form(void)
{
	/* int h = V_HANDLE; */
	XColor cols[2];

	V(("vsc_form[%d]: (%d,%d,%d,%d,%d) ", V_HANDLE, LM_W(INTIN(0)), LM_W(INTIN(1)), LM_W(INTIN(2)), LM_W(INTIN(3)), LM_W(INTIN(4))));
	CHECK_V_HANDLE(V_HANDLE);
	cxi->byte_order = MSBFirst;
	cxi->bitmap_bit_order = MSBFirst;
	cxi->data = (char *)INTIN(5);
	cxi->bitmap_pad = 32;
	XSetForeground(display, cgc, white_pixel);
	XSetBackground(display, cgc, black_pixel);
	XPutImage(display, cmask, cgc, cxi, 0, 0, 0, 0, 16, 16);
	cxi->data = (char *)INTIN(21);
	XPutImage(display, cdata, cgc, cxi, 0, 0, 0, 0, 16, 16);
	cols[0].pixel = FIX_COLOR(LM_W(INTIN(4)));
#if TRACE_VDI
	    V(("FIX_COLOR(%d) = %d; ",LM_W(INTIN(4)),FIX_COLOR(LM_W(INTIN(4)))));
#endif
	cols[1].pixel = FIX_COLOR(LM_W(INTIN(3)));
#if TRACE_VDI
	    V(("FIX_COLOR(%d) = %d\n",LM_W(INTIN(3)),FIX_COLOR(LM_W(INTIN(3)))));
#endif
	V((" Cursor fg/bg = %ld,%ld\n", cols[0].pixel, cols[1].pixel));
	if (depth == 1)
	{
		if (cols[0].pixel == white_pixel)
		{
			cols[0].red = cols[0].green = cols[0].blue = 0xffff;
			cols[1].red = cols[1].green = cols[1].blue = 0x0;
		} else
		{
			cols[0].red = cols[0].green = cols[0].blue = 0x0;
			cols[1].red = cols[1].green = cols[1].blue = 0xffff;
		}
	} else
	{
		XQueryColors(display, cmap, cols, 2);
	}
	cursors[0] = XCreatePixmapCursor(display, cdata, cmask, &cols[0], &cols[1],
			LM_W(INTIN(0)), LM_W(INTIN(1)));
	XDefineCursor(display, xw, cursors[0]);
	return CALL_ROM_VDI;
}

/********************************************************************/

static int ax, ay, aw, ah, acw, ach;
static int start_esc = 0;

int vdi_output_c(char c)
{
	int h = phys_handle;
	int y;
	XGCValues gv;
	
	if (h < 0 || xw == None)
		return FALSE;
	XGetGCValues(display, vwk[h]->gc,
		GCFunction|GCPlaneMask|GCForeground|GCBackground|GCLineWidth|GCLineStyle|GCCapStyle
		|GCJoinStyle|GCFillStyle|GCFillRule|GCTile|GCStipple|GCTileStipXOrigin|
		GCTileStipYOrigin|GCFont|GCSubwindowMode|GCGraphicsExposures|GCClipXOrigin|
		GCClipYOrigin|GCDashOffset|GCArcMode, &gv);
	if (start_esc)
	{
		switch (c)
		{
		case 'A':
			if (ay > 0)
				--ay;
			break;
		case 'B':
			if (ay < (ah - 1))
				++ay;
			break;
		case 'C':
			if (ax < (aw - 1))
				++ax;
			break;
		case 'D':
			if (ax > 0)
				--ax;
			break;
		case 'E':
			ax = ay = 0;
			clear_window();
			break;
		case 'H':
			ax = ay = 0;
			break;
		case 'I':
			/* TODO */
			break;
		case 'J':
			/* TODO */
			break;
		case 'K':
			XSetForeground(display, rgc, FIX_COLOR(WHITE));
#if TRACE_VDI
	    V(("FIX_COLOR(WHITE=%d) = %d; ",WHITE,FIX_COLOR(WHITE)));
#endif
			XFillRectangle(display, xw, rgc, ax*acw, ay*ach, vdi_w - ax * acw, ach);
			XBUF(/* TODO */);
			XSetForeground(display, rgc, FIX_COLOR(BLACK));
#if TRACE_VDI
	    V(("FIX_COLOR(BLACK=%d) = %d\n",BLACK,FIX_COLOR(BLACK)));
#endif
			break;
		}
		start_esc = 0;
		return TRUE;
	}
	switch (c)
	{
	case 0x0d:
		ax = 0;
		break;
	case 0x0a:
		if (ay == ah - 1)
		{
			XSetForeground(display, rgc, FIX_COLOR(WHITE));
#if TRACE_VDI
	    V(("FIX_COLOR(WHITE=%d) = %d; ",WHITE,FIX_COLOR(WHITE)));
#endif
			XCopyArea(display, xw, xw, vwk[h]->gc, 0, ach, vdi_w, vdi_h - ach, 0, 0);
			XFillRectangle(display, xw, rgc, 0, vdi_h-ach, vdi_w, ach);
			XBUF(XCopyArea(display, xbuffer, xbuffer, vwk[h]->gc, 0, ach, vdi_w, vdi_h - ach, 0, 0);)
			XBUF(XFillRectangle(display, xw, rgc, 0, vdi_h-ach, vdi_w, ach);)
			XSetForeground(display, rgc, FIX_COLOR(BLACK));
#if TRACE_VDI
	    V(("FIX_COLOR(BLACK=%d) = %d\n",BLACK,FIX_COLOR(BLACK)));
#endif
		} else
		{
			ay++;
		}
		break;
	case 0x1b:
		start_esc = 1;
		break;
	default:
		y = ay * ach + sysfont[vwk[h]->font_index]->ascent;
		XDrawImageString(display, xw, vwk[h]->gc, ax * acw, y, &c, 1);
		XBUF(XDrawImageString(display, xbuffer, vwk[h]->gc, ax * acw, y, &c, 1);)
		if (ax < (aw-1))
			ax++;
		break;
	}
	return TRUE;
}

static int vdi_escape(void)
{
	int h = V_HANDLE;
	int op = V_SUBCODE;

	CHECK_V_HANDLE(h);
	switch (op)
	{
	case 1:
		V(("vq_chcells[%d]\n", h));
		ach = CHAR_HEIGHT();
		acw = 8;
		ah = vdi_h / ach;
		aw = vdi_w / acw ;
		SM_W(INTOUT(0), ah);
		SM_W(INTOUT(1), aw);
		return TRUE;
	case 2:
		V(("v_exit_cur[%d]\n", h));
		clear_window();
		break;
	case 3:
		V(("v_enter_cur[%d]\n", h));
		vwk[h]->clipping = FALSE;
		set_clipping(vwk[h]);
		vwk[h]->wrmode = MD_REPLACE;
		vwk[h]->text_color = BLACK;
		vwk[h]->fill_color = WHITE;
		init_font(vwk[h]);
		clear_window();
		ax = ay = 0;
		ach = CHAR_HEIGHT();
		acw = 8;
		ah = vdi_h / ach;
		aw = vdi_w / acw ;
		break;
	case 4:
		V(("v_curup[%d]\n", h));
		if (ay > 0)
			ay--;
		break;
	case 5:
		V(("v_curdown[%d]\n", h));
		if (ay < ah - 1)
			ay++;
		break;
	case 6:
		V(("v_curright[%d]\n", h));
		if (ax < aw - 1)
			ax++;
		break;
	case 7:
		V(("v_curleft[%d]\n", h));
		if (ax > 0)
			ax--;
		break;
	case 8:
		V(("v_curhome[%d]\n", h));
		ax = ay = 0;
		break;
	case 9:
		V(("v_eeos[%d]\n", h));
		XClearArea(display, xw, 0, ay * ach, vdi_w, vdi_h - ay * ach, False);
		XBUF(XClearArea(display, xbuffer, 0, ay * ach, vdi_w, vdi_h - ay * ach, False);)
		break;
	case 10:
		V(("v_eeol[%d]\n", h));
		XClearArea(display, xw, ax * acw, ay * ach, vdi_w - ax * acw, ach, False);
		XBUF(XClearArea(display, xbuffer, ax * acw, ay * ah, vdi_w - ax * acw, ach, False);)
		break;
	case 11:
		V(("vs_curaddress[%d]: %d, %d\n", h, LM_W(INTIN(0)), LM_W(INTIN(1)) ));
		ax = MIN(aw - 1, LM_W(INTIN(0)));
		ay = MIN(ah - 1, LM_W(INTIN(1)));
		break;
	case 12:
		V(("v_curtext[%d]\n", h));
		XDrawString16(display, xw, vwk[h]->gc, ax * aw, ay * ah, (XChar2b *) INTIN(0), V_NINTIN);
		XBUF(XDrawString16(display, xbuffer, vwk[h]->gc, ax * aw, ay * ah, (XChar2b *) INTIN(0), V_NINTIN);)
		break;
	case 13:
		V(("v_rvon[%d]\n", h));
		break;
	case 14:
		V(("v_rvoff[%d]\n", h));
		break;
	case 15:
		V(("vq_curaddress[%d]\n", h));
		SM_W(INTOUT(0), ax);
		SM_W(INTOUT(1), ay);
		break;
	case 16:
		V(("vq_tabstatus[%d]\n", h));
		return FALSE;
	case 17:
		V(("v_hardcopy[%d]\n", h));
		return FALSE;
	case 18:
		V(("v_dspcur[%d]\n", h));
		return FALSE;
	case 19:
		V(("v_rmcur[%d]\n", h));
		return FALSE;
	case 20:
		V(("v_form_adv[%d]\n", h));
		return FALSE;
	case 21:
		V(("v_output_window[%d]\n", h));
		return FALSE;
	case 22:
		V(("v_clear_disp_list[%d]\n", h));
		return FALSE;
	case 23:
		V(("v_bit_image[%d]\n", h));
		return FALSE;
	case 77:
		V(("vq_calibrate[%d]\n", h));
		return FALSE;
	default:
		V(("vdi_escape[%d]: %d\n", h, op));
		return FALSE;
	}
	return !UPDATE_PHYS;
}

/********************************************************************/

static int vst_point(void)
{
	int p;
	int i;
	int h = V_HANDLE;

	p = LM_W(INTIN(0));
	V(("vst_point[%d]: %d\n", h, p));
	CHECK_V_HANDLE(h);
	for (i = 0; i < SYSFONTS-1; i++)
	{
		if (sfinfo[i+1].point > p)
			break;
	}
	SM_W(PTSOUT(0), sfinfo[i].width);
	SM_W(PTSOUT(1), sfinfo[i].height);
	SM_W(PTSOUT(2), sfinfo[i].cellwidth);
	SM_W(PTSOUT(3), sfinfo[i].cellheight);
	SM_W(INTOUT(0), sfinfo[i].point);
	change_font(h, i);
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vr_trnfm(void)
{
	UL s = LM_UL(CONTRL(7));
	UL sa = LM_UL(MEM(s));
	UL d = LM_UL(CONTRL(9));
	UL da = LM_UL(MEM(d));
	int height = LM_W(MEM(s + 6));

	V(("vr_trnfm[%d]: (%d/%d)[%d] S: 0x%lx (0x%lx[%d]), D: 0x%lx (0x%lx[%d]) {%d,%d,%d}\n",
			(sa == 0 || sa == vbase),	/* fr_s */
			(da == 0 || da == vbase),	/* to_s */
			LM_W(MEM(s + 12)),
			V_HANDLE,
			(long)s,
			(long)sa,
			LM_W(MEM(s + 10)),
			(long)d,
			(long)da,
			LM_W(MEM(d + 10)),
			LM_W(MEM(s + 4)),			/* width */
			height,
			LM_W(MEM(s + 8))));
	CHECK_V_HANDLE(V_HANDLE);

#if 0
	XRectangle r;
	int width, height, i, bpl, bpp, j, n;
	int h = V_HANDLE;

	width = LM_W(MEM(s + 4));

	bpl = LM_W(MEM(s + 8)) * 2;
	bpp = bpl * height;
	n = LM_W(MEM(s + 12));
	/* FIXME for in-place */
	if (sf == 0)
	{
		for (i = 0; i < height; i++)
			for (j = 0; j < n; j++)
				memcpy(MEM(da) + bpl * i + j * bpp, MEM(sa) + i * (bpl + j) * 4, bpl);
	} else
	{
		for (i = 0; i < height; i++)
			for (j = 0; j < n; j++)
				memcpy(MEM(da) + (j + 1) * bpl * 4, MEM(sa) + i * bpl + j * bpp, bpl);
	}
#else
	if (sa == 0)
		sa = vbase;
	if (da == 0)
		da = vbase;
	memcpy(MEM(da), MEM(sa), LM_W(MEM(s + 8)) * 2 * height * LM_W(MEM(s + 12)));
#endif
	return !UPDATE_PHYS;
}

/********************************************************************/

static int vst_effects(void)
{
	int e;
	int h = V_HANDLE;

	e = LM_W(INTIN(0));
	V(("vst_effects[%d]: %d\n", h, e));
	CHECK_V_HANDLE(h);
	vwk[h]->font_eff = e;
	return CALL_ROM_VDI;
}

/********************************************************************/

static int v_clsvwk(void)
{
	int h = V_HANDLE;

	V(("v_clsvwk[%d]\n", V_HANDLE));
	CHECK_V_HANDLE(h);
	do_clswk(h);
	return CALL_ROM_VDI;
}

/********************************************************************/

static int v_clswk(void)
{
	int h = V_HANDLE;
	
	V(("v_clswk[%d]\n", h));
	CHECK_V_HANDLE(h);
	if (h == phys_handle)
	{
		close_all_wk();
		phys_handle = -1;
	} else
	{
		do_clswk(h);
	}
	return CALL_ROM_VDI;
}

/********************************************************************/

static int vqt_fontinfo(void)
{
	int h = V_HANDLE;
	VWK *v = vwk[h];
	sysfontinfo *sf = &sfinfo[v->font_index];
	
	V(("vqt_fontinfo[%d]: ", h));
	CHECK_V_HANDLE(h);
	SM_W(INTOUT(0), 0); /* minade */
	SM_W(INTOUT(1), 255); /* maxade */
	SM_W(PTSOUT(0), sf->cellwidth); /* max width */
	SM_W(PTSOUT(1), sf->bottom);
	SM_W(PTSOUT(2), sf->left_offset + sf->right_offset);
	SM_W(PTSOUT(3), sf->descent);
	SM_W(PTSOUT(4), sf->left_offset);
	SM_W(PTSOUT(5), sf->half);
	SM_W(PTSOUT(6), sf->right_offset);
	SM_W(PTSOUT(7), sf->ascent);
	SM_W(PTSOUT(8), 0);
	SM_W(PTSOUT(9), sf->top);
	V_NINTOUT(2);
	V_NPTSOUT(5);

	V(("(%d,%d, %d,%d,%d,%d,%d, %d, %d,%d,%d)\n",
		LM_W(INTOUT(0)), LM_W(INTOUT(1)),
		LM_W(PTSOUT(1)), LM_W(PTSOUT(3)), LM_W(PTSOUT(5)), LM_W(PTSOUT(7)), LM_W(PTSOUT(9)),
		LM_W(PTSOUT(0)),
		LM_W(PTSOUT(2)), LM_W(PTSOUT(4)), LM_W(PTSOUT(6)) ));

	return CALL_ROM_VDI; /* xxx */
}

/********************************************************************/

static int vqt_attributes(void)
{
	int h = V_HANDLE;
	VWK *v = vwk[h];
	sysfontinfo *sf = &sfinfo[v->font_index];

	V(("vqt_attributes[%d]: ", h));
	CHECK_V_HANDLE(h);
	SM_W(INTOUT(0), 1);
	SM_W(INTOUT(1), v->text_color);
	SM_W(INTOUT(2), v->text_rotation);
	SM_W(INTOUT(3), v->font_hor);
	SM_W(INTOUT(4), v->font_ver);
	SM_W(INTOUT(5), v->wrmode);
	SM_W(PTSOUT(0), sf->width);
	SM_W(PTSOUT(1), sf->height);
	SM_W(PTSOUT(2), sf->cellwidth);
	SM_W(PTSOUT(3), sf->cellheight);
	V_NINTOUT(6);
	V_NPTSOUT(2);
	V(("(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\n", LM_W(INTOUT(0)), LM_W(INTOUT(1)), LM_W(INTOUT(2)), LM_W(INTOUT(3)), LM_W(INTOUT(4)), LM_W(INTOUT(5)), LM_W(PTSOUT(0)), LM_W(PTSOUT(1)), LM_W(PTSOUT(2)), LM_W(PTSOUT(3)) ));

	return CALL_ROM_VDI; /* xxx */
}

/********************************************************************/

static int vql_attributes(void)
{
	int h = V_HANDLE;
	VWK *v = vwk[h];

	V(("vql_attributes[%d]: ", h));
	CHECK_V_HANDLE(h);
	SM_W(INTOUT(0), v->line_type);
	SM_W(INTOUT(1), v->line_color);
	SM_W(INTOUT(2), v->wrmode);
	SM_W(INTOUT(3), v->line_ends / 3);
	SM_W(INTOUT(4), v->line_ends % 3);
	SM_W(PTSOUT(0), v->line_width);
	SM_W(PTSOUT(1), 0);
	V_NINTOUT(5);
	V_NPTSOUT(1);
	V(("(%d,%d,%d,%d,%d,%d)\n", LM_W(INTOUT(0)), LM_W(INTOUT(1)), LM_W(INTOUT(2)), LM_W(INTOUT(3)), LM_W(INTOUT(4)), LM_W(PTSOUT(0)) ));
	return CALL_ROM_VDI; /* xxx */
}

/********************************************************************/

static int vqm_attributes(void)
{
	int h = V_HANDLE;
	VWK *v = vwk[h];

	V(("vql_attributes[%d]: ", h));
	CHECK_V_HANDLE(h);
	SM_W(INTOUT(0), v->marker_type);
	SM_W(INTOUT(1), v->marker_color);
	SM_W(INTOUT(2), v->wrmode);
	SM_W(PTSOUT(0), v->marker_height);
	SM_W(PTSOUT(1), v->marker_height);
	V_NINTOUT(3);
	V_NPTSOUT(1);
	V(("(%d,%d,%d,%d,%d)\n", LM_W(INTOUT(0)), LM_W(INTOUT(1)), LM_W(INTOUT(2)), LM_W(PTSOUT(0)), LM_W(PTSOUT(1)) ));
	return CALL_ROM_VDI; /* xxx */
}

/********************************************************************/

static int vqf_attributes(void)
{
	int h = V_HANDLE;
	VWK *v = vwk[h];

	V(("vqf_attributes[%d]: ", h));
	CHECK_V_HANDLE(h);
	SM_W(INTOUT(0), v->fill_interior);
	SM_W(INTOUT(1), v->fill_color);
	SM_W(INTOUT(2), v->fill_style);
	SM_W(INTOUT(3), v->wrmode);
	SM_W(INTOUT(4), v->fill_perimeter);
	V_NINTOUT(5);
	V_NPTSOUT(0);
	V(("(%d,%d,%d,%d,%d)\n", LM_W(INTOUT(0)), LM_W(INTOUT(1)), LM_W(INTOUT(2)), LM_W(INTOUT(3)), LM_W(INTOUT(4)) ));
	return CALL_ROM_VDI; /* xxx */
}

/********************************************************************/

static int vq_extnd(void)
{
	V(("vq_extnd[%d]\n", V_HANDLE));
	CHECK_V_HANDLE(V_HANDLE);
	/* TODO: different physical workstations */
	memcpy(INTOUT(0), &work_out_buf[0], 45*2);
	memcpy(PTSOUT(0), &work_out_buf[45], 12*2);
	return CALL_ROM_VDI;
}

/********************************************************************/

IGNORE(v_cellarray)
NYI(vs_color)
NYI(vq_color)
IGNORE(vq_cellaray)
NYI(v_contourfill)
NYI(v_get_pixel)
NYI(vqin_mode)
NYI(vqt_extent)
NYI(vqt_width)
NYI(vst_load_fonts)
NYI(vst_unload_fonts)
IGNORE(v_show_c)
IGNORE(v_hide_c)
IGNORE(vex_butv)
IGNORE(vex_motv)
IGNORE(vex_curv)
IGNORE(vex_timv)
NYI(vqt_name)

/********************************************************************/

static int (*vdifunc[]) (void) =
{
	v_opnwk,						/* X */
	v_clswk,
	v_clrwk,
	v_updwk,
	vdi_escape,
	v_pline,
	v_pmarker,
	v_gtext,
	v_fillarea,
	v_cellarray,
	vdi_gdp,
	vst_height,						/* X 5, 13 */
	vst_rotation,
	vs_color,
	vsl_type,						/* X */
	vsl_width,						/* X */
	vsl_color,
	vsm_type,
	vsm_height,
	vsm_color,
	vst_font,
	vst_color,
	vsf_interior,
	vsf_style,
	vsf_color,
	vq_color,
	vq_cellaray,
	v_locator,						/* vrq_, vsm_   */
	v_valuator,						/* -''-          */
	v_choice,						/* ''-          */
	v_string,						/* X  -''-           */
	vswr_mode,
	vsin_mode,						/* X */
	nothing,
	vql_attributes,
	vqm_attributes,
	vqf_attributes,
	vqt_attributes,
	vst_alignment,
	nothing, nothing, nothing, nothing, nothing, nothing, nothing, nothing,
	nothing, nothing, nothing, nothing, nothing, nothing, nothing, nothing,
	nothing, nothing, nothing, nothing, nothing, nothing, nothing, nothing,
	nothing, nothing, nothing, nothing, nothing, nothing, nothing, nothing,
	nothing, nothing, nothing, nothing, nothing, nothing, nothing, nothing,
	nothing, nothing, nothing, nothing, nothing, nothing, nothing, nothing,
	nothing, nothing, nothing, nothing, nothing, nothing, nothing, nothing,
	nothing, nothing, nothing, nothing,
	v_opnvwk,
	v_clsvwk,
	vq_extnd,
	v_contourfill,
	vsf_perimeter,
	v_get_pixel,
	vst_effects,
	vst_point,
	vsl_ends,
	vro_cpyfm,
	vr_trnfm,						/* X */
	vsc_form,
	vsf_udpat,
	vsl_udsty,						/* X */
	vr_recfl,
	vqin_mode,
	vqt_extent,
	vqt_width,
	vex_timv,						/* X */
	vst_load_fonts,
	vst_unload_fonts,
	vrt_cpyfm,						/* X */
	v_show_c,						/* X */
	v_hide_c,						/* X */
	vq_mouse,						/* X */
	vex_butv,						/* X */
	vex_motv,
	vex_curv,
	vq_key_s,						/* X */
	vs_clip,						/* X */
	vqt_name,
	vqt_fontinfo,
};

#define N_VDIFUNC (sizeof(vdifunc)/sizeof(vdifunc[0]))
int vdi_done = 0;

int Vdi(void)
{
	W c;
	UL pblock;
	
	if (vdi)
	{
	    if ( !vdi_done ) {
		vdi_init();
		vdi_done = 1;
		OnExit( vdi_exit );
	    }

		pblock = DREG(1);
#if TRACE_VDI
		last_pblock = pblock;
#endif
#if 0
		SET_IPL(7);
#endif
		control = LM_UL(MEM(pblock));
		intin = LM_UL(MEM(pblock + 4));
		ptsin = LM_UL(MEM(pblock + 8));
		intout = LM_UL(MEM(pblock + 12));
		ptsout = LM_UL(MEM(pblock + 16));
		c = V_OPCODE;
		if (c >= 1 && c <= N_VDIFUNC)
		{
			if (vdifunc[c - 1] ())
			{
				return TRUE;
			}
		} else
		{
			V(("VDI%d[%d]\n", V_OPCODE, V_HANDLE));
		}
	}
	return FALSE;
}


static void fix_vars(int width, int height, int planes)
{
	UW *a;

	T(("Fixing Line-A variables... %dx%dx%d\n", width, height, planes));
	a = (UW *)MEM(abase);
	SM_W(a + 1, (width * planes) / 8);
	SM_W(a - 1, (width * planes) / 8);
	SM_W(a - 23, CHAR_HEIGHT());
	SM_W(a - 22, width / 8 - 1);
	SM_W(a - 21, height / CHAR_HEIGHT() - 1);
	SM_W(a - 20, ((width * planes) / 8) * CHAR_HEIGHT());
	SM_W(a - 6, width);
	SM_W(a - 2, height);
	SM_W(a - 0x2b4 / 2, width - 1);
	SM_W(a - 0x2b2 / 2, height - 1);
#if 0
	fprintf(stderr, "FBASE=%08x, CUR_FONT=%08x, DEF_FONT=%08x, [%d,%d,%d,%d]\n",
		LM_UL(a + 0x54 / 2), LM_UL(a - 0x38a / 2), LM_UL(a - 0x1cc / 2),
		LM_W(a - 0x1f2 / 2), LM_W(a - 0x1f0 / 2), LM_W(a - 0x1ee / 2), LM_W(a - 0x1ec / 2));
	if (CHAR_HEIGHT() == 16)
	{
		SM_UL(a - 0x1cc / 2, LM_UL(MEM(fontbase + 8)));
		SM_UL(a - 0x38a / 2, LM_UL(MEM(fontbase + 8)));
	}
	fprintf(stderr, "-> FBASE=%08x, CUR_FONT=%08x, DEF_FONT=%08x\n",
		LM_UL(a + 0x54 / 2), LM_UL(a - 0x38a / 2), LM_UL(a - 0x1cc / 2));
#endif
}

void vdi_printtab(void)
{
	int i;
	
	V(("vdi_maptab: (%d/%d planes) %s\n", scr_planes, vdi_planes, priv_cmap?"private":"global"));
	for (i = 0; i < (1 << scr_planes); i++)
	{
		V(("%3d -> %3d  %8lx\n", i, vdi_maptab[i], (long)mapcol[vdi_maptab[i]] ));
	}
}

void vdi_post(void)
{
	UW *a;
	UL pblock;
	
	SP += 6;
	pblock = LM_UL(MEM(SP));
	pc = pblock;
	SP += 4;
	pblock = LM_UL(MEM(SP));
	SP += 4;
	
#if TRACE_VDI
	if (vdi)
	{
		if (pblock != last_pblock)
			V(("vdi_post: !pblock\n"));
		if (control != LM_UL(MEM(pblock)))
			V(("vdi_post: !control\n"));
	}
#endif
	control = LM_UL(MEM(pblock));
	if (V_OPCODE != 1 && V_OPCODE != 100)
	{
		return;
	}
	intin = LM_UL(MEM(pblock + 4));
	ptsin = LM_UL(MEM(pblock + 8));
	intout = LM_UL(MEM(pblock + 12));
	ptsout = LM_UL(MEM(pblock + 16));
	a = (UW *)MEM(abase);
	vdi_planes = LM_W(a);
	if (fix_screen)
	{
		SM_W(INTOUT(0), vdi_w - 1);
		SM_W(INTOUT(1), vdi_h - 1);
		fix_vars(vdi_w, vdi_h, vdi_planes);
	}
	if (!vdi)
		return;
	
	/* fix the lookup table */
	if (V_OPCODE == 1)
	{
		if (LM_W(INTIN(0)) <= LAST_SCREEN_DEVICE)
		{
			int i;

			for (i = 0; i < MAX_VDI_COLS; i++)
				vdi_maptab[i] = (1 << vdi_planes) - 1;
			for (i = 0; i < (1 << vdi_planes); i++)
			    if ( indexed_color )
				vdi_maptab[i] = vdi_maptab16[i] & ((1 << vdi_planes) - 1);
			    else switch ( vdi_planes ) {
			      case 1:
				  vdi_maptab[i] = (i & 1)? BlackPixel(display,XDefaultScreen(display))
				      : WhitePixel(display,XDefaultScreen(display));
				  break;
			      default:
				  vdi_maptab[i] = pixel_value_by_index[vdi_maptab16[i]];
				  break;
			    }
				
			phys_handle = V_HANDLE;
			init_vwk(phys_handle, phys_handle);
#if TRACE_VDI
			V(("v_opnwk[%d]: %d\n", LM_W(INTIN(0)), V_HANDLE));
			vdi_printtab();
			V(("workout:\n"));
			for (i = 0; i < 45; i++)
				V(("%d: %d\n", i, LM_W(INTOUT(i))));
			for (i = 0; i < 12; i++)
				V(("%d: %d\n", i+45, LM_W(PTSOUT(i))));
#endif
		} else
		{
			init_vwk(V_HANDLE, V_HANDLE);
		}
	} else
	{
		V(("v_opnvwk[%d]: %d\n", phys_handle, V_HANDLE));
		init_vwk(V_HANDLE, phys_handle);
	}
	memcpy(&work_out_buf[0], INTOUT(0), 45*2);
	memcpy(&work_out_buf[45], PTSOUT(0), 12*2);
}

void Init_Linea(void)
{
	abase = DREG(0);
	fontbase = AREG(1);
	fprintf(stderr,"Initializing Line-A... (abase=%08lx)\n", (long)abase);
	if (fix_screen)
		fix_vars(scr_width, scr_height, scr_planes);
}
