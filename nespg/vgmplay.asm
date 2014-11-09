.setcpu		"6502"
.autoimport	on

VGM_PTR_LSB = $00
VGM_PTR_MSB = $01
VGM_PTR_BANK = $02

WAIT_FRAMES = $03

PREV_VBLANK = $10
WRITE_ADDR = $20

_INITIAL_VGM_PTR_LSB = <vgmdata
_INITIAL_VGM_PTR_MSB = >vgmdata

; iNESヘッダ
.segment "HEADER"
	.byte	$4E, $45, $53, $1A	; "NES" Header
	.byte	$20			; PRG-BANKS
	.byte	$00			; CHR-BANKS
	.byte	$40			; Vetrical Mirror
	.byte	$00			; 
	.byte	$00, $00, $10, $00	; 
	.byte	$00, $00, $00, $00	; 

.segment "STARTUP"
; リセット割り込み
.proc	Reset

; スクリーンオフ
	lda	#$00
	sta	$2000
	sta	$2001
	lda	#$88
	sta	$2000
	
; 各種初期化
	lda #$00
	sta PREV_VBLANK
	lda #_INITIAL_VGM_PTR_LSB
	sta VGM_PTR_LSB
	lda #_INITIAL_VGM_PTR_MSB
	sta VGM_PTR_MSB
	lda #60
	sta WAIT_FRAMES
	lda #0
	sta VGM_PTR_BANK
	
	lda #$40
	sta WRITE_ADDR+1
	
	lda #$06
	sta $8000
	lda VGM_PTR_BANK
	sta $8001
	
	jmp Main
.endproc

.segment "STARTUP"
.proc	Main
mainloop:
	jmp mainloop
	; VBLANK待ち
	;lda $2002
	;cmp PREV_VBLANK
	;beq mainloop
	;sta PREV_VBLANK
	;cmp #0
	;beq mainloop
	;
@next:
	;rts
.endproc

.segment "STARTUP"
.proc	NMIMain
dec WAIT_FRAMES
	ldx WAIT_FRAMES
	beq @next
	rti
@next:
	ldy #0
	lda (VGM_PTR_LSB), y
	
@regset:
; 書き込み
	cmp #$b4
	bne @waitcmd
	
	jsr incAddr
	lda (VGM_PTR_LSB), y
	sta WRITE_ADDR
	
	jsr incAddr
	lda (VGM_PTR_LSB), y
	sta (WRITE_ADDR), y
	
	jsr incAddr
	jmp	@next
	
@waitcmd:
; 1フレームwait
	cmp #$62
	bne @waitcmd2
	jsr incAddr
	lda #1
	sta WAIT_FRAMES
	rti
	
@waitcmd2:
; ２フレーム以上のwait
	cmp #$61
	bne @endcmd
	jsr incAddr
	lda (VGM_PTR_LSB), y
	sta WAIT_FRAMES
	jsr incAddr
@endcmd:
	rti
.endproc

.proc incAddr
	pha
	inc VGM_PTR_LSB
	bne @end1
	inc VGM_PTR_MSB
	lda VGM_PTR_MSB
	AND #$20
	beq @end1
	lda #0
	sta VGM_PTR_MSB
	inc VGM_PTR_BANK
	lda #$06
	sta $8000
	lda VGM_PTR_BANK
	sta $8001
	lda #_INITIAL_VGM_PTR_MSB
	sta VGM_PTR_MSB
@end1:
	pla
	rts
.endproc

.segment "VECINFO"
	.word	NMIMain
	.word	Reset
	.word	$0000

; データ
.segment "VGMDATA"
vgmdata:
	.byte $00
	;.incbin	"famicom_0.bin"
