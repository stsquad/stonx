/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef CONFIG_H
#define CONFIG_H

/* allow gcc -ansi compilation if wanted: */
#ifndef __STRICT_ANSI__
#define NZREG "%ebx"
#ifndef __cplusplus
/* g++ seems to dislike using the frame pointer register for a register
 * variable...
 */
#define PCREG "%ebp"
#endif /* __cplusplus */
#endif /* __STRICT_ANSI__ */
#undef SRREG
#define IS_BIG_ENDIAN 0
#define SMALL 0 /* MJK: i686-2.0.34-linux can do, hope others too */

#define STONX_AUDIO_LINUX

#define STONX_JOYSTICK_LINUX
#define HW_SUPPORTS_JOYSTICK

#endif /* CONFIG_H */
