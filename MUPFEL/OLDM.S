

********
mediach.s
********
	.globl	mediach
	
mediach:
			move.w	4(sp),d0
			movem.l	d1-a6,-(a7)
			move.w	d0,mydev
			add.b	#'A',d0
			move.b	d0,fspec	; set drive for search first
			
loop:		clr.l	-(sp)		; get super mode, leave old ssp
			move.w	#$20,-(sp)
			trap	#1
			addq	#6,sp
			move.l	d0,-(sp)
			move.w	#$20,-(sp)
			
			move.l	$472,oldgetbpb
			move.l	$47e,oldmediach
			move.l	$476,oldrwabs
			
			move.l	#newgetbpb,$472
			move.l	#newmediach,$47e
			move.l	#newrwabs,$476
			
			; Fopen a file on that drive
			
			clr.w	-(sp)
			move.l	#fspec,-(sp)
			move.w	#$3d,-(sp)
			trap	#1
			addq	#8,sp
			
			; Fclose the handle we just got
			
			tst.l	d0
			bmi		noclose
			
			move.w	d0,-(sp)
			move.w	#$3e,-(sp)
			trap	#1
			addq	#4,sp

noclose:
			moveq	#0,d7
			cmp.l	#newgetbpb,$472
			bne		done
			
			moveq	#1,d7
			move.l	oldgetbpb,$472
			move.l	#0,oldgetbpb
			move.l	oldmediach,$47e
			move.l	#0,oldmediach
			move.l	oldrwabs,$476
			move.l	#0,oldrwabs
			
done:		trap	#1		; go back to user mode
			addq	#6,sp
			
			move.l	d7,d0
			movem.l	(a7)+,d1-a6
			rts
			
; newgetbpb: if its our device, uninstall vectors, in any case,
; call the old getbpb vector to really get it

			.dc.b	"XBRAMDCH"
oldgetbpb:	.dc.l	0

newgetbpb:
			move.l	oldgetbpb,a0
			move.w	mydev,d0
			cmp.w	4(sp),d0
			bne		dooldg
			
			move.l	oldgetbpb,$472
			move.l	#0,oldgetbpb
			move.l	oldmediach,$47e
			move.l	#0,oldmediach
			move.l	oldrwabs,$476
			move.l	#0,oldrwabs

dooldg:		jmp		(a0)
			
			

; new mediach: if it's our device, return 2: else call old

			.dc.b	"XBRAMDCH"
oldmediach:	.dc.l	0

newmediach:
			move.w	mydev,d0
			cmp.w	4(sp),d0
			bne		dooldm
			moveq.l	#2,d0	; definitively changed
			rts

dooldm:		move.l	oldmediach,a0
			jmp		(a0)
			
			
; newrwabs: return E_CHG (-14) if it's my device

			.dc.b	"XBRAMDCH"
oldrwabs:	.dc.l	0

newrwabs:	move.w	mydev,d0
			cmp.w	#14,d0
			bne		dooldr
			moveq.l	#-14,d0
			rts

dooldr:		move.l	oldrwabs,a0
			jmp		(a0)
			
			.data

fspec:		dc.b	"X:\X",0

			.bss
			
mydev:		.ds.w	1

			.end