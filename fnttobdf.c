/* fnttobdf
 * This utility converts GEM Bitmap fonts (.FNT) to BDF files (.bdf), which
 * can then be converted to .pcf files using "bdftopcf".
 * It can also convert the fonts to ISO-Latin1 encoding.
 *
 * Written by M.Yannikos (Apr 15 1995)
 */
#include "defs.h"
#include "fonthdr.h"
#include <stdio.h>
#include <string.h>

#define S 100000
#define X(_x) _x = (((_x)&0xff)<<8)|((_x)>>8)
#define XP(_t,_x) {UL y=(UL)_x; _x = (_t)(((y&0xff)<<24)|((y&0xff00)<<8)|\
			((y>>8)&0xff00)|((y>>24)&0xff));}

#ifndef MIN
#define MIN(_x,_y) ((_x)<(_y)?(_x):(_y))
#endif

UB m[S];
static int tr[256], invtr[256];

char *
font_name(n, p, w, h, t, f, i)
char *n;
int p, w, h, t, f, i;
{
	static char s[200];
	if (t)
	{
		sprintf (s, "-misc-%s-medium-r-normal--%d-%d-100-100-%c-100-iso8859-1",
				n, p, 10*p, (f&8)?'m':'p');
	}
	else
	{
		sprintf (s, "%s-%d-%d", n, p, i);
	}
	return s;
}

/* inefficient */
void
print_line (dat, off, w)
UW *dat;
int off, w;
{
	UL x=0;
	int j=0;
	do
	{
		int p = off/16;
		int b = MIN(16-(off&15),w);
		if (j+b>32) b = (32-j);
#if 0
		fprintf (stderr, "off=%d, w=%d, j=%d, p=%d, x=%ld b=%d\n",
			off, w, j, p, x, b);
#endif
		x |= UBITS(15-(off&15)-b+1,15-(off&15),dat[p]) << (32-j-b);
		j += b;
		off += b;
		w -= b;
		if (j == 32)
		{
			j=0;
			printf ("%08x", x);
			x=0;
		}
	} while (w > 0);
	if (j>24) printf ("%08x", x);
	else if (j>16) printf ("%06x", x>>8);
	else if (j>8) printf ("%04x", x>>16);
	else if (j>0) printf ("%02x", x>>24);
	printf ("\n");
}

void
fnttobdf (b, l, out, t, info)
UB *b;
int l, t, info;
FILE *out;
{
	UW *u;
	UB *hh;
	UW *ho,*hd;
	int i;
	FONT_HDR *h = (FONT_HDR *)b;
#if WORDS_BIGENDIAN
	if ((b[67] & 4) == 0)
	{
		fprintf (stderr, "Font header is in Intel format, swapping...\n");
#else
	if (b[67] & 4)
	{
		fprintf (stderr, "Font header is in Motorola format, swapping...\n");
#endif
		X(h->font_id);
		X(h->point);
		for (u=(UW *)&(h->first_ade); u<(UW *)&(h->hor_table); u++)
			X(*u);
		X(h->form_width);
		X(h->form_height);
		XP(UL,h->hor_table);
		XP(UL,h->off_table);
		XP(UL,h->dat_table);
		hh = (UB *)((char *)h+h->hor_table);
		ho = (UW *)((char *)h+h->off_table);
		hd = (UW *)((char *)h+h->dat_table);
		for (u=ho; u<=ho+h->last_ade-h->first_ade+1; u++)
			X(*u);
#if !WORDS_BIGENDIAN
		for (u=hd; u<hd+h->form_height*h->form_width/2; u++)
			X(*u);
#endif
	}
	else
	{
		hh = (UB *)((char *)h+h->hor_table);
		ho = (UW *)((char *)h+h->off_table);
		hd = (UW *)((char *)h+h->dat_table);
	}
	if (info)
	{
		printf ("Name: %s\nSize: %dpt, %s %dx%d\n",
			h->name, h->point, (h->flags&8) ? "monospaced":"proportional max.",
			h->max_cell_width, h->form_height);
		return;
	}
	fprintf (stderr, "Writing font %s(chars %d..%d)\n",
		h->name,h->first_ade,h->last_ade);
	printf ("STARTFONT 2.1\nFONT %s\nSIZE %d 100 100\n"
			"FONTBOUNDINGBOX %d %d 0 0\nSTARTPROPERTIES 5\n"
			"FONT_ASCENT %d\nFONT_DESCENT %d\nSPACING \"%c\"\n"
			"CHARSET_REGISTRY \"ISO8859\"\nCHARSET_ENCODING \"1\"\n"
			"ENDPROPERTIES\nCHARS %d\n",
			font_name(h->name, h->point, h->max_cell_width, h->form_height,
					t, h->flags, h->font_id),
			h->point, h->max_cell_width, h->form_height,
#if 0
			h->top+(h->form_height-h->top-h->bottom), h->bottom,
#else
			h->form_height-h->bottom, h->bottom,
#endif
			(h->flags & 8) ? 'M' : 'P',
			h->last_ade-h->first_ade+1);
	for (i=0; i<=h->last_ade-h->first_ade; i++)
	{
		int j;
		int o = ho[i];
		int w = ho[i+1]-o;
		printf ("STARTCHAR dec%d\nENCODING %d\nSWIDTH %d 0\nDWIDTH %d 0\n"
				"BBX %d %d %d %d\nBITMAP\n",
				i+h->first_ade, tr[i+h->first_ade], 100*w, w, w, h->form_height,
#if 0
				0, -h->descent);
#else	
				0, -h->bottom);
#endif
		for (j=0; j<h->form_height; j++)
		{
			print_line (hd+h->form_width*j/2, o, w);
		}
		printf ("ENDCHAR\n");
	}
	printf ("ENDFONT\n");
}



/* Data for Atari ST to ISO Latin-1 code conversions.  */
/* Written by Andreas Schwab and taken from GNU recode version 2.4 */

static int known_pairs[][2] =
  {
    {128, 199},			/* C, */
    {129, 252},			/* u" */
    {130, 233},			/* e' */
    {131, 226},			/* a^ */
    {132, 228},			/* a" */
    {133, 224},			/* a` */
    {134, 229},			/* aa */
    {135, 231},			/* c, */
    {136, 234},			/* e^ */
    {137, 235},			/* e" */
    {138, 232},			/* e` */
    {139, 239},			/* i" */
    {140, 238},			/* i^ */
    {141, 236},			/* i` */
    {142, 196},			/* A" */
    {143, 197},			/* AA */
    {144, 201},			/* E' */
    {145, 230},			/* ae */
    {146, 198},			/* AE */
    {147, 244},			/* o^ */
    {148, 246},			/* o" */
    {149, 242},			/* o` */
    {150, 251},			/* u^ */
    {151, 249},			/* u` */
    {152, 255},			/* y" */
    {153, 214},			/* O" */
    {154, 220},			/* U" */
    {155, 162},			/* \cent */
    {156, 163},			/* \pound */
    {157, 165},			/* \yen */
    {158, 223},			/* \ss */

    {160, 225},			/* a' */
    {161, 237},			/* i' */
    {162, 243},			/* o' */
    {163, 250},			/* u' */
    {164, 241},			/* n~ */
    {165, 209},			/* N~ */
    {166, 170},			/* a_ */
    {167, 186},			/* o_ */
    {168, 191},			/* ?' */

    {170, 172},			/* \neg */
    {171, 189},			/* 1/2 */
    {172, 188},			/* 1/4 */
    {173, 161},			/* !` */
    {174, 171},			/* `` */
    {175, 187},			/* '' */
    {176, 227},			/* a~ */
    {177, 245},			/* o~ */
    {178, 216},			/* O/ */
    {179, 248},			/* o/ */

    {182, 192},			/* A` */
    {183, 195},			/* A~ */
    {184, 213},			/* O~ */
    {185, 168},			/* diaeresis */
    {186, 180},			/* acute accent */

    {188, 182},			/* pilcrow sign */
    {189, 169},			/* copyright sign */
    {190, 174},			/* registered trade mark sign */

    {221, 167},			/* paragraph sign, section sign */

    {230, 181},			/* mu, micro */

    {241, 177},			/* +- */

    {246, 247},			/* \div */

    {248, 176},			/* \deg */

    {250, 183},			/* \cdot */

    {253, 178},			/* ^2 */
    {254, 179},			/* ^3 */
  };
#define NUMBER_OF_PAIRS (sizeof (known_pairs) / sizeof (known_pairs[0]))

#define EQ(_i,_y) (strcmp(argv[_i],_y) == 0)

int
main (argc, argv)
int argc;
char *argv[];
{
	FILE *in,*out;
	int l, i, t=0, info=0;
	in=stdin;
	out=stdout;
	for (i=1; i<argc; i++)
	{
		if (EQ(i,"-tr")) t=1;
		else if (EQ(i,"-i")) info=1;
	}
	l = fread (m, 1, S, in);
	if (t)
	{
		for (i=0; i<NUMBER_OF_PAIRS; i++)
		{
			tr[known_pairs[i][0]] = known_pairs[i][1];
			invtr[known_pairs[i][1]] = known_pairs[i][0];
		}
	}
	for (i=0; i<256; i++)
	{
		if (tr[i] == 0 && invtr[i] == 0)
			tr[i] = i;
	}
	fnttobdf (m, l, out, t, info);
	return 0;
}
