/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef GENDEFS_H
#define GENDEFS_H

#include "defs.h"
#include "am.h"
#include "mem.h"
#include "emulator.h"
#include "native.h"

/* 0=8 expanded */
#define QBITS UBITS

#define BYTE B
#define WORD W
#define LONG L
#define UBYTE UB
#define UWORD UW
#define ULONG UL

#define MASKBYTE BMASK
#define MASKWORD WMASK
#define MASKLONG LMASK
#define MASKUBYTE BMASK
#define MASKUWORD WMASK
#define MASKULONG LMASK

#define CS_BYTE CS_B
#define CS_WORD CS_W
#define CS_LONG CS_L
#define CL_BYTE CL_B
#define CL_WORD CL_W
#define CL_LONG CL_L
#define SR_BYTE SR_B
#define SR_WORD SR_W
#define SR_LONG SR_L
#define LR_BYTE LR_B
#define LR_WORD LR_W
#define LR_LONG LR_L
#define LM_BYTE LM_B
#define LM_WORD LM_W
#define LM_LONG LM_L

#define LDREG_BYTE LDREG_B
#define LDREG_UBYTE LDREG_UB
#define LDREG_WORD LDREG_W
#define LDREG_UWORD LDREG_UW
#define LDREG_LONG LDREG_L
#define LDREG_ULONG LDREG_UL
#define SDREG_BYTE SDREG_B
#define SDREG_WORD SDREG_W
#define SDREG_LONG SDREG_L
#define SDREG_UBYTE SDREG_UB
#define SDREG_UWORD SDREG_UW
#define SDREG_ULONG SDREG_UL

#define DREG_BYTE DREG_B
#define DREG_WORD DREG_W
#define DREG_LONG DREG_L

#define SIZEBYTE 1
#define SIZEWORD 2
#define SIZELONG 4
#define SIZEUBYTE 1
#define SIZEUWORD 2
#define SIZEULONG 4

#define TEST_T (1)
#define TEST_F (0)
#define TEST_LS test_cond(3)
#define TEST_HI test_cond(2)
#define TEST_CS (CCR & MASK_CC_C)
#define TEST_CC (!TEST_CS) 
#define TEST_EQ (nz_save == 0) 
#define TEST_NE (!TEST_EQ) 
#define TEST_VS (CCR & MASK_CC_V) 
#define TEST_VC (!TEST_VS) 
#define TEST_MI (nz_save < 0) 
#define TEST_PL (!TEST_MI) 
#define TEST_GE test_cond(12)
#define TEST_LT test_cond(13)
#define TEST_GT test_cond(14)
#define TEST_LE test_cond(15)

#define SET_VCX_ADD do {\
		if ((x^s) >= 0 && (s^r) < 0) sr |= MASK_CC_V;\
		if ((x&s) < 0 || (~r & (x|s)) < 0) sr |= MASK_CC_C|MASK_CC_X;\
		} while(0)

#define SET_VCX_SUB do {\
	if ((x^s) < 0 && (s^r) >= 0) sr |= MASK_CC_V;\
	if ((r&s) < 0 || (~x & (r|s)) < 0) sr |= MASK_CC_C|MASK_CC_X;\
		} while(0)
#define CHECK_TRACE() do{if (sr&SR_T) {flags &= ~F_TRACE1; flags |= F_TRACE0;}}while(0)
#define CHECK_NOTRACE() do{if ((sr&SR_T)==0) flags &= ~(F_TRACE0|F_TRACE1);}while (0)
#define SBITS(_l,_h,_x)	((int) ((((_x)>>(_l))^(1<<((_h)-(_l))))&\
						((1<<((_h)-(_l)+1))-1)) - (1<<((_h)-(_l))))
#if 0
#define EXTB(_x) ((int)(((_x)^0x80)&0xff)-128)
#endif

#define UBITS(_l,_h,_x) ((((unsigned int)(_x))>>(_l))&((1<<((_h)-(_l)+1))-1))
#define BIT(_n,_x)	(((_x)>>(_n))&1)

#define UBITS(_l,_h,_x) ((((unsigned int)(_x))>>(_l))&((1<<((_h)-(_l)+1))-1))

#define V_ABS_L_BYTE() V_ABS_L
#define V_ABS_W_BYTE() V_ABS_W
#define V_AN_D_BYTE(_x) V_AN_D(_x)
#define V_AN_R_BYTE(_x) V_AN_R(_x)
#define V_AN_PD_BYTE(_x) V_AN_PD(_x,1)
#define V_AN_PI_BYTE(_x) V_AN_PI(_x,1)
#define V_AN_PD_BYTE_All(_x) V_AN_PD_B_All(_x)
#define V_AN_PI_BYTE_All(_x) V_AN_PI_B_All(_x)
#define V_AN_I_BYTE(_x) V_AN_I(_x)
#define V_PC_D_BYTE() V_PC_D
#define V_PC_R_BYTE() V_PC_R

#define V_ABS_L_WORD() V_ABS_L
#define V_ABS_W_WORD() V_ABS_W
#define V_AN_D_WORD(_x) V_AN_D(_x)
#define V_AN_R_WORD(_x) V_AN_R(_x)
#define V_AN_PD_WORD(_x) V_AN_PD(_x,2)
#define V_AN_PI_WORD(_x) V_AN_PI(_x,2)
#define V_AN_I_WORD(_x) V_AN_I(_x)
#define V_PC_D_WORD() V_PC_D
#define V_PC_R_WORD() V_PC_R

#define V_ABS_L_LONG() V_ABS_L
#define V_ABS_W_LONG() V_ABS_W
#define V_AN_D_LONG(_x) V_AN_D(_x)
#define V_AN_R_LONG(_x) V_AN_R(_x)
#define V_AN_PD_LONG(_x) V_AN_PD(_x,4)
#define V_AN_PI_LONG(_x) V_AN_PI(_x,4)
#define V_AN_I_LONG(_x) V_AN_I(_x)
#define V_PC_D_LONG() V_PC_D
#define V_PC_R_LONG() V_PC_R

#define V_IMM_BYTE() V_IMM_B
#define V_IMM_WORD() V_IMM_W
#define V_IMM_LONG() V_IMM_L

#define GENFUNC_PROTO(_x) void _x (unsigned int iw)

extern B BCD_ADD(B a, B b);
extern B BCD_SUB(B a, B b);

extern UB testcond[];

#if 1
static INLINE int test_cond (int x)
{
	FIX_CCR();
	return testcond[((CCR)<<4)|x];
}
#endif
#endif /* GENDEFS_H */
