/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#if 0
#include <setjmp.h>
#endif
#include "config.h"
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
#include "monitor.h"
#endif
#define CHECK_TRACE() do{if (sr&SR_T) {flags &= ~F_TRACE1; flags |= F_TRACE0;}}while(0)
#define CHECK_NOTRACE() do{if ((sr&SR_T)==0) flags &= ~(F_TRACE0|F_TRACE1);}while (0)
#define SBITS(_l,_h,_x)	((int) ((((_x)>>(_l))^(1<<((_h)-(_l))))&\
						((1<<((_h)-(_l)+1))-1)) - (1<<((_h)-(_l))))
#define UBITS(_l,_h,_x) ((((unsigned int)(_x))>>(_l))&((1<<((_h)-(_l)+1))-1))
#define BIT(_n,_x)	(((_x)>>(_n))&1)

#define DEBUG_CPU 0

#if DEBUG_CPU
extern FILE *debugfile;
#define DBG(_args...) fprintf( debugfile, ## _args )
#else
#define DBG(_args...)
#endif


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
	else if (s > 0x800 && s <= romstart1 )
	{
		BUS_ERROR(s, 1);
	}
#endif 
	else if ( (tos1 && s < TRIM(CARTEND)) || (!tos1 && (s <= tosend1 || (s >= TRIM(CARTSTART) && s < TRIM(CARTEND)) )) )
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
		if (s > 0x800 && s <= romstart2 )
		{
			BUS_ERROR(s, 1);	/* bus err or addr. err 1st?*/
		}
		else
#endif
		if ( (tos1 && (s <= TRIM(CARTEND-2)) )
		     || (!tos1 && (s <= tosend2 || (s >= TRIM(CARTSTART) && s <= TRIM(CARTEND-2)) ) ) )
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
		if (s > 0x800 && s <= romstart4 )
			BUS_ERROR(s, 1);	/* bus err or addr. err 1st?*/
		else
#endif
		if ( (tos1 && (s <= TRIM(CARTEND-4)) )
		     || (!tos1 && (s <= (tosend4) || (s >= TRIM(CARTSTART) && s <= TRIM(CARTEND-4)) ) ) )
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
	return TOS_VERSION(tosstart) >= 0x300 &&
		TOS_VERSION(tosstart) < 0x400;
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
#if MONITOR
	/*
	** Signal the monitor that an exception has occured
	*/
	signal_monitor(GENERAL_EXCEPTION,NULL);
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
      /* 2001-02-24 (Thothy): Added Line-F to the list for running TOS 1 */
    if (verbose && n != T_VBL && n != T_200Hz && n != T_ACIA 
	&& n != T_TRAP_BIOS && n != T_TRAP_XBIOS && n != T_TRAP_GEMDOS 
	&& n != T_TRAP_GEM && n != T_LINEA && n!=T_LINEF )
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
	update_monitor(dreg, sr, pc);
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
#if NATIVE_BIOS
	  if ( verbose )
	      fprintf(stderr,"BIOS(%02x) -> %lx\n", LM_W(MEM(SP)), nw);
	  if (Bios())
	  {
	      if ( verbose )
		  fprintf(stderr,"Bios %d done as native\n",LM_W(MEM(SP)));
	      return;
	  }
#endif  /* NATIVE_BIOS */
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


/* All BCD functions are not very well tested! */

B BCD_ADD(B a, B b)
{
	int t;

	sr &= ~(MASK_CC_X|MASK_CC_C);

	t = (a & 0xF) + (b & 0xF) + GET_X_BIT();
	if (t >= 0xA)
		t += 0x6;

	t += (a & 0xF0) + (b & 0xF0);
	if (t >= 0xA0)
		t += 0x60;

	if (t & 0xFF00)
	{
		t &= 0x0FF;
		sr |= MASK_CC_X|MASK_CC_C;
	}
	if (t != 0)
		UNSET_Z();
	if (t & 0x80)
		SET_N();

	return t;
}

B BCD_SUB(B a, B b)
{
	signed int t, hi;

	sr &= ~(MASK_CC_X|MASK_CC_C);

	t = (b & 0xF) - (a & 0xF) - GET_X_BIT();
	hi = (b & 0xF0) - (a & 0xF0);
	if (t < 0)
	{
		t += 10;
		hi -= 0x10;
	}
	if (hi < 0)
	{
		hi -= 0x60;
	}
	t += hi;

	if (t & 0xFF00)
	{
		t &= 0x0FF;
		sr |= MASK_CC_X|MASK_CC_C;
	}
	if (t != 0)
		UNSET_Z();
	if (t & 0x80)
		SET_N();

	return t;
}


/* -------------------- SPECIAL EVENTS DISPATCHER --------------------------- */
/* The process_flags function is called every instruction loop */
/* Flags are set by many things but signal something that will affect */
/* program flow (like exceptions) */
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

	/* Process HBL event */
	/*	if (flags & F_HBL)
	  {
	    if (IPL_OK(6))
	      {
		EXCEPTION(T_HBL);
		flags &= ~F_HBL;
		return;
	      }
	      }*/
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
                #if MONITOR
	        signal_monitor(VBL,NULL);
	        #endif
		machine.screen_shifter();
		EXCEPTION(T_VBL);
		SET_IPL(5);
		flags &= ~F_VBL;
	}
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

/* Count of instructions executed - useful for debuging */
int instruction_count=0;
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
#if PROFILE
		if (flags) process_flags(iw);
#elif MONITOR
		if (in_monitor==0)
		  {
		    if (flags) process_flags();
		  }
#else
		if (flags) process_flags();
#endif

		iw=LM_UW(MEM(pc));
		/* moved monitor so its at start of next instruction*/
#if MONITOR
		if (update_monitor(dreg, sr, pc)) 
		  {
		    return;
		  } else {
		    instruction_count++;
		  }
#endif
		DBG ("Execute pc=%lx, iw=%x, count=%ld\n",
		     pc,
		     iw,
		     instruction_count);
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
void exfunc (void)
{
	fprintf (stderr, "Instructions: %d\n", instruction_count);
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
	SM_UL(ADDR(0),LM_UL(MEM(tosstart)));
	SM_UL(ADDR(4),LM_UL(MEM(tosstart+4)));
	SP = LM_UL(ADDR(0));
	pc = LM_UL(ADDR(4));
}
#endif
