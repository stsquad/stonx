/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef OPTIONS_H
#define OPTIONS_H

#include "config.h"

/* Some systems can't handle large BSS segments, so this may be set
 * in config.h
 */
#ifndef SMALL
#define SMALL 1
#endif

/* Draw everything to a pixmap as well in the Xlib-VDI driver so we can 
 * redraw at exposure events?
 */
#define XBUFFER 1

/* Blitter emulation */
#define BLITTER 1

/* Unix filesystem interface */
#define UNIX_FS 1

/* Double Timer-C interrupts */
#define DOUBLE_HZ200 1

/* Generator options */
/*
 *  default is GEN_FUNCTAB
 *  GEN_LABELTAB is gcc-only
 *  THREADING doesn't work
#define GEN_LABELTAB
#define THREADING
#define GEN_SWITCH
 */
#define GEN_FUNCTAB

/* make sure we can handle the generated code */
#if defined(GEN_LABELTAB) && !defined(__GNUC__)
#undef GEN_LABELTAB
#define GEN_SWITCH
#endif

#define TIMER_A 1

/* set this to 1 for hardcoded AltGr emulation with german keyboards */
#define ALTGR_HARDCODED 1

/* Set this to 1, if you want the cursor be grabbed by STonX Main Window.
 * At this mode you may stop emulator using key PAUSE.
*/
#define GRABMODE 0

/* Set this to 1, if you want to show a small cross to represent the
 * X-Cursor at STonX Main Window (cannot be combined with GRABMODE 1)
 * You can switch on and of the X-Cursor using key PAUSE.
 */
#define SHOW_X_CURSOR 0

/* Set this to 1, if you want to emulate left double click with mouse
 * button 2. If this is off and SHOW_X_CURSOR is on, button 2 also
 * switches X-Cursor on and off.
 */
#define SIMULATE_DOUBLE_CLICK 0

/* Set this to 1, if you want to ungrab STonX Main Window with mouse button 2.
 * This only works if GRABMODE is 1 and SIMULATE_DOUBLE_CLICK is 0.
 */
#define MOUSE2_UNGRABS 1

/* Set this to 1, if you want SHIFT-PAUSE to exit STonX
 * set this to 0, if you want SHIFT-PAUSE to init configuration.
 * NOTE: configuration menu does work or not, seems to be buggy 
 */
#define SHIFT_PAUSE_EXITS 1

/* Set this to 1, if you want STonXFS for MiNT */
#define MINT_STONXFS 1

/* Set this to 1, if you want to use the STonXFS-Bypass at STonX
 * This only works, if MINT_STONXFS is set to 1
 */
#define MINT_USE_BYPASS 0

/* Set this to 1, if you want /dev/serial2 support for MiNT.
 * This only works, if MODEM1 is set to 0
 */
#define MINT_SERIAL 1

/* Set this to 1 for simply MODEM1 emulation.
 * This only works, if MINT_SERIAL is set to 0  */
#define MODEM1 0 

/* Set this to 1 for communication device for MiNT */
#define MINT_COM 1

/* Set this to 1, if you want Atari- to X-clipboard mapping and v.v. */
#define CLIPBOARD 0 /* Does not work, will result in compiler errors */

#ifdef HW_SUPPORTS_JOYSTICK
/* Set this to 1, if you want joystick support */
#define JOYSTICK 1
#else
#define JOYSTICK 0
#endif

/* Set this to 1 if you want STonX to emulate natively the BIOS.   */
/* - This is untested and as long as you are using a real tos.img, */
/* there is no need for this, so let it set at 0                   */
#define NATIVE_BIOS 0


/* Set this to 1 if you want a Monitor with which to observe the */
/* internal state of the emulator and trace through the code     */
#ifndef MONITOR
#define MONITOR 0
#endif



/*
 * Don't change following tests 
 */

#if GRABMODE
# undef SHOW_X_CURSOR
# define SHOW_X_CURSOR 0
#endif

#if MODEM1 && MINT_SERIAL
#error You can only set one of MODEM1 and MINT_SERIAL to 1
#endif

/* the following are mostly obsolete, please ignore... */
#define STE 0
#define DEBUG 0
#define CARTRIDGE 1
#define WATCH 0
#define TRACE 0
#define MMAP 0
#define NO_FLOODING 0
#define PROTECT_ROM 1
#define SAFE 0
#define PROFILE 0
#define SFP 0
#define MIDI 0

#endif /* OPTIONS_H */
