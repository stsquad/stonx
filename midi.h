/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef MIDI_H
#define MIDI_H

#include "defs.h"

extern void init_midi(void);
extern void midi_send(unsigned char data);

#endif /* MIDI_H */
