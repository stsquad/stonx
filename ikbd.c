/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#include "defs.h"
#include "debug.h"
#include "mem.h"
#include "ikbd.h"
#include "screen.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int verbose;
#define KB_DEBUG 0
#define KEYMAP_HARDCODED 0

/* special codes sent by the IKBD: */
#define IKBD_STATUS		0xf6
#define IKBD_ABS_MOUSE	0xf7
#define IKBD_REL_MOUSE	0xf8
#define IKBD_TIMEOFDAY	0xfc
#define IKBD_BOTH_JOY	0xfd
#define IKBD_JOY0		0xfe
#define IKBD_JOY1		0xff

/* IKBD modes/state */
struct 
{
	unsigned
	int mouse_mode:2,
		btn_mode:1,
		btn_rep:2,
		joy_mode:3,
		data_on:1,
		y_at_top:1,
		mode:3;
} ikbd;

#define MODE_MOUSE_KEYCODE	0x0002
#define MODE_DATA_ON		0x0001
#define MODE_FIRE_BUTTON	3

#define JOY_OFF		0
#define JOY_FIRE	1
#define JOY_EVENT	2
#define JOY_INTERR	3
#define JOY_MONITOR	4

#define MOUSE_OFF	0
#define MOUSE_REL	1
#define MOUSE_ABS	2
#define MOUSE_KEY	3

#define BTN_DEFAULT	0
#define BTN_KEYS	1

#define STATUS_VALID (ikbd.joy_mode != JOY_FIRE && ikbd.joy_mode != JOY_MONITOR)
#define MAXARGS		8
#define MAXBUF 		16383
static int mode, mousemode;
static int needed, argnum=0;
static int y_at_top = 1;
static int mktx, mkty;		/* mouse key threshold */
static int thresx, thresy; 	/* mouse motion threshold */
static int ascalex, ascaley;/* absolute mode scale */
static int command;
static int args[MAXARGS];
#define GETW(_x) ((args[_x]<<8)+args[(_x)+1])
static int buffer[MAXBUF];
int ikbd_inbuf=0;
static int ikbd_bufpos=0;

int mouse_freeze=0;
int *keycodes;

/* screen.c */
extern char **ksyms;
#if 0
extern Display *display;
#endif
extern int kmin, kmax;

#if JOYSTICK
#define JOY_UP       1
#define JOY_DOWN     2
#define JOY_LEFT     4
#define JOY_RIGHT    8
#define JOY_BUTTON 128

int button_state = 0;
#endif

#ifdef STONX_JOYSTICK_LINUX
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <sys/ioctl.h>

void ikbd_send (int value);

int joy_fd;
int joy_state = 0;

void init_joystick(void) {
  char name[128];

  if((joy_fd = open ("/dev/js0", O_RDONLY | O_NONBLOCK))<0) {
    perror("opening /dev/js0");
  } else {
    if (ioctl(joy_fd, JSIOCGNAME(sizeof(name)), name) < 0)
      strncpy(name, "Unknown", sizeof(name));

    printf("Using joystick: %s\n", name);
  }
}

void kill_joystick(void) {
  if(joy_fd >= 0)
    close(joy_fd);
}

void joystick_update(void) {
  struct js_event e;
  static int axis[2], but=0;
  int new_state;

  if(joy_fd >= 0) {
    while(read (joy_fd, &e, sizeof(struct js_event)) 
	  == sizeof(struct js_event)) {
      switch(e.type) {
      case JS_EVENT_BUTTON:
	/* handle only one button */
	if(e.number == 0)
	  but = e.value;
	
	break;
	
      case JS_EVENT_AXIS:
	/* handle two axes */
	if((e.number == 0)||(e.number == 1)) {
	  if(e.value < -100)     axis[e.number] = -1;
	  else if(e.value > 100) axis[e.number] =  1;
	  else                   axis[e.number] =  0;
	}

	break;
      }
    }
  }

  new_state = 
    ((axis[1] == -1)?JOY_UP:0)|
    ((axis[1] ==  1)?JOY_DOWN:0)|
    ((axis[0] == -1)?JOY_LEFT:0)|
    ((axis[0] ==  1)?JOY_RIGHT:0)|
    ((but)?JOY_BUTTON:0);

  if(joy_state != new_state) {

    ikbd_send (0xff);
    ikbd_send (new_state);

    joy_state = new_state;
  }

  /* set button via right mouse button */
  if(button_state != but) {
    ikbd_button (2, but);
    button_state = but;
  }
}
#endif

/* This is the keycode translation table. It is dependent on your kind of X
 * terminal, and on the mapping you want to use for the ST keys.
 * To make your own table, you'll need to examine the `xmodmap -pke' output
 * on your system, and write the corresponding ST keycode as the item with
 * the index of the keycode you want it on in the array below.
 *
 * Maybe, sometime in the future, a translation table based on Keysyms would
 * be a better idea...
 */
#if 0
int keycodes[] =
{
#if KEYMAP_HARDCODED
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_F1, ST_F2, ST_F10, ST_F3, ST_UNDEF, ST_F4, ST_UNDEF, ST_F5, ST_UNDEF,
	ST_F6, ST_UNDEF, ST_F7, ST_F8, ST_F9, ST_ALT,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDO,
	ST_UNDEF,
	ST_UNDEF,
	ST_ESC,
	ST_1, ST_2, ST_3, ST_4, ST_5, ST_6, ST_7, ST_8, ST_9, ST_0, ST_MINUS,
	ST_EQUAL, ST_GRAVE, ST_BS,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_UNDEF,
	ST_DELETE,
	ST_UNDEF,
	ST_UNDEF,
	ST_TAB, ST_Q, ST_W, ST_E, ST_R, ST_T, ST_Y, ST_U, ST_I, ST_O, ST_P,
	ST_SQ_OPEN, ST_SQ_CLOSE, ST_DELETE, ST_UNDEF, ST_UNDEF, ST_UP,
	ST_UNDEF, ST_KP_MINUS,
	ST_UNDEF, ST_UNDEF, ST_UNDEF, ST_UNDEF, ST_CONTROL, ST_A, ST_S, ST_D, ST_F,
	ST_G, ST_H, ST_J, ST_K, ST_L, ST_SEMIC, ST_APOST, ST_BACKSL, ST_RETURN,
	ST_KP_ENTER, ST_LEFT, ST_UNDEF, ST_RIGHT, ST_INSERT, ST_UNDEF, ST_UNDEF,
	ST_UNDEF, ST_UNDEF, ST_LSH, ST_Z, ST_X, ST_C, ST_V, ST_B, ST_N, ST_M,
	ST_COMMA, ST_DOT, ST_SLASH, ST_RSH, ST_UNDEF, ST_UNDEF, ST_DOWN, ST_UNDEF,
	ST_UNDEF, ST_UNDEF, ST_UNDEF, ST_HELP, ST_CAPSLOCK, ST_UNDEF, ST_SPACE,
	ST_UNDEF, ST_UNDEF, ST_UNDEF, ST_KP_PLUS
#else
#include "keytab.c"
#endif
};
#endif

int ikbd_readbuf (void)
{
	int pos = (ikbd_bufpos - ikbd_inbuf)&MAXBUF;
	UB x;
	if (ikbd_inbuf-- > 0)
	{
		if (ikbd_inbuf == 0)
		{
			/* Clear GPIP/I4 */
			SM_UB(MEM(0xfffc00),0);
			x = LM_UB(MEM(0xfffa01));
			x |= 0x10;
			SM_UB(MEM(0xfffa01),x);
		}
#if KB_DEBUG
		fprintf(stderr, "IKBD read code %2x (%d left)\n", buffer[pos], ikbd_inbuf);
#endif
		return buffer[pos];
	}
	else
	{
		ikbd_inbuf = 0;
		return 0xa2;
	}
}

void ikbd_send (int value)
{
	int pos;
	UB x;
	value &= 0xff;
	if (ikbd_inbuf <= MAXBUF)
	{
		buffer[ikbd_bufpos] = value;
		ikbd_bufpos++;
		ikbd_bufpos &= MAXBUF;
		ikbd_inbuf++;
	}
	if ((LM_UB(MEM(0xfffa09)) & 0x40) == 0) return;
	if (DEBUG) D(("IKBD sends %2x (->buffer pos %d)\n", value, ikbd_bufpos-1));
	/* set Interrupt Request */
	x = LM_UB(MEM(0xfffc00));
	x |= 0x80 | 0x01;
	SM_UB(MEM(0xfffc00),x);
	/* signal ACIA interrupt */
	flags |= F_ACIA;
	/* IPRB/I4 is not set at all */
	/* GPIP/I4 */
	x = LM_UB(MEM(0xfffa01));
	x &= ~0x10;
	SM_UB(MEM(0xfffa01),x);
	/* ISRB/I4 */
	x = LM_UB(MEM(0xfffa11));
	x |= 0x40;
	SM_UB(MEM(0xfffa11),x);
}

void ikbd_key (int keycode, int pressed)
{
	int k = keycodes[keycode];
	if (!pressed) k |= 0x80;
	ikbd_send (k);
}

static int btn[3];

static int ox=0, oy=0, px=0,py=0;

void ikbd_send_relmouse (void)
{
	ikbd_send (0xf8 | (btn[0]<<1) | btn[1]);
	ikbd_send (px-ox);
	ikbd_send (py-oy);
	ox = px;
	oy = py;
}

void ikbd_adjust (int dx, int dy)
{
	int tx=dx,ty=dy;

	while (dx < -128 || dx > 127 || dy < -128 || dy > 127)
	{
		if (dx < -128) tx=-128;
		else if (dx > 127) tx=127;
		if (dy < -128) ty=-128;
		else if (dy > 127) ty=127;
		ikbd_send (0xf8 | (btn[0]<<1) | btn[1]);
		ikbd_send (tx);
		ikbd_send (ty);
		dx -= tx;
		dy -= ty;
	}
	if (dy != 0 || dx != 0)
	{
		ikbd_send (0xf8 | (btn[0]<<1) | btn[1]);
		ikbd_send (dx);
		ikbd_send (dy);
	}
	ox=px;
	oy=py;
}

void ikbd_force(void)
{
	px = ox;
	py = ox;
	if (mouse_freeze) return;
	ikbd_send_relmouse();
}
void ikbd_button (int button, int pressed)
{
        /* handle two buton mice */
        if(button == 3) button = 2;

	btn[button-1] = pressed;
#if KB_DEBUG
	fprintf (stderr, "IKBD sends button %d press (%d)\n",button,pressed);
#endif
	ikbd_send_relmouse();
}

void ikbd_pointer (int x, int y, int max_x, int max_y)
{
	px = x;
	py = y;
	if (mouse_freeze) return;
#if !NO_FLOODING
	ikbd_send_relmouse();
#endif
}

static void set_mouse_button_action (int action_mode)
{
	if (action_mode & 4)
		ikbd.btn_mode = BTN_KEYS;
	else
	{
		ikbd.btn_mode = BTN_DEFAULT;
		ikbd.btn_rep = action_mode & 3;
	}
}

static void set_abs_mouse (int xmax, int ymax)
{
}

static void set_mouse_keycode (int dx, int dy)
{
	ikbd.mouse_mode = MOUSE_KEY;
	mktx = dx;
	mkty = dy;
}

static void set_mouse_threshold (int x, int y)
{
	thresx = x;
	thresy = y;
}

static void set_mouse_scale (int x, int y)
{
	if (ikbd.mouse_mode != MOUSE_ABS) return;
	ascalex = x;
	ascaley = y;
}

static void load_mouse_pos (int sx, int sy)
{
}

static void set_joy_monitor (int rate)
{
}

static void set_joy_keycode (int rx, int ry, int tx, int ty, int vx, int vy)
{
}

static void clock_set (int YY, int MM, int DD, int hh, int mm, int ss)
{
	/* ignored */
}

static void memory_load (int address, int num, int *data)
{
}

static void memory_read (int address)
{
}

static void reset (void)
{
}

static void controller_execute (int address)
{
}

static void report_mouse_mode (void)
{
}

static void report_mouse_threshold (void)
{
}

static void report_mouse_scale (void)
{
}

static void report_mouse_y_at_top (void)
{
}

static void report_mouse_enable (void)
{
}

static void report_joystick_mode (void)
{
}

static void report_joystick_enable (void)
{
}

static void interrogate_mouse (void)
{
}

static void interrogate_joy (void)
{
}

static void interrogate_clock (void)
{
}

void ikbd_execute (int data)
{
	if (needed)	/* need more */
	{
		args[argnum++] = data;
		if (--needed == 0)
		{
			switch (command)
			{
				case 0x80:	if (args[0] == 1) reset(); break;
				case 0x07:	set_mouse_button_action (args[0]); break;
				case 0x09:	set_abs_mouse (GETW(0), GETW(2)); break;
				case 0x0a:	set_mouse_keycode (args[0], args[1]); break;
				case 0x0b:	set_mouse_threshold (args[0], args[1]); break;
				case 0x0c:	set_mouse_scale (args[0], args[1]); break;
				case 0x0e:	load_mouse_pos (GETW(1), GETW(3)); break;
				case 0x17:	set_joy_monitor (args[0]); break;
				case 0x19:	set_joy_keycode (args[0], args[1], args[2],
											args[3], args[4], args[5]); break;
				case 0x1b:	clock_set (args[0], args[1], args[2], args[3],
										args[4], args[5]); break;
				case 0x20:	if (argnum < args[2]) needed += args[2];
							else memory_load (GETW(0), args[2], &(args[3]));
							break;
				case 0x21:	memory_read (GETW(0)); break;
				case 0x22:	controller_execute (GETW(0)); break;
			}
		}
		argnum = 0;
	}
	else /* a command starts */
	switch (command = data)
	{
		case 0x80:	needed = 1; break;
	
		/* set mouse button action */
		case 0x07:	needed = 1; break;

		/* set relative mouse positioning */
		case 0x08:	ikbd.mouse_mode = MOUSE_REL; break;

		/* set absolute mouse positioning */
		case 0x09:	needed = 4; break;

		/* set mouse keycode mode */
		case 0x0a:	needed = 2; break;

		/* status inquiry for the above */
		case 0x88:	
		case 0x89:
		case 0x8a:	if (STATUS_VALID) report_mouse_mode(); break;

		/* set mouse threshold */
		case 0x0b:	needed = 2; break;
		case 0x8b:	if (STATUS_VALID) report_mouse_threshold(); break;

		/* set mouse scale */
		case 0x0c:	needed = 2; break;
		case 0x8c:	if (STATUS_VALID) report_mouse_scale(); break;

		/* interrogate mouse position */
		case 0x0d:	interrogate_mouse (); break;

		/* load mouse position */
		case 0x0e:	needed = 5; break;

		/* set y=0 at bottom */
		case 0x0f:	ikbd.y_at_top = 0; break;

		/* set y=0 at top */
		case 0x10:	ikbd.y_at_top = 1; break;
		case 0x8f:	
		case 0x90:	if (STATUS_VALID) report_mouse_y_at_top (); break;

		/* resume */
		case 0x11:	ikbd.data_on = 1; break;

		/* disable mouse */
		case 0x12:	ikbd.mouse_mode = MOUSE_OFF; break;
		case 0x92:	if (STATUS_VALID) report_mouse_enable(); break;

		/* pause output */
		case 0x13:	ikbd.data_on = 0; break;

		/* set joystick event reporting */
		case 0x14:	ikbd.joy_mode = JOY_EVENT; break;

		/* set joystick interrogation mode */
		case 0x15:	ikbd.joy_mode = JOY_INTERR; break;

		/* joystick interrogation */
		case 0x16:	interrogate_joy(); break;

		/* set joystick monitoring */
		case 0x17:	needed = 1; break;

		/* set fire button monitoring */
		case 0x18:	ikbd.joy_mode = JOY_FIRE; break;

		/* set joystick keycode mode */
		case 0x19:	needed = 6; break;

		case 0x94:
		case 0x95:
		case 0x99: if (STATUS_VALID) report_joystick_mode(); break;

		/* disable joysticks */
		case 0x1a:	ikbd.joy_mode = JOY_OFF; break;
		case 0x9a:	if (STATUS_VALID) report_joystick_enable(); break;

		/* time-of-day clock set (ignored) */
		case 0x1b:	needed = 6; break;

		/* interrogate time-of-day clock */
		case 0x1c:	interrogate_clock(); break;

		/* memory load (will increase `needed' later) */
		case 0x20:	needed = 3; break; 

		/* memory read */
		case 0x21:	needed = 2; break;

		/* controller execute */
		case 0x22:	needed = 2; break;
	}
}

void init_ikbd(void)
{	machine.init_keys();
	SM_UB(MEM(0xfffc00),0x0e);
}

/* called when a byte is sent to the IKBD */
void ikbd_send_byte (B v)
{
#if KB_DEBUG
	fprintf(stderr,"Sending %d to the IKBD\n",(int)v&0xff);
#endif
	if (v == 22)
	{
		ikbd_send(0xfe);
		ikbd_send(0);
	}
}

