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

extern int tos1;
extern long tosstart, tosstart1, tosstart2, tosstart4;
extern long tosend, tosend1, tosend2, tosend4;
extern long romstart, romstart1, romstart2, romstart4;
extern long romend, romend1, romend2, romend4;

extern int stonx_exit(void); /* does some killing but no exit() */
extern int stonx_shutdown(int mode); /* React on atari shutdown 
				      * (0 = halt, 1 = warmboot, 2 = coldboot)
				      */

#endif /* MAIN_H */

