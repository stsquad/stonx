#include "defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "mem.h"
#include "utils.h"
#include "screen.h"
#include "svgalib_vdi.h"
#include "debug.h"
#include "main.h"

#ifndef TRACE_VDI
#define TRACE_VDI 0
#endif
#define UPDATE_PHYS 0
#define DUAL_MODE 1
#define CALL_ROM_VDI !(UPDATE_PHYS || DUAL_MODE)

#if TRACE_VDI
#define V(_x) debug _x
static UL last_pblock;
#else
#define V(_x) 
#endif


#if XBUFFER
#define XBUF(_x) _x
#else
#define XBUF(_x)
#endif

#define MAX_VWK 100
#define MAX_POINTS 1000
#define MAX_VDI_COLS	256

#define FIX_COLOR(_c) (priv_cmap ? vdi_maptab[(_c)&(MAX_VDI_COLS-1)]\
					: mapcol[vdi_maptab[(_c)&(MAX_VDI_COLS-1)]])

#define LAST_SCREEN_DEVICE 10

#define CHAR_HEIGHT() ((vdi && scr_height >= 400) || scr_planes==1 ? 16 : 8)


#define DEV_LOCATOR 1
#define DEV_VALUATOR 2
#define DEV_CHOICE 3
#define DEV_STRING 4
#define MODE_UNDEFINED 0
#define MODE_REQUEST 1
#define MODE_SAMPLE 2

#define WHITE 0
#define BLACK 1

#define CONTRL(_x) MEM(control+2*(_x))
#define INTIN(_x) MEM(intin+2*(_x))
#define V_OPCODE     LM_W(CONTRL(0))
#define INTOUT(_x) MEM(intout+2*(_x))
#define V_HANDLE     LM_W(CONTRL(6))
#define PTSOUT(_x) MEM(ptsout+2*(_x))

#define vdi_w scr_width
#define vdi_h scr_height

UL abase;
UL fontbase;

static UW work_out_buf[128];
static UL control, intin, ptsin, intout, ptsout;
static int vdi_planes;
static int vdi_maptab16[] = { 0, 15, 1, 2, 4, 6, 3, 5, 7, 8, 9, 10, 12, 14, 11, 13 };
static int vdi_maptab[MAX_VDI_COLS];
static int phys_handle = -1;

int vdi_output_c(char c)
{
    return TRUE; /* call TOS */
}

static void fix_vars(int width, int height, int planes)
{
	UW *a;

	T(("Fixing Line-A variables... %dx%dx%d\n", width, height, planes));
	a = (UW *)MEM(abase);
	SM_W(a + 1, (width * planes) / 8);
	SM_W(a - 1, (width * planes) / 8);
	SM_W(a - 23, CHAR_HEIGHT());
	SM_W(a - 22, width / 8 - 1);
	SM_W(a - 21, height / CHAR_HEIGHT() - 1);
	SM_W(a - 20, ((width * planes) / 8) * CHAR_HEIGHT());
	SM_W(a - 6, width);
	SM_W(a - 2, height);
	SM_W(a - 0x2b4 / 2, width - 1);
	SM_W(a - 0x2b2 / 2, height - 1);
}

void vdi_post(void)
{
	UW *a;
	UL pblock;
	
	SP += 6;
	pblock = LM_UL(MEM(SP));
	pc = pblock;
	SP += 4;
	pblock = LM_UL(MEM(SP));
	SP += 4;
	
	control = LM_UL(MEM(pblock));
	if (V_OPCODE != 1 && V_OPCODE != 100)
	{
		return;
	}
	intin = LM_UL(MEM(pblock + 4));
	ptsin = LM_UL(MEM(pblock + 8));
	intout = LM_UL(MEM(pblock + 12));
	ptsout = LM_UL(MEM(pblock + 16));
	a = (UW *)MEM(abase);
	vdi_planes = LM_W(a);
	if (fix_screen)
	{
		SM_W(INTOUT(0), vdi_w - 1);
		SM_W(INTOUT(1), vdi_h - 1);
		fix_vars(vdi_w, vdi_h, vdi_planes);
	}
	if (!vdi)
		return;
	
	/* fix the lookup table */
	fprintf(stderr,"V_OPCODE=%d\n",V_OPCODE);
	memcpy(&work_out_buf[0], INTOUT(0), 45*2);
	memcpy(&work_out_buf[45], PTSOUT(0), 12*2);
}

void Init_Linea(void)
{
	abase = DREG(0);
	fontbase = AREG(1);
	fprintf(stderr,"Initializing Line-A... (abase=%08lx)\n", (long)abase);
	if (fix_screen)
		fix_vars(scr_width, scr_height, scr_planes);
}

int Vdi(void)
{
    return FALSE; /* call TOS-vdi */
}
