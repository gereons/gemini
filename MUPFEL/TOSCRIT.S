; toscrit.s
; Critical-Error Handler fuer Programme ohne Maus!
; 30.09.89

	.globl	toscritic
	
	.data

;	.dc.b     "@(#)D:\TC\MUPFEL\TOSCRIT.S, compiled 30 Sep 1989",0
	
	.text

toscritic:
	lea		Message,a0
	bsr		PrintIt
	move.l	#$20002,-(sp)
	trap	#13				; Bconin
	addq.l	#4,sp
	and.w	#$5f,d0
	cmp.b	#'A',d0
	beq		IstA
	cmp.b	#'R',d0
	beq		IstR
	cmp.b	#'I',d0
	bne		toscritic
	
	clr.l	d0
	rts
	
IstA:
	move.w	4(sp),d0
	ext.l	d0
	rts

IstR:
	moveq	#1,d0
	swap	d0
	rts
	
PrintIt:
	clr.l	d0
	move.b	(a0)+,d0
	tst.b	d0
	beq		endprint
	
	move.l	a0,-(sp)
	move.w	d0,-(sp)
	move.w	#$2,-(sp)
	move.w	#$3,-(sp)
	trap	#13
	addq.l	#6,sp
	move.l	(sp)+,a0
	bra		PrintIt
endprint:
	rts
	
	.data
	
Message:
	.dc.b	13,10,"BIOS-Error: ",27,"pA",27,"qbort, ",27,"pR",27,"qetry, "
	.dc.b	"or ",27,"pI",27,"qgnore?",0
