/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */

/* When the bug in gcc 2.6.0-2.6.3 is fixed, define as 'static' ... */
#define STATIC static

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#if 0
#include <setjmp.h>
#endif
#include "defs.h"
#include "iofuncs.h"
#include "io.h"
#include "mem.h"
#include "cpu.h"
#include "debug.h"
#include "am.h"
#include "main.h"
#include "screen.h"
#include "xlib_vdi.h"
#include "gemdos.h"
#include "syscalls.h"
#include "blitter.h"
#include "native.h"
#include "utils.h"
#include "cookie.h"

#define EXCEPTION(n) exception(n, LM_UL(EXCEPTION_VECTOR(n)))
#define NOTREACHED assert(0);

#if MONITOR
extern int in_monitor;
#endif
#define CHECK_TRACE() do{if (sr&SR_T) {flags &= ~F_TRACE1; flags |= F_TRACE0;}}while(0)
#define CHECK_NOTRACE() do{if ((sr&SR_T)==0) flags &= ~(F_TRACE0|F_TRACE1);}while (0)
#define SBITS(_l,_h,_x)	((int) ((((_x)>>(_l))^(1<<((_h)-(_l))))&\
						((1<<((_h)-(_l)+1))-1)) - (1<<((_h)-(_l))))
#define UBITS(_l,_h,_x) ((((unsigned int)(_x))>>(_l))&((1<<((_h)-(_l)+1))-1))
#define BIT(_n,_x)	(((_x)>>(_n))&1)

UL dummy0[100];
L dreg[17];

#if !defined(PCREG) || !defined(__GNUC__)
L pc;
#endif

#if !defined(SRREG) || !defined(__GNUC__)
UW sr;	/* includes CCR */
#endif

#if !defined(NZREG) || !defined(__GNUC__)
L nz_save;
#endif

UL flags=0;
UW cw;
UL shadow_pc;

/* these should be global register variables... */
UL vaddr;
void *addr;
/* tables for handling special addresses */
#include "iotab1.c"
#include "iotab2.c"

static INLINE B LOAD_TABLE1(UL s)
{
	UB x = IOTAB1_flags_LOAD[s-TRIM(IOTAB1_LO)];
	if (x == 0) BUS_ERROR(s, 1);
	else if (x == 1) return LM_B(ADDR(s));
	else return IOTAB1_funcs_LOAD[x-2]();
	NOTREACHED return 0;
}


static INLINE B LOAD_TABLE2(UL s)
{
	UB x = IOTAB2_flags_LOAD[s-TRIM(IOTAB2_LO)];
	if (x == 0) BUS_ERROR(s, 1);
	else if (x == 1) return LM_B(ADDR(s));
	else return IOTAB2_funcs_LOAD[x-2]();
	NOTREACHED return 0;
}




static INLINE void STORE_TABLE1(UL d, B v)
{
	UB x = IOTAB1_flags_STORE[d-TRIM(IOTAB1_LO)];
	if (x == 0) BUS_ERROR(d, 0);
	else if (x == 1) SM_B(ADDR(d), v);
	else IOTAB1_funcs_STORE[x-2](v);
}


static INLINE void STORE_TABLE2(UL d, B v)
{
	UB x = IOTAB2_flags_STORE[d-TRIM(IOTAB2_LO)];
	if (x == 0) BUS_ERROR(d, 0);
	else if (x == 1) SM_B(ADDR(d), v);
	else IOTAB2_funcs_STORE[x-2](v);
}


/* The following few functions handle I/O and other 'special' memory accesses
 */
B LS_B (UL s)
{
	if (!SUPERVISOR_MODE) BUS_ERROR (s, 1);
#if PROTECT_ROM
#if SMALL
#if TOS_1
	else if (s > 0x800 && s < TRIM(CARTSTART))
#else
	else if (s > 0x800 && s < TRIM(TOSSTART))
#endif
	{
		BUS_ERROR(s, 1);
	}
#endif 
#if TOS_1
	else if (s < TRIM(CARTEND))
#else
	else if (s < TRIM(TOSEND) || (s >= TRIM(CARTSTART) && s < TRIM(CARTEND)))
#endif
		return LM_B(ADDR(s));
#endif
	else if (s < TRIM(IOTAB1_LO)) BUS_ERROR (s, 1);
#if DEBUG_MEMACC
	{
#endif
	if (s <= TRIM(IOTAB1_HI))
	{
		return LOAD_TABLE1(s);
	}
	else if (s >= TRIM(IOTAB2_LO) && s <= TRIM(IOTAB2_HI))
	{
		return LOAD_TABLE2(s);
	}
	else BUS_ERROR (s, 1);
#if DEBUG_MEMACC
	}
#endif
	NOTREACHED return 0;
}

W LS_W (UL s)
{
	if (s & 1) ADDRESS_ERROR (s, 1);
	else
	{
#if PROTECT_ROM
#if SMALL
#if TOS_1
		if (s > 0x800 && s < TRIM(CARTSTART-1))
#else
		if (s > 0x800 && s < TRIM(TOSSTART-1))
#endif
		{
			BUS_ERROR(s, 1);	/* bus err or addr. err 1st?*/
		}
		else
#endif
#if TOS_1
		if (s < TRIM(CARTEND-1))
#else
		if (s < TRIM(TOSEND-1) || (s >= TRIM(CARTSTART) && s < TRIM(CARTEND-1)))
#endif
			return LM_W(ADDR(s));
		else
#endif
		if (s < TRIM(IOTAB1_LO)) BUS_ERROR (s, 1);
		else
		{
		return (LS_B(s)<<8)|(LS_B(s+1) & 0xff);
		}
	}
	NOTREACHED return 0;
}

L LS_L (UL s)
{
	if (s & 1) ADDRESS_ERROR (s, 1);
	else
	{
#if PROTECT_ROM
#if SMALL
#if TOS_1
		if (s > 0x800 && s < TRIM(CARTSTART-3))
#else
		if (s > 0x800 && s < TRIM(TOSSTART-3))
#endif
			BUS_ERROR(s, 1);	/* bus err or addr. err 1st?*/
		else
#endif
#if TOS_1
		if (s < TRIM(CARTEND-3))
#else
		if (s < TRIM(TOSEND-3) || (s >= TRIM(CARTSTART) && s < TRIM(CARTEND-3)))
#endif
			return LM_L(ADDR(s));
		else
#endif
		if (s < TRIM(IOTAB1_LO)) BUS_ERROR (s, 1);
		else
		{
		return (LS_W(s)<<16)|(LS_W(s+2)&0xffff);
		}
	}
	NOTREACHED return 0;
}

void SS_B (UL d, B v)
{
	if (!SUPERVISOR_MODE) BUS_ERROR (d, 0);
	if (d < 0x800) SM_B(ADDR(d), v);
	else if (d < TRIM(IOTAB1_LO)) /* ROM */
	{
		BUS_ERROR (d, 0);
	}
	else if (d <= TRIM(IOTAB1_HI)) STORE_TABLE1(d, v);
	else if (d >= TRIM(IOTAB2_LO) && d <= TRIM(IOTAB2_HI)) STORE_TABLE2(d, v);
	else BUS_ERROR (d, 0);
}

void SS_W (UL d, W v)
{
	if (d & 1) ADDRESS_ERROR (d, 0);
	else
	{
		if (d < 0x800 && SUPERVISOR_MODE) SM_W(ADDR(d), v);
		else
		{
			SS_B(d, v>>8);
			SS_B(d+1,v&0xff);
		}
	}
}

void SS_L (UL d, L v)
{
	SS_W(d, v>>16);
	SS_W(d+2, v&0xffff);
}

static INLINE void SAVE_ADDRESS (void *d)
{
	addr = d;
}
static INLINE B CLS_B (UL s)
{
	addr = 0;
	vaddr = s;
	return CL_B(s);
}
static INLINE UB CLS_UB (UL s)
{
	addr = 0;
	vaddr = s;
	return CL_UB(s);
}
static INLINE W CLS_W (UL s)
{
	addr = 0;
	vaddr = s;
	return CL_W(s);
}
static INLINE UW CLS_UW (UL s)
{
	addr = 0;
	vaddr = s;
	return CL_UW(s);
}
static INLINE L CLS_L (UL s)
{
	addr = 0;
	vaddr = s;
	return CL_L(s);
}
static INLINE UL CLS_UL (UL s)
{
	addr = 0;
	vaddr = s;
	return CL_UL(s);
}
static INLINE void US_B (B x)
{
	if (!addr) CS_B (vaddr, x);
	else SR_B(addr, x);
}
static INLINE void US_UB (UB x)
{
	if (!addr) CS_UB (vaddr, x);
	else SR_UB(addr, x);
}
static INLINE void US_W (W x)
{
	if (!addr) CS_W (vaddr, x);
	else SR_W(addr, x);
}
static INLINE void US_UW (UW x)
{
	if (!addr) CS_UW (vaddr, x);
	else SR_UW(addr, x);
}
static INLINE void US_L (L x)
{
	if (!addr) CS_L (vaddr, x);
	else SR_L(addr, x);
}
static INLINE void US_UL (UL x)
{
	if (!addr) CS_UL (vaddr, x);
	else SR_UL(addr, x);
}
/* ---- */
int is_tos3xx(void)
{
	return TOS_VERSION(TOSSTART) >= 0x300 &&
		TOS_VERSION(TOSSTART) < 0x400;
}

static int check_tos3xx(int exception)
{
#if 0
    if (is_tos3xx() && PC_IN_ROM(pc))
    {
	if ( verbose )
	    fprintf(stderr, "%s exception raised, pc=%08lx, iw=%04x\n",
		    exception == T_LINEF ? "Line F" : "ILLEGAL",
		    PC_VADDR(pc), LM_UW(PC_MEM(pc)));
	if (cputype >= 68030)
	    cleanup(4);
	if ( verbose )
	    fprintf(stderr, "cpu type set to 68030\n");
	cputype = 68030;
	init_tt_hardware();
	return TRUE;
    }
#endif
    if (LM_UL(EXCEPTION_VECTOR(exception)) == 0)
    {
	/* exception vector not yet set by TOS */
	cleanup(4);
    }
    return FALSE;
}

void enter_user_mode (void)
{
	L tmp;
	sr &= ~SR_S;
	tmp = AREG(7);
	AREG(7) = AREG(8);
	AREG(8) = tmp;
}

void enter_supervisor_mode (void)
{
	L tmp;
	sr |= SR_S;
	tmp = AREG(7);
	AREG(7) = AREG(8);
	AREG(8) = tmp;
}

static INLINE void V_EX (UL addr, int rw)
{
	UW tsr = sr;
	FIX_CCR();
	if (!SUPERVISOR_MODE)
		enter_supervisor_mode();
	flags &= ~(F_TRACE0|F_TRACE1); /* no Tracing during exceptions */
	sr &= ~SR_T;
	PUSH_UL(pc);
	PUSH_UW(tsr);
#if 1
	PUSH_UW(0);	/* should be cw */
	PUSH_UL(addr);
	PUSH_UW((tsr & 0xff00) | (rw ? 0x10 : 0)); /* BUG (ssr) */
#endif
#if 0
	sr &= 0xf8ff;
	sr |= 0x0700;	/* BUG */
#endif
}

void EX_BUS_ERROR (UL addr, int rw)
{
#if 1
    if ( verbose )
	fprintf(stderr,"Bus error at %lx, sp=%lx, sr=%x addr=%lx rw=%d\n",
		(long)pc,(long)SP,(UW)sr,(long)addr,rw);
#endif
    V_EX (addr, rw);
}

void EX_ADDRESS_ERROR (UL addr, int rw)
{
    if ( verbose )
	fprintf(stderr,"Address error at %lx\n",(long)pc);
    V_EX (addr, rw);
}

static void exception (int n, UL nw)
{
    UW tsr;
    L new_pc;
    
    FIX_CCR();
    tsr = sr;
#if 1 /* 2001-01-25 (MJK): Tracing all "unused" exceptions */
    if (n != T_VBL && n != T_200Hz && n != T_TRAP_BIOS && n != T_ACIA 
	&& n != T_LINEA && n != T_TRAP_XBIOS && n != T_TRAP_GEMDOS 
	&& n != T_TRAP_GEM
	&& verbose)
    {
	fprintf(stderr,
		"Exception %d at pc=%08lx,sp=%08lx (iw=%04x) -> %08lx\n",
		n, (long)pc, (long)SP, LM_UW(MEM(pc)), (long)nw);
    }
#endif
    if (n < T_TRAP_0 || n > T_TRAP_15)
    {
	flags &= ~(F_TRACE0|F_TRACE1); /* no Tracing during exceptions */
	sr &= ~SR_T;
    }
    
#if 0
    if ( verbose )
	fprintf(stderr,"Switching to supervisor mode\n");
#endif
    if (!SUPERVISOR_MODE)
	enter_supervisor_mode();
#if 0
    if (cputype >= 68010)
	PUSH_UW(0); 
#endif
    PUSH_UL(pc);
    PUSH_UW(tsr);
    new_pc=nw;
    pc=new_pc; /* CHECK_PC! */
#if 0
    if ( verbose )
	fprintf(stderr,"New PC=%lx\n",pc);
#endif
}

void ex_breakpt (void)
{
	pc -= 2;
#if MONITOR
	if (!enter_monitor())
#endif
		EXCEPTION(T_ILLEGAL);
}

void ex_privileged(void)
{
	EXCEPTION(T_PRIVILEGED);
}

void ex_chk(void)
{
	EXCEPTION(T_CHK);
}

void ex_overflow(void)
{
	EXCEPTION(T_OVERFLOW);
}

void ex_div0(void)
{
	EXCEPTION(T_DIV0);
}

void ex_linea(void)
{
	T(("LINEA Exception %04x at pc=%08lx -> %08lx\n", LM_UW(MEM(pc)), pc, LM_UL(EXCEPTION_VECTOR(T_LINEA))));
	EXCEPTION(T_LINEA);
}

void ex_linef(void)
{
#if 0
	if (cputype >= 68030)
	{
		UW opcode2;
		
		switch (LM_UW(PC_MEM(pc)) & 0x0f80)
		{
		case 0x0000:
			pc+=2;
			opcode2=LM_UW(PC_MEM(pc));
			pc+=2;
			switch (opcode2 & 0xe000)
			{
			case 0x0000:
			case 0x4000:
			case 0x5000:
			case 0x6000: /* pmove */
				pc+=4;
				return;
			case 0x2000: /* pflush and friends */
				pc -= 4;
				ex_illegal();
				break;
			}
		case 0x0300: /* fsave/frestore */
			pc += 6;
			return;
		}
	} else
#endif
	{
		if (check_tos3xx(T_LINEF))
			return;
	}
	
	EXCEPTION(T_LINEF);
}

void ex_trap(int n)
{
    UL nw;
    
#if DEBUG_TRAPS
#  define DT(x) T(x)
#else
#  define DT(x)
#endif
    
    switch (n)
    {
      case T_TRAP_GEMDOS:
#if 1
	  init_gemdos();
#endif
	  nw = LM_UL(EXCEPTION_VECTOR(n));
	  DT(("GEMDOS(%02x) -> %lx\n", nw));
	  break;
      case T_TRAP_GEM:
#if NOTYET
	  if (DREG(0) == 200)	/* AES */
	  {
	      nw = LM_UL(EXCEPTION_VECTOR(n));
	      DT(("AES(%08x) -> %lx\n", DREG(1), nw));
#if REDRAW && 0
	      if (redraw_flag)
	      { /* window must be updated, pretend form_dial(FMD_FINISH,...) */
		  nw = CART_AES_REDRAW;
		  redraw_flag = 0; /* important! */
	      }
#endif /* REDRAW && 0 */
	      if (Aes())
		  return;
	  } else
#endif /* NOTYET */
	  if (DREG(0) == 115)
	  {
	      nw = LM_UL(EXCEPTION_VECTOR(n));
	      DT(("VDI(%02x,%08x) -> %lx\n", DREG(1), nw));
	      if (Vdi())
		  return;
	      PUSH_UL(DREG(1));
	      PUSH_UL(pc);
	      PUSH_UL(6);
	      PUSH_UW(0xa0ff);
	      pc = SP;
	  } else
	  {
	      nw = LM_UL(EXCEPTION_VECTOR(n));
	      DT(("AES/VDI(%02x,%08x) -> %lx\n", DREG(0), DREG(1), nw));
	  }
	  break;
      case T_TRAP_BIOS:
	  init_cookie();
	  nw = LM_UL(EXCEPTION_VECTOR(n));
#if 0
	  if ( verbose )
	      fprintf(stderr,"BIOS(%02x) -> %lx\n", LM_W(MEM(SP)), nw);
	  if (Bios())
	  {
	      if ( verbose )
		  fprintf(stderr,"Bios %d done as native\n",LM_W(MEM(SP)));
	      return;
	  }
#endif
	  break;
      case T_TRAP_XBIOS:
	  nw = LM_UL(EXCEPTION_VECTOR(n));
	  DT(("XBIOS(%02x) -> %lx\n", LM_W(MEM(SP)), nw));
	  break;
      default:
	  nw = LM_UL(EXCEPTION_VECTOR(n));
	  DT(("Trap #%d -> %lx\n", n-T_TRAP_0, nw));
	  break;
    }
    exception(n, nw);
}


#if 0
void EXCEPTION (int n)
{
    UW tsr;
    UL newpc;
    FIX_CCR();
    tsr = sr;
    newpc = LM_UL(ADDR(n*4));
#if 1
#if 0
    if (n!=28 && n!=69 && verbose)
	fprintf(stderr,"Exception %d at pc=%08lx\n",n,pc);
#endif
    TRACE_SYSCALL(n);
#endif
    if (n < 32 || n >= 48)
    {
	flags &= ~(F_TRACE0|F_TRACE1); /* no Tracing during exceptions */
	sr &= ~SR_T;
    }
#if DEBUG_EXCEPT
#if DEBUG_TRAPS
    if (n >= 32 && n < 48) 
	switch (n-32)
	{
	  case 1: D("GEMDOS(%02x) -> %lx\n", LM_W(MEM(SP)), newpc); break;
	  case 2: D("AES/VDI(%02x,%08x) -> %lx\n", DREG(0), DREG(1),newpc); break;
	  case 13: D("BIOS(%02x) -> %lx\n", LM_W(MEM(SP)), newpc); break;
	  case 14: D("XBIOS(%02x) -> %lx\n", LM_W(MEM(SP)) ,newpc); break;
	  default: D("Trap #%d -> %lx\n", n-32, newpc); break;
	}
    else
#endif	
    {
    }
#endif
#if 1
    if (n == 34 && vdi_mode)
    {
#if REDRAW
	if (DREG(0) == 200 && redraw_flag)	/* AES */
	{
	    /* window must be updated, pretend form_dial(FMD_FINISH,...) */
	    newpc = 0xfa0100;
	    redraw_flag = 0; /* important! */
	}
	else
#endif
	    if (DREG(0) == 115)
	    {
		if (Vdi()) return;
		PUSH_UL(DREG(1));
		PUSH_UL(pc);
		PUSH_UL(6);
		PUSH_UW(0xa0ff);
		pc=SP;
	    }
    }
#endif
    if (!SUPERVISOR_MODE)
	enter_supervisor_mode();
    PUSH_UL(pc);
    PUSH_UW(tsr);
    pc = newpc;	/* should CHECK_PC() here? */
}
#endif

void TRAP (int n)
{
    EXCEPTION(32+n);
}

void ILLEGAL (void)
{
    pc -= 2;
    if ( verbose )
	fprintf(stderr,"Illegal Exception at %08lx\n",(long)pc);
    EXCEPTION (T_ILLEGAL);
}

/* All BCD functions are UNTESTED! */
#define BCD2DEC(_x) ((((_x)&0xf0)>>4)*10+((_x)&0xf))
#define DEC2BCD(_x) ((((_x)/10)<<4)+((_x)%10))

B BCD_ADD(B a, B b)
{
	int q,w;
	sr &= ~MASK_CC|MASK_CC_X;
	q = BCD2DEC(a);
	w = BCD2DEC(b);
	w += q + GET_X_BIT();
	if (w >= 100)
	{
		w -= 100;
		sr |= MASK_CC_X|MASK_CC_C;
	}
	if (w != 0) UNSET_Z();	/* ???? */
	return DEC2BCD(w);
}

B BCD_SUB(B a, B b)
{
	int q,w;
	sr &= ~MASK_CC|MASK_CC_X;
	q = BCD2DEC(a);
	w = BCD2DEC(b);
	w -= q + GET_X_BIT();
	if (w < 0)
	{
		w += 100;
		sr |= MASK_CC_X|MASK_CC_C;
	}
	if (w != 0) UNSET_Z();	/* ???? */
	return DEC2BCD(w);
}

/* -------------------- SPECIAL EVENTS DISPATCHER --------------------------- */
#define MFP_ICOUNT_DEF 500
static int syst_count=0;
static int last_acia=0;
static int mfp_prio=0;
static int mfp_Timer_A_count=0;
static int mfp_icount=MFP_ICOUNT_DEF;
extern volatile UB mfp_TADR;
extern int mfp_TA_delay;
static UB some_space[256];
#if PROFILE
static void process_flags (unsigned int iw)
{
	if (flags & F_PROFILE)
	{
		extern int prof_freq[];
		extern unsigned char prof_maptab[];
		prof_freq[prof_maptab[iw]]++;
	}
#else
static void process_flags (void)
{
#endif
	if (flags & F_CONFIG)
	{
		/* fix stack here... */
		flags &= ~F_CONFIG;
		exception(0,0xfa2000);
		return;
	}
#if TIMER_A
#if 0
	if (timer_a)
	{
#endif
		if ((flags & F_TIMERA_ON))
		{
			if (mfp_Timer_A_count)	/* need to generate Timer A interrupts */
			{
				mfp_Timer_A_count--;
#if 0
fprintf(stderr,"Timer A interrupt\n");
#endif
				if (IPL_OK(6))
				{
					EXCEPTION(T_TIMERA);
					SET_IPL(7);
					return;
				}
			}
			else if (--mfp_icount <= 0)
			{
				mfp_Timer_A_count = mfp_get_count();
				mfp_icount = MFP_ICOUNT_DEF;
			}
		}
#if 0
	}
#endif
#endif /* TIMER_A */
	if (flags & F_BLIT)
	{
		Do_Blit();
		flags &= ~F_BLIT;
	}
	else
	if ((flags & F_ACIA) && IPL_OK(6)
			/* && mfp_prio < 6 */ /* && syst_count != last_acia */)
	{
/*		mfp_prio = 6; */
		EXCEPTION(T_ACIA);
	/*	last_acia = syst_count; */
		SET_IPL(7);
		flags &= ~F_ACIA;
	}
	else
	if ((flags & F_RCV_FULL) && IPL_OK(6))
	{
		EXCEPTION(T_RCV_FULL);
		SET_IPL(7);
		flags &= ~F_RCV_FULL;
	}
	if (flags & F_TRACE0)
	{
		flags &= ~F_TRACE0;
		flags |= F_TRACE1;
	}
	else if (flags & F_TRACE1)
	{
		EXCEPTION(T_TRACE);
	}
#if MONITOR
	if (!in_monitor)
	{
#endif
#if DOUBLE_HZ200
	if ((flags & (F_200Hz|F_200HzB)) && (IPL_OK(6) /* && mfp_prio < 5 */
#else
	if ((flags & (F_200Hz)) && (IPL_OK(6) /* && mfp_prio < 5 */
#endif
			&& (LM_UB(MEM(0xfffa11)) & 0x20) == 0))
	{
		/* Set it -> Timer C interrupt is being processed */
		SM_UB(MEM(0xfffa11),LM_UB(MEM(0xfffa11)) | 0x20);
		EXCEPTION(T_200Hz);
		SET_IPL(7);
#if DOUBLE_HZ200
		if (flags & F_200Hz)
		{
			flags &= ~F_200Hz;
			flags |= F_200HzB;
		}
		else flags &= ~F_200HzB;
#else
		flags &= ~F_200Hz;
#endif
	}
	else if ((flags & F_VBL) && IPL_OK(4))
	{
		machine.screen_shifter();
		EXCEPTION(T_VBL);
		SET_IPL(5);
		flags &= ~F_VBL;
	}
#if MONITOR
	}
#endif
}

UB testcond[] = {
1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,
1,0,0,1,0,1,1,0,1,0,1,0,1,0,1,0,
1,0,1,0,1,0,1,0,0,1,1,0,0,1,0,1,
1,0,0,1,0,1,1,0,0,1,1,0,0,1,0,1,
1,0,0,1,1,0,0,1,1,0,1,0,1,0,0,1,
1,0,0,1,0,1,0,1,1,0,1,0,1,0,0,1,
1,0,0,1,1,0,0,1,0,1,1,0,0,1,0,1,
1,0,0,1,0,1,0,1,0,1,1,0,0,1,0,1,
1,0,1,0,1,0,1,0,1,0,0,1,0,1,0,1,
1,0,0,1,0,1,1,0,1,0,0,1,0,1,0,1,
1,0,1,0,1,0,1,0,0,1,0,1,1,0,1,0,
1,0,0,1,0,1,1,0,0,1,0,1,1,0,1,0,
1,0,0,1,1,0,0,1,1,0,0,1,0,1,0,1,
1,0,0,1,0,1,0,1,1,0,0,1,0,1,0,1,
1,0,0,1,1,0,0,1,0,1,0,1,1,0,0,1,
1,0,0,1,0,1,0,1,0,1,0,1,1,0,0,1,
1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,
1,0,0,1,0,1,1,0,1,0,1,0,1,0,1,0,
1,0,1,0,1,0,1,0,0,1,1,0,0,1,0,1,
1,0,0,1,0,1,1,0,0,1,1,0,0,1,0,1,
1,0,0,1,1,0,0,1,1,0,1,0,1,0,0,1,
1,0,0,1,0,1,0,1,1,0,1,0,1,0,0,1,
1,0,0,1,1,0,0,1,0,1,1,0,0,1,0,1,
1,0,0,1,0,1,0,1,0,1,1,0,0,1,0,1,
1,0,1,0,1,0,1,0,1,0,0,1,0,1,0,1,
1,0,0,1,0,1,1,0,1,0,0,1,0,1,0,1,
1,0,1,0,1,0,1,0,0,1,0,1,1,0,1,0,
1,0,0,1,0,1,1,0,0,1,0,1,1,0,1,0,
1,0,0,1,1,0,0,1,1,0,0,1,0,1,0,1,
1,0,0,1,0,1,0,1,1,0,0,1,0,1,0,1,
1,0,0,1,1,0,0,1,0,1,0,1,1,0,0,1,
1,0,0,1,0,1,0,1,0,1,0,1,1,0,0,1,
};

#if 0
#include "old.c"
#endif

#ifdef GEN_FUNCTAB
void Nullfunc (unsigned int iw)
{
	ILLEGAL();
}
#ifdef DECOMPILER
#define CODE_FUNC static inline
#else
#define CODE_FUNC
#endif
#include "code.c"
#endif

#ifdef GEN_SWITCH
#include "gendefs.h"
#endif

int count=0;
UL ex_addr;
int ex_rw;
#ifndef DECOMPILER
#if 1||MONITOR||!(__i486__ && linux)
void execute (UL new_pc)
{
#ifdef GEN_LABELTAB
#define CODE_LABEL_TABLE
#include "code.c"
#endif
	pc = new_pc;

#if __cplusplus
// exceptions
restart:
try{
#endif /* __cplusplus */
	for (;;)
	{
		unsigned int iw;
#if MONITOR
		if (update_monitor(dreg, sr, pc) == 'q') return;
		count++;
#endif
#if PROFILE
		if (flags) process_flags(iw);
		iw=LM_UW(MEM(pc));
#else
		if (flags) process_flags();
		iw=LM_UW(MEM(pc));
#endif
		pc+=2;
#if defined(GEN_FUNCTAB)
		jumptab[iw](iw);
#elif defined(GEN_SWITCH)
#include "code.c"
#elif defined(GEN_LABELTAB)
#undef CODE_LABEL_TABLE
#include "code.c"
#endif
	}
#if __cplusplus
}
catch(int i)
{
	UL newpc;
#if 0
	fprintf(stderr,"Exception at pc=%lx!\n",pc);
#endif
	if (i==2)
	{
		newpc=LM_UL(ADDR(8));
		EX_BUS_ERROR(ex_addr,ex_rw);
		fprintf(stderr,"Returned from exception\n");
		pc=newpc;
	}
	else if (i==3)
	{
		newpc=LM_UL(ADDR(12));
		EX_ADDRESS_ERROR(ex_addr,ex_rw);
		pc=newpc;
	}
	else
	{
		ILLEGAL();
	}
	fprintf(stderr,"restarting at pc=%lx\n",(long)pc);
	goto restart;
}
#endif /* __cplusplus */
	exit(0); /* tell GCC that we won't ever come back */
}
#endif

void execute_start (UL new_pc)
{
	execute (new_pc);
}

#if MONITOR
void exfunc (int status)
{
	fprintf (stderr, "Instructions: %d\n", count);
}
#endif

void init_cpu (void)
{
	int i;
#if MONITOR
	OnExit (exfunc);
#endif
	memset ((char *)dreg, 1, 17*4);
	sr = 0x2700;
	SM_UL(ADDR(0),LM_UL(MEM(TOSSTART)));
	SM_UL(ADDR(4),LM_UL(MEM(TOSSTART+4)));
	SP = LM_UL(ADDR(0));
	pc = LM_UL(ADDR(4));
#if 0
	fprintf(stderr,"Longwords at 0, 4 initialized to %0lx,%0lx\n",
		(long)SP,(long)pc);
#endif
	if ((TOSSTART>>16) != LM_UW(ADDR(4)))
	{
	    fprintf(stderr,"This STonX binary has been compiled for use with a %dKB TOS image!\n(Versions %s)\n",TOS_1 ? 192 : 256,TOS_1 ? "1.00, 1.02, 1.04" : "1.62, 1.7, 2.0X");
	    exit(1);
	}
}
#endif
