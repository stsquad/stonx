#include "config.h"
#include "fonthdr.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

B mem[1]; 	/* dummy */
#if !defined(PCREG) || !defined(__GNUC__)
L pc;		/* dummy */
#endif
static UB m[512*1024];
static UB *hunt[]={
"\0\01\0\0106x6 system font\0",
"\0\01\0\0118x8 system font\0",
"\0\01\0\0128x16 system font\0",
};
static int len[]={20,20,21};
static int siz[]={1756,2650,4864};
static long offset=0xe00000;

void
dumpfont(x, l, s, f)
UB *x;
int l, s;
FILE *f;
{
	int i;
	FONT_HDR *h;
	UB *b = (UB *)malloc(s);
	for (i=0; i<sizeof(m)&&memcmp(&m[i],x,l); i++);
	if (i<sizeof(m))
	{
		(void)memcpy(b, &m[i], (size_t)s);
		h = (FONT_HDR *)b;
		memcpy(b+88, &m[LM_UL(&(h->off_table))-offset], 514);
		memcpy(b+602, &m[LM_UL(&(h->dat_table))-offset], s-602);
		SM_UL(&(h->off_table),sizeof(*h));
		SM_UL(&(h->dat_table),sizeof(*h)+257*2);
		fwrite (b, s, 1, f);
	}
	else
	{
		fprintf (stderr, "Couldn't find some fonts...\n");
	}
	free(b);
}

int
main (argc, argv)
int argc;
char *argv[];
{
	int i;
	size_t l;
	FILE *f;
	char s[100];

	l = fread (m, 1, sizeof(m), stdin);
	if (l<(size_t)200000)
	{
		fprintf (stderr, "%ld bytes - Looks like a 192KB TOS image...\n", (long)l);
		offset=0xfc0000;
	}
	for (i=0; i<3; i++)
	{
		sprintf (s, "System%d.fnt", i);
		f = fopen(s, "w");
		dumpfont(hunt[i], len[i], siz[i], f);
		fclose(f);
	}
	return 0;
}
