| bypasscall (GNU C version)

	.TEXT
	
	.globl	_jsr68000
	
	.equ	OPC_NATIVE, 	0xa0ff		| Opcode of native call
	.equ	SBC_RTS68000,	0x00010400	| Subcodes for rts68000

_jsr68000:
	move.l	a0,-(a7)
	move.l	a0,-(a7)
	move.l	8(a7),a0			| fctaddr
	move.l	a0,4(a7)			| to do a rts after all
	lea.l	rts68000(pc),a0
	move.l	a0,8(a7)			| fctaddr have to finish by rts
	move.l	(a7)+,a0
	rts

rts68000:
	.dc.w	OPC_NATIVE
	.dc.l	SBC_RTS68000
	bra	rts68000				| this will never happen!
