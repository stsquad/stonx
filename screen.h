/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */

#ifndef SCREEN_H
#define SCREEN_H

#include <stdio.h>

typedef struct
{
    const char * const name;                 /* Name of machine driver */
    int  (*process_arg)(char *optname, int argc, char *argv[]); /* process a argument */
    void (*show_arghelp)(FILE *f);                   /* show more help */
    void (*process_signal)(int sig);

    void (*screen_open)(void);
    void (*screen_close)(void);
    void (*screen_shifter)(void);
    void (*init_keys)(void);
    void (*fullscreen)(int *x, int *y);
} Machine_Specific;
extern Machine_Specific machine;
#define COLS 16
#define CHUNK_LINES 8
#define MAX_CHUNKS 1000
#define MAX_AGE 20

extern void svga_screen_open(void);
extern void svga_screen_close(void);
extern void svga_screen_shifter(void);
extern void svga_init_keys(void);
extern void x_screen_open(void);
extern void x_screen_close(void);
extern void x_screen_shifter(void);
extern void x_init_keys(void);
extern void st16c_to_z (unsigned char *st, unsigned char *data);
extern void st4c_to_z200 (unsigned char *st, unsigned char *data);
extern void st4c_to_z (unsigned char *st, unsigned char *data);
extern void stmono_to_xy (unsigned char *st, unsigned char *data);

extern int chunk_age[MAX_CHUNKS];
extern int *keycodes;

typedef struct
{
	int w, h;
} SCRDEF;

extern SCRDEF scr_def[];
extern int draw_chunk[];

/*------------------------------------------------------------------*/
/* This is our information about the current shifter mode/state: 	*/
/* (from hw.c) 														*/
extern int shiftmod;	/* shift mode 								*/
extern UW *color;		/* palette, rasters not possible! :-( 		*/
extern volatile UL vbase;		/* screen address (Physbase) 				*/
/*------------------------------------------------------------------*/
extern UL abase;
extern int scr_width,scr_height,scr_planes;
extern int priv_cmap;
extern char mapcol[COLS];
extern int chunky,depth;
extern int scanlines;
extern int shiftmod,refresh;
#endif
