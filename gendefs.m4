dnl Important: US_* should be last because it doesn't abort
dnl unchecked: BCD,shift/rotate
dnl bugs: N flag in shift/rotate

#include "defs.h"
#include "cpu.h"
#include "gendefs.h"
#include "mem.h"
#include "gemdos.h"
#include "native.h"

define(LS_DREG_BYTE,`{B *v_$2=DREG_BYTE($1); $2=LR_BYTE(v_$2);')
define(US_DREG_BYTE,`SR_BYTE(v_$1,$1);}')
define(LS_AN_I_BYTE,`{UL v_$2=V_AN_I_BYTE($1); $2=CL_BYTE(v_$2);')
define(US_AN_I_BYTE,`CS_BYTE(v_$1,$1);}')
define(LS_AN_PI_BYTE,`{UL v_$2=V_AN_PI_BYTE($1); $2=CL_BYTE(v_$2);')
define(US_AN_PI_BYTE,`CS_BYTE(v_$1,$1);}')
define(LS_AN_PD_BYTE,`{UL v_$2=V_AN_PD_BYTE($1); $2=CL_BYTE(v_$2);')
define(US_AN_PD_BYTE,`CS_BYTE(v_$1,$1);}')
define(LS_AN_PI_BYTE_All,`{UL v_$2=V_AN_PI_BYTE_All($1); $2=CL_BYTE(v_$2);')
define(US_AN_PI_BYTE_All,`CS_BYTE(v_$1,$1);}')
define(LS_AN_PD_BYTE_All,`{UL v_$2=V_AN_PD_BYTE_All($1); $2=CL_BYTE(v_$2);')
define(US_AN_PD_BYTE_All,`CS_BYTE(v_$1,$1);}')
define(LS_AN_D_BYTE,`{UL v_$2=V_AN_D_BYTE($1); $2=CL_BYTE(v_$2);')
define(US_AN_D_BYTE,`CS_BYTE(v_$1,$1);}')
define(LS_AN_R_BYTE,`{UL v_$2=V_AN_R_BYTE($1); $2=CL_BYTE(v_$2);')
define(US_AN_R_BYTE,`CS_BYTE(v_$1,$1);}')
define(LS_ABS_W_BYTE,`{UL v_$2=V_ABS_W_BYTE(); $2=CL_BYTE(v_$2);')
define(US_ABS_W_BYTE,`CS_BYTE(v_$1,$1);}')
define(LS_ABS_L_BYTE,`{UL v_$2=V_ABS_L_BYTE(); $2=CL_BYTE(v_$2);')
define(US_ABS_L_BYTE,`CS_BYTE(v_$1,$1);}')

define(LS_DREG_WORD,`{W *v_$2=DREG_WORD($1); $2=LR_WORD(v_$2);')
define(US_DREG_WORD,`SR_WORD(v_$1,$1);}')
define(LS_AN_I_WORD,`{UL v_$2=V_AN_I_WORD($1); $2=CL_WORD(v_$2);')
define(US_AN_I_WORD,`CS_WORD(v_$1,$1);}')
define(LS_AN_PI_WORD,`{UL v_$2=V_AN_PI_WORD($1); $2=CL_WORD(v_$2);')
define(US_AN_PI_WORD,`CS_WORD(v_$1,$1);}')
define(LS_AN_PD_WORD,`{UL v_$2=V_AN_PD_WORD($1); $2=CL_WORD(v_$2);')
define(US_AN_PD_WORD,`CS_WORD(v_$1,$1);}')
define(LS_AN_D_WORD,`{UL v_$2=V_AN_D_WORD($1); $2=CL_WORD(v_$2);')
define(US_AN_D_WORD,`CS_WORD(v_$1,$1);}')
define(LS_AN_R_WORD,`{UL v_$2=V_AN_R_WORD($1); $2=CL_WORD(v_$2);')
define(US_AN_R_WORD,`CS_WORD(v_$1,$1);}')
define(LS_ABS_W_WORD,`{UL v_$2=V_ABS_W_WORD(); $2=CL_WORD(v_$2);')
define(US_ABS_W_WORD,`CS_WORD(v_$1,$1);}')
define(LS_ABS_L_WORD,`{UL v_$2=V_ABS_L_WORD(); $2=CL_WORD(v_$2);')
define(US_ABS_L_WORD,`CS_WORD(v_$1,$1);}')

define(LS_DREG_LONG,`{L *v_$2=DREG_LONG($1); $2=LR_LONG(v_$2);')
define(US_DREG_LONG,`SR_LONG(v_$1,$1);}')
define(LS_AN_I_LONG,`{UL v_$2=V_AN_I_LONG($1); $2=CL_LONG(v_$2);')
define(US_AN_I_LONG,`CS_LONG(v_$1,$1);}')
define(LS_AN_PI_LONG,`{UL v_$2=V_AN_PI_LONG($1); $2=CL_LONG(v_$2);')
define(US_AN_PI_LONG,`CS_LONG(v_$1,$1);}')
define(LS_AN_PD_LONG,`{UL v_$2=V_AN_PD_LONG($1); $2=CL_LONG(v_$2);')
define(US_AN_PD_LONG,`CS_LONG(v_$1,$1);}')
define(LS_AN_D_LONG,`{UL v_$2=V_AN_D_LONG($1); $2=CL_LONG(v_$2);')
define(US_AN_D_LONG,`CS_LONG(v_$1,$1);}')
define(LS_AN_R_LONG,`{UL v_$2=V_AN_R_LONG($1); $2=CL_LONG(v_$2);')
define(US_AN_R_LONG,`CS_LONG(v_$1,$1);}')
define(LS_ABS_W_LONG,`{UL v_$2=V_ABS_W_LONG(); $2=CL_LONG(v_$2);')
define(US_ABS_W_LONG,`CS_LONG(v_$1,$1);}')
define(LS_ABS_L_LONG,`{UL v_$2=V_ABS_L_LONG(); $2=CL_LONG(v_$2);')
define(US_ABS_L_LONG,`CS_LONG(v_$1,$1);}')

define(L_DREG_BYTE,`$2=LDREG_BYTE($1);')
define(S_DREG_BYTE,`SDREG_BYTE($1,$2);')
define(L_AN_I_BYTE,`$2=CL_BYTE(V_AN_I_BYTE($1));')
define(S_AN_I_BYTE,`CS_BYTE(V_AN_I_BYTE($1),$2);')
define(L_AN_PI_BYTE,`$2=CL_BYTE(V_AN_PI_BYTE($1));')
define(S_AN_PI_BYTE,`CS_BYTE(V_AN_PI_BYTE($1),$2);')
define(L_AN_PD_BYTE,`$2=CL_BYTE(V_AN_PD_BYTE($1));')
define(S_AN_PD_BYTE,`CS_BYTE(V_AN_PD_BYTE($1),$2);')
define(L_AN_PI_BYTE_All,`$2=CL_BYTE(V_AN_PI_BYTE_All($1));')
define(S_AN_PI_BYTE_All,`CS_BYTE(V_AN_PI_BYTE_All($1),$2);')
define(L_AN_PD_BYTE_All,`$2=CL_BYTE(V_AN_PD_BYTE_All($1));')
define(S_AN_PD_BYTE_All,`CS_BYTE(V_AN_PD_BYTE_All($1),$2);')
define(L_AN_D_BYTE,`$2=CL_BYTE(V_AN_D_BYTE($1));')
define(S_AN_D_BYTE,`CS_BYTE(V_AN_D_BYTE($1),$2);')
define(L_AN_R_BYTE,`$2=CL_BYTE(V_AN_R_BYTE($1));')
define(S_AN_R_BYTE,`CS_BYTE(V_AN_R_BYTE($1),$2);')
define(L_ABS_W_BYTE,`$2=CL_BYTE(V_ABS_W_BYTE());')
define(S_ABS_W_BYTE,`CS_BYTE(V_ABS_W_BYTE(),$2);')
define(L_ABS_L_BYTE,`$2=CL_BYTE(V_ABS_L_BYTE());')
define(S_ABS_L_BYTE,`CS_BYTE(V_ABS_L_BYTE(),$2);')
define(L_PC_D_BYTE,`$2=CL_BYTE(V_PC_D_BYTE());')
define(S_PC_D_BYTE,`CS_BYTE(V_PC_D_BYTE(),$2);')
define(L_PC_R_BYTE,`$2=CL_BYTE(V_PC_R_BYTE());')
define(S_PC_R_BYTE,`CS_BYTE(V_PC_R_BYTE(),$2);')
define(L_IMM_BYTE,`$2=LM_BYTE(MEM(V_IMM_BYTE()));')
define(S_IMM_BYTE,`SM_BYTE(MEM(V_IMM_BYTE()),$2);')

define(L_DREG_WORD,`$2=LDREG_WORD($1);')
define(S_DREG_WORD,`SDREG_WORD($1,$2);')
define(L_AREG_WORD,`$2=LDREG_WORD($1+8);')
define(S_AREG_WORD,`SDREG_WORD($1+8,$2);')
define(L_AN_I_WORD,`$2=CL_WORD(V_AN_I_WORD($1));')
define(S_AN_I_WORD,`CS_WORD(V_AN_I_WORD($1),$2);')
define(L_AN_PI_WORD,`$2=CL_WORD(V_AN_PI_WORD($1));')
define(S_AN_PI_WORD,`CS_WORD(V_AN_PI_WORD($1),$2);')
define(L_AN_PD_WORD,`$2=CL_WORD(V_AN_PD_WORD($1));')
define(S_AN_PD_WORD,`CS_WORD(V_AN_PD_WORD($1),$2);')
define(L_AN_D_WORD,`$2=CL_WORD(V_AN_D_WORD($1));')
define(S_AN_D_WORD,`CS_WORD(V_AN_D_WORD($1),$2);')
define(L_AN_R_WORD,`$2=CL_WORD(V_AN_R_WORD($1));')
define(S_AN_R_WORD,`CS_WORD(V_AN_R_WORD($1),$2);')
define(L_ABS_W_WORD,`$2=CL_WORD(V_ABS_W_WORD());')
define(S_ABS_W_WORD,`CS_WORD(V_ABS_W_WORD(),$2);')
define(L_ABS_L_WORD,`$2=CL_WORD(V_ABS_L_WORD());')
define(S_ABS_L_WORD,`CS_WORD(V_ABS_L_WORD(),$2);')
define(L_PC_D_WORD,`$2=CL_WORD(V_PC_D_WORD());')
define(S_PC_D_WORD,`CS_WORD(V_PC_D_WORD(),$2);')
define(L_PC_R_WORD,`$2=CL_WORD(V_PC_R_WORD());')
define(S_PC_R_WORD,`CS_WORD(V_PC_R_WORD(),$2);')
define(L_IMM_WORD,`$2=LM_WORD(MEM(V_IMM_WORD()));')
define(S_IMM_WORD,`SM_WORD(MEM(V_IMM_WORD()),$2);')

define(L_DREG_LONG,`$2=LDREG_LONG($1);')
define(S_DREG_LONG,`SDREG_LONG($1,$2);')
define(L_AREG_LONG,`$2=LDREG_LONG($1+8);')
define(S_AREG_LONG,`SDREG_LONG($1+8,$2);')
define(L_AN_I_LONG,`$2=CL_LONG(V_AN_I_LONG($1));')
define(S_AN_I_LONG,`CS_LONG(V_AN_I_LONG($1),$2);')
define(L_AN_PI_LONG,`$2=CL_LONG(V_AN_PI_LONG($1));')
define(S_AN_PI_LONG,`CS_LONG(V_AN_PI_LONG($1),$2);')
define(L_AN_PD_LONG,`$2=CL_LONG(V_AN_PD_LONG($1));')
define(S_AN_PD_LONG,`CS_LONG(V_AN_PD_LONG($1),$2);')
define(L_AN_D_LONG,`$2=CL_LONG(V_AN_D_LONG($1));')
define(S_AN_D_LONG,`CS_LONG(V_AN_D_LONG($1),$2);')
define(L_AN_R_LONG,`$2=CL_LONG(V_AN_R_LONG($1));')
define(S_AN_R_LONG,`CS_LONG(V_AN_R_LONG($1),$2);')
define(L_ABS_W_LONG,`$2=CL_LONG(V_ABS_W_LONG());')
define(S_ABS_W_LONG,`CS_LONG(V_ABS_W_LONG(),$2);')
define(L_ABS_L_LONG,`$2=CL_LONG(V_ABS_L_LONG());')
define(S_ABS_L_LONG,`CS_LONG(V_ABS_L_LONG(),$2);')
define(L_PC_D_LONG,`$2=CL_LONG(V_PC_D_LONG());')
define(S_PC_D_LONG,`CS_LONG(V_PC_D_LONG(),$2);')
define(L_PC_R_LONG,`$2=CL_LONG(V_PC_R_LONG());')
define(S_PC_R_LONG,`CS_LONG(V_PC_R_LONG(),$2);')
define(L_IMM_LONG,`$2=LM_LONG(MEM(V_IMM_LONG()));')
define(S_IMM_LONG,`SM_LONG(MEM(V_IMM_LONG()),$2);')

define(ABCD_REG,`
BYTE s,x,*d;
s = LDREG_BYTE($2);
d = DREG_B($1);
x = BCD_ADD(s,LR_B(d));
SR_B(d,x);
THREAD
')

define(ABCD_MEM,`
BYTE s,x;
UL d;
s = CL_B(V_AN_PD_BYTE_All($2));
d = V_AN_PD_BYTE_All($1);
x = BCD_ADD(s,CL_B(d));
CS_B(d,x);
THREAD
')

define(ORI,`
$1 x;
$1 imm=$2;
LS_$3($4,x)
sr &= ~MASK_CC|MASK_CC_X;
x |= imm;
SET_NZ(x);
US_$3(x);
THREAD
')

define(ORI_TO_CCR,`
BYTE imm=$1;
FIX_CCR(); sr &= ~0xff; sr |= imm & 0xff;
THREAD
')

define(ORI_TO_SR,`
WORD imm=$1;
FIX_CCR();
if (!SUPERVISOR_MODE) ex_privileged();
else sr |= imm;
CHECK_TRACE();
THREAD;
')

define(MOVEP_MR,`
ULONG d=AREG($3)+$2;
ifelse($1,`WORD',`SDREG_W($4,(CL_B(d)<<8)|(CL_B(d+2)&0xff));',
	`SDREG_L($4,(CL_B(d)<<24)|((CL_B(d+2)&0xff)<<16)
	|((CL_B(d+4)&0xff)<<8)|(CL_B(d+6)&0xff));')
THREAD;
')

define(MOVEP_RM,`
U$1 x;
ULONG d=AREG($4)+$2;
x=LDREG_U$1($3);
ifelse($1,`WORD',`CS_B(d,x>>8);CS_B(d+2,x&0xff);',
`CS_B(d,x>>24);
 CS_B(d+2,(x>>16)&0xff);CS_B(d+4,(x>>8)&0xff);CS_B(d+6,x&0xff);')
THREAD;
')

define(DO_BTST,`
UNSET_Z(); if (!((($1)>>($2))&1)) SET_Z();
')

define(BTST_DR,`DO_BTST(DREG($2),DREG($1)&31); THREAD')

define(BTST_DM,`
int x;
L_$2($3,x);
DO_BTST(x,DREG($1)&7);
THREAD
')

define(BTST_SR,`
int n=$1;
DO_BTST(DREG($2),n&31);
THREAD
')

define(BTST_SM,`
int x;
int n=$1;
L_$2($3,x);
DO_BTST(x,n&7);
THREAD
')

define(DO_BCHG,`
UNSET_Z();
if (!((($1)>>($2))&1)) SET_Z();
$1 ^= 1<<($2);
THREAD
')

define(BCHG_DR,`DO_BCHG(DREG($2),DREG($1)&31); THREAD')

define(BCHG_DM,`
int x;
int n;
LS_$2($3,x);
n=DREG($1)&7;
DO_BCHG(x,n)
US_$2(x);
THREAD
')

define(BCHG_SR,`
int n=$1;
DO_BCHG(DREG($2),n&31);
THREAD
')

define(BCHG_SM,`
int x;
int n=$1;
LS_$2($3,x);
DO_BCHG(x,n&7)
US_$2(x);
THREAD
')

define(DO_BCLR,`
UNSET_Z();
if (!((($1)>>($2))&1)) SET_Z();
else $1 ^= 1<<($2);
THREAD;
')

define(BCLR_DR,`DO_BCLR(DREG($2),DREG($1)&31); THREAD')

define(BCLR_DM,`
int x,n;
LS_$2($3,x);
n=DREG($1)&7;
DO_BCLR(x,n)
US_$2(x);
THREAD
')

define(BCLR_SR,`
int n=$1;
DO_BCLR(DREG($2),n&31);
THREAD
')

define(BCLR_SM,`
int x;
int n=$1;
LS_$2($3,x);
DO_BCLR(x,n&7)
US_$2(x);
THREAD
')

define(DO_BSET,`
UNSET_Z();
if (!((($1)>>($2))&1))
{
	SET_Z();
	$1 |= 1<<($2);
}
THREAD
')

define(BSET_DR,`DO_BSET(DREG($2),DREG($1)&31); THREAD')

define(BSET_DM,`
int x,n;
LS_$2($3,x);
n=DREG($1)&7;
DO_BSET(x,n)
US_$2(x);
THREAD
')

define(BSET_SR,`
int n=$1;
DO_BSET(DREG($2),n&31);
THREAD
')

define(BSET_SM,`
int x;
int n=$1;
LS_$2($3,x);
DO_BSET(x,n&7)
US_$2(x);
THREAD
')

define(ANDI,`
$1 x,imm=$2;
LS_$3($4,x);
sr &= ~MASK_CC | MASK_CC_X;
x &= imm;
SET_NZ(x);
US_$3(x);
THREAD
')

define(ANDI_CCR,`
UBYTE imm=$1;
FIX_CCR();
sr &= ~MASK_CC | (imm & MASK_CC);
CHECK_CCR();
THREAD
')

define(ANDI_TO_SR,`
UWORD imm=$1;
if (!SUPERVISOR_MODE) ex_privileged();
else
{
	FIX_CCR();
	sr &= imm;  /* BUG!!!! -> CCR!!! */
	CHECK_CCR();
	if ((imm & SR_S) == 0)  /* going to user-mode */
		enter_user_mode();
	CHECK_NOTRACE();
}
THREAD
')

define(SUBI,`
$1 r,x,s;
s=$2;
LS_$3($4,x);
sr &= ~MASK_CC;
r = x-s;
SET_VCX_SUB;
x=r;
SET_NZ(x);
US_$3(x);
THREAD
')

define(ADDI,`
$1 r,x,s;
s=$2;
LS_$3($4,x);
sr &= ~MASK_CC;
r = x+s;
SET_VCX_ADD;
x=r;
SET_NZ(x);
US_$3(x);
THREAD
')

define(EORI,`
$1 x,imm=$2;
LS_$3($4,x);
sr &= ~MASK_CC | MASK_CC_X;
x ^= imm;
SET_NZ(x);
US_$3(x);
THREAD
')

define(EORI_CCR,`
B imm=$1;
FIX_CCR();
sr ^= imm & MASK_CC;
CHECK_CCR();
THREAD
')

define(EORI_TO_SR,`
W imm=$1;
if (!SUPERVISOR_MODE) ex_privileged();
else
{   /* TODO: correct so that only valid SR bits are affected */
	FIX_CCR();  /* BUG!!!! -> CCR! */
	sr ^= imm;
	CHECK_CCR();
	if (!SUPERVISOR_MODE) enter_user_mode();
	CHECK_TRACE();
	CHECK_NOTRACE();
}
THREAD
')

define(DO_CMP,`
{
$1 r,s,x;
s=$2; x=$3;
UNSET_NZ();
sr &= ~MASK_CC|MASK_CC_X;
r = x - s;
if ((x^s) < 0 && (s^r) >= 0) sr |= MASK_CC_V;
if ((r&s) < 0 || (~x & (r|s)) < 0) sr |= MASK_CC_C;
SET_NZ(r);
}
THREAD
')

define(CMPI,`
$1 xx,imm; 
imm=$2;
L_$3($4,xx);
DO_CMP($1,imm,xx);
THREAD
')

define(MOVE,`
int x;
sr&= ~MASK_CC | MASK_CC_X;
L_$1($2,x);
SET_NZ(x);
S_$3($4,x);
THREAD
')

define(MOVEA,`
LONG x;
L_$1($2,x);
SDREG_LONG($3+8,x);
THREAD
')

define(NEGX,`
$1 r,x;
LS_$2($3,x);
r = -x;
if (sr & MASK_CC_X) r--;
sr &= ~MASK_CC;
if (r) SET_NZ(r);
else UNSET_N();
if ((r&x) < 0) sr |= MASK_CC_V;
if ((r|x) < 0) sr |= MASK_CC_C|MASK_CC_X;
x=r;
US_$2(x);
THREAD
')

define(MOVE_SR,`
FIX_CCR();
S_$1($2,sr);	/* FIXME: read before write according to manual! */
THREAD
')

define(CHK,`
int s,u;
s=LDREG_W($3);
L_$1($2,u);
if (s>u) UNSET_N();
else if (s<0) SET_N();
if (s<0||s>u) ex_chk();
THREAD
')

define(LEA,`
UL t=V_$1($2);
AREG($3)=t;
THREAD
')

define(CLR,`
UNSET_N();
sr &= ~(MASK_CC_V|MASK_CC_C);
SET_Z();
S_$2($3,0);
THREAD
')

define(NEG,`
$1 r,x;
LS_$2($3,x);
sr &= ~MASK_CC;
r = -x;
SET_NZ(r);
if ((r|x)<0) sr |= MASK_CC_C|MASK_CC_X; /* Manual is not clear about this */
if ((r&x)<0) sr |= MASK_CC_V;
x=r;
US_$2(x);
THREAD
')

define(MOVE_TO_CCR,`
W d;
L_$1($2,d);
sr &= ~0xff;
sr |= d & 0xff;     /* BUG!!!! */
CHECK_CCR();
THREAD
')

define(NOT,`
$1 x;
LS_$2($3,x);
sr &= ~MASK_CC|MASK_CC_X;
x = ~x;
SET_NZ(x);
US_$2(x);
THREAD
')

define(MOVE_TO_SR,`
W d;
if (!SUPERVISOR_MODE) ex_privileged();
else
{
	L_$1($2,d);
	sr = d & SR_IMPLEMENTED;    /* BUG!!!! -> set CCR before or after? */
        if (!SUPERVISOR_MODE) enter_user_mode();
	CHECK_CCR();
	CHECK_TRACE();
	CHECK_NOTRACE();
}
THREAD
')

define(NBCD,`
BYTE x;
LS_$1($2,x);
x=BCD_SUB(x,0);
US_$1(x);
THREAD
')

define(SWAP,`
int tmp,reg=$1;
sr &= ~MASK_CC|MASK_CC_X;
tmp = (DREG(reg)>>16)&0xffff;
DREG(reg)<<=16;
DREG(reg)|=tmp;
SET_NZ(DREG(reg));
THREAD
')

define(PEA,`
UL t=V_$1($2);
PUSH_UL(t);
THREAD
')

define(EXT,`
$1 x,reg=$2;
sr &= ~MASK_CC|MASK_CC_X;
ifelse($1,`LONG',`x=DREG(reg)=LDREG_W(reg);',`x=LDREG_B(reg);SDREG_W(reg,x);')
SET_NZ(x);
THREAD
')

define(MOVEM_RM_CA,`
int mask,i;
UL x;
mask=$2;
x=V_$3($4);
for (i=0;i<16;i++)
{
	if (mask&(1<<i))
	{
		CS_$1(x,LDREG_$1(i));
		x += SIZE$1;
	}
}
THREAD
')

define(MOVEM_RM_PD0,`
if (mask&(1<<$1))
{
	AREG(an) -= SIZE$2;
	if (7-$1 == an) CS_$2(V_AN_I_$2(an),s);
	else CS_$2(V_AN_I_$2(an), LDREG_$2(15-$1));
}
THREAD
')

define(MOVEM_RM_PD,`
int mask,an,s,i;
mask=$2;
an=$3;
s=LDREG_$1(an+8);
for(i=0;i<16;i++)
MOVEM_RM_PD0(i,$1);
THREAD
')

define(MOVEM_MR_C,`
int mask,i;
UL x;
mask=$2;
x=V_$3($4);
for (i=0;i<16;i++)
{
	if (mask&(1<<i))
	{
		DREG(i) = (LONG)CL_$1(x);
		x += SIZE$1;
	}
}
THREAD
')

define(MOVEM_MR_PI0,`
if (mask&(1<<$1))
{
	if ($1-8 != an) DREG($1)=(LONG)CL_$2(V_AN_I_$2(an));
	AREG(an) += SIZE$2;
}
THREAD
')

define(MOVEM_MR_PI,`
int mask,an,i;
UL x;
mask=$2;
an=$3;
for (i=0;i<16;i++)
MOVEM_MR_PI0(i,$1);
THREAD
')

define(TST,`
$1 x;
sr &= ~MASK_CC|MASK_CC_X;
L_$2($3,x);
SET_NZ(x);
THREAD
')

define(TAS,`
BYTE x;
LS_$1($2,x);
sr &= ~MASK_CC|MASK_CC_X;
SET_NZ(x);
x |= 0x80;
US_$1(x);
THREAD
')

define(TRAP,`
int n=$1;
ex_trap(CPU_EXCEPTION_TRAP_0+n);
')

define(LINK,`
WORD displ;
LONG r;
int reg;
displ=$1;
reg=$2;
r=AREG(reg);
PUSH_L(r);
AREG(reg)=SP;
SP+=displ;
THREAD
')

define(UNLK,`
int reg=$1;
SP=AREG(reg);
AREG(reg)=POP_L();
THREAD
')

define(MOVE_TO_USP,`
if (!SUPERVISOR_MODE) ex_privileged();
else AREG(8)=AREG($1);
THREAD
')

define(MOVE_USP,`
if (!SUPERVISOR_MODE) ex_privileged();
else AREG($1)=AREG(8);
THREAD
')

define(RESET,`
if (!SUPERVISOR_MODE) ex_privileged();
else /* FIXME */;
THREAD
')

define(NOP,`THREAD')

define(STOP,`
if (!SUPERVISOR_MODE) ex_privileged();
else
{
	sr = $1;
	/* FIXME */

	if ((sr & 0x0700) == 0x0700)
	{
		/* CPU has been halted, so exit STonX now! */
		stonx_exit();
		exit(0);
	}
}
THREAD
')

define(RTE,`
if (!SUPERVISOR_MODE)  ex_privileged();
else
{
        sr = POP_UW();  /* FIXME!!!! */
        CHECK_CCR();
        pc = POP_UL();
        if (!SUPERVISOR_MODE) enter_user_mode();
        /* not sure about the order */
        CHECK_PC();
        CHECK_TRACE();
        CHECK_NOTRACE();
}
THREAD
')

define(RTS,`
pc = POP_UL();
CHECK_PC();
THREAD
')

define(TRAPV,`
if (sr & MASK_CC_V) ex_privileged();
THREAD
')

define(RTR,`
UW ccr;
ccr = POP_UW() & 0xff;
sr &= ~0xff;
sr |= ccr;  /* FIXME!!!! */
CHECK_CCR();
pc = POP_UL();
CHECK_PC();
THREAD
')

define(JSR,`
ULONG t=V_$1($2);
PUSH_UL(pc); pc=t; CHECK_PC();
THREAD_CHECK
')

define(JMP,`
pc=V_$1($2);
CHECK_PC();
THREAD_CHECK
')

define(ADDQ,`
$1 r,x,s;
s=$2;
LS_$3($4,x);
sr &= ~MASK_CC;
r = x+s;
SET_VCX_ADD;
SET_NZ(r);
x=r;
US_$3(x);
THREAD
')

define(ADDQ_AREG,`
AREG($3)+=$2;
THREAD
')

define(SCC,`
S_$2($3,TEST_$1 ? -1:0);
THREAD
')

define(DBCC,`
int r;
WORD x,displ=$2;
if (!TEST_$1)
{
	r=$3;
	x=LDREG_WORD(r)-1;
	SDREG_WORD(r,x);
	if (x != -1)
	{
		pc += displ-2;
		CHECK_PC();
	}
}
THREAD_CHECK
')

define(SUBQ,`
$1 r,x,s;
s=$2;
LS_$3($4,x);
sr &= ~MASK_CC;
r = x-s;
SET_VCX_SUB;
SET_NZ(r);
x=r;
US_$3(x);
THREAD
')

define(SUBQ_AREG,`
AREG($3)-=$2;
THREAD
')

define(BRA,`
int displ=$1;
pc += displ-2;
CHECK_PC();
THREAD_CHECK
')

define(BRAS,`
pc += $1;
CHECK_PC();
THREAD_CHECK
')

define(BSR,`
int displ=$1;
PUSH_UL(pc);
pc += displ-2;
CHECK_PC();
THREAD_CHECK
')

define(BSRS,`
PUSH_UL(pc);
pc += $1;
CHECK_PC();
THREAD_CHECK
')

define(BCC,`
int displ=$2;
if (TEST_$1) {pc+=displ-2; CHECK_PC();}
THREAD_CHECK
')

define(BCCS,`
if (TEST_$1) {pc+=$2; CHECK_PC();}
THREAD_CHECK
')

define(MOVEQ,`
LONG x=$1;
sr &= ~MASK_CC|MASK_CC_X;
SET_NZ(x);
DREG($2)=x;
THREAD
')

define(OR_EA,`
$1 r,x,s;
int reg;
reg=$4;
L_$2($3,s);
x=LDREG_$1(reg);
sr &= ~MASK_CC|MASK_CC_X;
r = s | x;
SET_NZ(r);
SDREG_$1(reg,r);
THREAD
')

define(AND_EA,`
$1 r,x,s;
int reg;
reg=$4;
L_$2($3,s);
x=LDREG_$1(reg);
sr &= ~MASK_CC|MASK_CC_X;
r = s & x;
SET_NZ(r);
SDREG_$1(reg,r);
THREAD
')

define(DIVU,`
UWORD d;
ULONG r,x;
int reg;
reg=$3;
L_$1($2,d);
x=LDREG_UL(reg);
if (d==0) ex_div0();
else
{
	sr &= ~MASK_CC|MASK_CC_X;
	r = x / d;
	if (((UWORD) r) != r) sr |= MASK_CC_V;
	else
	{
		SET_NZ((LONG)((WORD) r));
		r &= 0xffff;
		r |= (x % d) << 16;
		DREG(reg) = r;
	}
#if 0
	fprintf(stderr,"divu: %d / %d = <%d,%d> = %0x sr=%04x nz=%d\n",
		x, d, (x/d), (x%d), r,sr,nz_save);
#endif
}
THREAD
')

define(SBCD_REG,`
BYTE s,x,*d;
s = LDREG_BYTE($2);
d = DREG_B($1);
x = BCD_SUB(s,LR_B(d));
SR_B(d,x);
THREAD
')

define(SBCD_MEM,`
BYTE s,x;
UL d;
s = CL_B(V_AN_PD_BYTE_All($2));
d = V_AN_PD_BYTE_All($1);
x = BCD_SUB(s,CL_B(d));
CS_B(d,x);
THREAD
')

define(OR_REG,`
$1 r,x,s;
s=LDREG_$1($2);
LS_$3($4,x);
sr &= ~MASK_CC|MASK_CC_X;
r = s | x;
SET_NZ(r);
x=r;
US_$3(x);
THREAD
')

define(AND_REG,`
$1 r,x,s;
s=LDREG_$1($2);
LS_$3($4,x);
sr &= ~MASK_CC|MASK_CC_X;
r = s & x;
SET_NZ(r);
x=r;
US_$3(x);
THREAD
')


define(DIVS,`
WORD d;
LONG r;
int x,reg=$3;
x=DREG(reg);
L_$1($2,d);
if (d==0) ex_div0();
else
{
	sr &= ~MASK_CC|MASK_CC_X;
	r = x/d;
	if (((WORD)r)!=r) sr |= MASK_CC_V;
	else
	{
		SET_NZ(((WORD)r));
		r &= 0xffff;
		r |= (x%d) << 16;
		DREG(reg) = r;
	}
#if 0
	fprintf(stderr,"* divs: %d / %d = <%d,%d> = %0x sr=%04x nz=%d\n",
		x, d, (x/d), (x%d), r, sr, nz_save);
#endif
}
THREAD
')

define(SUB_EA,`
$1 r,x,s;
int n=$4;
L_$2($3,s);
x=LDREG_$1(n);
sr &= ~MASK_CC;
r = x - s;
SET_VCX_SUB;
SET_NZ(r);
SDREG_$1(n,r);
THREAD
')

define(SUB_REG,`
$1 s,r,x;
s=LDREG_$1($2);
LS_$3($4,x);
sr &= ~MASK_CC;
r = x - s;
SET_VCX_SUB;
SET_NZ(r);
x=r;
US_$3(x);
THREAD
')

define(SUBA,`
LONG x;
L_$2($3,x);
AREG($4) -= x;
THREAD
')

define(SUBX_DREG,`
$1 s,x,r,t;
int n=$3;
t=GET_X_BIT();
sr &= ~MASK_CC | MASK_CC_Z; /* FIXME? */
s=LDREG_$1($2);
x=LDREG_$1(n);
r=x-s-t;
SET_VCX_SUB;
if (r) SET_NZ(r);
else UNSET_N();
SDREG_$1(n,r);
THREAD
')

define(SUBX_PD,`
$1 s,x,r,t;
int n=$3;
t=GET_X_BIT();
sr &= ~MASK_CC | MASK_CC_Z;
ifelse($1,`BYTE',`
L_AN_PD_BYTE_All($2,s);
LS_AN_PD_BYTE_All(n,x);',`
L_AN_PD_$1($2,s);
LS_AN_PD_$1(n,x);')
r=x-s-t;
SET_VCX_SUB;
UNSET_Z();
SET_NZ(r);
x=r;
US_AN_PD_$1(x);
THREAD
')

define(LINEA,`
pc -= 2; ex_linea();
THREAD
')

define(NATIVE,`
ULONG f=$1;
call_native(SP+4,f);
THREAD
')

define(CMP,`
$1 xx,yy;
L_$2($3,xx);
yy=LDREG_$1($4);
DO_CMP($1,xx,yy);
THREAD
')

define(CMPA,`
LONG xx,yy;
L_$2($3,xx);
yy=AREG($4);
DO_CMP(LONG,xx,yy);
THREAD
')

define(EOR,`
$1 r,x,s;
s=LDREG_$1($2);
LS_$3($4,x);
sr &= ~MASK_CC|MASK_CC_X;
r = s^x;
SET_NZ(r);
x=r;
US_$3(x);
THREAD
')

define(CMPM,`
$1 a,b;
ifelse($1,`BYTE',`
L_AN_PI_BYTE_All($2,a);
L_AN_PI_BYTE_All($3,b);
',`
L_AN_PI_$1($2,a);
L_AN_PI_$1($3,b);
')
DO_CMP($1,a,b);
THREAD
')

define(ADD_EA,`
$1 r,x,s;
int n=$4;
L_$2($3,s);
x=LDREG_$1(n);
sr &= ~MASK_CC;
r = x + s;
SET_VCX_ADD;
SET_NZ(r);
SDREG_$1(n,r);
THREAD
')

define(MULU,`
UWORD d,m;
ULONG r;
int n=$3;
L_$1($2,d);
m=LDREG_UWORD(n);
sr &= ~MASK_CC|MASK_CC_X;
r = d * m;
SET_NZ((LONG)r);
DREG(n) = (LONG)r;
THREAD
')

define(ADD_REG,`
$1 s,r,x;
s=LDREG_$1($2);
LS_$3($4,x);
sr &= ~MASK_CC;
r = x + s;
SET_VCX_ADD;
SET_NZ(r);
x=r;
US_$3(x);
THREAD
')

define(EXG_DD,`
L t;
t=DREG($1);
DREG($1)=DREG($2);
DREG($2)=t;
THREAD
')

define(EXG_AA,`
L t;
t=AREG($1);
AREG($1)=AREG($2);
AREG($2)=t;
THREAD
')

define(EXG_DA,`
L t;
t=DREG($1);
DREG($1)=AREG($2);
AREG($2)=t;
THREAD
')

define(MULS,`
WORD d,m;
LONG r;
int n=$3;
L_$1($2,d);
m=LDREG_WORD(n);
sr &= ~MASK_CC|MASK_CC_X;
r = d * m;
SET_NZ(r);
DREG(n) = r;
THREAD
')

define(ADDA,`
LONG x;
L_$2($3,x);
AREG($4) += x;
THREAD
')

define(ADDX_DREG,`
$1 s,x,r,t;
int n=$3;
t=GET_X_BIT();
sr &= ~MASK_CC | MASK_CC_Z; /* FIXME? */
s=LDREG_$1($2);
x=LDREG_$1(n);
r=x+s+t;
SET_VCX_ADD;
if (r) SET_NZ(r);
else UNSET_N();
SDREG_$1(n,r);
THREAD
')

define(ADDX_PD,`
$1 s,x,r,t;
int n=$3;
t=GET_X_BIT();
sr &= ~MASK_CC | MASK_CC_Z;
ifelse($1,`BYTE',`
L_AN_PD_BYTE_All($2,s);
LS_AN_PD_BYTE_All(n,x);',`
L_AN_PD_$1($2,s);
LS_AN_PD_$1(n,x);')
r=x+s+t;
SET_VCX_ADD;
UNSET_Z();
SET_NZ(r);
x=r;
US_AN_PD_$1(x);
THREAD
')

define(DO_ASR,`
sr &= ~MASK_CC|MASK_CC_X;
if ($1>0)
{
	sr &= ~MASK_CC_X;
	if ((x>>($1-1))&1) sr |= (MASK_CC_X|MASK_CC_C);
	x>>=$1;
}
SET_NZ((LONG)x);
')

define(DO_ASL,`
sr &= ~MASK_CC|MASK_CC_X;
{
	if ($1>0)
	{
		sr &= ~MASK_CC_X;
#if 0
		if ((x << ($1-1))<0) sr |= (MASK_CC_X|MASK_CC_C); /* bug ? */
#else
		if ((x>>(8*SIZE$2-$1))&1) sr |= (MASK_CC_C|MASK_CC_X);
#endif
		if ((x&(x+(1<<($1-1))))&~((1<<(8*SIZE$2-1-$1))-1)) sr |= MASK_CC_V;
		x <<= $1;
	}
}
SET_NZ((LONG)x);
')

define(ASR_IMM,`
int c=$2,n=$3;
$1 x;
x=LDREG_$1(n);
DO_ASR(c);
SDREG_$1(n,x);
THREAD
')

define(ASR_DREG,`
int c,n;
$1 x;
c=DREG($2)&63;
n=$3;
x=LDREG_$1(n);
#if SHWRAP32
if (c>=32)
{
	sr &= ~(MASK_CC|MASK_CC_X);
	if (x<0)
	{
		sr |= (MASK_CC_X|MASK_CC_C);
		x=-1;
		SET_NZ(-1);
	}
	else
	{
		
		sr &= ~MASK_CC;
		SET_NZ(0);
		x=0;
	}
}
else
#endif
DO_ASR(c);
SDREG_$1(n,x);
THREAD
')

define(ASR_MEM,`
WORD x;
LS_$1($2,x);
DO_ASR(1);
US_$1(x);
THREAD
')


define(ASL_IMM,`
int c=$2,n=$3;
$1 x;
x=LDREG_$1(n);
DO_ASL(c,$1);
SDREG_$1(n,x);
THREAD
')

define(ASL_DREG,`
int c,n;
$1 x;
c=DREG($2)&63;
n=$3;
x=LDREG_$1(n);
#if SHWRAP32
if (c>=32)
{
	sr &= ~(MASK_CC|MASK_CC_X);
	if (x&1) sr |= (MASK_CC_X|MASK_CC_C);
	if (x) sr |= MASK_CC_V;
	SET_NZ(0);
	x=0;
}
else
#endif
{ DO_ASL(c,$1); }
SDREG_$1(n,x);
THREAD
')

define(ASL_MEM,`
WORD x;
LS_$1($2,x);
DO_ASL(1,WORD);
US_$1(x);
THREAD
')

define(DO_LSR,`
sr &= ~MASK_CC|MASK_CC_X;
if ($1>0)
{
	sr &= ~MASK_CC_X;
	if ((x >> ($1-1)) & 1) sr |= (MASK_CC_X|MASK_CC_C);
	x>>=$1;
}
SET_NZ((LONG)x);
THREAD
')

define(DO_LSL,`
{ $2 y;
sr &= ~MASK_CC|MASK_CC_X;
if ($1>0)
{
	sr &= ~(MASK_CC_X);
#if 0
	if ((x << ($1-1))<0) sr |= (MASK_CC_C|MASK_CC_X);
#else
	if ((x>>(8*SIZE$2-$1))&1) sr |= (MASK_CC_C|MASK_CC_X);
#endif
	x <<= $1;
}
y=($2)x;
SET_NZ(y);
}
')

define(LSR_IMM,`
int c=$2,n=$3;
U$1 x;
x=LDREG_U$1(n);
DO_LSR(c);
SDREG_U$1(n,x);
THREAD
')

define(LSR_DREG,`
int c,n;
U$1 x;
c=DREG($2)&63;
n=$3;
x=LDREG_U$1(n);
#if SHWRAP32
if (c>=32)
{
	sr &= ~MASK_CC;
	SET_NZ(0);
	x=0;
}
else
#endif
DO_LSR(c);
SDREG_U$1(n,x);
THREAD
')

define(LSR_MEM,`
UWORD x;
LS_$1($2,x);
DO_LSR(1);
US_$1(x);
THREAD
')

define(LSL_IMM,`
int c=$2,n=$3;
$1 x;
x=LDREG_$1(n);
DO_LSL(c,$1);
SDREG_$1(n,x);
THREAD
')

define(LSL_DREG,`
int c,n;
$1 x;
c=DREG($2)&63;
n=$3;
x=LDREG_$1(n);
#if SHWRAP32
if (c>=32)
{
	sr &= ~(MASK_CC|MASK_CC_X);
	if (x&1) sr |= (MASK_CC_C|MASK_CC_X);
	SET_NZ(0);
	x=0;
}
else
#endif
{ DO_LSL(c,$1); }
SDREG_$1(n,x);
THREAD
')

define(LSL_MEM,`
WORD x;
LS_$1($2,x);
DO_LSL(1,WORD);
US_$1(x);
THREAD
')

define(DO_ROR,`
{ $2 y;
sr &= ~MASK_CC|MASK_CC_X;
if ($1>0)
{
	int t=x<<((8*SIZE$2)-$1);
	if ((x >> ($1-1)) & 1) sr |= MASK_CC_C;
	x>>=$1;
	x|=t;
}
y=($2)x;
SET_NZ(y); /* FIXME */
}
')

define(DO_ROL,`
{ $2 y;
sr &= ~MASK_CC|MASK_CC_X;
if ($1>0)
{
	unsigned long t;
	t = x&MASK$2;
	t >>= (8*SIZE$2-$1);
	if (t&1) sr |= MASK_CC_C;
	x <<= $1;
	x |= t;
}
y=($2)x;
SET_NZ(y);
}
')

define(ROR_IMM,`
int c=$2,n=$3;
U$1 x;
x=LDREG_U$1(n);
DO_ROR(c,$1);
SDREG_U$1(n,x);
THREAD
')

define(ROR_DREG,`
int c,n;
U$1 x;
c=DREG($2)&(8*SIZE$1-1);
n=$3;
x=LDREG_U$1(n);
DO_ROR(c,$1);
SDREG_U$1(n,x);
THREAD
')

define(ROR_MEM,`
UWORD x;
LS_$1($2,x);
x&=0xffff;
DO_ROR(1,WORD);
US_$1(x);
THREAD
')

define(ROL_IMM,`
int c=$2,n=$3;
$1 x;
x=LDREG_$1(n);
DO_ROL(c,$1);
SDREG_$1(n,x);
THREAD
')

define(ROL_DREG,`
int c,n;
$1 x;
c=DREG($2)&(8*SIZE$1-1);
n=$3;
x=LDREG_$1(n);
DO_ROL(c,$1);
SDREG_$1(n,x);
THREAD
')

define(ROL_MEM,`
WORD x;
LS_$1($2,x);
DO_ROL(1,WORD);
US_$1(x);
THREAD
')

define(DO_ROXR,`
{
$2 y;
int t;
sr &= ~MASK_CC|MASK_CC_X;
t=GET_X_BIT();
if ($1>0)
{
	unsigned int bot,tt;
#if 0
	tt = ((x<<1)|t)<<(8*SIZE$2-$1);
	x >>= $1-1;
	t = x&1;
	sr &= ~MASK_CC_X;
	sr |= t << 4;
	x >>= 1;
	x |= tt;
#else
    tt = (x >> ($1-1)) & 1;
	bot = x << (8*SIZE$2-$1);   /* bits that are rotated out */
	sr &= ~MASK_CC_X;
	x >>= $1;
	x |= t << (8*SIZE$2-$1);
	x |= bot << 1;
	t=tt;
	sr |= t << 4;
#endif
}
sr |= t; /* carry */
y=($2)x;
SET_NZ(y);
}
')

define(DO_ROXL,`
{
$2 y;
int t,tt;
sr &= ~MASK_CC|MASK_CC_X;
t=GET_X_BIT();
if ($1>0)
{
	unsigned int top;
	top = x>>(8*SIZE$2-$1);	/* bits that are rotated out */
	sr &= ~MASK_CC_X;
	x <<= $1;
	x |= t << ($1-1);
	x |= top >> 1;
	t = top & 1;
	sr |= t << 4;

}
sr |= t;
y=($2)x;
SET_NZ(y);
}
')

define(ROXR_IMM,`
int c=$2,n=$3;
U$1 x;
x=LDREG_U$1(n);
DO_ROXR(c,$1);
SDREG_U$1(n,x);
THREAD
')

define(ROXR_DREG,`
int c,n;
U$1 x;
c=DREG($2)&63;
n=$3;
x=LDREG_U$1(n);
DO_ROXR(c,$1);
SDREG_U$1(n,x);
THREAD
')

define(ROXR_MEM,`
UWORD x;
LS_$1($2,x);
DO_ROXR(1,WORD);
US_$1(x);
THREAD
')

define(ROXL_IMM,`
int c=$2,n=$3;
U$1 x;
x=LDREG_U$1(n);
DO_ROXL(c,$1);
SDREG_U$1(n,x);
THREAD
')

define(ROXL_DREG,`
int c,n;
U$1 x;
c=DREG($2)&63;
n=$3;
x=LDREG_U$1(n);
DO_ROXL(c,$1);
SDREG_U$1(n,x);
THREAD
')

define(ROXL_MEM,`
UWORD x;
LS_$1($2,x);
DO_ROXL(1,WORD);
US_$1(x);
THREAD
')

define(LINEF,`
pc -= 2;
ex_linef();
THREAD
')

define(Illegal,`ILLEGAL();')

