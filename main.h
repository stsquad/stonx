/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef MAIN_H
#define MAIN_H

/* still a bit unflexible, but it will have to do for now... */
#define MODE_MONO 	0
#define MODE_COLOR	1
extern int gr_mode;

/* for monochrome mode only, so far */
extern int scr_width, scr_height;

extern int vdi, vdi_mode;
extern int boot_dev;
extern int verbose;
extern int fix_screen;
extern int timer_a;
extern int midi;
extern int realtime;

extern int stonx_exit(void); /* does some killing but no exit() */
extern int stonx_shutdown(int mode); /* React on atari shutdown 
				      * (0 = halt, 1 = warmboot, 2 = coldboot)
				      */

#endif /* MAIN_H */

