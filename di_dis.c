/*
** dis.c
**
** dis takes a pointer to a memory address returns text string
** that represents assembler statement for that line, and returns
** pointer to start of next instruction
**
** Written by Jim Patchell (with minor mods for STonX by Alex)
** Mods:
**    1. Changed data types (code originally ran on 68k itself)
**    2. Use STonX memory access routines (LM_UW etc..), which also solved
**       the horrid byte ordering problem I was seeing with my buffer copy hack!
**
*/

#include <stdio.h>
#include <string.h>

/* From STonX headers */
#include "gendefs.h"

typedef struct {
        char *op;                                               /* pointer to assembly nemonic	    */
	UWORD op_code;   	                                /* base op code	                    */
	UWORD mask;		                                /* mask to get rid of variable data */
	UWORD *(*d_funct)(UWORD *c,char *s,WORD index);	        /* function to call	            */
}OP_DEF;

/*
** size tokens
*/

#define BYTE_SIZE	256
#define WORD_SIZE	257
#define LONG_SIZE	258

#define TRUE	1
#define FALSE	0

/*
** tables for disasembler code
*/
#ifdef MEGAMAX
extern Never();
#endif

static UWORD *not_known(UWORD *c,char *s,WORD index);
static UWORD *type_1(UWORD *c,char *s,WORD index);
static UWORD *type_2(UWORD *c,char *s,WORD index);
static UWORD *type_3(UWORD *c,char *s,WORD index);      /*      SUB,SUBA and others... */
static UWORD *type_4(UWORD *c,char *s,WORD index);
static UWORD *type_5(UWORD *c,char *s,WORD index);
static UWORD *type_6(UWORD *c,char *s,WORD index);
static UWORD *type_7(UWORD *c,char *s,WORD index);
static UWORD *type_8(UWORD *c,char *s,WORD index);	/*	branch type instructions	*/
static UWORD *type_9(UWORD *c,char *s,WORD index);	/*	unlink	*/
static UWORD *type_10(UWORD *c,char *s,WORD index);	/*	rts	*/
static UWORD *type_11(UWORD *c,char *s,WORD index);	/*	LSR	*/
static UWORD *type_12(UWORD *c,char *s,WORD index);	/*	EXT	*/
static UWORD *type_13(UWORD *c,char *s,WORD index);	/*	CMPI	*/
static UWORD *type_14(UWORD *c,char *s,WORD index);	/*	ABCD	*/
static UWORD *type_15(UWORD *c,char *s,WORD index);	/*	CLR     */
static UWORD *type_16(UWORD *c,char *s,WORD index);	/*	chk	*/
static UWORD *type_17(UWORD *c,char *s,WORD index);	/*	DBcc	*/
static UWORD *type_18(UWORD *c,char *s,WORD index);	/*	EXG	*/
static UWORD *type_19(UWORD *c,char *s,WORD index);	/*	CMPM	*/
static UWORD *type_20(UWORD *c,char *s,WORD index);	/*	BTST #	*/
static UWORD *type_21(UWORD *c,char *s,WORD index);	/*	BTST	*/
static UWORD *type_22(UWORD *c,char *s,WORD index);	/*	moveq	*/
static UWORD *type_23(UWORD *c,char *s,WORD index);	/*	movem	*/
static UWORD *type_24(UWORD *c,char *s,WORD index);	/*	TRAP	*/
static UWORD *type_25(UWORD *c,char *s,WORD index);	/*	STOP	*/
static UWORD *type_26(UWORD *c,char *s,WORD index);	/*	MOVE SR	*/
static UWORD *type_27(UWORD *c,char *s,WORD index);	/*	ANDI SR	*/
static UWORD *type_28(UWORD *c,char *s,WORD index);	/*	move USP*/
static UWORD *type_29(UWORD *c,char *s,WORD index);	/*	MOVEP	*/
static UWORD *effective_address(UWORD *a,char *s,UWORD ea,WORD index,WORD op_mode);
static WORD xlate_size(WORD *size,WORD type);

static const OP_DEF table[] = {
	"ABCD",0xc100,0xf1f0,type_14,
	"ADD",0xd000,0xf000,type_3,		/*	add, adda	*/

	"RESET",0x4e70,0xffff,type_10,
	"RTE",0x4e73,0xffff,type_10,
	"RTR",0x4e77,0xffff,type_10,
	"RTS",0x4e75,0xffff,type_10,
	"STOP",0x4e72,0xffff,type_25,
	"TRAPV",0x4e76,0xffff,type_10,
	"ILLEGAL",0x4afc,0xffff,type_10,
	"NOP",0x4e71,0xffff,type_10,

	"SWAP",0x4840,0xfff8,type_5,
	"UNLK",0x4e58,0xfff8,type_9,
	"LINK",0x4e50,0xfff8,type_1,

	"TRAP",0x4e40,0xfff0,type_24,
	"%6lx   MOVE   %s,USP",0x4e60,0xfff8,type_28,	/*	move USP	*/
	"%6lx   MOVE   USP,%s",0x4e68,0xfff8,type_28,	/*	move USP	*/

	"TAS",0x4ac0,0xffc0,type_5,
	"JMP",0x4ec0,0xffc0,type_5,
	"JSR",0x4e80,0xffc0,type_5,
	"%6lx   MOVE   CCR,%s",0x42c0,0xffc0,type_26,	/*	move from ccr	*/
	"%6lx   MOVE   %s,CCR",0x44c0,0xffc0,type_26,	/*	move to ccr	*/
	"%6lx   MOVE   SR,%s",0x40c0,0xffc0,type_26,	/*	move from SR	*/
	"%6lx   MOVE   %s,SR",0x46c0,0xffc0,type_26,	/*	move to SR	*/
	"NBCD",0x4800,0xffc0,type_15,
	"PEA",0x4840,0xffc0,type_5,

	"EXT",0x4800,0xfe30,type_12,

	"MOVEM",0x4880,0xfb80,type_23,

	"CHK",0x4180,0xf1c0,type_16,
	"LEA",0x41c0,0xf1c0,type_2,

	"TST",0x4a00,0xff00,type_7,
	"CLR",0x4200,0xff00,type_15,
	"NEG",0x4400,0xff00,type_15,
	"NEGX",0x4000,0xff00,type_15,
	"NOT",0x4600,0xff00,type_15,


	"DBCC",0x54c8,0xfff8,type_17,
	"DBCS",0x55c8,0xfff8,type_17,
	"DBEQ",0x57c8,0xfff8,type_17,
	"DBF",0x51c8,0xfff8,type_17,
	"DBGE",0x5cc8,0xfff8,type_17,
	"DBGT",0x5ec8,0xfff8,type_17,
	"DBHI",0x52c8,0xfff8,type_17,
	"DBLE",0x5fc8,0xfff8,type_17,
	"DBLS",0x53c8,0xfff8,type_17,
	"DBLT",0x5dc8,0xfff8,type_17,
	"DBMI",0x5bc8,0xfff8,type_17,
	"DBNE",0x56c8,0xfff8,type_17,
	"DBPL",0x5ac8,0xfff8,type_17,
	"DBT",0x50c8,0xfff8,type_17,
	"DBVC",0x58c8,0xfff8,type_17,
	"DBVS",0x59c8,0xfff8,type_17,
	"SCC",0x54c0,0xffc0,type_5,
	"SCS",0x55c0,0xffc0,type_5,
	"SEQ",0x57c0,0xffc0,type_5,
	"SF",0x51c0,0xffc0,type_5,
	"SGE",0x5cc0,0xffc0,type_5,
	"SGT",0x5ec0,0xffc0,type_5,
	"SHI",0x52c0,0xffc0,type_5,
	"SLE",0x5fc0,0xffc0,type_5,
	"SLS",0x53c0,0xffc0,type_5,
	"SLT",0x5dc0,0xffc0,type_5,
	"SMI",0x5bc0,0xffc0,type_5,
	"SNE",0x56c0,0xffc0,type_5,
	"SPL",0x5ac0,0xffc0,type_5,
	"ST",0x50c0,0xffc0,type_5,
	"SVC",0x58c0,0xffc0,type_5,
	"SVS",0x59c0,0xffc0,type_5,
	"ADDQ",0x5000,0xf100,type_6,
	"ADDX",0xd000,0xf130,type_14,
	"EXG",0xc100,0xf130,type_18,
	"MULS",0xc1c0,0xf1c0,type_16,
	"MULU",0xc0c0,0xf1c0,type_16,
	"AND",0xc000,0xf000,type_3,
	"ASL",0xe100,0xf118,type_11,
	"ASR",0xe000,0xf118,type_11,
	"BCC",0x6400,0xff00,type_8,
	"BCS",0x6500,0xff00,type_8,
	"BEQ",0x6700,0xff00,type_8,
	"BGE",0x6c00,0xff00,type_8,
	"BGT",0x6e00,0xff00,type_8,
	"BHI",0x6200,0xff00,type_8,
	"BLE",0x6f00,0xff00,type_8,
	"BLS",0x6300,0xff00,type_8,
	"BLT",0x6d00,0xff00,type_8,
	"BMI",0x6b00,0xff00,type_8,
	"BNE",0x6600,0xff00,type_8,
	"BPL",0x6a00,0xff00,type_8,
	"BVC",0x6800,0xff00,type_8,
	"BVS",0x6900,0xff00,type_8,
	"BRA",0x6000,0xff00,type_8,
	"BSR",0x6100,0xff00,type_8,
	"CMPM",0xb108,0xf138,type_19,
	"CMP",0xb000,0xf000,type_3,	/*	cmp,cmpa	*/
	"DIVS",0x81c0,0xf1c0,type_16,
	"DIVU",0x80c0,0xf1c0,type_16,
	"EOR",0xd000,0xf000,type_3,
	"LSR",0xe008,0xf118,type_11,
	"LSR",0xe2c0,0xffc0,type_11,
	"LSL",0xe108,0xf118,type_11,
	"LSL",0xe3c0,0xffc0,type_11,
	"MOVE.B",0x1000,0xf000,type_4,
	"MOVE.W",0x3000,0xf000,type_4,
	"MOVE.L",0x2000,0xf000,type_4,
	"MOVEQ",0x7000,0xf100,type_22,
	"SBCD",0x8100,0xf1f0,type_14,
	"OR",0x8000,0xf000,type_3,
	"%6lx   ORI   #$%x,CCR",0x003c,0xffff,type_27,	/*	ORI #<data>,CCR	*/
	"%6lx   ORI   #$%x,SR",0x007c,0xffff,type_27,	/*	ORI #data,SR		*/
	"%6lx   EORI   #$%x,CCR",0x0a3c,0xffff,type_27,
	"%6lx   EORI   #$%x,SR",0x0a7c,0xffff,type_27,
	"%6lx   ANDI   #$%x,CCR",0x023c,0xffff,type_27,	/*	ANDI #<data>,CCR	*/
	"%6lx   ANDI   #$%x,SR",0x027c,0xffff,type_27,	/*	ANDI #data,SR		*/
	"%6lx   MOVEP.W  %x(%s),%s",0x0108,0xf1f8,type_29,
	"%6lx   MOVEP.W   %s,%x(%s)",0x0188,0xf1f8,type_29,
	"%6lx   MOVEP.L  %x(%s),%s",0x0148,0xf1f8,type_29,
	"%6lx   MOVEP.L   %s,%x(%s)",0x01c8,0xf1f8,type_29,
	"BTST",0x0800,0xffc0,type_20,
	"BSET",0x08c0,0xffc0,type_20,
	"BCHG",0x0840,0xffc0,type_20,
	"BCLR",0x0880,0xffc0,type_20,
	"SUBI",0x0400,0xff00,type_13,
	"EORI",0x0a00,0xff00,type_13,	/*	eori	*/
	"CMPI",0x0c00,0xff00,type_13,
	"ANDI",0x0200,0xff00,type_13,
	"ADDI",0x0600,0xff00,type_13,
	"ORI",0x0000,0xff00,type_13,	/*	ORI	*/

	"BTST",0x0100,0xf1c0,type_21,
	"BSET",0x01c0,0xf1c0,type_21,
	"BCHG",0x0140,0xf1c0,type_21,
	"BCLR",0x0180,0xf1c0,type_21,

	"ROL",0xe118,0xf118,type_11,
	"ROR",0xe018,0xf118,type_11,
	"ROL",0xe7c0,0xffc0,type_11,
	"ROR",0xe6c0,0xffc0,type_11,
	"ROXL",0xe110,0xf118,type_11,
	"ROXR",0xe010,0xf118,type_11,
	"ROXL",0xe5c0,0xffc0,type_11,
	"ROXR",0xe4c0,0xffc0,type_11,
	"SUBQ",0x5100,0xf100,type_6,
	"SUBA",0x90c0,0xf0c0,type_3,   /* SUBA can look like a SUBX! */
	"SUBX",0x9100,0xf130,type_14,
	"SUB",0x9000,0xf000,type_3,
	NULL,0,0,not_known
};

static const char *adr_regs[] = {
	"A0","A1","A2","A3","A4","A5","A6","A7"};

static const char *data_regs[] = {
	"D0","D1","D2","D3","D4","D5","D6","D7"};

static const char *sizes[] = {".B",".W",".L"};

void *dis(void *c,char *s)
{
	/*
	** This function looks a the word pointed to by "c" and does
	** a one instruction disasembly based on this pointer
	** The function then returns a pointer to the start of
	** the next instruction
	*/
	WORD i;
	UWORD *code;
	WORD loop;
	WORD table_entry;
	UWORD instruction_word;

	code = (UWORD *) c;
	instruction_word = LM_UW(MEM(code));

	for(i=0,loop = TRUE;(table[i].mask != 0) && loop;++i)
	{
		/*	find the first opcode	*/
	  if((instruction_word & table[i].mask) == table[i].op_code)
		{
			table_entry = i;
			loop = FALSE;	/*	stop looping	*/
		}
	}
	if(loop)
	  { /* no instruction found - assume data */
	    sprintf(s,"%6lx   DC.W   #$%x",c,instruction_word);
	    return (c+2);
	}
        else
        {
		return((void *)(*table[table_entry].d_funct)(c,s,table_entry));
        }
}

static UWORD *type_29(UWORD *c,char *s,WORD index)
{
	/*	MOVEP	move peripheral data	*/
	ULONG adr;
	WORD data_reg,adr_reg;
	WORD displacement;
	UWORD direction;

	adr = LM_UL(MEM(c));
	data_reg = (LM_UW(MEM(c)) & 0x0e00) >> 9;
	adr_reg = LM_UW(MEM(c)) & 0x07;
	direction = LM_UW(MEM(c)) & 0x080;
	++c;
	displacement = LM_UW(MEM(c));
	if(direction)
		sprintf(s,table[index].op,adr,data_regs[data_reg],displacement,adr_regs[adr_reg]);
	else
		sprintf(s,table[index].op,adr,displacement,adr_regs[adr_reg],data_regs[data_reg]);
	return(++c);
}

static UWORD *type_28(UWORD *c,char *s,WORD index)
{
	sprintf(s,table[index].op,(ULONG)c,adr_regs[LM_UW(MEM(c)) & 0x7]);
	return(++c);
}

static UWORD *type_27(UWORD *c,char *s,WORD index)
{
	/*	ANDI SR	*/
	ULONG adr;
/*	WORD data;	*/

	adr = LM_UL(MEM(c));
        c++;
	sprintf(s,table[index].op,adr,LM_UW(MEM(c)));
	return(++c);
}

static UWORD *type_26(UWORD *c,char *s,WORD index)
{
	/*	move sr	*/
	long adr;
	WORD source;
	char e_a[30];

	adr = c;
	source = LM_UW(MEM(c)) & 0x3f;
	c = effective_address(c,e_a,source,index,WORD_SIZE);
	sprintf(s,table[index].op,adr,e_a);
	return(++c);
}

static UWORD *type_25(UWORD *c,char *s,WORD index)
{
	/*	STOP	*/
	long adr;

	adr = LM_UL(MEM(c));
	c++;
	sprintf(s,"%6lx   %s   #$%x",adr,table[index].op,LM_UW(MEM(c)));
	return(++c);
}

static UWORD *type_24(UWORD *c,char *s,WORD index)
{
	/*	trap	*/
	WORD vector;

	sprintf(s,"%6lx   %s   #$%x",(long)c,table[index].op,LM_UW(MEM(c)) & 0x0f);
	return(++c);
}
	
static UWORD *type_23(UWORD *c,char *s,WORD index)
{
	/*	movem	*/
	long adr;
	UWORD reg_mask;
	WORD direction,size;
	WORD source;
	char e_a[30];
	char regs[30];
	char t[8];
	UWORD mask;
	WORD i;
	WORD mode;
	WORD flag,first;
	char *f,*d;

	adr = (long)c;
	source = LM_UW(MEM(c)) & 0x3f;
	size = (LM_UW(MEM(c)) & 0x040) >> 6;
	direction = (LM_UW(MEM(c)) & 0x0400) >> 10;
	++c;
	reg_mask = LM_UW(MEM(c));
	c = effective_address(c,e_a,source,index,0);
	/*
	process register mask
	*/
	mode = (source & 0x38) >> 3;
	flag = FALSE;
	first = TRUE;
	regs[0] = '\0';
	if(mode == 4)	/*	predecrement	*/
		mask = 0x080;
	else
		mask = 0x0100;
	for(i=0;i<8;++i)
	{
		if(mask & reg_mask)	/*	is this bit set?	*/
		{
			if(first)
			{
				sprintf(regs,"%s",adr_regs[i]);
				first = FALSE;
				flag++;
			}
			else if (flag == 0)
			{
				sprintf(t,"/%s",adr_regs[i]);
				strcat(regs,t);
				++flag;
			}
			else if (flag)
			{
				++flag;
			}
		}
		else if (flag)
		{
			if(flag > 2)
			{
				sprintf(t,"-%s",adr_regs[i-1]);
				strcat(regs,t);
			}
			else if (flag == 2)
			{
				sprintf(t,"/%s",adr_regs[i-1]);
				strcat(regs,t);
			}
			flag = 0;
		}
		if(mode == 4)
			mask >>= 1;
		else
			mask <<= 1;
	}
	if(flag > 1)
	{
		sprintf(t,"-%s",adr_regs[i-1]);
		strcat(regs,t);
	}
	flag = 0;
	if(mode == 4)	/*	predecrement	*/
		mask = 0x8000;
	else
		mask = 0x01;
	for(i=0;i<8;++i)
	{
		if(mask & reg_mask)	/*	is this bit set?	*/
		{
			if(first)
			{
				sprintf(regs,"%s",data_regs[i]);
				first = FALSE;
				flag++;
			}
			else if (flag == 0)
			{
				sprintf(t,"/%s",data_regs[i]);
				strcat(regs,t);
				++flag;
			}
			else if (flag)
			{
				++flag;
			}
		}
		else if (flag)
		{
			if(flag > 2)
			{
				sprintf(t,"-%s",data_regs[i-1]);
				strcat(regs,t);
			}
			else if (flag == 2)
			{
				sprintf(t,"/%s",data_regs[i-1]);
				strcat(regs,t);
			}
			flag = 0;
		}
		if(mode == 4)
			mask >>= 1;
		else
			mask <<= 1;
	}
	if(flag > 1)
	{
		sprintf(t,"-%s",data_regs[i-1]);
		strcat(regs,t);
	}
	if(!direction)
	{
		f = regs;
		d = e_a;
	}
	else
	{
		f = e_a;
		d = regs;
	}
	if(size)
		size = 2;
	else
		size = 1;
	sprintf(s,"%6lx   %s%s  %s,%s",adr,table[index].op,sizes[size],f,d);
	++c;
	return(c);
}


static UWORD *type_22(UWORD *c,char *s,WORD index)
{
	/*	moveq	*/
	long adr;
	UWORD dest_reg;
	WORD data;

	adr = (long)c;
	data = LM_UW(MEM(c)) & 0xff;
	dest_reg = (LM_UW(MEM(c)) & 0x0e00) >> 9;
	data = (WORD)( (char)data);
	sprintf(s,"%6lx   %s  #$%x,%s",adr,table[index].op,data,data_regs[dest_reg]);
	++c;
	return(c);
}

static UWORD *type_21(UWORD *c,char *s,WORD index)
{
	long adr;
	UWORD dest_reg;
	UWORD source;
	char e_a[40];	/*	place to put effective address	*/

	adr = (long)c;
	dest_reg = (LM_UW(MEM(c)) & 0x0e00) >> 9;	/*	get destination reg	*/
	source = LM_UW(MEM(c)) & 0x3f;				/*	this is an effective address	*/
	c = effective_address(c,e_a,source,index,0);	/*	do effective address	*/
	sprintf(s,"%6lx   %s  %s,%s",adr,table[index].op,data_regs[dest_reg],e_a);
	++c;
	return(c);
}

static UWORD *type_20(UWORD *c,char *s,WORD index)
{
	/*	BTST # type instruction	*/
	long adr;
	WORD dest;
	UWORD il_data;	/*	imediate data	*/
	char e_a[40];

	adr = (long)c;
	dest = LM_UW(MEM(c)) & 0x3f;
	++c;	                                         /*	point to immeadieate data	*/
	il_data = LM_UW(MEM(c));
	c = effective_address(c,e_a,dest,index,0);
	sprintf(s,"%6lx   %s  #$%lx,%s",adr,table[index].op,il_data,e_a);
	++c;
	return(c);
}

static UWORD *type_19(UWORD *c,char *s,WORD index)
{
	/*	CMPM type instructions	*/
	long adr;
	WORD dest;
	WORD source;
	WORD size;

	adr = (long)c;
	source = LM_UW(MEM(c)) & 0x07;
	dest = (LM_UW(MEM(c)) & 0x0e00) >> 9;
	size = (LM_UW(MEM(c)) & 0x0c0) >> 6;
	sprintf(s,"%6lx   %s%s  (%s)+,(%s)+",adr,table[index].op,sizes[size],adr_regs[source],adr_regs[dest]);
	++c;
	return(c);
}

static UWORD *type_18(UWORD *c,char *s,WORD index)
{
	/*	EXG type instructions	*/
	long adr;
	WORD dest;
	WORD source;
	WORD op_mode;
	char *rx,*ry;

	adr = (long)c;
	dest = LM_UL(MEM(c)) & 0x07;
	source = (LM_UW(MEM(c)) & 0x0e00) >> 9;
	op_mode = (LM_UW(MEM(c)) & 0xf8) >> 3;
	if(op_mode == 0x08)
	{
		rx = data_regs[source];
		ry = data_regs[dest];
	}
	else if (op_mode == 0x09)
	{
		rx = adr_regs[source];
		ry = adr_regs[dest];
	}
	else if (op_mode == 0x11)
	{
		rx = data_regs[source];
		ry = adr_regs[dest];
	}
	sprintf(s,"%6lx   %s  %s,%s",adr,table[index].op,rx,ry);
	++c;
	return(c);
}

static UWORD *type_17(UWORD *c,char *s,WORD index)
{
	/*	DBcc type instructions	*/
	long adr;
	long dest;
	UWORD displacement;
	WORD reg;

	adr = (long)c;
	reg = LM_UW(MEM(c)) & 0x07;
	++c;
	displacement = LM_UW(MEM(c));
	dest = (long)c + (long)((WORD)displacement);
	sprintf(s,"%6lx   %s  %s,$%lx",adr,table[index].op,data_regs[reg],dest);
	++c;
	return(c);
}

static UWORD *type_16(UWORD *c,char *s,WORD index)
{
	/*	CHK type instruction	*/
	long adr;
	WORD dest;
	WORD reg;
	char e_a[40];

	adr = (long)c;
	dest = LM_UW(MEM(c)) & 0x3f;
	reg = (LM_UW(MEM(c)) & 0x0e00) >> 9;
	c = effective_address(c,e_a,dest,index,WORD_SIZE);
	sprintf(s,"%6lx   %s  %s,%s",adr,table[index].op,e_a,data_regs[reg]);
	++c;
	return(c);
}

static UWORD *type_15(UWORD *c,char *s,WORD index)
{
	/*	CLR type instruction	*/
	long adr;
	WORD dest;
	WORD size;
	char e_a[40];

	adr = (long)c;
	dest = LM_UW(MEM(c)) & 0x3f;
	size = (LM_UW(MEM(c)) & 0x0c0) >> 6;
	c = effective_address(c,e_a,dest,index,0);
	sprintf(s,"%6lx   %s%s  %s",adr,table[index].op,sizes[size],e_a);
	++c;
	return(c);
}

static UWORD *type_14(UWORD *c,char *s,WORD index)
{
	/*	ABCD type instruction	*/
	long adr;
	WORD dest;
	WORD source;
	WORD dest_type;
	WORD size;

	adr = (long)c;
	dest = (LM_UW(MEM(c)) & 0x0e00) >> 9;
	source = LM_UW(MEM(c)) & 0x7;
	dest_type = (LM_UW(MEM(c)) & 0x08) >> 3;
	size = (LM_UW(MEM(c)) & 0xc0) >> 6;
	if (dest_type)
		sprintf(s,"%6lx   %s%s  -(%s),-(%s)",adr,table[index].op,sizes[size],adr_regs[source],adr_regs[dest]);
	else
		sprintf(s,"%6lx   %s%s  %s,%s",adr,table[index].op,sizes[size],data_regs[source],data_regs[dest]);
	++c;
	return(c);
}

static UWORD *type_13(UWORD *c,char *s,WORD index)
{
	/*	ADDI type instruction	*/
	long adr;
	WORD dest;
	WORD size;
	long il_data;	/*	imediate data	*/
	char e_a[40];

	adr = (long)c;
	dest = LM_UW(MEM(c)) & 0x3f;
	size = (LM_UW(MEM(c)) & 0x0c0) >> 6;
	++c;	/*	point to immeadieate data	*/
	if(size == 2)	/*	long operation	*/
	{
		++c;	/*	point to immeadieate data	*/
		il_data = LM_UL(MEM(c));
		++c;
	}
	else
	{
		il_data = LM_UL(MEM(c));
	}
	c = effective_address(c,e_a,dest,index,0);
	sprintf(s,"%6lx   %s%s  #$%lx,%s",adr,table[index].op,sizes[size],il_data,e_a);
	++c;
	return(c);
}


static UWORD *type_12(UWORD *c,char *s,WORD index)
{
	/*	EXT type instruction	*/
	long adr;
	WORD op_mode;
	WORD reg;

	adr = (long)c;
	op_mode = ((LM_UW(MEM(c)) & 0x1c0) >> 6) - 1;
	reg = LM_UW(MEM(c)) & 0x07;
	sprintf(s,"%6lx   %s%s  %s",adr,table[index].op,sizes[op_mode],data_regs[reg]);
	++c;
	return(c);
}

static UWORD *type_11(UWORD *c,char *s,WORD index)
{
	/*	LSR type instructions	*/
	long adr;
	WORD size;
	WORD dest;
	char e_a[40];
	WORD type;
	WORD count;

	adr = (long)c;
	size = (LM_UW(MEM(c)) & 0x0c0) >> 6;
	if(size < 3)	/*	register shifts	*/
	{
		type = (LM_UW(MEM(c)) & 0x100) >> 8;
		dest = (LM_UW(MEM(c)) & 0x07);
		count = (LM_UW(MEM(c)) & 0x0e00)>>9;
		if(type)
			sprintf(s,"%6lx   %s%s  %s,%s",adr,table[index].op,sizes[size],data_regs[count],data_regs[dest]);
		else
		{
			if(count  == 0)
				count = 8;
			sprintf(s,"%6lx   %s%s  #$%x,%s",adr,table[index].op,sizes[size],count,data_regs[dest]);
		}
	}
	else	/*	memory shifts	*/
	{
		dest = (LM_UW(MEM(c)) & 0x3f);
		c = effective_address(c,e_a,dest,index,0);
		sprintf(s,"%6lx   %s   %s",adr,table[index].op,e_a);
	}
	++c;
	return(c);
}

static UWORD *type_10(UWORD *c,char *s,WORD index)
{
	/*	RTS type instructions	*/
	long adr;

	adr = (long)c;
	sprintf(s,"%6lx   %s",adr,table[index].op);
	++c;
	return(c);
}

static UWORD *type_9(UWORD *c,char *s,WORD index)
{
	/*	UNLINK type instructions	*/
	long adr;
	WORD reg;

	adr = (long)c;
	reg = LM_UW(MEM(c)) & 0x07;
	sprintf(s,"%6lx   %s   %s",adr,table[index].op,adr_regs[reg]);
	++c;
	return(c);
}

static UWORD *type_8(UWORD *c,char *s,WORD index)
{
	/*	Branch type instructions	*/
	long adr;
	long dest;
	UWORD displacement;
	char *size;

	adr = (long)c;
	displacement = LM_UW(MEM(c)) & 0xff;
	if(displacement == 0)
	{
		++c;
		displacement = LM_UW(MEM(c));
		size = "";
	}
	else
	{
		size = ".S";
		displacement = (UWORD)( (WORD)( (char)displacement) );
	}
	dest = (long)c + (long)((WORD)displacement) + 2l;
	sprintf(s,"%6lx   %s%s  $%lx",adr,table[index].op,size,dest);
	++c;
	return(c);
}

static UWORD *type_7(UWORD *c,char *s,WORD index)
{
	/*	TST type instructions	*/

	long adr;
	WORD dest;
	char e_a[40];
	WORD size;
	WORD op_mode;

	adr = (long)c;
	dest = LM_UW(MEM(c)) & 0x3f;
	size = (LM_UW(MEM(c)) & 0x0c0) >> 6;
	op_mode = xlate_size(&size,0);
	c = effective_address(c,e_a,dest,index,0);
	sprintf(s,"%6lx   %s%s   %s",adr,table[index].op,sizes[size],e_a);
	++c;
	return(c);
}

static UWORD *type_6(UWORD *c,char *s,WORD index)
{
	/*	ADDQ type instructions	*/

	long adr;
	WORD dest;
	char e_a[40];
	WORD data;
	WORD size;
	WORD op_mode;

	adr = (long)c;
	dest = LM_UW(MEM(c)) & 0x3f;
	size = (LM_UW(MEM(c)) & 0x0c0) >> 6;
	op_mode = xlate_size(&size,0);	/*	normal translate	*/
	data = (LM_UW(MEM(c)) & 0x0e00) >> 9;
	if (data == 0)
		data = 8;
	c = effective_address(c,e_a,dest,index,0);	/*	do effective address	*/
	++c;
	sprintf(s,"%6lx   %s%s  #$%x,%s",adr,table[index].op,sizes[size],data,e_a);
	return(c);
}

static UWORD *type_5(UWORD *c,char *s,WORD index)
{
	/*	PEA type instructions	*/

	long adr;
	char e_a[40];
	WORD source;

	adr = (long)c;
	source = LM_UW(MEM(c)) & 0x3f;
	c = effective_address(c,e_a,source,index,0);	/*	do effective address	*/
	sprintf(s,"%6lx   %s   %s",adr,table[index].op,e_a);
	++c;
	return(c);
}

static UWORD *type_4(UWORD *c,char *s,WORD index)
{
	/*	This is for move instructions	*/
	long adr;
	char e_a1[40],e_a2[40];
	WORD size;
	WORD source,dest;
	WORD op_mode;

	adr = (long)c;
	size = (LM_UW(MEM(c)) & 0x3000) >> 12;
	op_mode = xlate_size(&size,1);
	source = LM_UW(MEM(c)) & 0x3f;
	dest = (LM_UW(MEM(c)) & 0x0fc0) >> 6;
	/*	on dest, mode and reg are in different order, so swap them	*/
	dest = ((dest & 0x07) << 3) | (dest >> 3);
	c = effective_address(c,e_a1,source,index,op_mode);	/*	do effective address	*/
	c = effective_address(c,e_a2,dest,index,0);	/*	do effective address	*/
	sprintf(s,"%6lx   %s  %s,%s",adr,table[index].op,e_a1,e_a2);
	++c;
	return(c);
}

static UWORD *type_3(UWORD *c,char *s,WORD index)
{
	long adr;
	UWORD dest_reg;
	UWORD source;
	char e_a[40];	/*	place to put effective address	*/
	WORD op_mode;
	char **regs;
	WORD dir;
	WORD size;

	adr = (long)c;
	dest_reg = (LM_UW(MEM(c)) & 0x0e00) >> 9;	/*	get destination reg	*/
	source = LM_UW(MEM(c)) & 0x3f;			/*	this is an effective address	*/
	op_mode = (LM_UW(MEM(c)) & 0x01c0) >> 6;	/*	get op mode	*/
	if(op_mode == 3 || op_mode == 7)
	{
	  dir = 0; /*was 1*/
		regs = adr_regs;
		if(op_mode == 3)
		  size = 1; /*WORD*/
		else
		  size = 2; /*LONG*/
	}
	else
	{
		regs = data_regs;
		size = op_mode & 0x03;
		dir = (op_mode & 0x4) >> 2;		/*	direction	*/
		/*op_mode = xlate_size(&size,0); optimise*/
	}
	op_mode = xlate_size(&size,0);
	c = effective_address(c,e_a,source,index,op_mode);	/*	do effective address	*/
	if(!dir)
		sprintf(s,"%6lx   %s%s  %s,%s",adr,table[index].op,sizes[size],e_a,regs[dest_reg]);
	else
		sprintf(s,"%6lx   %s%s  %s,%s",adr,table[index].op,sizes[size],regs[dest_reg],e_a);
	++c;
	return(c);
}

static UWORD *type_2(UWORD *c,char *s,WORD index)
{
	long adr;
	UWORD dest_reg;
	UWORD source;
	char e_a[40];	/*	place to put effective address	*/

	adr = (long)c;
	dest_reg = (LM_UW(MEM(c)) & 0x0e00) >> 9;	/*	get destination reg	*/
	source = LM_UW(MEM(c)) & 0x3f;				/*	this is an effective address	*/
	c = effective_address(c,e_a,source,index,0);	/*	do effective address	*/
	sprintf(s,"%6lx   %s %s,%s",adr,table[index].op,e_a,adr_regs[dest_reg]);
	++c;	/*	next instruction	*/
	return(c);
}

static UWORD *type_1(UWORD *c,char *s,WORD index)
{
	WORD the_reg;
	long adr;

	adr = (long)c;
	the_reg = LM_UW(MEM(c)) & 0x07;	/*	get register number	*/
	++c;	                /*	point to word displacement	*/
	sprintf(s,"%6lx   %s  %s,#$%x",adr,table[index].op,adr_regs[the_reg],*c);
	++c;
	return(c);
}

static UWORD *not_known(UWORD *c,char *s,WORD index)
{
	sprintf(s,"%6lx Don't know how to deal with this yet %s",(long)c,table[index].op);
	return(NULL);
}

static WORD xlate_size(WORD *size,WORD type)
{
	if(type == 0)	/*	main type	*/
	{
		switch(*size)
		{
			case 0:
				return(BYTE_SIZE);
			case 1:
				return(WORD_SIZE);
			case 2:
				return(LONG_SIZE);
		}
	}
	else if(type == 1)	/*	for move op	*/
	{
		switch(*size)
		{
			case 1:
				*size = 0;
				return(BYTE_SIZE);
			case 2:
				*size = 2;
				return(LONG_SIZE);
			case 3:
				*size = 1;
				return(WORD_SIZE);
		}
	}
}

	
static UWORD *effective_address(UWORD *a,char *s,UWORD ea,WORD index,WORD op_mode)

/* UWORD *a;		address pointer	*/
/* WORD index;		index into opcode table	*/

{
	WORD mode;
	WORD reg;
	UWORD displacement;
	WORD short_adr;
	WORD index_reg;
	WORD index_reg_ind;
	WORD index_size;
	char *s1;
	long a1,a2;

	mode = (ea & 0x38) >> 3;
	reg = ea & 0x7;
	switch(mode)
	{
		case 0:
			sprintf(s,"%s",data_regs[reg]);
			break;
		case 1:
			sprintf(s,"%s",adr_regs[reg]);
			break;
		case 2:	/*	address indirect	*/
			sprintf(s,"(%s)",adr_regs[reg]);
			break;
		case 3:
			sprintf(s,"(%s)+",adr_regs[reg]);
			break;
		case 4:
			sprintf(s,"-(%s)",adr_regs[reg]);
			break;
		case 5:	/*	address with displacement	*/
			++a;	/*	next mem location	*/
			displacement = LM_UW(MEM(a));
			sprintf(s,"$%x(%s)",displacement,adr_regs[reg]);
			break;
		case 6:	/*	indexed address with displacement	*/
			++a;
			displacement = LM_UW(MEM(a)) & 0xff;
			index_reg = (LM_UW(MEM(a)) & 0x7000) >> 12;
			index_reg_ind = (LM_UW(MEM(a)) & 0x8000)>> 15;
			index_size = (LM_UW(MEM(a)) & 0x8000) >> 11;
			if(index_size)
			{
				s1 = ".L";
			}
			else
			{
				s1 = ".W";
			}
			if(index_reg_ind)
			{
				/*	address reg is index reg	*/
				sprintf(s,"$%x(%s,%s%s)",displacement,adr_regs[reg],adr_regs[index_reg],s1);
			}
			else
			{
				/*	data reg is index reg	*/
				sprintf(s,"$%x(%s,%s%s)",displacement,adr_regs[reg],data_regs[index_reg],s1);
			}
			break;
		case 7:
			switch(reg)	/*	other modes	*/
			{
				case 0:		/*	absolute short	*/
					++a;
					short_adr = LM_UW(MEM(a));
					sprintf(s,"$%lx.S",(long)short_adr);
					break;
				case 1:		/*	absolute long	*/
					++a;
					a1 = LM_UW(MEM(a));
					++a;
					a2 = LM_UW(MEM(a));
					a1 = (a1 << 16) + a2;
					sprintf(s,"$%lx.L",a1);
					break;
				case 2:	/*	program counter with displacement	*/
					++a;
					displacement = LM_UW(MEM(a));
					a1 = (long)a + (long)displacement;
					sprintf(s,"$lx(PC)",a1);
					break;
				case 3:
					++a;
					a1 = (long)((char)(LM_UW(MEM(a)) & 0xff));
					index_reg = (LM_UW(MEM(a)) & 0x7000) >> 12;
					index_reg_ind = (LM_UW(MEM(a)) & 0x8000)>> 15;
					index_size = (LM_UW(MEM(a)) & 0x8000) >> 11;
					a1 += (long)a;
					if(index_size)
					{
						s1 = ".L";
					}
					else
					{
						s1 = ".W";
					}
					if(index_reg_ind)
					{
						/*	address reg is index reg	*/
						sprintf(s,"$%lx(PC,%s%s)",a1,adr_regs[index_reg],s1);
					}
					else
					{
						/*	data reg is index reg	*/
						sprintf(s,"$%lx(PC,%s%s)",a1,data_regs[index_reg],s1);
					}
					break;
				case 4:
					/*
					** This one is tough
					*/
					if(op_mode == LONG_SIZE)
					{
						++a;
						a1 = LM_UW(MEM(a));
						++a;
						a2 = LM_UW(MEM(a));
						a1 = (a1 << 16) + a2;
						sprintf(s,"#$%lx.L",a1);
					}
					else if (op_mode == WORD_SIZE || op_mode == BYTE_SIZE)
					{
						++a;
						displacement = LM_UW(MEM(a));
						if(op_mode == WORD_SIZE)
							sprintf(s,"#$%x.W",displacement);
						else if (op_mode == BYTE_SIZE)
							sprintf(s,"#$%x.B",displacement);
					}
					break;
			}	/*	end of switch reg	*/
			break;
	}	/*	end of switch mode	*/
	return(a);
}

#ifdef MAIN
main()
{
	char code[80];
	void *p;

	p = (void *)Never;
	do {
		p = dis(p,code);
		prWORDf("%s\n",code);	/*	prWORD out code	*/
	}while(p != NULL);
}

#endif
