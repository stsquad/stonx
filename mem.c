/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#define IN_MEM
#include "defs.h"
#include "mem.h"
#include "io.h"
#include <stdio.h>

extern int boot_dev, drive_bits;
extern int warmboot,fix_screen,scr_height,scr_width,scr_planes;
extern volatile UL vbase;
void init_mem (void)
{
	int mem_wanted;
	UL new_top;
	if (fix_screen)
	{
#if SMALL
#define PHYSTOP 0x400000
#else
#define PHYSTOP 0xe00000
#endif
		mem_wanted = (scr_height*scr_width*scr_planes)/8;
		new_top = (PHYSTOP-mem_wanted)&-1024;
		SM_UL(MEM(0x420),0x752019f3);
		SM_UL(MEM(0x43a),0x237698aa);
		SM_UL(MEM(0x51a),0x5555aaaa);	/* might break older TOSes <1.02? */
		fprintf(stderr,"Initializing memory for custom screen size... (top=%lx)\n",(long)new_top);
#if 0
		SM_UL(MEM(0x426),0x31415926);
		SM_UL(MEM(0x42a),0xfa1400);
#endif
		SM_UL(MEM(0x4ba),16000);
		SM_UL(MEM(0x436),new_top);
		SM_UL(MEM(0x42e),LM_UL(MEM(0x436)));
#if 0
		fprintf(stderr,"phystop is now %lx\n",LM_UL(MEM(0x42e)));
#endif
		SM_UL(MEM(0x44e),LM_UL(MEM(0x42e)));
#if SMALL
		SM_UB(MEM(0x424),10);
#else
		SM_UB(MEM(0x424),15);
#endif
		set_vbase(new_top);
	}
	else if (warmboot)
	{
		SM_UL(MEM(0x420),0x752019f3);
		SM_UL(MEM(0x43a),0x237698aa);
		SM_UL(MEM(0x51a),0x5555aaaa);	/* might break older TOSes <1.02? */
#if SMALL
		SM_UB(MEM(0x424),10);
		SM_UL(MEM(0x42e),0x400000);
		SM_UL(MEM(0x436),0x3f8000);
#else
		SM_UB(MEM(0x424),15);
		SM_UL(MEM(0x42e),0xe00000);
		SM_UL(MEM(0x436),0xdf8000);
#endif
		SM_UL(MEM(0x4ba),16000);
	}
	SM_W(ADDR(0x446), boot_dev);
	SM_W(ADDR(0x4a6), 0); /* _nflops may be different if we have FDC emul.*/
	SM_L(ADDR(0x4c2), drive_bits);
#if 0
	fprintf (stderr, "boot_dev = %d cmdload = %d\n",boot_dev,
		LM_W(ADDR(0x482)));
#endif
}

void mc68000_reset(void)
{	/* pc = TOSSTART; ????
	sr = 0x2700;
	*/
}
