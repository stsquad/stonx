/* cycles: std0 = 4/6/8 (add,or,and...); std1 = 8/12; std2 = 8/6/8 (aregs)
 * std3 = 8/16 for dreg, 12/20+ for mem; std4 = 4/8,8/12+; std5 = 4/8,18/30
 * std6 = 6/8+2n; std7 = 4/6,8/12
 * conditionals: "not taken" in 68000.def, difference handled inside func.
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "amdefs.h"
#include "options.h"

#define VERBOSE 0
#define WARN_DOUBLE 0
#define KLUDGE 1
#define EXTERN 1
#define FUNCS_PER_FILE 300

#define Strdup(_x) strcpy((char *)malloc(strlen(_x)+1),_x)

#define UBITS(_l,_h,_x) ((((unsigned int)(_x))>>(_l))&((1<<((_h)-(_l)+1))-1))
#define SBITS(_l,_h,_x) ((int) ((((_x)>>(_l))^(1<<((_h)-(_l))))&\
                        ((1<<((_h)-(_l)+1))-1)) - (1<<((_h)-(_l))))

#define T_DREG 	1
#define T_AREG 	2
#define T_EA 	3
#define T_SIZE 	4
#define T_SIZE1	5
#define T_S8	6
#define T_Q3	7
#define T_COND	8
#define T_CONDA	9
#define T_IEA 	10
#define T_U4 	11
#define T_U12 	12
#define T_SI8	13
#define T_SI16	14
#define T_SI32	15
#define T_SI	16

typedef struct
{
	int t_id;
	int t_arg;
} Type;

typedef struct
{
	int lo, hi;
	Type *t;
	char id;
} Variable;

typedef struct
{
	int maskbits, maskset, nvars, defsize, nfuncs;
	Variable *v;
	char *call;
	int *f;
} Pattern;

#define MAX_PAT 1000
#define MAX_FUNCS 65536

static Pattern *pat[MAX_PAT];
static char *funcs[MAX_FUNCS];
static unsigned char isize[65536];

static int npat=0,nfunc=0;

static int eq (char *s, char *t)
{
	return strcmp(s,t)==0;
}

Pattern *newpattern (void)
{
	Pattern *p;
	p = (Pattern *)malloc(sizeof(*p));
	p->maskbits=p->maskset=p->nvars=p->defsize=p->nfuncs=0;
	p->v=(Variable *)NULL;
	p->f=(int *)NULL;
	return p;
}

int newvar (Pattern *p)
{
	int n=p->nvars;
	if (p->v==NULL)
		p->v=(Variable *)malloc(sizeof(Variable));
	else
		p->v=(Variable *)realloc(p->v,(n+1)*sizeof(Variable));
	p->v[n].lo=p->v[n].hi=0;
	p->v[n].id='?';
	p->v[n].t=(Type *)NULL;
	p->nvars++;
	return n;
}

int newfunc (Pattern *p, char *call)
{
	int n=p->nfuncs;
	if (p->f==NULL)
		p->f=(int *)malloc(sizeof(int));
	else 
		p->f=(int *)realloc(p->f,(n+1)*sizeof(int));
	p->nfuncs++;
	funcs[nfunc]=call;
	return p->f[n]=nfunc++;
}

Pattern *s2bits (char *s)
{
	int i,j;
	char id;
	Pattern *p=newpattern();
	for (i=0; i<16; i++)
	{
		switch(s[i])
		{
			case '1':
				p->maskset|=(1<<(15-i));
			case '0':
				p->maskbits|=(1<<(15-i));
				break;
			default:
				id=s[i];
				j=newvar(p);
				p->v[j].id=id;
				p->v[j].hi=15-i;
				while(s[++i]==id);
				i--;
				p->v[j].lo=15-i;
				break;
		}
	}
	return p;
}

int findvar (Pattern *p, char id)
{
	int i;
	for (i=0; i<p->nvars && id!=p->v[i].id; i++);
	if (i==p->nvars)
	{
		fprintf(stderr,"Unknown var %c in TYPE\n",id);
		exit(1);
	}
	return i;
}

void settype (Type *t, Pattern *p, char id)
{		
	int i;
	switch(t->t_id)
	{
		case T_SI:
		case T_SI8:
		case T_SI16:
		case T_SI32:
			i=newvar(p);
			p->v[i].t=t; p->v[i].id=id; return;
	}
	p->v[findvar(p, id)].t=t;
}

char type_sign (Type *t)
{
	switch (t->t_id)
	{
		case T_DREG:
		case T_AREG:
		case T_EA:
		case T_IEA:
		case T_SIZE:
		case T_SIZE1:
		case T_COND:
		case T_CONDA:
		case T_U4:
		case T_U12:
			return 'U';
		case T_S8:
		case T_SI8:
		case T_SI16:
		case T_SI32:
		case T_SI:
			return 'S';
		case T_Q3:
			return 'Q';
		default:
			return '?';
	}
}

int type_getval (Variable *v, int iw)
{	
	int q;
	if (v->t->t_id == T_IEA)
	{
		int x=UBITS(v->lo,v->hi,iw);
		return ((x>>3)&7)|((x&7)<<3);
	}
	switch(type_sign(v->t))
	{
		case 'S': return SBITS(v->lo,v->hi,iw); 
		case 'U': return UBITS(v->lo,v->hi,iw); 
		case 'Q':
			q=UBITS(v->lo,v->hi,iw);
			return q==0?8:q;
	}
	return -9999;
}

#define size1_id(_x) size_id((_x)+1)

char *size_id (int n)
{
	if (n==0) return "BYTE";
	else if (n==1) return "WORD";
	else if (n==2) return "LONG";
	else return "???";
}

char *size_id0 (int s)
{
	switch(s)
	{
	case 1: return "BYTE";
	case 2: return "WORD";
	case 4: return "LONG";
	}
	return "???";
}

char *ea_id (int n, int size, char *x)
{
	static char s[1000];
	char *c;
	c = size_id0(size);
	switch (n>>3)
	{
		case 0:	sprintf(s,"DREG_%s,(%s&7)",c,x); break;
		case 1: sprintf(s,"AREG_%s,(%s&7)",c,x); break;
		case 2: sprintf(s,"AN_I_%s,(%s&7)",c,x); break;
		case 3:
			if (strcmp(c,"BYTE")==0&&(n==31||n==39))
				sprintf(s,"AN_PI_%s_All,7",c);
			else sprintf(s,"AN_PI_%s,(%s&7)",c,x);
			break;
		case 4:
			if (strcmp(c,"BYTE")==0&&(n==31||n==39))
				sprintf(s,"AN_PD_%s_All,7",c);
			else sprintf(s,"AN_PD_%s,(%s&7)",c,x);
			break;
		case 5: sprintf(s,"AN_D_%s,(%s&7)",c,x); break;
		case 6: sprintf(s,"AN_R_%s,(%s&7)",c,x); break;
		case 7: 
		switch (n&7)
		{
			case 0: sprintf(s,"ABS_W_%s,",c); break;
			case 1: sprintf(s,"ABS_L_%s,",c); break;
			case 2: sprintf(s,"PC_D_%s,",c); break;
			case 3: sprintf(s,"PC_R_%s,",c); break;
			case 4: sprintf(s,"IMM_%s,",c); break;
		}
	}
	return s;
}

char *iea_id (int n, int size, char *x)
{
	static char s[1000];
	char *c;
	c = size_id0(size);
	switch (n>>3)
	{
		case 0:	sprintf(s,"DREG_%s,(%s>>3)",c,x); break;
		case 1: sprintf(s,"AREG_%s,(%s>>3)",c,x); break;
		case 2: sprintf(s,"AN_I_%s,(%s>>3)",c,x); break;
		case 3: 
			if (strcmp(c,"BYTE")==0&&(n==31||n==39))
				sprintf(s,"AN_PI_%s_All,7",c);
			else sprintf(s,"AN_PI_%s,(%s>>3)",c,x);
			break;
		case 4: 
			if (strcmp(c,"BYTE")==0&&(n==31||n==39))
				sprintf(s,"AN_PD_%s_All,7",c);
			else sprintf(s,"AN_PD_%s,(%s>>3)",c,x);
			break;
		case 5: sprintf(s,"AN_D_%s,(%s>>3)",c,x); break;
		case 6: sprintf(s,"AN_R_%s,(%s>>3)",c,x); break;
		case 7: 
		switch (n&7)
		{
			case 0: sprintf(s,"ABS_W_%s,",c); break;
			case 1: sprintf(s,"ABS_L_%s,",c); break;
			case 2: sprintf(s,"PC_D_%s,",c); break;
			case 3: sprintf(s,"PC_R_%s,",c); break;
			case 4: sprintf(s,"IMM_%s,",c); break;
		}
	}
	return s;
}

int parse_am_class (char *s)
{
	if (eq(s,"AM_A")) return AM_A;
	else if (eq(s,"AM_MA")) return AM_MA;
	else if (eq(s,"AM_M")) return AM_M;
	else if (eq(s,"AM_DA")) return AM_DA;
	else if (eq(s,"AM_D")) return AM_D;
	else if (eq(s,"AM_C")) return AM_C;
	else if (eq(s,"AM_CA")) return AM_CA;
	else if (eq(s,"AM_DB")) return AM_DB;
	else if (eq(s,"AM_DNI")) return AM_DNI;
	else if (eq(s,"*")) return AM_ANY;
	else
	{
		fprintf(stderr,"Invalid AM class %s\n",s);
		exit(1);
	}
}

int ea_to_am (int n)
{
	switch(n>>3)
	{
		case 0:	return AM_DREG;
		case 1: return AM_AREG;
		case 2: return AM_AN_I;
		case 3: return AM_AN_PI;
		case 4: return AM_AN_PD;
		case 5: return AM_AN_D;
		case 6: return AM_AN_R;
		case 7:
		switch (n&7)
		{
			case 0: return AM_ABS_W;
			case 1: return AM_ABS_L;
			case 2: return AM_PC_D;
			case 3: return AM_PC_R;
			case 4: return AM_IMM;
		}
	}
	return 0;
}

int validate_ea (int n, int ea_mask)
{
	return (ea_mask & ea_to_am(n)) != 0;
}

int val2size (int n)
{
	if (n<2) return n+1;
	else return 4;
}

int opsize (Pattern *p, int iw)
{
	int i;
	if (p->defsize != 0) return p->defsize;
	for (i=0; i<p->nvars; i++)
	{
		Variable *v=&(p->v[i]);
		if (v->t->t_id == T_SIZE)
			return val2size(UBITS(v->lo,v->hi,iw));
		else if (v->t->t_id == T_SIZE1)
			return UBITS(v->lo,v->hi,iw) == 0 ? 2:4;
	}
	return 0;
}

int validate (Pattern *p, Variable *v, int iw)
{	/* TODO Areg/Byte */
	int n=type_getval(v,iw);
	switch (v->t->t_id)
	{
		case T_SI:
		case T_SI8:
		case T_SI16:
		case T_SI32:	return 1;

		case T_AREG:
		case T_DREG:	return n>=0 && n<=7;
		case T_SIZE:	return n>=0 && n<=2;
		case T_SIZE1:	return n==0 || n==1;
		case T_S8:		return n>=-128 && n<=127;
		case T_U4:		return n>=0 && n<=15;
		case T_U12:		return n>=0 && n<=4095;
		case T_Q3:		return n>=1 && n<=8;
		case T_COND:	return n>=2 && n<=15;
		case T_CONDA:	return n>=0 && n<=15;
		case T_EA:	
		case T_IEA:		return validate_ea(n,v->t->t_arg)
			&& !(opsize(p,iw)==1&&(n>>3)==1);
	}
	return 0;
}

char *gencond (int n)
{
	switch(n)
	{
		case 4:	return "CC";
		case 5: return "CS";
		case 7: return "EQ";
		case 12: return "GE";
		case 14: return "GT";
		case 2: return "HI";
		case 15: return "LE";
		case 3: return "LS";
		case 13: return "LT";
		case 11: return "MI";
		case 6: return "NE";
		case 10: return "PL";
		case 8: return "VC";
		case 9: return "VS";

		case 0: return "T";
		case 1: return "F";
	}
	return "??";
}

int expand (Type *t, int n)
{
	switch (t->t_id)
	{
		case T_EA:
		case T_IEA:
			if ((n>>3) == 7) return 1;
		case T_U4:
		case T_U12:
		case T_S8:
		case T_AREG:
		case T_DREG:
			return 0;
		case T_Q3:
			return n==8;
	}
	return 1;
}

char *print_var (Pattern *p, Variable *v, int iw)
{
	static char s[1000];
	char x[100];
	int ex=0;
	int n=type_getval(v,iw);
	if ((ex=(expand(v->t,n)
		||((v->t->t_id==T_IEA||v->t->t_id==T_EA)
				&& (n==047||n==037)
				&& (opsize(p,iw)==1))))) sprintf(x,"%d",n);
	else sprintf(x,"%cBITS(%d,%d,iw)",type_sign(v->t),v->lo,v->hi);/*BUG*/
	switch (v->t->t_id)
	{
		case T_DREG: sprintf(s,"%s",x); break;
		case T_AREG: sprintf(s,"%s",x); break;
		case T_SIZE: sprintf(s,"%s",size_id(n)); break;
		case T_SIZE1: sprintf(s,"%s",size1_id(n)); break;
		case T_IEA:sprintf(s,"%s",iea_id(n,opsize(p,iw),x)); break;
		case T_EA: sprintf(s,"%s",ea_id(n,opsize(p,iw),x)); break;
		case T_S8: 
		case T_U4: 
		case T_U12: sprintf(s,"%s",x); break;
		case T_Q3: if (ex) sprintf(s,"%d",n); else sprintf(s,"%s",x); break;
		case T_COND: sprintf(s,"%s",gencond(n)); break;
		case T_CONDA: sprintf(s,"%s",gencond(n)); break;
		case T_SI8:		strcpy(s,"GET_SI8"); break;
		case T_SI16:	strcpy(s,"GET_SI16"); break;
		case T_SI32:	strcpy(s,"GET_SI32"); break;
		case T_SI:		sprintf(s,"GET_SI%d",opsize(p,iw)*8); break;
	}
	return s;
}

int calc_easize (int n, int s)
{
	if (n<050) return 0;
	else if (n==071||(n==074&&s==4)) return 4;
	return 2;
}

int calc_varsize (Pattern *p, Variable *v, int iw)
{
	int s;
	int n=type_getval(v,iw);
	switch (v->t->t_id)
	{
		case T_DREG: 
		case T_AREG: 
		case T_SIZE: 
		case T_SIZE1:	return 0;
		case T_IEA:
		case T_EA: return calc_easize(n,opsize(p,iw));
		case T_S8: 
		case T_U4: 
		case T_U12: 
		case T_Q3: 
		case T_COND: 
		case T_CONDA: return 0;
		case T_SI8:		return 2;
		case T_SI16:	return 2;
		case T_SI32:	return 4;
		case T_SI:		if ((s=opsize(p,iw))==1) return 2; else return s;
	}
	return -999;
}

Type *parse_typedef (char *s)
{
	Type *t;
	char x[1000];
	t = (Type *)malloc(sizeof(*t));
	if (eq(s,"Dreg"))
		t->t_id = T_DREG;
	else if (eq(s,"Areg"))
		t->t_id = T_AREG;
	else if (eq(s,"Size"))
		t->t_id = T_SIZE;
	else if (eq(s,"Size1"))
		t->t_id = T_SIZE1;
	else if (eq(s,"S8"))
		t->t_id = T_S8;
	else if (eq(s,"U4"))
		t->t_id = T_U4;
	else if (eq(s,"U12"))
		t->t_id = T_U12;
	else if (eq(s,"SI"))
		t->t_id = T_SI;
	else if (eq(s,"SI8"))
		t->t_id = T_SI8;
	else if (eq(s,"SI16"))
		t->t_id = T_SI16;
	else if (eq(s,"SI32"))
		t->t_id = T_SI32;
	else if (eq(s,"Q3"))
		t->t_id = T_Q3;
	else if (eq(s,"Cond"))
		t->t_id = T_COND;
	else if (eq(s,"CondAll"))
		t->t_id = T_CONDA;
	else if (sscanf(s,"Ea(%s",x)==1)
	{
		if (x[strlen(x)-1]==')') x[strlen(x)-1]=0;
		t->t_id = T_EA;
		t->t_arg = parse_am_class(x);
	}
	else if (sscanf(s,"IEa(%s",x)==1)
	{
		if (x[strlen(x)-1]==')') x[strlen(x)-1]=0;
		t->t_id = T_IEA;
		t->t_arg = parse_am_class(x);
	}
	else
	{
		fprintf(stderr, "Invalid TYPE %s\n",s);
		exit(1);
	}
	return t;
}

void gobble (void)
{
	int c;
	while (isspace((c=getchar())&0xff));
	ungetc(c,stdin);
}

void parse_decl (void)
{
	char decl[1000];
	gobble();
	if (scanf("DECL:%16s",decl) != 1 || strlen(decl) != 16)
	{
		fprintf(stderr, "invalid DECL\n");
		exit(1);
	}
	pat[npat++] = s2bits(decl);
}

void parse_type (void)
{
	char t[1000];
	char id[100];
	Type *td;
	int i=0,j=0,k,l;
	gobble();
	if (scanf("TYPE:%s",t) != 1)
	{
pt_error:
		fprintf(stderr, "invalid TYPE\n");
		exit(1);
	}
	if (t[0]==';') return; 	/* empty */
	for(;;)
	{
		while(isalpha(t[i]&0xff)) id[j++]=t[i++];
		if (t[i++]!=':') goto pt_error;
		for (l=i; t[l]&&t[l]!=';'; l++);
		if (!t[l]) goto pt_error;
		t[l]=0;
		td=parse_typedef(&t[i]);
		for (k=0; k<j; k++) 
			settype(td, pat[npat-1], id[k]);
		i=l+1;
		j=0;
		if (!t[i]) break;
	}
}

void parse_call (void)
{
	char c[1000], s=0;
	int i;
	gobble();
	if ((scanf("CALL%c",&s))==1 && ((s==':' && scanf("%s",c)==1)
										|| scanf(":%s",c)==1))
	{
		switch(s)
		{
			case 'B': pat[npat-1]->defsize=1; break;
			case 'W': pat[npat-1]->defsize=2; break;
			case 'L': pat[npat-1]->defsize=4; break;
		}
	}
	else
	{
		fprintf(stderr,"invalid CALL\n");
		exit(1);
	}
	for (i=0; c[i]; i++)
	{
		if (c[i]=='$' && isalpha(c[i+1]&0xff))
			(void) findvar(pat[npat-1], c[i+1]);	/* assertion */
	}
	pat[npat-1]->call = Strdup(c);
}

static Pattern *tab[65536];
static int iw2op[65536];
static int fcalls[65536];

Pattern *select_pattern (int iw)
{
	int i;
	Pattern *f=NULL;
	for (i=0; i<npat; i++)
	{
		Pattern *p=pat[i];
		if ((iw & p->maskbits) == p->maskset)
		{
			int j;
			for (j=0; j<p->nvars && validate(p,&(p->v[j]),iw); j++);
			if (j==p->nvars)
			{
				if (f != NULL)
				{
#if WARN_DOUBLE
					fprintf(stderr,"Found both %s and %s for $%4x\n",
							f->call,p->call,iw);
#endif
				}
				else
				{
					f=p;
					iw2op[iw]=i;
				}
			}
		}
	}
	return f;
}

char *print_call (Pattern *p, int iw)
{
	int i;
	static char sx[1000];
	char *s;
	char *c=p->call;
	s=sx;
	*s=0;
	for (i=0; c[i]; i++)
	{
		if (c[i]=='$' && isalpha(c[i+1]&0xff))
		{
			strcpy(s,print_var(p,&(p->v[findvar(p,c[i+1])]),iw));
			s+=strlen(s);
			i++;
		}
		else *s++=c[i];
	}
	*s=0;
	return sx;
}

int addfunc (Pattern *p, char *call)
{
	int i;
	for (i=0; i<p->nfuncs; i++)
	{
		if (eq(call,funcs[p->f[i]]))
			return p->f[i];
	}
	return newfunc(p,Strdup(call));
}

int calc_size (Pattern *p, int iw)
{
	int i;
	int s=2;
	if (p==NULL) return 0;
	for (i=0; i<p->nvars; i++)
		s += calc_varsize(p,&(p->v[i]),iw);
	return s;
}

int
main(void)
{
	FILE *f;
	int i,c=0,dc=0,xnf,fi=0,foff=0;
	for (i=0; i<65536; i++) iw2op[i]=-1; /* illegal etc. */
	do
	{
		parse_decl();
		parse_type();
		parse_call();
		dc++;
		gobble();
	} while(!feof(stdin));
	fprintf(stderr,"%d declarations\n",dc);
	for (i=0; i<65536; i++)
		tab[i] = select_pattern (i);
	for (i=0; i<65536; i++)
		isize[i] = calc_size (tab[i],i);
	for (i=0; i<65536; i++)
	{
		if ((i&0xff)==0xff)
			fprintf(stderr,"\rgenerating: %4x funcs=%d",i,nfunc);
		if (tab[i])
		{
			c++;
			fcalls[i]=addfunc(tab[i],print_call(tab[i],i));
		}
	}
#if defined(GEN_SWITCH)
	fprintf(stderr,"\n");
	printf("switch(iw){\n");
	for (i=0; i<nfunc; i++)
	{
		int j;
		for (j=0; j<65536; j++)
		{
			if (tab[j] && fcalls[j] == i)
			{
				printf("case 0x%04x:\n",j);
			}
		}
		printf("{\n%s\n}\nbreak;\n",funcs[i]);
		fprintf(stderr,"\rwriting code block %d",i);
	}
	fprintf(stderr,"\n");
	printf("default: ILLEGAL(); break;\n}\n");
#elif defined(GEN_LABELTAB)
	fprintf(stderr,"\n");
	printf("\n#ifdef CODE_LABEL_TABLE\nstatic void *labeltab[]={\n");
	for (i=0; i<65536; i++)
	{
		if (tab[i]) printf("&&L%d,",fcalls[i]);
		else printf("&&LNil,");
		if ((i&7)==7) printf("\n");
	}
	printf("\n};\n#else /* CODE_LABEL_TABLE */\n");
	for (i=0; i<nfunc; i++)
		printf("L%d:{\n%s ;\n continue;}\n",i,funcs[i]);
	printf("LNil: ILLEGAL(); continue;\n#endif /* CODE_LABEL_TABLE */");
#else
#if EXTERN
	for (i=0; i<nfunc; i++)
	{
		printf("extern GENFUNC_PROTO(Func%d);\n",i);
	}
	xnf=nfunc;
	fprintf(stderr,"\nWriting functions...");
	while(xnf>0)
	{
		char fn[100];
		int i;
		sprintf(fn,"code%04d.m4",foff);
		fprintf(stderr,"%d...",foff);
		f = fopen(fn,"w");
		fprintf(f,"include(gendefs.m4)\n");
		for (i=0; i<FUNCS_PER_FILE && i<xnf; i++)
		{
			fprintf(f,"GENFUNC_PROTO(Func%d){ %s; }\n",i+foff,funcs[i+foff]);
		}
		foff+=FUNCS_PER_FILE;
		fclose(f);
		xnf -= i;
	}
#else
	for (i=0; i<nfunc; i++)
	{
		printf("GENFUNC_PROTO(Func%d){ %s; }\n",i,funcs[i]);
	}
#endif
	fprintf(stderr,"\n%d slots filled\n",c);
	printf("#ifndef DECOMPILER\n");
	printf("static GENFUNC_PROTO((*jumptab[65536]))={\n");
	for (i=0; i<65536; i++)
	{
		if (tab[i])
		{
			printf("Func%d,",fcalls[i]);
#if VERBOSE
			printf("/* %04x: %s [%d] */",i,funcs[fcalls[i]],isize[i]);
#endif
		}
		else printf ("Nullfunc,");
		if ((i & 7)==7) printf("\n");
	}
	printf("\n};\n");
	printf("#endif /* DECOMPILER */\n");
#endif /* GEN_SWITCH */
#if PROFILE
	f = fopen("proftabs.c","w");
	fprintf(f,"#define NUM_OPCODES %d\n",npat);
	fprintf(f,"unsigned char prof_maptab[65536]={\n");
	for (i=0; i<65536; i++)
	{
		fprintf(f,"%d,",iw2op[i]);
		if ((i&15)==15) fprintf(f,"\n");
	}
	fprintf(f,"};\n");
	fprintf(f,"char *prof_opname[%d]={\n",npat);
	for (i=0; i<npat; i++)
	{
		fprintf(f,"\"%s_%d\",\n",pat[i]->call,i);
	}
	fprintf(f,"};\nint dummy12345,prof_freq[%d];\n",npat);
	fclose(f);
#endif
	f = fopen("gentabs.c","w");
	fprintf(f,"short iw_to_func[65536]={\n");
	for (i=0; i<65536; i++)
	{
		fprintf(f,"%d,",tab[i]?fcalls[i]:-1);
		if ((i&7)==7) fprintf(f,"\n");
	}
	fprintf(f,"};\n");
	fprintf(f,"unsigned char isize[65536]={\n");
	for (i=0; i<65536; i++)
	{
		fprintf (f,"%d,",isize[i]);
		if ((i&31)==31) fprintf(f,"\n");
	}
	fprintf(f,"};\n");
	fclose(f);
	return 0;
}
