/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef AMDEFS_H
#define AMDEFS_H

#define AM_DREG 	0x0001
#define AM_AREG 	0x0002
#define AM_AN_I 	0x0004	/* Always allowed */
#define AM_AN_PI	0x0008
#define AM_AN_PD	0x0010
#define AM_AN_D		0x0020	/* Always allowed */
#define AM_AN_R		0x0040	/* Always allowed */
#define AM_ABS_W	0x0080	/* Always allowed */
#define AM_ABS_L	0x0100	/* Always allowed */
#define AM_IMM		0x0200
#define AM_PC_D		0x0400
#define AM_PC_R		0x0800

#define AM_X		0x0fff
#define AM_A		(AM_X & ~(AM_IMM|AM_PC_D|AM_PC_R))
#define AM_MA		(AM_A & ~(AM_DREG|AM_AREG))
#define AM_M		(AM_MA|AM_PC_D|AM_PC_R)
#define AM_DA		(AM_MA|AM_DREG)
#define AM_D		(AM_X & ~AM_AREG)
#define AM_C		(AM_X & ~(AM_DREG|AM_AREG|AM_AN_PI|AM_AN_PD|AM_IMM))
#define AM_CA		(AM_C & ~(AM_PC_D|AM_PC_R))	
#define AM_MVM_RM	(AM_CA|AM_AN_PD)	
#define AM_MVM_MR	(AM_C|AM_AN_PI)	
#define AM_DB		(AM_D & ~AM_DREG)
#define AM_DNI		(AM_D & ~AM_IMM)
#define AM_ANY		(-1)

#endif /* AMDEFS_H */

