||| ================================================================
||| Part of STonX-fs (see main.c)
||| This module calls the natvie functions
||| Copyright (c) Markus Kohm, 1998
||| Version 0.1
|||
||| You have to assemble this at TOS or with a cross-assembler!
||| ================================================================

.DATA
	.extern	_stonx_filesys, _stonx_fs_devdrv
	 
.TEXT

	.equ	OPC_NATIVE,	0xa0ff		| Opcode of native call
	.equ	SBC_STX_FS,	0x00010000	| Subcodes for STonX-fs
	.equ	SBC_STX_FS_DEV,	0x00010100	| Subcodes for STonX-fs dev
	.equ	SBC_STX_SER,	0x00010200	| Subcodes for STonX-fs Serial
	.equ	SBC_STX_COM,	0x00010300	| Subcodes for STonX-fs communication

	.globl _stx_fs_native_init, _stx_fs_root, _stx_fs_lookup, _stx_fs_creat
	.globl _stx_fs_getdev, _stx_fs_getxattr, _stx_fs_chattr, _stx_fs_chown
	.globl _stx_fs_chmode, _stx_fs_mkdir, _stx_fs_rmdir, _stx_fs_remove,
	.globl _stx_fs_getname, _stx_fs_rename, _stx_fs_opendir, _stx_fs_readdir
	.globl _stx_fs_rewinddir, _stx_fs_closedir, _stx_fs_pathconf
	.globl _stx_fs_dfree, _stx_fs_writelabel, _stx_fs_readlabel 
	.globl _stx_fs_symlink, _stx_fs_readlink, _stx_fs_hardlink
	.globl _stx_fs_fscntl, _stx_fs_dskchng, _stx_fs_release
	.globl _stx_fs_dupcookie, _stx_fs_sync, _stx_fs_mknod, _stx_fs_unmount

	.globl _stx_fs_dev_open, _stx_fs_dev_write, _stx_fs_dev_read, _stx_fs_dev_lseek
	.globl _stx_fs_dev_ioctl, _stx_fs_dev_datime, _stx_fs_dev_close
	.globl _stx_fs_dev_select, _stx_fs_dev_unselect

	.globl _stx_ser_open, _stx_ser_write, _stx_ser_read, _stx_ser_lseek
	.globl _stx_ser_ioctl, _stx_ser_datime, _stx_ser_close
	.globl _stx_ser_select, _stx_ser_unselect
	.globl _stx_ser_b_instat,  _stx_ser_b_in, _stx_ser_b_outstat
	.globl _stx_ser_b_out, _stx_ser_b_rsconf

	.globl _stx_com_open, _stx_com_write, _stx_com_read, _stx_com_close
	.globl _stx_com_ioctl

	.extern	_jsr68000
	
_stx_fs_native_init:		| (struct kerinfo *kernel,UW fs_devnum)
	move.l	4(a7),a0	| kerinfo
	move.w	8(a7),d0	| fs device-number
	pea	_jsr68000	| callbypass
	move.l	a0,-(a7)	
	move.w	d0,-(a7)
	pea	_stonx_fs_devdrv
	pea	_stonx_filesys
	moveq	#-32,d0		| if STonXfs4MiNT not installed!
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS
	lea.l	0x12(a7),a7
	rts			| if ok then d0 = 0 else d0 = -32
	
_stx_fs_root:	
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x01
	rts
	
_stx_fs_lookup:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x02
	rts
	
_stx_fs_creat:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x03
	rts
	
_stx_fs_getdev:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x04
	rts
	
_stx_fs_getxattr:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x05
	rts
	
_stx_fs_chattr:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x06
	rts
	
_stx_fs_chown:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x07
	rts
	
_stx_fs_chmode:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x08
	rts
	
_stx_fs_mkdir:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x09
	rts
	
_stx_fs_rmdir:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x0A
	rts
	
_stx_fs_remove:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x0B
	rts
	
_stx_fs_getname:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x0C
	rts
	
_stx_fs_rename:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x0D
	rts
	
_stx_fs_opendir:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x0E
	rts
	
_stx_fs_readdir:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x0F
	rts
	
_stx_fs_rewinddir:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x10
	rts
	
_stx_fs_closedir:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x11
	rts
	
_stx_fs_pathconf:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x12
	rts
	
_stx_fs_dfree:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x13
	rts
	
_stx_fs_writelabel:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x14
	rts
	
_stx_fs_readlabel:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x15
	rts
	
_stx_fs_symlink:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x16
	rts
	
_stx_fs_readlink:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x17
	rts
	
_stx_fs_hardlink:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x18
	rts
	
_stx_fs_fscntl:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x19
	rts
	
_stx_fs_dskchng:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x1A
	rts
	
_stx_fs_release:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x1B
	rts
	
_stx_fs_dupcookie:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x1C
	rts
	
_stx_fs_sync:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x1D
	rts

_stx_fs_mknod:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x1E
	rts

_stx_fs_unmount:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS+0x1F
	rts
	
|||
||| FS device driver
|||

_stx_fs_dev_open:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS_DEV+0x01
	rts
	
_stx_fs_dev_write:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS_DEV+0x02
	rts
	
_stx_fs_dev_read:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS_DEV+0x03
	rts

_stx_fs_dev_lseek:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS_DEV+0x04
	rts
	
_stx_fs_dev_ioctl:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS_DEV+0x05
	rts

_stx_fs_dev_datime:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS_DEV+0x06
	rts
		
_stx_fs_dev_close:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS_DEV+0x07
	rts

_stx_fs_dev_select:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS_DEV+0x08
	rts

_stx_fs_dev_unselect:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_FS_DEV+0x09
	rts


||| ---------------------------------------------------------------------------
||| Serial device driver

_stx_ser_open:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SER+0x01
	rts
	
_stx_ser_write:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SER+0x02
	rts
	
_stx_ser_read:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SER+0x03
	rts

_stx_ser_lseek:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SER+0x04
	rts
	
_stx_ser_ioctl:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SER+0x05
	rts

_stx_ser_datime:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SER+0x06
	rts
		
_stx_ser_close:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SER+0x07
	rts

_stx_ser_select:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SER+0x08
	rts

_stx_ser_unselect:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SER+0x09
	rts

||| BIOS emulation
_stx_ser_b_instat:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SER+0x0A
	rts

_stx_ser_b_in:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SER+0x0B
	rts

_stx_ser_b_outstat:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SER+0x0C
	rts

_stx_ser_b_out:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SER+0x0D
	rts

_stx_ser_b_rsconf:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SER+0x0E
	rts


||| ---------------------------------------------------------------------------
||| Communication device driver

_stx_com_open:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_COM+0x01
	rts
	
_stx_com_write:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_COM+0x02
	rts
	
_stx_com_read:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_COM+0x03
	rts

_stx_com_ioctl:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_COM+0x05
	rts

_stx_com_close:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_COM+0x07
	rts

||| EOF
