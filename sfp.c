#include "defs.h"
#include "cpu.h"
#include "mem.h"
#include <stdio.h>

#define DEBUG_SFP 1

/* --- FPU stuff --- */
static UL FPcr,FPsr,FPiar;

void FP_FMOVE_FPcr(UL data, int reg)
{
	switch(reg)
	{
		case 1: FPiar = data; break;
		case 2: FPsr = data; break;
		case 4: FPcr = data; break;
	}
}

/* --- SFP stuff --- */
#define UNDEFD_L(_x) B LOAD_B_ ## _x(void){return 0;}
#define UNDEFD_S(_x) void STORE_B_ ## _x(B v){}

static UW 	sfp_response, sfp_control, sfp_save, sfp_restore, sfp_op,
			sfp_command, sfp_cond, sfp_select;
static UL	sfp_operand, sfp_iaddr, sfp_oaddr;

#define SET_BYTE(_x,_n,_v) _x &= ~(0xff<<((_n)*8)); _x |= ((_v)&0xff)<<((_n)*8);
#define GET_BYTE(_x,_n) (((_x)>>((_n)*8))&0xff)

B LOAD_B_fffa40(void)
{
	return GET_BYTE(sfp_response,1);
}

B LOAD_B_fffa41(void)
{
#if DEBUG_SFP
	fprintf(stderr,"Reading sfp_response: %04x\n",sfp_response);
#endif
	return GET_BYTE(sfp_response,0);
}

void STORE_B_fffa42(B v)
{
	SET_BYTE(sfp_control,1,v);
}

void STORE_B_fffa43(B v)
{
	SET_BYTE(sfp_control,0,v);
#if DEBUG_SFP
	fprintf(stderr,"sfp_control set to %04x\n",sfp_control);
#endif
}

void STORE_B_fffa40(B v)
{
	SET_BYTE(sfp_response,1,v);
}

void STORE_B_fffa41(B v)
{
	SET_BYTE(sfp_response,0,v);
#if DEBUG_SFP
	fprintf(stderr,"sfp_response set to %04x\n",sfp_response);
#endif
}

void STORE_B_fffa46(B v)
{
	SET_BYTE(sfp_restore,1,v);
}
void STORE_B_fffa47(B v)
{
	SET_BYTE(sfp_restore,0,v);
#if DEBUG_SFP
	fprintf(stderr,"sfp_restore set to %04x\n",sfp_restore);
#endif
}

void STORE_B_fffa4a(B v)
{
	SET_BYTE(sfp_command,1,v);
}

void STORE_B_fffa4b(B v)
{
	SET_BYTE(sfp_command,0,v);
#if DEBUG_SFP
	fprintf(stderr,"sfp_command set to %04x\n",sfp_command);
#endif
	switch(sfp_command>>13)
	{
		case 4:
			FP_FMOVE_FPcr(sfp_operand,(sfp_command>>10)&7);
			break;
		default:
			break;
	}
}

void STORE_B_fffa4e(B v)
{
	SET_BYTE(sfp_cond,1,v);
}
void STORE_B_fffa4f(B v)
{
	SET_BYTE(sfp_cond,0,v);
#if DEBUG_SFP
	fprintf(stderr,"sfp_cond set to %04x\n",sfp_cond);
#endif
}

void STORE_B_fffa50(B v)
{
	SET_BYTE(sfp_operand,3,v);
#if DEBUG_SFP
	fprintf(stderr,"sfp_operand set to %08lx\n",(long)sfp_operand);
#endif
}
void STORE_B_fffa51(B v)
{
	SET_BYTE(sfp_operand,2,v);
#if DEBUG_SFP
	fprintf(stderr,"sfp_operand set to %08lx\n",(long)sfp_operand);
#endif
}
void STORE_B_fffa52(B v)
{
	SET_BYTE(sfp_operand,1,v);
#if DEBUG_SFP
	fprintf(stderr,"sfp_operand set to %08lx\n",(long)sfp_operand);
#endif
}
void STORE_B_fffa53(B v)
{
	SET_BYTE(sfp_operand,0,v);
#if DEBUG_SFP
	fprintf(stderr,"sfp_operand set to %08lx\n",(long)sfp_operand);
#endif
}

/* bogus */
void STORE_B_fffa54(B v)
{
	SET_BYTE(sfp_select,1,v);
}
void STORE_B_fffa55(B v)
{
	SET_BYTE(sfp_select,0,v);
#if DEBUG_SFP
	fprintf(stderr,"sfp_select set to %04x\n",sfp_select);
#endif
}

void STORE_B_fffa58(B v)
{
	SET_BYTE(sfp_iaddr,3,v);
}
void STORE_B_fffa59(B v)
{
	SET_BYTE(sfp_iaddr,2,v);
}
void STORE_B_fffa5a(B v)
{
	SET_BYTE(sfp_iaddr,1,v);
}
void STORE_B_fffa5b(B v)
{
	SET_BYTE(sfp_iaddr,0,v);
#if DEBUG_SFP
	fprintf(stderr,"sfp_iaddr set to %08lx\n",(long)sfp_iaddr);
#endif
}

void STORE_B_fffa5c(B v)
{
	SET_BYTE(sfp_oaddr,3,v);
}
void STORE_B_fffa5d(B v)
{
	SET_BYTE(sfp_oaddr,2,v);
}
void STORE_B_fffa5e(B v)
{
	SET_BYTE(sfp_oaddr,1,v);
}
void STORE_B_fffa5f(B v)
{
	SET_BYTE(sfp_oaddr,0,v);
#if DEBUG_SFP
	fprintf(stderr,"sfp_oaddr set to %08lx\n",(long)sfp_oaddr);
#endif
}

UNDEFD_L(fffa42)
UNDEFD_L(fffa43)
UNDEFD_L(fffa44)
UNDEFD_L(fffa45)
UNDEFD_L(fffa46)
UNDEFD_L(fffa47)
UNDEFD_L(fffa48)
UNDEFD_L(fffa49)
UNDEFD_L(fffa4a)
UNDEFD_L(fffa4b)
UNDEFD_L(fffa4c)
UNDEFD_L(fffa4d)
UNDEFD_L(fffa4e)
UNDEFD_L(fffa4f)
UNDEFD_L(fffa50)
UNDEFD_L(fffa51)
UNDEFD_L(fffa52)
UNDEFD_L(fffa53)
UNDEFD_L(fffa54)
UNDEFD_L(fffa55)
UNDEFD_L(fffa56)
UNDEFD_L(fffa57)
UNDEFD_L(fffa58)
UNDEFD_L(fffa59)
UNDEFD_L(fffa5a)
UNDEFD_L(fffa5b)
UNDEFD_L(fffa5c)
UNDEFD_L(fffa5d)
UNDEFD_L(fffa5e)
UNDEFD_L(fffa5f)

UNDEFD_S(fffa44)
UNDEFD_S(fffa45)
UNDEFD_S(fffa48)
UNDEFD_S(fffa49)
UNDEFD_S(fffa4c)
UNDEFD_S(fffa4d)
UNDEFD_S(fffa56)
UNDEFD_S(fffa57)

void init_sfp(void)
{
	sfp_response=2;
}

