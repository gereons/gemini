;
; shell.s  -  Interface for _shell_p
; 22.05.90
;

	.import	doshellcall
	.export	shellcall
	.export	_xresetid, initreset, exitreset
	.export	doreboot
	.data
	
;	dc.w		'@(#)D:\TC\MUPFEL\SHELL.S, compiled 04 Dec 1989',0

_shellcount:	dc.l		0
_command:		ds.l		1

_mystack:		ds.l		32			; our temp stack
_oldstack:	ds.l		32			; caller's saved SP

	.text

shellcall:
	move.l	4(a7),_command			; get command line
	movem.l	d1-d7/a0-a6,-(a7)		; save all registers

	move.l	_shellcount,d2			; index into array
	move.l	#_oldstack,a3			; base of _oldstack array
	add.l	d2,a3				; add offset
	move.l	a7,(a3)				; remember old SP

	move.l	#8192,-(a7)			; get 8K
	move.w	#$48,-(a7)			; Malloc
	trap		#1					; Gemdos
	addq.l	#6,a7				; fix stack

	move.w	#-1,d7				; prepare for catastrophe

	move.l	_shellcount,d2			; index into array
	move.l	#_mystack,a3			; base of _mystack array
	add.l	d2,a3				; add offset
	move.l	d0,(a3)				; remember stack
		
	beq		nostack				; Catastrophe!
	
	move.l	_command,a0			; get commandline
	add.l	#8188,d0				; new stack address
	move.l	d0,a7				; install it

	addq.l	#4,_shellcount			; increment index
	jsr		doshellcall			; call mupfel
	subq.l	#4,_shellcount			; decrement index

	move.w	d0,d7				; save return code

	move.l	_shellcount,d2			; index into array
	move.l	#_oldstack,a3			; base of _oldstack array
	add.l	d2,a3				; add offset

	move.l	(a3),a7				; restore old stack

	move.l	#_mystack,a3			; base of _mystack array
	add.l	d2,a3				; add offset

	move.l	(a3),-(a7)			; our temp stack address
	move.w	#$49,-(a7)			; Mfree
	trap		#1					; Gemdos
	addq.l	#6,a7				; fix stack

nostack:
	move.l	d7,d0				; restore return code
	movem.l	(a7)+,d1-d7/a0-a6		; restore registers
	rts							; done


_resvalid		equ	$426
_resvector	equ	$42a
_shell_p		equ	$4f6
RESMAGIC		equ	$31415926

initreset:
	move.l	_resvalid,valsave
	move.l	_resvector,vecsave
	move.l	#clearshell,_resvector
	move.l	#RESMAGIC,_resvalid
	rts
	
exitreset:
	move.l	valsave,_resvalid
	move.l	vecsave,_resvector
	rts

	dc.b		'XBRA'
_xresetid:
	dc.b		'MUPF'
vecsave:
	dc.l		0
clearshell:
	clr.l	_shell_p
	move.l	vecsave,_resvector
	move.l	valsave,_resvalid
	jmp		(a6)

doreboot:
	movea.l	0,sp
	movea.l	4,a0
	jmp		(a0)
	
	.bss
	
valsave:
	ds.l		1
	
	
; trickstack:	ds.b	16384