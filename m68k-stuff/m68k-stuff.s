	.TEXT
	.equ	OPC_NATIVE,	0xa0ff		| Opcode of native call
	.equ	SBC_STX_FS,	0x00010000	| Subcodes for STonX-fs
	.equ	SBC_STX_FS_DEV,	0x00010100	| Subcodes for STonX-fs dev
	.equ	SBC_STX_SER,	0x00010200	| Subcodes for STonX-fs Serial
	.equ	SBC_STX_COM,	0x00010300	| Subcodes for STonX-fs communi
	.equ    SBC_STX_BYPASS, 0x00010400      | Subcodes for STonX Bypass
	.equ	SBC_STX_SHTDWN,	0x00008000	| Subcodes for STonX Shutdown
	
begin:	.dc.b	'b','e','g','i','n',':',0,0
shutdown:				| L _cdecl shutdown(UW mode)
	move.w	4(A7),D0
	move.w	D0,-(A7)
	moveq.l	#-32,D0			| if call, does not work correct
	.dc.w	OPC_NATIVE
	.dc.l	SBC_STX_SHTDWN
	addq.l	#4,A7
	rts
	.dc.b	':','e','n','d'
end:	
