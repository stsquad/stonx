/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef IO_H
#define IO_H

#include "defs.h"

extern void set_vbase(UL n);
extern void init_hardware(void);
extern void kill_hardware(void);

#if MODEM1
extern void write_serial (B v);
extern B read_serial (void);
extern int serial_stat (void);
extern void init_serial(void);
#endif /* MODEM1 */

extern int init_parallel(void);
extern void write_parallel(unsigned char c);
unsigned char read_parallel(unsigned char c);
extern void done_parallel(void);
extern int parallel_stat(void);

#if TIMER_A
extern long mfp_get_count(void);
#endif /* TIMER_A */

#endif /* IO_H */
