/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 *
 *	SVGALIB (Linux) Stuff,
 *	Martin Griffiths, January 1996.
 *	
 * BUGS:	
 * - if the -refresh rate is too high, starting up (switching modes?) is
 *   not reliable (hangs sometimes, especially if the sound server is 
 *   running). Apparently SVGAlib doesn't like it when interrupts are
 *   nested. Need to check that the timer signal handlers can't nest.
 *   Perhaps a solution would be to set old_shiftmod only *after* change_mode()
 *   has been called (in case it is interrupted before it switches the modes;
 *   then it would stay in the old mode, thinking it's in the current mode).
 *   [no, didn't fix it]
 */
 
#include "config.h"
#include "options.h"

static int dummy;

#include "defs.h"
#include "mem.h"
#include "screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vga.h> 
#include <vgagl.h>
#include <vgamouse.h>
#include <vgakeyboard.h>
#include "ikbd.h"

#define COLS 16

static int oldvga_mode;
static volatile int old_shiftmod=0;	/* last known shift mode, used to detect changes */
static int display_mode=0;/* curr. displayed mode (*shiftmod changed by inter.*/
static B *buf;
static int mx=320,my=200,mb=0;
static int refcount=0;
static UW old_color[16];	/* last known color palette */
int kmin,kmax;

extern int svga_process_arg(char *optname, int argc, char *argv[]);
extern void svga_show_arghelp(FILE *f);

Machine_Specific machine = {
    "VGA",                        /* QUESTION: Should this be SVGA? */
    svga_process_arg,
    svga_show_arghelp,
    NULL,                         /* No process_signal */
    svga_screen_open,
    svga_screen_close,
    svga_screen_shifter,
    svga_init_keys,
    NULL
};

/*
  Process a special VGA machine option

  argv[0] is the first argument of the option
  argc    is the max. number of arguments

  Return:
      -1  it's not a special VGA machine option
       0  option without argument processed
    else  number of arguments processed
*/
int svga_process_arg(char *optname, int argc, char *argv[]) {
    /* It's not a special VGA machine option */
    return -1;
}

/*
  Show useage/help of all special VGA machine options 
*/
void svga_show_arghelp(FILE *f) {
}

/*
 *	Here we have the stuff to convert between PC and ST scancodes 
 */

#define SCANCODE_0 11
#define SCANCODE_1 2
#define SCANCODE_2 3
#define SCANCODE_3 4
#define SCANCODE_4 5 
#define SCANCODE_5 6
#define SCANCODE_6 7 
#define SCANCODE_7 8
#define SCANCODE_8 9
#define SCANCODE_9 10
#define SCANCODE_F11 87
#define SCANCODE_F12 88
#define SCANCODE_LEFTSHIFT 42
#define SCANCODE_RIGHTSHIFT 54
#define SCANCODE_BS 14
#define SCANCODE_TAB 15
#define SCANCODE_MINUS 12
#define SCANCODE_EQUAL 13
#define SCANCODE_SQ_OPEN 26
#define SCANCODE_SQ_CLOSE 27
#define SCANCODE_SEMIC 39
#define SCANCODE_COMMA 51
#define SCANCODE_DOT 52
#define SCANCODE_SLASH 53
#define SCANCODE_KEYPADMULT 55 
#define SCANCODE_KEYPADDOT 83 
#define SCANCODE_KEYPADDIV 98 
#define SCANCODE_CAPSLOCK 58
#define SCANCODE_APOST 40
#define SCANCODE_GRAVE 41
#define SCANCODE_BACKSL 43
#define SCANCODE_ISO 86 
#define SCANCODE_PAUSE 96 
#define SCANCODE_PRTSCR 99
#define SCANCODE_PGUP 104 
#define SCANCODE_PGDOWN 109 
#define SCANCODE_INS 110
#define SCANCODE_DEL 111

static int keyflag[256];

static int svga_keytab[][2] =
{
	{ SCANCODE_F1,ST_F1 },
	{ SCANCODE_F2,ST_F2 },
	{ SCANCODE_F3,ST_F3 },
	{ SCANCODE_F4,ST_F4 },
	{ SCANCODE_F5,ST_F5 },
	{ SCANCODE_F6,ST_F6 },
	{ SCANCODE_F7,ST_F7 },
	{ SCANCODE_F8,ST_F8 },
	{ SCANCODE_F9,ST_F9 },
	{ SCANCODE_F10,ST_F10 },

	{ SCANCODE_ESCAPE,ST_ESC },
	{ SCANCODE_1,ST_1 },
	{ SCANCODE_2,ST_2 },
	{ SCANCODE_3,ST_3 },
	{ SCANCODE_4,ST_4 },
	{ SCANCODE_5,ST_5 },
	{ SCANCODE_6,ST_6 },
	{ SCANCODE_7,ST_7 },
	{ SCANCODE_8,ST_8 },
	{ SCANCODE_9,ST_9 },
	{ SCANCODE_0,ST_0 },
	{ SCANCODE_MINUS,ST_MINUS },
	{ SCANCODE_EQUAL,ST_EQUAL },

	{ SCANCODE_BACKSL,ST_BACKSL },
	{ SCANCODE_GRAVE,ST_GRAVE }, /* '_ */
	{ SCANCODE_APOST,ST_APOST }, 

	{ SCANCODE_ISO,ST_ISO },

	{ SCANCODE_BS,ST_BS },
	{ SCANCODE_TAB,ST_TAB },
	{ SCANCODE_Q,ST_Q },
	{ SCANCODE_W,ST_W },
	{ SCANCODE_E,ST_E },
	{ SCANCODE_R,ST_R },
	{ SCANCODE_T,ST_T },
	{ SCANCODE_Y,ST_Y },
	{ SCANCODE_U,ST_U },
	{ SCANCODE_I,ST_I },
	{ SCANCODE_O,ST_O },
	{ SCANCODE_P,ST_P },
	{ SCANCODE_SQ_OPEN,ST_SQ_OPEN },	/* [ */
	{ SCANCODE_SQ_CLOSE,ST_SQ_CLOSE },	/* ] */
	{ SCANCODE_ENTER,ST_RETURN },

	{ SCANCODE_LEFTCONTROL,ST_CONTROL },
	{ SCANCODE_A,ST_A },
	{ SCANCODE_S,ST_S },
	{ SCANCODE_D,ST_D },
	{ SCANCODE_F,ST_F },
	{ SCANCODE_G,ST_G },
	{ SCANCODE_H,ST_H },
	{ SCANCODE_J,ST_J },
	{ SCANCODE_K,ST_K },
	{ SCANCODE_L,ST_L },
	{ SCANCODE_SEMIC,ST_SEMIC },
	{ SCANCODE_LEFTSHIFT,ST_LSH },
	{ SCANCODE_Z,ST_Z },
	{ SCANCODE_X,ST_X },
	{ SCANCODE_C,ST_C },
	{ SCANCODE_V,ST_V },
	{ SCANCODE_B,ST_B },
	{ SCANCODE_N,ST_N },
	{ SCANCODE_M,ST_M },
	{ SCANCODE_COMMA,ST_COMMA },
	{ SCANCODE_DOT,ST_DOT },
	{ SCANCODE_SLASH,ST_SLASH },
	{ SCANCODE_RIGHTSHIFT,ST_RSH },
	{ SCANCODE_LEFTALT,ST_ALT },
	{ SCANCODE_SPACE,ST_SPACE },
	{ SCANCODE_CAPSLOCK,ST_CAPSLOCK },
	{ SCANCODE_KEYPAD0,ST_KP_0 },
	{ SCANCODE_KEYPAD1,ST_KP_1 },
	{ SCANCODE_KEYPAD2,ST_KP_2 },
	{ SCANCODE_KEYPAD3,ST_KP_3 },
	{ SCANCODE_KEYPAD4,ST_KP_4 },
	{ SCANCODE_KEYPAD5,ST_KP_5 },
	{ SCANCODE_KEYPAD6,ST_KP_6 },
	{ SCANCODE_KEYPAD7,ST_KP_7 },
	{ SCANCODE_KEYPAD8,ST_KP_8 },
	{ SCANCODE_KEYPAD9,ST_KP_9 },
	{ SCANCODE_CURSORBLOCKUP,ST_UP },
	{ SCANCODE_CURSORBLOCKLEFT,ST_LEFT },
	{ SCANCODE_CURSORBLOCKRIGHT,ST_RIGHT }, 
	{ SCANCODE_CURSORBLOCKDOWN,ST_DOWN }, 
	{ SCANCODE_INS,ST_INSERT },
	{ SCANCODE_DEL,ST_DELETE },
	{ SCANCODE_KEYPADENTER,ST_KP_ENTER },
	{ SCANCODE_KEYPADPLUS,ST_KP_PLUS },
	{ SCANCODE_KEYPADMINUS,ST_KP_MINUS },
	{ SCANCODE_KEYPADMULT,ST_KP_MULT },
	{ SCANCODE_KEYPADDOT,ST_KP_DOT },
	{ SCANCODE_KEYPADDIV,ST_KP_DIV },
	{ SCANCODE_PRTSCR,ST_HELP },
	{ SCANCODE_PAUSE,ST_UNDO },
	{ SCANCODE_PGUP,ST_KP_OPEN },
	{ SCANCODE_PGDOWN,ST_KP_CLOSE },
	{ SCANCODE_RIGHTALT,ST_CAPSLOCK },
	{ SCANCODE_RIGHTCONTROL,ST_CONTROL }
};

void svga_init_keys()
{	extern int *keycodes;
	unsigned int i;
#if 0 
	fprintf(stderr,"SVGALIB Keycode convertor initialising...\n");
#endif
	kmin = 0;
	kmax = 255;
	keycodes = (int *)malloc(256 * sizeof(int));
	for (i=0 ; i < 256 ; i++)
	{	keycodes[i] = ST_UNDEF;	
		keyflag[i] = 0;
	}
	for (i=0 ; i < sizeof(svga_keytab)/(sizeof(int)*2) ; i++)
		keycodes[svga_keytab[i][0]] = svga_keytab[i][1];
}

static void mymouse_handler(int button, int dx, int dy)
{	mx += dx;
	my += dy;
	mb = button;
}

static int lastcode;

static void mykey_handler(int scancode,int state)
{
	if (keyflag[scancode] == state)
 		return;			/*already in this state */
    	else
		keyflag[scancode] = state;
	switch (scancode)
	{	case SCANCODE_F11:
			mc68000_reset();
			break;	
		case SCANCODE_F12:
			(void) svga_screen_close();
			/*fprintf(stderr,"Last code was:%d\n",lastcode);*/
			exit(0);
			break;
		default:
			break;	
	}
	lastcode=scancode;
	ikbd_key(scancode,(state==KEY_EVENTPRESS));
}

static void process_events(void)
{	
	if (mouse_update())
	{	ikbd_pointer(mx,my,639,399);
		ikbd_button(1, (mb & MOUSE_LEFTBUTTON) != 0);
		ikbd_button(2, (mb & MOUSE_RIGHTBUTTON) != 0);
	}
	keyboard_update();
}


/* Change the window colormap according to the contents of old_color[].
 * The colors are converted from ST/STe format (least-significant 9/12 bits
 * of a 16 bit integer) to the 6 bit per RGB component format used in VGA.
 */

static void change_colors (void)
{	int i;
	unsigned long pixels[COLS];

	if (display_mode == 2)
	{
		gl_setpalettecolor(0,63,63,63);
		gl_setpalettecolor(15,0,0,0); 
	}
	else for (i=0; i<COLS; i++)
	{	int r,g,b,c;
		c = (i < (display_mode == 0 ? 16 : 4) ? old_color[i] : 0);
#if !STE
		c &= 0x777;
#endif
		r 	= (((c>>8)&7)<<3)|((c>>11)<<2);
		g	= (((c>>4)&7)<<3)|(((c>>7)&1)<<2);
		b	= ((c&7)<<3)|(((c>>3)&1)<<2);
		gl_setpalettecolor(i,r,g,b);
	}

}

/* Do something appropriate when the 'shiftmod' hardware register has changed:
 */

static void change_mode (void)
{
	int i;

	display_mode = shiftmod;

	scr_width = scr_def[display_mode].w;
    scr_height = scr_def[display_mode].h;
    scr_planes=4>>display_mode;

	if (chunky)
	{
		for (i=0; i<scr_height/CHUNK_LINES; i++)
			draw_chunk[i]=1;
	}
	switch (display_mode)
	{
		case 0:
			vga_setmode(G320x200x256);
			gl_setcontextvga(G320x200x256);
			change_colors();
			break;

		case 1:
			vga_setmode(G640x200x16);
			gl_setcontextvga(G640x200x16);
			change_colors();
			break;

		case 2:
			vga_setmode(G640x480x2);
			gl_setcontextvga(G640x480x2);
			change_colors();
			break;
	}
	mouse_seteventhandler(mymouse_handler); 
}

/* Update the screen using the shifter mode known when change_mode was last
 * called...
 */

static void show_screen (void)
{
	switch (display_mode)
	{
		case 0:	
			{
				int i;
				unsigned char *x=vga_getgraphmem();
				st16c_to_z ((unsigned char *)MEM(vbase), x);
				if (chunky) for (i=0; i<scr_height/CHUNK_LINES; i++)
								draw_chunk[i]=0;
			}
			break;
		case 1:
			{	unsigned char *m=(unsigned char *)buf;
				int i,j,k;		
				st4c_to_z200 ((unsigned char *)MEM(vbase), m);
				if (chunky)
				{
					for (i=0,j=0; i<200; i+=CHUNK_LINES, j++)
					{
						if (draw_chunk[j])
						{
							for (k=0; k<CHUNK_LINES; k++,m+=640)
								vga_drawscanline(i+k,m);
							draw_chunk[j]=0;
						}
						else m+=CHUNK_LINES*640;
					}
				}
				else for (i=0; i<200; i++,m+=640) vga_drawscanline(i,m);
			} 
			break;
		case 2: 
			{	unsigned char *m=(unsigned char *)MEM(vbase);
				int i;
#ifdef CHUNKY_NOT_IMPLEMENTED_YET_FOR_SVGA_AND_MONO
				int j,k;		
				if (chunky)
				{
					for (i=0,j=0; i<400; i+=CHUNK_LINES, j++)
					{
						if (draw_chunk[j])
						{
							for (k=0; k<CHUNK_LINES; k++,m+=80)
								vga_drawscanline(i+k,m);
							draw_chunk[j]=0;
						}
						else m+=80*CHUNK_LINES;
					}
				}
				else
#endif
				for (i=0 ; i<400 ;i++,m+=80) vga_drawscanline(i,m);
			} 
			break;
	
	}
#if 0 
	vga_waitretrace();
#endif

}

/* This function is called periodically (from a signal handler) to check what's
 * new with the shifter mode, colors, and update ("shift") the screen. It
 * should probably rewritten to check whether it's necessary to update the
 * screen, since for color modes this is a very costly operation...
 *
 * The Shifter status information is changed in hw.c, where the hardware regs
 * are read.
 */

void svga_screen_shifter (void)
{
	int i, w, pal = 0;

	process_events();	
	if (++refcount < refresh) return;
	refcount=0;

	for (i=0; i<16; i++)
	{
		if ((w = LM_UW(&color[i])) != old_color[i])
		{
			pal = 1;	/* palette has changed */
			old_color[i] = w;
		}
	}

	if (pal) change_colors();	/* update the window colormap */

	if (old_shiftmod != shiftmod)
	{
		change_mode();
		old_shiftmod = shiftmod;
	}

	show_screen();
}

void svga_screen_open(void)
{	vga_init();
	vga_setmousesupport(1);
/*	vga_runinbackground(1);*/
	oldvga_mode = vga_getcurrentmode();
	mouse_init("/dev/mouse",vga_getmousetype(),16);
#if 1
	change_mode();
	show_screen();
#endif
	if (keyboard_init() != 0) exit(0);
	keyboard_seteventhandler(mykey_handler);
    	buf = (B *)malloc (640*400*4);
	depth = 1;
	priv_cmap = 1;
}

void svga_screen_close (void)
{	mouse_close();
	keyboard_close();
	free(buf);
	vga_setmode(oldvga_mode);
}
