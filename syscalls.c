/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#include "defs.h"
#include "mem.h"
#include "syscalls.h"
#include <stdio.h>

typedef struct 
{
	char *name;
	char *fmt;
	void (*morefunc)(void);
	char *desc[10];
} syscall_desc;

#if TRACE_SYSCALLS
void print_mpb(void)
{
}

static syscall_desc bios_calls[]=
{
	{"Getmpb","a",print_mpb,{"MPB",}},
	{"Bconstat","w",NULL,{"dev",}},
	{"Bconin","w",NULL,{"dev",}},
	{"Bconout","ww",NULL,{"dev","c"}},
	{"Rwabs","wawwwl",NULL,{"rwflag","buf","count","recno","dev","lrecno"}},
	{"Setexc","wa",NULL,{"vecnum","vec"}},
	{"Tickcal","",NULL,{NULL}},
	{"Getbpb","w",NULL,{"dev"}},
	{"Bcostat","w",NULL,{"dev"}},
	{"Mediach","w",NULL,{"dev"}},
	{"Drvmap","",NULL,{NULL}},
	{"Kbshift","w",NULL,{"mode"}},
};

static syscall_desc gemdos_calls[]=
{
};


static void parse_call(syscall_desc *s)
{
	UL off=2;
	char *x;
	int i;
	for (x=s->fmt, i=0; *x; i++)
	{
		fprintf(stderr,"%s=",s->desc[i]);
		switch(*x)
		{
		case 'a': fprintf(stderr,"0x%06lx",(long)LM_UL(MEM(SP+off))); off+=4; break;
		case 'b': fprintf(stderr,"%d",LM_B(MEM(SP+off))); off+=2; break;
		case 'w': fprintf(stderr,"%d",LM_W(MEM(SP+off))); off+=2; break;
		case 'l': fprintf(stderr,"%ld",(long)LM_L(MEM(SP+off))); off+=4; break;
		default: fprintf(stderr,"(invalid format)");
		}
		if (*++x) fprintf(stderr,", ");
	}
	fprintf(stderr,")\n");
}

static void sys_bios(void)
{
	int i;
	UL off=2;
	char *x;
	int n=LM_W(MEM(SP));
	if (n<0 || n>=(sizeof(bios_calls)/sizeof(bios_calls[0])))
	{
		fprintf(stderr,"Invalid BIOS call (%d)\n",n);
		return;
	}
	fprintf(stderr,"BIOS #%d: %s(",n,bios_calls[n].name);
	parse_call(&bios_calls[n]);
}

static void sys_gemdos(void)
{
	int i;
	UL off=2;
	char *x;
	int n=LM_W(MEM(SP));
	if (n<0 || n>=(sizeof(gemdos_calls)/sizeof(gemdos_calls[0])))
	{
		fprintf(stderr,"Invalid GEMDOS call (%d)\n",n);
		return;
	}
	fprintf(stderr,"GEMDOS #%d: %s(",n,gemdos_calls[n].name);
	parse_call(&gemdos_calls[n]);
}

void syscall(int nex)
{
	switch(nex)
	{
		case 45: sys_bios(); break;
		case 33: sys_gemdos(); break;
	}
}

#endif
