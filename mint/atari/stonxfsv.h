/*
 * Use this at STonX and the Atari part
 */

#ifndef _stonxfsv_h_
#define _stonxfsv_h_

/* version of XFS (major.minor) */
#define MINT_FS_V_MAJOR    0
#define MINT_FS_V_MINOR    5

/* definitions for Atari part */
#define MINT_FS_NAME    "nativefs"   /* mount point at u:\\ */
#define MINT_SER_NAME   "serial2"    /* device name in u:\\dev\\ */
#define MINT_COM_NAME   "stonx"      /* device name in u:\\dev\\ */

/* Following shouldn't be here but we don't want to share many files with
 * stonx
 */
#ifndef UL
#  define UL    unsigned long
#endif
#ifndef UW
#  define UW    unsigned short
#endif
#ifndef W
#  define W     short
#endif
#ifndef CHAR
#  define CHAR  char
#endif

typedef struct _stonx_cookie {
    UL   magic;         /* COOKIE_STon */
#ifndef COOKIE_STon
# define COOKIE_STon ((UL)0x53546f6eL) /* STon */
#endif
    UL   cookielen;     /* Length of the structure (28) */
    UW   stonx_major;   /* Major version of STonX (e.g. 0) */
    UW   stonx_minor;   /* Minor version of STonX (e.g. 6) */
    UW   stonx_pmajor;  /* Major version of STonX patch (e.g. 7) */
    UW   stonx_pminor;  /* Minor version of STonX patch (e.g. 1) */
    CHAR screenname[8]; /* First 8 chars of screen-type */
    UL   flags;         /* see below */
    /* The meaning of the MiNT flags is: */
#define STNX_IS_XFS    0x00000001L /* stonxfs is compiled into STonX */
#define STNX_IS_BYPASS 0x00000002L /* bypass at stonxfs is compiled into STonX
				    * (You do not need to know this.) */
#define STNX_IS_SERIAL 0x00000004L /* serial MiNT device is compiled into 
				    * STonX */
#define STNX_IS_MODEM1 0x00000008L /* simple MODEM1 emulation is compiled
				    * into STonX */
#define STNX_IS_COM    0x00000010L /* communication MiNT device is compiled
				    * into STonX */
#define STNX_IS_VDIUSE 0x00000020L /* STonX uses native build in vdi */
#ifdef atarist
    UL   (*shutdown)(W mode); /* react on Atari shutdown 
			       * (0 = halt, 1 = warmboot, 2 = coldboot) */
#else
    UL   shutdown_fct_addr;
#endif
} STONX_COOKIE;

#endif /* _stonxfsv_h_ */
