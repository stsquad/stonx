/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef XLIB_VDI_H
#define XLIB_VDI_H

#include <X11/Xlib.h>

extern int vdi_w, vdi_h, vdi_done;
extern Window xw;

extern void vdi_dispatch (void);
extern void vdi_redraw (int x,int y,int w,int h);
extern void vdi_post (void);
extern int Vdi (void);
extern void linea_post (UL as);
extern void linea_init (void);
extern void init_vdi (void);
extern void Init_Linea (void);
extern int vdi_output_c(char c);
extern void set_window_name(Window id, char *name);

#endif /* XLIB_VDI_H */

