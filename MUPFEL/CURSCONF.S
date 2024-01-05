		.super
		.text

		.export MyXBios, MyXBios030
		.import OldXBios
		
MyXBios:  move    usp,a0
          btst.b  #5,(a7)          ; Supervisor-Modus
          beq     _usp
          movea.l a7,a0 
          addq.w  #6,a0            ; 6 Bytes auf dem Stack 
_usp:     cmpi.w  #21,(a0)         ; Code fÅr 'cursconf'
          beq     cursconf
NormalXBios:
          move.l  OldXBios,a0      ; alten Vektor nehmen
          jmp     (a0)

MyXBios030:
          move    usp,a0
          btst.b  #5,(a7) 
          beq     usp030
          movea.l a7,a0 
          addq.w  #8,a0            ; 8 Bytes auf dem Stack
usp030:
          cmpi.w  #21,(a0) 
          beq     cursconf
          move.l  OldXBios,a0
          jmp     (a0)

cursconf:
          cmp.w   #5,2(a0)        ; getrate
          beq.b   NormalXBios
          rte                     ; Schluss
