/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */

#ifndef CPU_H
#define CPU_H
#include "defs.h"
#include "options.h"
#include "mem.h"

extern UL flags;
extern void init_cpu(void);
extern void execute_start (UL pc);
extern void ex_trap(int n);
extern void ex_illegal(void);
extern void ex_breakpt(void);
extern void ex_privileged(void);
extern void ex_chk(void);
extern void ex_overflow(void);
extern void ex_div0(void);
extern void ex_linea(void);
extern void ex_linef(void);

#if defined(GEN_FUNCTAB) && defined(THREADING) && 0

#include "gendefs.h"

extern GENFUNC_PROTO((*jumptab[]));

#define THREAD \
	iw=LM_UW(MEM(pc));\
		pc += 2;\
		jumptab[iw](iw);

#define THREAD_CHECK \
	if (flags) process_flags();\
	THREAD

#else
#define THREAD
#define THREAD_CHECK
#endif

void execute (UL new_pc)
#if defined(__GNUC__) && IS_BIG_ENDIAN
	 __attribute__ ((noreturn))
#endif
	;

extern void enter_user_mode(void);

void ILLEGAL(void);
void EXCEPTION(int);
void EX_BUS_ERROR(UL addr, int rw);
void EX_ADDRESS_ERROR(UL addr, int rw);
extern UL ex_addr;
extern int ex_rw;
#ifdef __cplusplus
// use C++ exceptions
#define BUS_ERROR(_a,_rw) do{ ex_addr=_a; ex_rw=_rw; throw(2); }while(0)
#define ADDRESS_ERROR(_a,_rw)  do{ ex_addr=_a; ex_rw=_rw; throw(3); }while(0)
#define ILLEGAL_INSTRUCTION throw(4) 
#else
/* use old C method with recursion (nasty)*/
#define BUS_ERROR(_a,_rw) do{UL _q=LM_UL(ADDR(8));\
                        EX_BUS_ERROR(_a,_rw);execute(pc=_q);}\
                        while(0)/*NOTREACHED*/
#define ADDRESS_ERROR(_a,_rw) do{UL _q=LM_UL(ADDR(12));\
                        EX_ADDRESS_ERROR(_a,_rw);execute(pc=_q);}\
                        while(0)/*NOTREACHED*/

#define ILLEGAL_INSTRUCTION {ILLEGAL(); execute(pc);}/*NOTREACHED*/
#endif

/* this macro may only be used at the end of an instruction!!! */
#if SAFE
#define CHECK_PC() do{if(pc&1)EX_ADDRESS_ERROR(pc,1);}while(0)
#else
#define CHECK_PC() 
#endif

#define PUSH_L(_x) do{ SP -= 4; CS_L(SP,_x); } while(0)
#define PUSH_UL(_x) do{ SP -= 4; CS_UL(SP,_x); } while(0)
#define PUSH_UW(_x) do{ SP -= 2; CS_UW(SP,_x); } while(0)
#define POP_UL() (SP+=4, CL_UL(SP-4))
#define POP_L() (SP+=4, CL_L(SP-4))
#define POP_UW() (SP+=2, CL_UW(SP-2))

#define ALIGNED_R(_x) do{if ((_x)&1) ADDRESS_ERROR(_x,1); } while (0);
#define ALIGNED_W(_x) do{if ((_x)&1) ADDRESS_ERROR(_x,0); } while (0);

static INLINE B CL_B (UL s)
{
	s = TRIM(s);
	if (NORMAL_ADDRESS_R_B(s)) return LM_B(ADDR(s));
	else return LS_B(s);
}
static INLINE UB CL_UB (UL s)
{
	s = TRIM(s);
	if (NORMAL_ADDRESS_R_B(s)) return LM_UB(ADDR(s));
	else return LS_UB(s);
}
static INLINE W CL_W (UL s)
{
	s = TRIM(s);
	ALIGNED_R(s);
	if (NORMAL_ADDRESS_R_W(s)) return LM_W(ADDR(s));
	else return LS_W(s);
}
static INLINE UW CL_UW (UL s)
{
	s = TRIM(s);
	ALIGNED_R(s);
	if (NORMAL_ADDRESS_R_W(s)) return LM_UW(ADDR(s));
	else return LS_UW(s);
}
static INLINE L CL_L (UL s)
{
	s = TRIM(s);
	ALIGNED_R(s);
	if (NORMAL_ADDRESS_R_L(s)) return LM_L(ADDR(s));
	else return LS_L(s);
}
static INLINE UL CL_UL (UL s)
{
	s = TRIM(s);
	ALIGNED_R(s);
	if (NORMAL_ADDRESS_R_L(s)) return LM_UL(ADDR(s));
	else return LS_UL(s);
}

static INLINE void CS_B (UL d, B v)
{
	d = TRIM(d);
	if (NORMAL_ADDRESS_W_B(d)) SM_B(ADDR(d), v);
	else SS_B(d, v);
}
static INLINE void CS_UB (UL d, UB v)
{
	d = TRIM(d);
	if (NORMAL_ADDRESS_W_B(d)) SM_UB(ADDR(d), v);
	else SS_UB(d, v);
}
static INLINE void CS_W (UL d, W v)
{
	d = TRIM(d);
	ALIGNED_W(d);
	if (NORMAL_ADDRESS_W_W(d)) SM_W(ADDR(d), v);
	else SS_W(d, v);
}
static INLINE void CS_UW (UL d, UW v)
{
	d = TRIM(d);
	ALIGNED_W(d);
	if (NORMAL_ADDRESS_W_W(d)) SM_W(ADDR(d), v);
	else SS_W(d, v);
}
static INLINE void CS_L (UL d, L v)
{
	d = TRIM(d);
	ALIGNED_W(d);
	if (NORMAL_ADDRESS_W_L(d)) SM_L(ADDR(d), v);
	else SS_L(d, v);
}
static INLINE void CS_UL (UL d, UL v)
{
	d = TRIM(d);
	ALIGNED_W(d);
	if (NORMAL_ADDRESS_W_L(d)) SM_UL(ADDR(d), v);
	else SS_UL(d, v);
}
#endif /* CPU_H */
