/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 */
#include "defs.h"
#include "mem.h"
#include "screen.h"
#include <stdio.h>
#include <string.h>

static unsigned char Ztab[256][4] = {
{0,0,0,0},{0,0,0,1},{0,0,1,0},{0,0,1,1},{0,1,0,0},{0,1,0,1},{0,1,1,0},{0,1,1,1},
{1,0,0,0},{1,0,0,1},{1,0,1,0},{1,0,1,1},{1,1,0,0},{1,1,0,1},{1,1,1,0},{1,1,1,1},
{0,0,0,2},{0,0,0,3},{0,0,1,2},{0,0,1,3},{0,1,0,2},{0,1,0,3},{0,1,1,2},{0,1,1,3},
{1,0,0,2},{1,0,0,3},{1,0,1,2},{1,0,1,3},{1,1,0,2},{1,1,0,3},{1,1,1,2},{1,1,1,3},
{0,0,2,0},{0,0,2,1},{0,0,3,0},{0,0,3,1},{0,1,2,0},{0,1,2,1},{0,1,3,0},{0,1,3,1},
{1,0,2,0},{1,0,2,1},{1,0,3,0},{1,0,3,1},{1,1,2,0},{1,1,2,1},{1,1,3,0},{1,1,3,1},
{0,0,2,2},{0,0,2,3},{0,0,3,2},{0,0,3,3},{0,1,2,2},{0,1,2,3},{0,1,3,2},{0,1,3,3},
{1,0,2,2},{1,0,2,3},{1,0,3,2},{1,0,3,3},{1,1,2,2},{1,1,2,3},{1,1,3,2},{1,1,3,3},
{0,2,0,0},{0,2,0,1},{0,2,1,0},{0,2,1,1},{0,3,0,0},{0,3,0,1},{0,3,1,0},{0,3,1,1},
{1,2,0,0},{1,2,0,1},{1,2,1,0},{1,2,1,1},{1,3,0,0},{1,3,0,1},{1,3,1,0},{1,3,1,1},
{0,2,0,2},{0,2,0,3},{0,2,1,2},{0,2,1,3},{0,3,0,2},{0,3,0,3},{0,3,1,2},{0,3,1,3},
{1,2,0,2},{1,2,0,3},{1,2,1,2},{1,2,1,3},{1,3,0,2},{1,3,0,3},{1,3,1,2},{1,3,1,3},
{0,2,2,0},{0,2,2,1},{0,2,3,0},{0,2,3,1},{0,3,2,0},{0,3,2,1},{0,3,3,0},{0,3,3,1},
{1,2,2,0},{1,2,2,1},{1,2,3,0},{1,2,3,1},{1,3,2,0},{1,3,2,1},{1,3,3,0},{1,3,3,1},
{0,2,2,2},{0,2,2,3},{0,2,3,2},{0,2,3,3},{0,3,2,2},{0,3,2,3},{0,3,3,2},{0,3,3,3},
{1,2,2,2},{1,2,2,3},{1,2,3,2},{1,2,3,3},{1,3,2,2},{1,3,2,3},{1,3,3,2},{1,3,3,3},
{2,0,0,0},{2,0,0,1},{2,0,1,0},{2,0,1,1},{2,1,0,0},{2,1,0,1},{2,1,1,0},{2,1,1,1},
{3,0,0,0},{3,0,0,1},{3,0,1,0},{3,0,1,1},{3,1,0,0},{3,1,0,1},{3,1,1,0},{3,1,1,1},
{2,0,0,2},{2,0,0,3},{2,0,1,2},{2,0,1,3},{2,1,0,2},{2,1,0,3},{2,1,1,2},{2,1,1,3},
{3,0,0,2},{3,0,0,3},{3,0,1,2},{3,0,1,3},{3,1,0,2},{3,1,0,3},{3,1,1,2},{3,1,1,3},
{2,0,2,0},{2,0,2,1},{2,0,3,0},{2,0,3,1},{2,1,2,0},{2,1,2,1},{2,1,3,0},{2,1,3,1},
{3,0,2,0},{3,0,2,1},{3,0,3,0},{3,0,3,1},{3,1,2,0},{3,1,2,1},{3,1,3,0},{3,1,3,1},
{2,0,2,2},{2,0,2,3},{2,0,3,2},{2,0,3,3},{2,1,2,2},{2,1,2,3},{2,1,3,2},{2,1,3,3},
{3,0,2,2},{3,0,2,3},{3,0,3,2},{3,0,3,3},{3,1,2,2},{3,1,2,3},{3,1,3,2},{3,1,3,3},
{2,2,0,0},{2,2,0,1},{2,2,1,0},{2,2,1,1},{2,3,0,0},{2,3,0,1},{2,3,1,0},{2,3,1,1},
{3,2,0,0},{3,2,0,1},{3,2,1,0},{3,2,1,1},{3,3,0,0},{3,3,0,1},{3,3,1,0},{3,3,1,1},
{2,2,0,2},{2,2,0,3},{2,2,1,2},{2,2,1,3},{2,3,0,2},{2,3,0,3},{2,3,1,2},{2,3,1,3},
{3,2,0,2},{3,2,0,3},{3,2,1,2},{3,2,1,3},{3,3,0,2},{3,3,0,3},{3,3,1,2},{3,3,1,3},
{2,2,2,0},{2,2,2,1},{2,2,3,0},{2,2,3,1},{2,3,2,0},{2,3,2,1},{2,3,3,0},{2,3,3,1},
{3,2,2,0},{3,2,2,1},{3,2,3,0},{3,2,3,1},{3,3,2,0},{3,3,2,1},{3,3,3,0},{3,3,3,1},
{2,2,2,2},{2,2,2,3},{2,2,3,2},{2,2,3,3},{2,3,2,2},{2,3,2,3},{2,3,3,2},{2,3,3,3},
{3,2,2,2},{3,2,2,3},{3,2,3,2},{3,2,3,3},{3,3,2,2},{3,3,2,3},{3,3,3,2},{3,3,3,3},
};

int scr_width,scr_height,scr_planes;
int depth;
int shift_mod;
char mapcol[COLS];
SCRDEF scr_def[3];
int draw_chunk[MAX_CHUNKS];
UB *old_smem;
int old_smem_size=0;

/* Convert an image in ST format (4 interleaved bitplanes) to ZPixmap format
 *
 * TODO: update the video address counter? (some programs wait for it...)
 * Solution: better return random values when the VAC registers are read
 *
 * This code assumes short = 16 bit and int = 32 bit and aligned data!
 */

#if IS_BIG_ENDIAN
#define LL(_x) *(_x)
#define SL(_x,_v) *(_x)=_v
#else
/* slow ... :-( */
#define LL(_x) LM_L(_x)	
#define SL(_x,_v) *(_x)=_v
/* #define SL(_x,_v) SM_L(_x,_v) */
#endif
void st16c_to_z (unsigned char *st, unsigned char *data)
{	
	int scr_bytes=scr_width*scr_height/2;
	int *y, lo, hi, i, idx, l, q, j,ch;
	unsigned short w1,w2,w3,w4,*x;
	unsigned int a1, a2, r1, r2, *r;
	int f0;
	int *ztab = (int *)Ztab;
	int chunk_bytes=(scr_width/2)*CHUNK_LINES;
	if (old_smem_size < scr_bytes)
	{
		if (old_smem_size != 0) free(old_smem);
		old_smem=(UB *)malloc(scr_bytes);
		old_smem_size=scr_bytes;
	}
	r = (unsigned int *)st;
	y = (int *)data;
	f0 = (scr_width/16)*CHUNK_LINES;
	for (ch=0; ch<scr_height/CHUNK_LINES; ch++)
	{
		if (chunky)
		{
			if (memcmp(st+ch*chunk_bytes,old_smem+ch*chunk_bytes,
						chunk_bytes) == 0)
			{
				y += 4*f0;
				r += 2*f0;
				continue;
			}
			draw_chunk[ch]|=1;
			memcpy(old_smem+ch*chunk_bytes,st+ch*chunk_bytes,chunk_bytes);
		}
		for (j=0; j<f0; j++)
		{
			r1 = LL(r);
			r2 = LL(r+1);
			r+=2;
			a1 = (r1 & 0xf0f0f0f); 
			w1 = (a1 << 4) | (a1 >> 16);
			a2 = (r2 & 0xf0f0f0f);
			w2 = (a2 << 4) | (a2 >> 16);
			SL(y+3,ztab[w1&0xff] + 4 * ztab[w2&0xff]);
			SL(y+1,ztab[w1>>8] + 4 * ztab[w2>>8]);
			a1 = (r1 & 0xf0f0f0f0); 
			w1 = (a1 >> 20) | a1;
			a2 = (r2 & 0xf0f0f0f0);
			w2 = (a2 >> 20) | a2;
			SL(y+2,ztab[w1&0xff] + 4 * ztab[w2&0xff]);
			SL(y,ztab[w1>>8] + 4 * ztab[w2>>8]);
			y += 4;
		}
		if (!priv_cmap)
		{
			int p=CHUNK_LINES*ch*scr_width;
			for (j=0; j<scr_width*CHUNK_LINES; j++)
			{
				data[p+j]=mapcol[data[p+j]];
			}
		}
	}
}

/* Convert an image in 2 interleaved bitplane format to ZPixmap with 400
 * lines. An optimizing compiler is a good idea ;-) (CSE)
 */
void st4c_to_z (unsigned char *st, unsigned char *data)
{
	int *r = (int *)st, r1, r2, ch;
	int i, j, k;
	int chunk_bytes=(scr_width/4)*CHUNK_LINES;
	int scr_bytes=scr_width*scr_height/4;
	if (old_smem_size < scr_bytes)
	{
		if (old_smem_size != 0) free(old_smem);
		old_smem=(UB *)malloc(scr_bytes);
		old_smem_size=scr_bytes;
	}
	for (ch=0; ch<scr_height/CHUNK_LINES; ch++)
	{
		if (chunky)
		{
			if (memcmp(st+ch*chunk_bytes,old_smem+ch*chunk_bytes,
						chunk_bytes) == 0)
			{
				data += scr_width*2*CHUNK_LINES;
				r += chunk_bytes/4;
				continue;
			}
			draw_chunk[ch]|=1;
			memcpy(old_smem+ch*chunk_bytes,st+ch*chunk_bytes,chunk_bytes);
		}
		for (k=0; k<CHUNK_LINES; k++, data += scr_width)
		for (i=0; i<scr_width/16; i++)
		{
			r1 = LL(r);
			r2 = ((r1>>16)&0x5555)|((r1<<1)&0xaaaa);
			if (priv_cmap)
			{
				for (j=0; j<8; j++)
				{
					*(data+15-2*j) = r2 & 3;
					*(data+scr_width+15-2*j) = r2 & 3;
					r2 >>= 2;
				}
				r2 = ((r1>>17)&0x5555)|(r1&0xaaaa);
				for (j=0; j<8; j++)
				{
					*(data+14-2*j) = r2 & 3;
					*(data+scr_width+14-2*j) = r2 & 3;
					r2 >>= 2;
				}
			}
			else
			{
				for (j=0; j<8; j++)
				{
					*(data+15-2*j) = mapcol[r2 & 3];
					*(data+scr_width+15-2*j) = mapcol[r2 & 3];
					r2 >>= 2;
				}
				r2 = ((r1>>17)&0x5555)|(r1&0xaaaa);
				for (j=0; j<8; j++)
				{
					*(data+14-2*j) = mapcol[r2 & 3];
					*(data+scr_width+14-2*j) = mapcol[r2 & 3];
					r2 >>= 2;
				}
			}
			data += 16;
			r++;
		}
	}
}

/* Convert an image in 2 interleaved bitplane format to ZPixmap with 200
 * lines. An optimizing compiler is a good idea ;-) (CSE)
 */
void st4c_to_z200 (unsigned char *st, unsigned char *data)
{
	int *r = (int *)st, r1, r2, ch;
	int i, j, k;
	int chunk_bytes=(scr_width/4)*CHUNK_LINES;
	int scr_bytes=scr_width*scr_height/4;
	if (old_smem_size < scr_bytes)
	{
		if (old_smem_size != 0) free(old_smem);
		old_smem=(UB *)malloc(scr_bytes);
		old_smem_size=scr_bytes;
	}
	for (ch=0; ch<scr_height/CHUNK_LINES; ch++)
	{
		if (chunky)
		{
			if (memcmp(st+ch*chunk_bytes,old_smem+ch*chunk_bytes,
						chunk_bytes) == 0)
			{
				data += scr_width*CHUNK_LINES;
				r += chunk_bytes/4;
				continue;
			}
			draw_chunk[ch]|=1;
			memcpy(old_smem+ch*chunk_bytes,st+ch*chunk_bytes,chunk_bytes);
		}
		for (k=0; k<CHUNK_LINES; k++)
		for (i=0; i<scr_width/16; i++,r++,data+=16)
		{
			r1 = LL(r);
			r2 = ((r1>>16)&0x5555)|((r1<<1)&0xaaaa);
			data[15] = r2 & 3; r2 >>= 2;
			data[13] = r2 & 3; r2 >>= 2;
			data[11] = r2 & 3; r2 >>= 2;
			data[9] = r2 & 3; r2 >>= 2;
			data[7] = r2 & 3; r2 >>= 2;
			data[5] = r2 & 3; r2 >>= 2;
			data[3] = r2 & 3; r2 >>= 2;
			data[1] = r2 & 3; r2 >>= 2;
			r2 >>= 2;
			r2 = ((r1>>17)&0x5555)|(r1&0xaaaa);
			data[14] = r2 & 3; r2 >>= 2;
			data[12] = r2 & 3; r2 >>= 2;
			data[10] = r2 & 3; r2 >>= 2;
			data[8] = r2 & 3; r2 >>= 2;
			data[6] = r2 & 3; r2 >>= 2;
			data[4] = r2 & 3; r2 >>= 2;
			data[2] = r2 & 3; r2 >>= 2;
			data[0] = r2 & 3; r2 >>= 2;
		}
	}
}

void stmono_to_xy (unsigned char *data, unsigned char *st)
{
	int i;
	int chunk_bytes;
	unsigned long *r=(unsigned long *)st;
	int scr_bytes=scr_width*scr_height/8;
	if (old_smem_size < scr_bytes)
	{
		if (old_smem_size != 0) free(old_smem);
		old_smem=(UB *)malloc(scr_bytes);
		old_smem_size=scr_bytes;
	}
	chunk_bytes = CHUNK_LINES*scr_width/8;
	if (!chunky)
	{
		memcpy(data,st,scr_height*scr_width/8);
		return;
	}
	for (i=0; i<scr_height/CHUNK_LINES; i++)
	{
		if (memcmp(st+i*chunk_bytes,old_smem+i*chunk_bytes,
					chunk_bytes) == 0)
		{
			r += chunk_bytes/sizeof(*r);
			data += chunk_bytes;
			continue;
		}
		else
		{	
			draw_chunk[i]|=1;
			memcpy(old_smem+i*chunk_bytes,st+i*chunk_bytes,chunk_bytes);
		}
		memcpy(data,r,chunk_bytes);
		r += chunk_bytes/sizeof(*r);
		data += chunk_bytes;
	}
}
