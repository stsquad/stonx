/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef TOSDEFS_H
#define TOSDEFS_H

#include "defs.h"
#include "mem.h"

typedef struct
{
	W ph_branch;
	L ph_misc[6];
	W ph_absflag;
} PH;

#define FONTBASE16	0x2000
#define PROGBASE	0x10000
#define SCREENBASE	0x20000
#define STACKBASE	0x40000
#define RESETSSP	0x40000
#define RESETPC		0x50000
#define ROMBASE		RESETPC

/* Hardware registers / memory-mapped I/O */

/* General: */
#define M_MEMCONF	0xff8001

/* Shifter: */
#define M_DBASEH	0xff8201
#define M_DBASEL	0xff8203
#if STE
#define M_DBASELOW	0xff820d
#endif
#define M_VCOUNTHI	0xff8205
#define M_VCOUNTMID	0xff8207
#define M_VCOUNTLOW	0xff8209
#define M_SYNCMODE	0xff820a
#define M_COLORS	0xff8240
#define M_SHIFTMD	0xff8260

/* MFP: */
#define M_GPIP		0xfffa01
#define M_AER		0xfffa03
#define M_DDR		0xfffa05
#define M_IERA		0xfffa07
#define M_IERB		0xfffa09
#define M_IPRA		0xfffa0b
#define	M_IPRB		0xfffa0d
#define M_ISRA		0xfffa0f
#define M_ISRB		0xfffa11
#define M_IMRA		0xfffa13
#define M_IMRB		0xfffa15
#define M_VR		0xfffa17
#define M_TACR		0xfffa19
#define M_TBCR		0xfffa1b
#define	M_TCDCR		0xfffa1d
#define M_TADR		0xfffa1f
#define M_TBDR		0xfffa21
#define M_TCDR		0xfffa23
#define M_TDDR		0xfffa25
#define M_SCR		0xfffa27
#define M_UCR		0xfffa29
#define M_RSR		0xfffa2b
#define M_TSR		0xfffa2d
#define M_UDR		0xfffa2f

/* ACIA */
#define M_KEYCTL1	0xfffc00
#define M_KEYCTL2	0xfffc01
#define M_KEYBD		0xfffc02

/* AY-3-8910 */
#define M_GIREAD	0xff8800
#define M_GISELECT	M_GIREAD
#define M_GIWRITE	0xff8802

/* MIDI-ACIA left out... */
/* DMA left out */

#endif /* TOSDEFS_H */
