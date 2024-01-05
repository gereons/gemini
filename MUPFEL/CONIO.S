;
; conio.s  -  interface for "BIOS in Windows"
; also contains a weird hack to set some Line A vars
; 17.03.90
;

		.export	conout, rconout
		.export	LAinit, storeLAcursor
		.export _vdiesc
		.import	disp_canchar		; Vector for conout
		.import	disp_rawchar		; Vector for rconout

		.data
	
;		dc.w		'@(#)D:\TC\MUPFEL\CONIO.S, compiled 11 Mar 1990',0

		.text
		
conout:	movem.l	d0-a6,_savereg
		move.w	6(sp),d0
		jsr		disp_canchar
		movem.l	_savereg,d0-a6
		rts

rconout:	movem.l	d0-a6,_savereg
		move.w	6(sp),d0
		jsr		disp_rawchar
		movem.l	_savereg,d0-a6
		rts

; init LineA.
; called once from mupfel.c by mupfelinit()
LAinit:
		movem.l	d0-a6,-(sp)		; save regs
		dc.w	$a000				; LineA-init
		move.l	a0,_vdiesc		; save pointer to vdiesc
		movem.l	(sp)+,d0-a6		; restore regs
		rts						; bye-bye

; void storeLAcursor(int x /* d0 */, int y /* d1 */)
; store cursor position in V_CUR_XY
storeLAcursor:
		move.l	a0,-(sp)
		move.l	_vdiesc,a0
		move.w	d0,-$1c(a0)	; V_CUR_XY[0]
		move.w	d1,-$1a(a0)	; V_CUR_XY[1]
		move.l	(sp)+,a0
		rts
		
		.bss

_savereg:	ds.l		32
_vdiesc:	ds.l		1
