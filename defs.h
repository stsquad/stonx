/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef DEFS_H
#define DEFS_H

#define UL unsigned int
#define UW unsigned short
#define L int
#define W short
#define UB unsigned char
#define B signed char

#ifndef INLINE
#define INLINE
#endif

#define TRUE 1
#define FALSE 0

#include "config.h"
#include "options.h"
#include "utils.h"

#if defined(__GNUC__) && defined(PCREG)
register L pc asm (PCREG);
#else
extern L pc;
#endif

#if defined(__GNUC__) && defined(SRREG)
register UW sr asm (SRREG);
#else
extern UW sr;
#endif

#if defined(__GNUC__) && defined(NZREG)
register L nz_save asm (NZREG);
#else
extern L nz_save;
#endif

/* BUGS... */
/*#define ROMSTART 	0xfa0000*/
#define CARTSTART 	0xfa0000
#define CARTEND 	0xfc0000
#define STONXCOOKIEMAGIC "STon"
#define STONXCOOKIEJAR  0xfbfc00
#define STONXCOOKIEJAR_SIZE (CARTEND - STONXCOOKIEJAR)
#define STONXCOOKIEADDR 0xfbf800

#define	SIM_HARDWARE 1
#define COMPACT 0
#define INTS_8_16_32 1

#if INTS_8_16_32
#define IS_NEG_B(_x) ((_x)<0)
#define IS_NEG_W(_x) ((_x)<0)
#define IS_NEG_L(_x) ((_x)<0)
#define IS_0_B(_x) ((_x) == 0)
#define IS_0_W(_x) ((_x) == 0)
#define IS_0_L(_x) ((_x) == 0)

#else /* !INTS_8_16_32 */

typedef int W,L;
#define B char
typedef unsigned int UW,UL;
typedef unsigned char UB;

#define IS_NEG_B(_x) ((_x) & 0x80)
#define IS_NEG_W(_x) ((_x) & 0x8000)
#define IS_NEG_L(_x) ((_x) & 0x80000000)
#define IS_0_B(_x) (((_x) & 0xff) == 0)
#define IS_0_W(_x) (((_x) & 0xffff) == 0)
#define IS_0_L(_x) (((_x) & 0xffffffff) == 0)

#endif /* INTS_8_16_32 */

#define BSIZE 1
#define UBSIZE 1
#define WSIZE 2
#define UWSIZE 2
#define LSIZE 4
#define ULSIZE 4
#define	BMASK	0xff
#define WMASK	0xffff
#define LMASK	0xffffffff

#if PROFILE
extern int prof_op;
#endif
extern UL flags;
extern UW cw;
extern UL shadow_pc;

#define IPL_OK(_x) ((sr & 0x700) <= ((_x) << 8))
#define SET_IPL(_x) do {sr &= 0xf8ff; sr |= (_x) << 8;} while (0)

#define SR_T    0x8000
#define SR_S    0x2000
#define SR_I    0x0700

#define SUPERVISOR_MODE ((sr & SR_S))
#define SR_IMPLEMENTED (SR_T|SR_S|SR_I|MASK_CC)

#define T_TRACE		9
#define T_PRIVILEGED    8
#define T_OVERFLOW      7
#define T_CHK           6
#define T_DIV0          5
#define T_ILLEGAL       4
#define T_BUSERR	2
#define T_ADDRERR	3
#define T_LINEA		10
#define T_LINEF		11
#define T_VBL           28
#define T_200Hz         69 /*MFP Timer C       */
#define T_ACIA		70 /*MFP Keyboard/MIDI */
#define T_FDC_HDC       71 /*MFP fdc/hdc interrupt (not implemented)*/
#define T_HBL           72 /*MFP Horizontal blank counter*/
#define T_RCV_FULL	76
#define T_TIMERA	77
#define T_TRAP_0 32
#define T_TRAP_15 47
#define T_TRAP_GEMDOS 33
#define T_TRAP_GEM 34
#define T_TRAP_BIOS 45
#define T_TRAP_XBIOS 46

#define EXCEPTION_VECTOR(_e) ADDR((_e)*4)

/* flags - these are set when things happen - the main execute loop polls the flags global*/
#define F_VBL 		0x0001
#define F_200Hz 	0x0002
#define F_ACIA		0x0004
#define F_BLIT  	0x0008
#define F_TRACE0	0x0010
#define F_TRACE1	0x0020
#define F_MONITOR	0x0040
#define F_RCV_FULL	0x0080
#define F_200HzB	0x0100
#define F_PROFILE	0x0200
#define F_TIMERA_ON	0x0400
/*#define F_CONFIG	0x0800*/ /* Not needed */
#define F_HBL           0x1000

extern L dreg[];

#define USP AREG(8)
#define DREG(_x) dreg[_x]
#define AREG(_x) dreg[(_x)+8]

#if WORDS_BIGENDIAN
#define DREG_B(_x) &(((B *)dreg)[4*(_x)+3])
#define DREG_UB(_x) &(((UB *)dreg)[4*(_x)+3])
#define DREG_W(_x) &(((W *)dreg)[2*(_x)+1])
#define DREG_UW(_x) &(((UW *)dreg)[2*(_x)+1])
#else
#define DREG_B(_x) &(((B *)dreg)[4*(_x)])
#define DREG_UB(_x) &(((UB *)dreg)[4*(_x)])
#define DREG_W(_x) &(((W *)dreg)[2*(_x)])
#define DREG_UW(_x) &(((UW *)dreg)[2*(_x)])
#endif

#define DREG_L(_x) &(((L *)dreg)[_x])
#define DREG_UL(_x) &(((UL *)dreg)[_x])
#define DREG_S(_x,_s) &(((_s *)dreg)[(4/(sizeof(_s)))*(_x)+((4/sizeof(_s))-1)])
#define AREG_B(_x) DREG_B((_x)+8)
#define AREG_W(_x) DREG_W((_x)+8)
#define AREG_L(_x) DREG_L((_x)+8)

#define SP AREG(7)

#define SBITS(_l,_h,_x) ((int) ((((_x)>>(_l))^(1<<((_h)-(_l))))&\
						((1<<((_h)-(_l)+1))-1)) - (1<<((_h)-(_l))))
#define UBITS(_l,_h,_x) ((((unsigned int)(_x))>>(_l))&((1<<((_h)-(_l)+1))-1))
#define BIT(_n,_x)  (((_x)>>(_n))&1)

#define MASK_CC_X 16
#define MASK_CC_N 8
#define MASK_CC_Z 4
#define MASK_CC_V 2
#define MASK_CC_C 1
#define MASK_CC   31
#define CCR (sr & MASK_CC)
#define GET_X_BIT() ((sr >> 4)&1)

#ifndef MIN
#define MIN(_a,_b) ((_a)<(_b)?(_a):(_b))
#endif
#ifndef MAX
#define MAX(_a,_b) ((_a)>(_b)?(_a):(_b))
#endif

#define SET_NZ(_x) nz_save=(_x)
#define UNSET_NZ() nz_save=1
#define UNSET_Z() do{if(nz_save==0)nz_save=1;}while(0)
#define UNSET_N() do{if(nz_save<0)nz_save=1;}while(0)
#define SET_Z() nz_save=0
#define SET_N() nz_save=-1
#define FIX_CCR() do{ sr &= ~(MASK_CC_Z|MASK_CC_N);\
	if ((nz_save) < 0) sr |= MASK_CC_N; else if (nz_save==0) sr |= MASK_CC_Z;\
	} while(0);
#define CHECK_CCR() do{\
	if (CCR&MASK_CC_Z) nz_save=0; else if (CCR&MASK_CC_N) nz_save=-1;\
	else nz_save=1;} while(0)

#endif /* DEFS_H */
