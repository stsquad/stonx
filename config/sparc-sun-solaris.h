/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef CONFIG_H
#define CONFIG_H

/* configuration file for SPARCs running Solaris 2.X (schorse) */

#define SRREG "%g5"
#define PCREG "%g6"
#define NZREG "%g7"

#define IS_BIG_ENDIAN 1
#define STONX_AUDIO_SPARC
#define SOLARIS
#define SMALL 1

#endif /* CONFIG_H */
