/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#ifndef AM_H
#define AM_H

#include "mem.h"
#include "amdefs.h"

#define V_AN_I(_r) AREG(_r)

#define V_AN_PI_B_All(_r) (AREG(_r)++,\
					((_r)==7?(AREG(_r)++,AREG(_r)-2):(AREG(_r)-1)))
#define V_AN_PD_B_All(_r) (AREG(_r)--,\
					((_r)==7?(AREG(_r)--,AREG(_r)):AREG(_r)))

/* Assume r!=A7 and s!=1 */
#define V_AN_PI(_r,_s) (AREG(_r) += _s,AREG(_r)-(_s))
#define V_AN_PD(_r,_s) (AREG(_r) -= _s)

#define V_AN_D(_r) (pc+=2, AREG(_r)+LM_W(MEM(pc-2)))/* BUG:Alignment! */
#define V_ABS_W (pc+=2, LM_W(MEM(pc-2))) /* BUG: Alignment! */
#define V_ABS_L (pc+=4, LM_UL(MEM(pc-4))) /* BUG: Alignment! */

#define V_IMM_B (pc+=2, pc-1)
#define V_IMM_W (pc+=2, pc-2)
#define V_IMM_L (pc+=4, pc-4)

static INLINE L V_AN_R(int reg)
{
	int x = LM_W(MEM(pc));	/* BUG: Alignment! */
	pc += 2;
	if (!BIT(11,x))
		return AREG(reg)+SBITS(0,7,x)+LDREG_W(UBITS(12,15,x));
	return AREG(reg)+SBITS(0,7,x)+DREG(UBITS(12,15,x));
}
#define V_PC_D (pc += 2, pc-2+LM_W(MEM((pc-2))))/* BUG: Alignment! */
#define V_PC_R xV_PC_R()
static INLINE L xV_PC_R(void)
{
	int x = LM_W(MEM(pc));	/* BUG: Alignment! */
	pc += 2;
	if (!BIT(11,x))
		return pc-2+SBITS(0,7,x)+LDREG_W(UBITS(12,15,x));
	return pc-2+SBITS(0,7,x)+DREG(UBITS(12,15,x));
}

#define AN_I(_r) MEM(V_AN_I(_r))
#define AN_PI(_r,_s) MEM(V_AN_PI(_r,_s))
#define AN_PD(_r,_s) MEM(V_AN_PD(_r,_s))
#define AN_D(_r) MEM(V_AN_D(_r))
#define ABS_W MEM(V_ABS_W)
#define ABS_L MEM(V_ABS_L)
#define AN_R(_r) MEM(V_AN_R(_r))
#define PC_D MEM(V_PC_D)
#define PC_R MEM(V_PC_R)

#endif /* AM_H */

