/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#include "defs.h"
#include "tosdefs.h"
#include "mem.h"
#if MONITOR
#include <stdio.h>
#include <ctype.h>
#include <curses.h>
#include "dis-asm.h"

#define WATCH 1
B *smem = 0;
W *spc	= 0;
extern wprintw(), printw();
extern WINDOW *stdscr;
static disassemble_info info;
static UL off = 0x10000;
static UL count = 0;
static UL bkpt = 0;
static int mode=0;
int in_monitor=0;

void printcc (int c, int t)
{
	if (t) attron (A_REVERSE);
	printw ("%c",c);
	if (t) attroff (A_REVERSE);
}

void init_monitor (void)
{
	initscr();
	cbreak();
	keypad(stdscr,TRUE);
	clear();
	refresh();
	INIT_DISASSEMBLE_INFO(info, stdscr);
	info.fprintf_func = (fprintf_ftype)wprintw;
	info.buffer = mem;
	info.buffer_vma = 0;
	info.buffer_length = MEMSIZE;
}

void kill_monitor (void)
{
	endwin();
}

int update_monitor (UL *regs, int sr, int pcoff)
{
	int i, j, x, c, cha=0, toppc, pos, lo1, eoi, yp, ddd=0;
	static UW v=0x4e71;
#if WATCH
	static UB wv=0, *watch=MEM(0);
#endif
	static UL lv=0;
	char cmd[100];
	UL u,s;
	if (count-- > 0 && TRIM(pcoff) != TRIM(bkpt)
#if WATCH
	&& LM_UB(watch) == wv
#endif
#if 0
	|| (mode == 1 && SUPERVISOR_MODE)
#endif
	) return;
#if WATCH
	wv = LM_UB(watch);
#endif
	in_monitor=1;
	toppc = pcoff;
	count = yp = 0;
loop:
	pos = toppc;
	for (i=0; i<8; i++)
	{
		move (i,0); printw ("D%d %04x %04x", i, regs[i]>>16, regs[i]&0xffff);
	}
	for (i=8; i<16; i++)
	{
		move (i,0); printw ("A%d %04x %04x", i-8, regs[i]>>16, regs[i]&0xffff);
	}
	if (sr & 0x2000)
	{
		u = regs[16]; s = regs[15];
	}
	else
	{
		u = regs[15]; s = regs[16];
	}
	move(16,0); printw ("US %04x %04x", u>>16, u&0xffff);
	move(17,0); printw ("SS %04x %04x", s>>16, s&0xffff);
	move(18,0); printw ("PC %08x", pcoff);
	move(18,14); if (SUPERVISOR_MODE) printw("+"); printw ("SR %04x", sr);
	move(18,23); printw ("CCR ");
	printcc('X',sr & 0x10);
	printcc('N',sr & 8);
	printcc('Z',sr & 4);
	printcc('V',sr & 2);
	printcc('C',sr & 1);
	for (i=0; i<15; i++)
	{
		move(i,14); printw ("%06x ",off+i*16);
		for (j=0; j<8; j++)
		{
			printw ("%04x ", LM_UW(MEM(off+i*16+j*2)));
		}
		for (j=0; j<16; j++)
		{
			int o = LM_UB(MEM(off+i*16+j));
			printw ("%c", isprint(o) ? o : '.');
		}
	}
	move(16,14);
	for (j=0; j<13; j++)
	{
		printw ("%04x ", LM_UW(MEM(u+j*2)));
	}
	move(17,14);
	for (j=0; j<13; j++)
	{
		printw ("%04x ", LM_UW(MEM(s+j*2)));
	}
	move(19,0); clrtobot();
	for (i=0; i<(LINES-20); i++)
	{
		int k;
		if (pos == pcoff) attron(A_UNDERLINE);
		if (pos == bkpt) attron(A_BOLD);
		move (i+19,36);
		k = print_insn_m68k(TRIM(pos), &info);
		if (i == 0) lo1 = k;
		move (i+19,0);
		printw("[%06x] ", TRIM(pos));
		for (j=0; j<(k/2); j++)
			printw("%04x ", LM_UW(MEM(pos+2*j)));
		if (pos == pcoff) attroff(A_UNDERLINE);
		if (pos == bkpt) attroff(A_BOLD);
		pos += k;
	}
	move(LINES-1,0); printw(">");
	refresh();
	cha = 1;
loop1:
	toppc = TRIM(toppc); off = TRIM(off); 
	switch (c = getch())
	{
		case 'm':	getstr(cmd); off = strtol(cmd, NULL, 16) & -2; goto loop;
		case 'p':	getstr(cmd); toppc = strtol(cmd, NULL, 16) & -2; goto loop;
		case '=': 	getstr(cmd); pcoff=toppc=pc=TRIM(strtol(cmd,NULL,16) & -2);
					goto loop;
		case 's':
		{
			getstr(cmd); 
			if (*cmd!=0) v = (UW)strtol(cmd,NULL,16);
			while (LM_UW(MEM(toppc)) != v) toppc+=2;
			goto loop;
		}			
		case 'S':
		{
			getstr(cmd); 
			if (*cmd!=0) lv = (UL)strtol(cmd,NULL,16);
			while (LM_UL(MEM(toppc)) != lv) toppc+=2;
			goto loop;
		}			
#if WATCH
		case 'w':	getstr(cmd); watch=MEM(strtol(cmd,NULL,16)); goto loop;
#endif
		case 'T':	mode ^= 1; goto loop;
		case 'r':	clear(); goto loop;
		case 'd':	off += 15 * 16; goto loop;
		case 'D':	off -= 15 * 16; goto loop;
		case 'z':	count = 10; return;
		case 'Z': 	count = 100000; in_monitor=0; return;
		case 'x':	count = 100; return;
		case 'X':	count = 1000000; in_monitor=0; return;
		case 'c':	count = 1000; return;
		case 'v':	count = 10000; return;
		case 'g':	count = 0x7ffffff; in_monitor=0; return;
		case '@':	sr |= 0x700; goto loop; 
		case 'n':	bkpt = TRIM(toppc+lo1); return;
		case 'b':   getstr(cmd); bkpt = strtol(cmd, NULL, 16) & -2; goto loop;
		case KEY_DOWN: toppc += lo1; goto loop;
		case KEY_UP: toppc -= 2; goto loop;
		case '>': toppc = pos; goto loop;
		case '<': toppc -= 2*(LINES-20); goto loop;
	}
	return c;
}

#endif /* MONITOR */

