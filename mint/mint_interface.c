/*
 * Part of STonX-fs
 * Copyright (c) Markus Kohm, 1998
 *
 * Translation of native calls (see STonX native.c).
 */

#ifdef __TOS__
#error This module hat to be part of STonX!
#endif

#include "../defs.h"
#include "../cpu.h"
#include "../mem.h"
#include "../options.h"

#ifndef MINT_STONXFS
# define MINT_STONXFS 1
#endif

#ifndef MINT_COM
# define MiNT_COM 1
#endif

#ifndef MINT_SERIAL
# define MINT_SERIAL 1
#endif

#ifndef MINT_USEBYPASS
# define MINT_USEBYPASS 0 /* we do not need it, so we deactivate it */
#endif

#if MINT_STONXFS
# include "mint_stonxfs_fs.h"
# include "mint_stonxfs_dev.h"
#endif

#if MINT_COM
# include "mint_comm.h"
#endif

#if MINT_SERIAL
# include "mint_serial.h"
#endif

#if ! MINT_STONXFS && MINT_USEBYPASS
#error MINT_USEBYPASS needs MINT_STONXFS
#endif

#ifdef DEBUG_MINT_INTERFACE
# define DBG( _args... ) fprintf( stderr, ## _args )
#else
# define DBG( _args... )
#endif


#if MINT_USEBYPASS
static int jsr68000(UL fctaddr);
static int rts68000(void);

static MINT_KERINFO *mint_kerinfo;
static UL            mint_callbypass;
#endif


#define CALL_FS     0x0000
#define CALL_DEV    0x0100
#define CALL_SER    0x0200
#define CALL_COM    0x0300


void mint_call_native(UL stack, UL func)
{
    unsigned     sfct = (unsigned)(func & 0xffff);
    UL         rv = 0;

    DBG("MiNT-STonX subfunction %#04x\n", sfct);
    
    switch(sfct)
    {
#if MINT_STONXFS
      case CALL_FS: /* Tell me where structure filesys is at atari memory */
	  rv = 0;
	  mint_fs_drv     = LM_UL(MEM(stack-4));     /* FILESYS */
	  mint_fs_devdrv  = LM_UL(MEM(stack));       /* DEVDRV  */
	  mint_fs_devnum  = LM_UW(MEM(stack+4));     /* device number */
#if MINT_USEBYPASS
	  mint_kerinfo    = MEM(LM_UL(MEM(stack+6)));/* kerinfo from MiNT */
	  mint_callbypass = LM_UL(MEM(stack+10));    /* address of call68000 */
#endif /* MINT_USEBYPASS */
	  DBG("MiNT fs:\n"
              " fs_drv     = %#08x\n"
	      " fs_devdrv  = %#08x\n"
	      " fs_devnum  = %#04x\n"
	      ,mint_fs_drv
	      ,mint_fs_devdrv
	      ,(int)mint_fs_devnum);

#if MINT_USEBYPASS
	  DBG(" kerinfo    = %#08lx\n"
	      " callbypass = %#08x\n"
	      ,(long)mint_kerinfo
	      ,mint_callbypass);
#endif /* MINT_USEBYPASS */
	  break;
	  
      case CALL_FS+0x01:
	  rv = mint_fs_root( LM_W(MEM(stack)), 
			     (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack+2))) ); 
	  break;
	  
      case CALL_FS+0x02:
	  rv = mint_fs_lookup( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
			       (const char *)MEM(LM_UL(MEM(stack+4))),
			       (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack+8))) ); 
	  break;
	  
      case CALL_FS+0x03:
	  rv = mint_fs_creat( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
			      (const char *)MEM(LM_UL(MEM(stack+4))), 
			      LM_UW(MEM(stack+8)), LM_W(MEM(stack+10)),
			      (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack+12))) ); 
	  break;
	  
      case CALL_FS+0x04:
	  rv = mint_fs_getdev( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
			       (L *)MEM(LM_L(MEM(stack+4))) ); 
	  break;
	  
      case CALL_FS+0x05:
	  rv = mint_fs_getxattr( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
				 (MINT_XATTR *)MEM(LM_UL(MEM(stack+4))) ); 
	  break;
	  
      case CALL_FS+0x06:
	  rv = mint_fs_chattr( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
			       LM_W(MEM(stack+4)) ); 
	  break;
	  
      case CALL_FS+0x07:
	  rv = mint_fs_chown( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
			      LM_W(MEM(stack+4)), LM_W(MEM(stack+6)) ); 
	  break;
	  
      case CALL_FS+0x08:
	  rv = mint_fs_chmode( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
			       LM_UW(MEM(stack+4)) ); 
	  break;
	  
      case CALL_FS+0x09:
	  rv = mint_fs_mkdir( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
			      (const char *)MEM(LM_UL(MEM(stack+4))), 
			      LM_UW(MEM(stack+8)) ); 
	  break;
	  
      case CALL_FS+0x0A:
	  rv = mint_fs_rmdir( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))),
			      (const char *)MEM(LM_UL(MEM(stack+4))) ); 
	  break;
	  
      case CALL_FS+0x0B:
	  rv = mint_fs_remove( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))),
			       (const char *)MEM(LM_UL(MEM(stack+4))) ); 
	  break;
	  
      case CALL_FS+0x0C:
	  rv = mint_fs_getname( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
				(MINT_FCOOKIE *)MEM(LM_UL(MEM(stack+4))), 
				(char *)MEM(LM_UL(MEM(stack+8))),
				LM_W(MEM(stack+12)) ); 
	  break;
	  
      case CALL_FS+0x0D:
	  rv = mint_fs_rename( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
			       (char *)MEM(LM_UL(MEM(stack+4))), 
			       (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack+8))), 
			       (const char *)MEM(LM_UL(MEM(stack+12))) );
	  break;
	  
      case CALL_FS+0x0E:
	  rv = mint_fs_opendir( (MINT_DIR *)MEM(LM_UL(MEM(stack))), 
				LM_W(MEM(stack+4)) ); 
	  break;
	  
      case CALL_FS+0x0F:
	  rv = mint_fs_readdir( (MINT_DIR *)MEM(LM_UL(MEM(stack))),
				(char *)MEM(LM_UL(MEM(stack+4))), 
				LM_W(MEM(stack+8)), 
				(MINT_FCOOKIE *)MEM(LM_UL(MEM(stack+10))) );
	  break;
	  
      case CALL_FS+0x10:
	  rv = mint_fs_rewinddir( (MINT_DIR *)MEM(LM_UL(MEM(stack))) ); 
	  break;
	  
      case CALL_FS+0x11:
	  rv = mint_fs_closedir( (MINT_DIR *)MEM(LM_UL(MEM(stack))) ); 
	  break;
	  
      case CALL_FS+0x12:
	  rv = mint_fs_pathconf( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
				 LM_W(MEM(stack+4)) ); 
	  break;
	  
      case CALL_FS+0x13:
	  rv = mint_fs_dfree( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))),
			      (L *)MEM(LM_UL(MEM(stack+4))) ); 
	  break;
	  
      case CALL_FS+0x14:
	  rv = mint_fs_writelabel( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
				   (const char *)MEM(LM_UL(MEM(stack+4))) );
	  break;
	  
      case CALL_FS+0x15:
	  rv = mint_fs_readlabel( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
				  (char *)MEM(LM_UL(MEM(stack+4))), 
				  LM_W(MEM(stack+8)) ); 
	  break;
	  
      case CALL_FS+0x16:
	  rv = mint_fs_symlink( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
				(const char *)MEM(LM_UL(MEM(stack+4))), 
				(const char *)MEM(LM_UL(MEM(stack+8))) ); 
	  break;
	  
      case CALL_FS+0x17:
	  rv = mint_fs_readlink( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
				 (char *)MEM(LM_UL(MEM(stack+4))), 
				 LM_W(MEM(stack+8)) ); 
	  break;
	  
      case CALL_FS+0x18:
	  rv = mint_fs_hardlink( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
				 (const char *)MEM(LM_UL(MEM(stack+4))),
				 (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack+8))),
				 (const char *)MEM(LM_UL(MEM(stack+12))) );
	  break;
	  
      case CALL_FS+0x19:
	  rv = mint_fs_fscntl( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
			       (const char *)MEM(LM_UL(MEM(stack+4))), 
			       LM_W(MEM(stack+8)), LM_L(MEM(stack+10)) ); 
	  break;
	  
      case CALL_FS+0x1A:
	  rv = mint_fs_dskchng( LM_W(MEM(stack)) ); 
	  break;
	  
      case CALL_FS+0x1B:
	  rv = mint_fs_release( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))) );
	  break;
	  
      case CALL_FS+0x1C:
	  rv = mint_fs_dupcookie( (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))), 
				  (MINT_FCOOKIE *)MEM(LM_UL(MEM(stack+4))) );
	  break;
	  
      case CALL_FS+0x1D:
	  rv = mint_fs_sync(); 
	  break;
	  
      case CALL_FS+0x1E:
	  rv = mint_fs_mknod((MINT_FCOOKIE *)MEM(LM_UL(MEM(stack))),
			     (const char *)MEM(LM_UL(MEM(stack+4))),
			     LM_L(MEM(stack+8)) ); 
	  break;
	  
      case CALL_FS+0x1F:
	  rv = mint_fs_unmount( LM_W(MEM(stack)) ); 
	  break;
	  
	  
      case CALL_DEV+0x01:
	  rv = mint_fs_dev_open( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))) );
	  break;
	  
      case CALL_DEV+0x02:
	  rv = mint_fs_dev_write( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
				  (const char *)MEM(LM_UL(MEM(stack+4))),
				  LM_L(MEM(stack+8)) );
	  break;
	  
      case CALL_DEV+0x03:
	  rv = mint_fs_dev_read( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
				 (char *)MEM(LM_UL(MEM(stack+4))),
				 LM_L(MEM(stack+8)) );
	  break;
	  
      case CALL_DEV+0x04:
	  rv = mint_fs_dev_lseek( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
				  LM_L(MEM(stack+4)), LM_W(MEM(stack+8)) );
	  break;
	  
      case CALL_DEV+0x005:
	  rv = mint_fs_dev_ioctl( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))), 
				  LM_W(MEM(stack+4)), 
				  MEM(LM_UL(MEM(stack+6))) );
	  break;
	  
      case CALL_DEV+0x06:
	  rv = mint_fs_dev_datime( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
				   (W *)MEM(LM_UL(MEM(stack+4))), 
				   LM_W(MEM(stack+8)) );
	  break;
	  
      case CALL_DEV+0x07:
	  rv = mint_fs_dev_close( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))), 
				  LM_W(MEM(stack+4)) );
	  break;
	  
      case CALL_DEV+0x08:
	  rv = mint_fs_dev_select( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
				   LM_L(MEM(stack+4)), LM_W(MEM(stack+8)) );
	  break;
	  
      case CALL_DEV+0x09:
	  rv = mint_fs_dev_unselect( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
				     LM_L(MEM(stack+4)), LM_W(MEM(stack+8)) );
            break;
#endif /* MINT_STONXFS */

#if MINT_SERIAL
      case CALL_SER+0x01:
	  rv = mint_ser_open( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))) );
	  break;
	  
      case CALL_SER+0x02:
	  rv = mint_ser_write( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
			       (const char *)MEM(LM_UL(MEM(stack+4))),
			       LM_L(MEM(stack+8)) );
	  break;
	  
      case CALL_SER+0x03:
	  rv = mint_ser_read( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
			      (char *)MEM(LM_UL(MEM(stack+4))),
			      LM_L(MEM(stack+8)) );
	  break;
	  
      case CALL_SER+0x04:
	  rv = mint_ser_lseek( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
			       LM_L(MEM(stack+4)), LM_W(MEM(stack+8)) );
	  break;
	  
      case CALL_SER+0x05:
	  rv = mint_ser_ioctl( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))), 
			       LM_W(MEM(stack+4)), MEM(LM_UL(MEM(stack+6))) );
	  break;
	  
      case CALL_SER+0x06:
	  rv = mint_ser_datime( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
				(W *)MEM(LM_UL(MEM(stack+4))), 
				LM_W(MEM(stack+8)) );
	  break;
	  
      case CALL_SER+0x07:
	  rv = mint_ser_close((MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
			      LM_W(MEM(stack+4)) );
	  break;
	  
      case CALL_SER+0x08:
	  rv = mint_ser_select((MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
			       LM_L(MEM(stack+4)), LM_W(MEM(stack+8)) );
	  break;
	  
      case CALL_SER+0x09:
	  rv = mint_ser_unselect((MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
				 LM_L(MEM(stack+4)), LM_W(MEM(stack+8)) );
	  break;
#endif /* MINT_SERIAL */	  
	  
#if MINT_COM
      case CALL_COM+0x01:
	  rv = mint_com_open( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))) );
	  break;
	  
      case CALL_COM+0x02:
	  rv = mint_com_write( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
			       (const char *)MEM(LM_UL(MEM(stack+4))),
			       LM_L(MEM(stack+8)) );
	  break;
	  
      case CALL_COM+0x03:
	  rv = mint_com_read( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
			      (char *)MEM(LM_UL(MEM(stack+4))),
			      LM_L(MEM(stack+8)) );
	  break;
	  
      case CALL_COM+0x05:
	  rv = mint_com_ioctl( (MINT_FILEPTR *)MEM(LM_UL(MEM(stack))), 
			       LM_W(MEM(stack+4)), MEM(LM_UL(MEM(stack+6))) );
	  break;
	  
      case CALL_COM+0x07:
	  rv = mint_com_close((MINT_FILEPTR *)MEM(LM_UL(MEM(stack))),
			      LM_W(MEM(stack+4)) );
	  break;
	  
      case CALL_COM+0x04:
      case CALL_COM+0x06:
      case CALL_COM+0x08:
      case CALL_COM+0x09:
	  DBG("comm function %#04x handled by MiNT native.\n", sfct);
	  break;
#endif /* MINT_COM */

#if MINT_USEBYPASS	  
      case 0x0400:        /* -> SBC_RTS68000 in atari bypass.c !! */
	  rv = rts68000();
	  /* rts68000 should never return */
	  if ( verbose )
	      fprintf( stderr, "FATAL: rts68000() returned!\n" );
	  break;
#endif /* MINT_STONXFS */

      default:
	  DBG("MiNT-STonX subfunction %#04x not defined!\n", sfct);
	  rv = TOS_EINVFN;
	  break;
    }
    DREG(0)=rv;
    return;
}


#if MINT_USEBYPASS
/*
 * Problem:  STonX has to call a function at emulation space
 * Solution: - save context and execute a bypass function
 */
#include <setjmp.h>

typedef struct jmpstack 
{
    struct jmpstack *tail;
    jmp_buf          env;
    L                pc;
} JMPSTACK;

static JMPSTACK *jmpstacktop;

/*
 * Call function at emulation space
 * no register saving and restoring is done, if needed do it yourself
 * --> fctaddr - address of the function at emulation space
 * <-- -1: error (not enough memory to do a setjmp/longjmp)
 */
static int jsr68000(UL fctaddr)
{
    JMPSTACK *newtop;
    if ( ( newtop = malloc( sizeof( JMPSTACK ) ) ) == NULL )
        return -1;
    newtop->tail = jmpstacktop;
    jmpstacktop = newtop;
    jmpstacktop->pc = pc;
    PUSH_UL( fctaddr );
    if ( setjmp( jmpstacktop->env ) )
    {
        newtop = jmpstacktop;
        jmpstacktop = jmpstacktop->tail;
        pc = jmpstacktop->pc;
        free( newtop );
    }
    else
        execute(mint_callbypass);
    return 0;
}


static int rts68000(void)
{
    if ( !jmpstacktop )
        return -1;
    longjmp( jmpstacktop->env, 1 );
}


#ifndef MEM_OFFSET
#define MEM_OFFSET(_a) ((UL)_a - (UL)mem)
#endif

W denyshare(MINT_FILEPTR *list, MINT_FILEPTR *f)
{
    PUSH_UL( MEM_OFFSET(f) );
    PUSH_UL( MEM_OFFSET(list) );
    jsr68000( LM_UL( &(mint_kerinfo->denyshare) ) );
    POP_UL();
    POP_UL();
    return DREG(0);
}


MINT_LOCK *denylock(MINT_LOCK *list, MINT_LOCK *new)
{
    PUSH_UL( MEM_OFFSET(new) );
    PUSH_UL( MEM_OFFSET(list) );
    jsr68000( LM_UL( &(mint_kerinfo->denylock) ) );
    POP_UL();
    POP_UL();
    return MEM(DREG(0));
}

#endif /* MINT_USEBYPASS */
