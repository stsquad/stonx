/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 *
 * include this before cpu.h if needed
 */
#ifndef MEM_H
#define MEM_H
#include "defs.h"
#include "main.h"  /* for tosstart and tosend */

extern void init_mem(void);
extern void mc68000_reset(void);

#define CHECK_ALIGNMENT 1
#define SWAP_LOAD_IN_MEMORY 0 
#define SWAP_STORE_IN_MEMORY 0 

#if SMALL
#define MEMSIZE 	0x800000
#define SMALL_MEM	0x400000
#else
#define MEMSIZE 	0x1000000
#endif

extern B mem[];

/* WARNING: Many of these Macros MUST NOT TAKE ARGUMENTS WITH SIDE EFFECTS!
 * One serious bug, which kept me busy for about 40 hours was, that I had
 * implemented PEA(ea) as PUSH_UL(CALC_AM_C(ea)), but if ea was SP-relative,
 * the SP-=4 of PUSH_UL occured before the AM-Calculation!
 */

#if PROTECT_ROM

#if SMALL

#define NORMAL_ADDRESS_W_B(_x) (((_x) >= 0x800) && ((_x) <= SMALL_MEM-1))
#define NORMAL_ADDRESS_W_W(_x) (((_x) >= 0x800) && ((_x) <= SMALL_MEM-2))
#define NORMAL_ADDRESS_W_L(_x) (((_x) >= 0x800) && ((_x) <= SMALL_MEM-4))
#define NORMAL_ADDRESS_R_B(_x)  ((((_x) >= 0x800) && ((_x) <= SMALL_MEM-1))\
			||(((_x) >= romstart) && ((_x) <= tosend1)))
#define NORMAL_ADDRESS_R_W(_x)  ((((_x) >= 0x800) && ((_x) <= SMALL_MEM-2))\
			||(((_x) >= romstart) && ((_x) <= tosend2)))
#define NORMAL_ADDRESS_R_L(_x)  ((((_x) >= 0x800) && ((_x) <= SMALL_MEM-4))\
			||(((_x) >= romstart) && ((_x) <= tosend4)))

#else /* !SMALL */

#define NORMAL_ADDRESS_W_B(_x) (((_x) >= 0x800) && ((_x) <= romstart1 ))
#define NORMAL_ADDRESS_W_W(_x) (((_x) >= 0x800) && ((_x) <= romstart2 ))
#define NORMAL_ADDRESS_W_L(_x) (((_x) >= 0x800) && ((_x) <= romstart4 ))
#define NORMAL_ADDRESS_R_B(_x) (((_x) >= 0x800) && ((_x) <= tosend1))
#define NORMAL_ADDRESS_R_W(_x) (((_x) >= 0x800) && ((_x) <= tosend2))
#define NORMAL_ADDRESS_R_L(_x) (((_x) >= 0x800) && ((_x) <= tosend4))

#endif /* SMALL */

#else /* !PROTECT_ROM */

#define NORMAL_ADDRESS_R_B(_x) (((_x) >= 0x800) && ((_x) <= romend1 ))
#define NORMAL_ADDRESS_R_W(_x) (((_x) >= 0x800) && ((_x) <= romend2 ))
#define NORMAL_ADDRESS_R_L(_x) (((_x) >= 0x800) && ((_x) <= romend4 ))
#define NORMAL_ADDRESS_W_B(_x) (((_x) >= 0x800) && ((_x) <= romend1 ))
#define NORMAL_ADDRESS_W_W(_x) (((_x) >= 0x800) && ((_x) <= romend2 ))
#define NORMAL_ADDRESS_W_L(_x) (((_x) >= 0x800) && ((_x) <= romend4 ))

#endif /* PROTECT_ROM */


#define PC_IN_ROM(pc) (TRIM(pc) >= tosstart  &&  TRIM(pc) < tosend)

#define ADDR(_x)	((void *)&(mem[_x]))
#define VADDR(_x) 	((UL)_x-(UL)ADDR(0))

#define MEM_MASK	(MEMSIZE-1)
#define TRIM(_x)	((_x)&MEM_MASK)
#define MEM(_x) (ADDR(((UL)(_x))&MEM_MASK))


/* -------------------------------------------------------------------------- */
/* Routines for loading/storing 8/16/32 bit ints, with no regard to byte ord. */
/* -------------------------------------------------------------------------- */

/* Load/Store primitives (without address checking) */
#define LOAD_B(_s) (*((B *)(_s)))
#define LOAD_UB(_s) (*((UB *)(_s)))
#define LOAD_W(_s) (*((W *)(_s)))
#define LOAD_UW(_s) (*((UW *)(_s)))
#define LOAD_L(_s) (*((L *)(_s)))
#define LOAD_UL(_s) (*((UL *)(_s)))

static INLINE L LU_L(L *s)
{
	W *ss = (W *)s;
	UW *tt = (UW *)s;
#if CHECK_ALIGNMENT && IS_BIG_ENDIAN
	if (((int)s&2)==0)
		return LOAD_L(s);
	else
#endif
	return (LOAD_W(ss)<<16)|LOAD_UW(tt+1);
}

static INLINE UL LU_UL (UL *s)
{
	UW *ss = (UW *)s;
#if CHECK_ALIGNMENT && IS_BIG_ENDIAN
	if (((int)s&2)==0)
		return LOAD_UL(s);
	else
#endif
	return (LOAD_UW(ss)<<16)|LOAD_UW(ss+1);
}

#define STORE_B(_d,_v) *((B *)_d) = _v
#define STORE_UB(_d,_v)	*((UB *)_d) = _v
#define STORE_W(_d,_v) *((W *)_d) = _v
#define STORE_UW(_d,_v) *((UW *)_d) = _v
#define STORE_L(_d,_v) *((L *)_d) = _v
#define STORE_UL(_d,_v) *((UL *)_d) = _v

static INLINE void SU_L (L *d, L v)
{
	W *dd = (W *)d;
#if CHECK_ALIGNMENT && IS_BIG_ENDIAN
	if (((int)d&2)==0)
	{
		STORE_L(d,v);
		return;
	}
#endif
	STORE_W(dd,v>>16);
	STORE_W(dd+1,v & 0xffff);	/* & is redundant (?) */
}
static INLINE void SU_UL (UL *d, UL v)
{
	UW *dd = (UW *)d;
#if CHECK_ALIGNMENT && IS_BIG_ENDIAN
	if (((int)d&2)==0)
	{
		STORE_UL(d,v);
		return;
	}
#endif
	STORE_UW(dd,v>>16);
	STORE_UW(dd+1,v & 0xffff);
}

/* -------------------------------------------------------------------------- */
/* Routines for loading/storing ints, with conversion to/from host byte ord.  */
/* -------------------------------------------------------------------------- */

/* the registers contain data in host format and are always aligned */
#define LR_B LOAD_B
#define LR_UB LOAD_UB
#define LR_W LOAD_W
#define LR_UW LOAD_UW
#define LR_L LOAD_L
#define LR_UL LOAD_UL
#define SR_B STORE_B
#define SR_UB STORE_UB
#define SR_W STORE_W
#define SR_UW STORE_UW
#define SR_L STORE_L
#define SR_UL STORE_UL

#define LM_B LOAD_B
#define LM_UB LOAD_UB
#define SM_B STORE_B
#define SM_UB STORE_UB

#if IS_BIG_ENDIAN
#define LM_W LOAD_W
#define LM_UW LOAD_UW
#define LM_L LU_L
#define LM_UL LU_UL
#define SM_W STORE_W
#define SM_UW STORE_UW
#define SM_L SU_L
#define SM_UL SU_UL
#else

#if defined(__i486__) && !defined(__STRICT_ANSI__)

static INLINE W LM_W(void *s)
{	register W x=*(W *)s;
	asm ( "rolw $8,%0\n" : "=c" (x) : "c" (x));
	return x;
}

static INLINE UW LM_UW(void *s)
{	register UW x=*(UW *)s;
	asm ( "rolw $8,%0\n" : "=c" (x) : "c" (x));
	return x;
}

static INLINE L LM_L(void *s)
{	register UL y=*(L *)s,x;
	asm ( "bswap %0\n" : "=S" (x) : "S" (y));
	return x;
}

static INLINE UL LM_UL(void *s)
{	register UL y=*(UL *)s,x;
	asm ( "bswap %0\n" : "=S" (x) : "S" (y));
	return x;
}

#ifdef COMPILE_WITHOUT_OPTIMIZATION
/* Remark: This only works with option -O0 not with -O3 */
static INLINE void SM_W(void *d, W v)
{	asm ( 	"xchgb %%ch,%%cl\n\t"
		"movw %%cx,(%%esi)\n"   
        : "=c" (v)
        : "c" (v), "S" (d));
}
#else
static INLINE void SM_W(void *d, W v)
{
#if SWAP_STORE_IN_MEMORY
        STORE_W(d,((v&0xff)<<8)|((v>>8)&0xff));
#else
        B *ds = (B *)d;
	UB *du = (UB *)d;
	STORE_B(ds,v>>8);
	STORE_UB(du+1,v&0xff);
#endif
}
#endif /* COMPILE_WITHOUT_OPTIMIZATION */

#ifdef COMPILE_WITHOUT_OPTIMIZATION
/* Remark: This only works with option -O0 not with -O3 */
static INLINE void SM_UW(void *d, UW v)
{	asm ( 	"xchgb %%ch,%%cl\n\t"
		"movw %%cx,(%%esi)\n"   
	: "=c" (v)
        : "c" (v), "S" (d));
}
#else
static INLINE void SM_UW(void *d, UW v)
{
#if SWAP_STORE_IN_MEMORY
        STORE_UW(d,(v<<8)|(v>>8));
#else
        UB *du = (UB *)d;
        STORE_UB(du,v>>8);
        STORE_UB(du+1,v&0xff);
#endif
}
#endif /* COMPILE_WITHOUT_OPTIMIZATION */

static INLINE void SM_L(void *d, L v)
{	asm (	"bswap %0\t\n"
		"movl %0,(%1)\n"   
        : 
        : "S" (v), "D" (d)
        );
}

static INLINE void SM_UL(void *d, UL v)
{	asm ( 	"bswap %0\n\t"
		"movl %0,(%1)\n"   
        : 
        : "S" (v), "D" (d)
        );
}

#else

static INLINE W LM_W(void *s)
{
#if SWAP_LOAD_IN_MEMORY
	W x = LOAD_W(s);
	return ((x&0xff)<<8)|((x>>8)&0xff);
#else
	B *ss = (B *)s;
	UB *su = (UB *)s;
	return (LOAD_B(ss)<<8)|LOAD_UB(su+1);
#endif
}
static INLINE UW LM_UW(void *s)
{
#if SWAP_LOAD_IN_MEMORY
	UW x = LOAD_UW(s);
	return ((x&0xff)<<8)|(x>>8);
#else
	UB *ss = (UB *)s;
	return (LOAD_UB(ss)<<8)|LOAD_UB(ss+1);
#endif
}
static INLINE L LM_L(void *s)
{
#if SWAP_LOAD_IN_MEMORY
	L x = LU_L(s);
	return ((x&0xff)<<24)|((x&0xff00)<<8)|((x>>8)&0xff00)|((x>>24)&0xff);
#else
	B *ss = (B *)s;
	UB *su = (UB *)s;
	return (LOAD_B(ss)<<24)|(LOAD_UB(su+1)<<16)
			|(LOAD_UB(su+2)<<8)|(LOAD_UB(su+3));
#endif
}
static INLINE UL LM_UL(void *s)
{
#if SWAP_LOAD_IN_MEMORY
	UL x = LU_UL(s);
	return (x<<24)|((x&0xff00)<<8)|((x>>8)&0xff00)|(x>>24);
#else
	UB *su = (UB *)s;
	return (LOAD_UB(su)<<24)|(LOAD_UB(su+1)<<16)
			|(LOAD_UB(su+2)<<8)|(LOAD_UB(su+3));
#endif
}

static INLINE void SM_W(void *d, W v)
{
#if SWAP_STORE_IN_MEMORY
	STORE_W(d,((v&0xff)<<8)|((v>>8)&0xff));
#else
	B *ds = (B *)d;
	UB *du = (UB *)d;
	STORE_B(ds,v>>8);
	STORE_UB(du+1,v&0xff);
#endif
}

static INLINE void SM_UW(void *d, UW v)
{
#if SWAP_STORE_IN_MEMORY
	STORE_UW(d,(v<<8)|(v>>8));
#else
	UB *du = (UB *)d;
	STORE_UB(du,v>>8);
	STORE_UB(du+1,v&0xff);
#endif
}

static INLINE void SM_L(void *d, L v)
{
#if SWAP_STORE_IN_MEMORY
	SU_L(d,((v&0xff)<<24)|((v&0xff00)<<8)|((v>>8)&0xff00)|((v>>24)&0xff));
#else
	B *ds = (B *)d;
	UB *du = (UB *)d;
	STORE_B(ds, v>>24);
	STORE_UB(du+1,(v>>16)&0xff);
	STORE_UB(du+2,(v>>8)&0xff);
	STORE_UB(du+3,v&0xff);
#endif
}

static INLINE void SM_UL(void *d, UL v)
{
#if SWAP_STORE_IN_MEMORY
	SU_UL(d,(v<<24)|((v&0xff00)<<8)|((v>>8)&0xff00)|(v>>24));
#else
	UB *du = (UB *)d;
	STORE_UB(du, v>>24);
	STORE_UB(du+1,(v>>16)&0xff);
	STORE_UB(du+2,(v>>8)&0xff);
	STORE_UB(du+3,v&0xff);
#endif
}
#endif

#endif /* IS_BIG_ENDIAN */

/* -------------------------------------------------------------------------- */
/* Macros for convenience													  */
/* -------------------------------------------------------------------------- */
#define LDREG_B(_x) LR_B(DREG_B(_x))
#define LDREG_UB(_x) LR_UB(DREG_B(_x))
#define LDREG_W(_x) LR_W(DREG_W(_x))
#define LDREG_UW(_x) LR_UW(DREG_W(_x))
#define LDREG_L(_x) (DREG(_x))
#define LDREG_UL(_x) LR_UL(DREG_L(_x))
#define SDREG_B(_x,_v) SR_B(DREG_B(_x),_v)
#define SDREG_UB(_x,_v) SR_UB(DREG_B(_x),_v)
#define SDREG_W(_x,_v) SR_W(DREG_W(_x),_v)
#define SDREG_UW(_x,_v) SR_UW(DREG_W(_x),_v)
#define SDREG_L(_x,_v) DREG(_x)=_v
#define SDREG_UL(_x,_v) SR_UL(DREG_UL(_x),_v)

#define ALIGNED_R(_x) do{if ((_x)&1) ADDRESS_ERROR(_x,1); } while (0);
#define ALIGNED_W(_x) do{if ((_x)&1) ADDRESS_ERROR(_x,0); } while (0);

B LS_B(UL s);
W LS_W(UL s);
L LS_L(UL s);
/* these need yet to be implemented separately - will save a few cycles */
#define LS_UB(_x) (LS_B(_x) & 0xff)
#define LS_UW(_x) (LS_W(_x) & 0xffff)
#define LS_UL(_x) (LS_L(_x) & 0xffffffff)
void SS_B(UL d, B v);
void SS_W(UL d, W v);
void SS_L(UL d, L v);
#define SS_UB(_x,_y) SS_B(_x,_y)
#define SS_UW(_x,_y) SS_W(_x,_y)
#define SS_UL(_x,_y) SS_L(_x,_y)

#endif /* MEM_H */
