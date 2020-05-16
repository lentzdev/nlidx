; ============================================================ Rev. 17 Jan 1993
; Routines for table driven CRC-16 & CRC-32, including building tables
; Refer to CRC.DOC for information and documentation.
; This file in 80x86 ASM is 100% interchangable with the portable CRC.C
; Initially written to see how CRC calculation could be optimized in assembly
; Reads the data in words instead bytes at a time, makes asm even 20-25% faster
; Tried specifics for 386 and up (SHR EAX,8 etc), but didn't wasn't much faster
; Source compiles cleanly in TASM. May require some minor adjustments for MASM.
; -----------------------------------------------------------------------------
; 
;	       Information collected and edited by Arjen G. Lentz
;	     Sourcecode in C and 80x86 ASM written by Arjen G. Lentz
;		  COPYRIGHT (C) 1992-1993; ALL RIGHTS RESERVED
;  
; 
; CONTACT ADDRESS
;  
; LENTZ SOFTWARE-DEVELOPMENT	Arjen Lentz @
; Langegracht 7B		AINEX-BBS +31-33-633916
; 3811 BT  Amersfoort		FidoNet 2:283/512
; The Netherlands		arjen_lentz@f512.n283.z2.fidonet.org
;  
;  
; DISCLAIMER
;  
; This information is provided "as is" and comes with no warranties of any
; kind, either expressed or implied. It's intended to be used by programmers
; and developers. In no event shall the author be liable to you or anyone
; else for any damages, including any lost profits, lost savings or other
; incidental or consequential damages arising out of the use or inability
; to use this information.
;  
;  
; LICENCE
;  
; This package may be freely distributed provided the files remain together,
; in their original unmodified form.
; All files, executables and sourcecode remain the copyrighted property of
; Arjen G. Lentz and LENTZ SOFTWARE-DEVELOPMENT.
; Licence for any use granted, provided this notice & CRC.DOC are included.
; For executable applications, credit should be given in the appropriate
; place in the program and documentation.
; These notices must be retained in any copies of any part of this
; documentation and/or software.
;
; Any use of, or operation on (including copying/distributing) any of
; the above mentioned files implies full and unconditional acceptance of
; this license and disclaimer.
;
; =============================================================================

; Make sure you set the model and language right, or your program will crash!
       .MODEL Large,C  ; Model	 : Tiny,Small,Medium,Compact,Large,Huge
		       ; Language: C,Pascal,Basic,Fortran,Prolog
IFDEF MODL
	  MODEL MODL,C
ELSEIFDEF _SMALL
	  MODEL SMALL,C
ELSEIFDEF _LARGE
	  MODEL LARGE,C
ENDIF


; -----------------------------------------------------------------------------
	.CODE
	LOCALS

	PUBLIC crc16init,  crc16block
	PUBLIC crc16rinit, crc16rblock
	PUBLIC crc32init,  crc32block


; -----------------------------------------------------------------------------
crc16init PROC
	ARG	crctab:DWORD
	ARG	poly:WORD
	USES	si,di

	sub	si,si			; i = 0
	mov	bx,[poly]		; BX = polynomial
	les	di,[crctab]		; ES:DI = crctab
	cld

@@itop: mov	ax,si			; crc = i
	mov	cx,8			; j = 8
@@jtop: shr	ax,1			; crc >>= 1
	jnc	@@jbit			; if (!carry)
	xor	ax,bx			;    crc ^= poly
@@jbit: loopnz	@@jtop			; if (--CX != 0 && crc) goto @@jtop
	stosw				; crctab[i] = crc

	inc	si
	cmp	si,255			; if (++i <= 255)
	jle	@@itop			;    goto @@itop

	ret
crc16init ENDP


; -----------------------------------------------------------------------------
crc16block PROC
	ARG	crctab:DWORD
	ARG	crc:WORD
	ARG	buf:DWORD
	ARG	len:WORD
	USES	ds,si,di

	mov	bx,[crc]		; BX = crc
	mov	cx,[len]		; CX = len
	lds	si,[buf]		; DS:SI = buf
	les	di,[crctab]		; ES:DI = crctab
	cld

	shr	cx,1
	jnc	@@even

	lodsb				; get single byte
	xor	bl,al			; BL ^= AL
	mov	ah,bh			; AH = BH
	sub	bh,bh			; BH = 0
	shl	bx,1			; BX *= 2 (mul for word array)
	mov	bx,[es:di + bx] 	; BX = crctab[BX]
	xor	bl,ah			; BL ^= AH

@@even: jcxz	@@fini			; if (!len) goto @@fini

@@itop: lodsw				; get next two bytes
	xor	bl,al			; first byte
	xor	ah,bh			; already merge with second byte
	sub	bh,bh
	shl	bx,1
	mov	bx,[es:di + bx]
	xor	bl,ah			; second byte
	mov	ah,bh
	sub	bh,bh
	shl	bx,1
	mov	bx,[es:di + bx]
	xor	bl,ah

	loop	@@itop			; if (--CX != 0) goto @@itop

@@fini: mov	ax,bx
	ret
crc16block ENDP


; -----------------------------------------------------------------------------
crc16rinit PROC
	ARG	crctab:DWORD
	ARG	poly:WORD
	USES	si,di

	sub	si,si			; i = 0
	mov	bx,[poly]		; BX = polynomial
	les	di,[crctab]		; ES:DI = crctab
	cld

@@itop: mov	ax,si			; crc = i << 8
	xchg	ah,al
	mov	cx,8			; j = 8
@@jtop: shl	ax,1			; crc <<= 1
	jnc	@@jbit			; if (!carry)
	xor	ax,bx			;    crc ^= poly
@@jbit: loopnz	@@jtop			; if (--CX != 0 && crc) goto @@jtop
	stosw				; crctab[i] = crc

	inc	si
	cmp	si,255			; if (++i <= 255)
	jle	@@itop			;    goto @@itop

	ret
crc16rinit ENDP


; -----------------------------------------------------------------------------
crc16rblock PROC
	ARG	crctab:DWORD
	ARG	crc:WORD
	ARG	buf:DWORD
	ARG	len:WORD
	USES	ds,si,di

	mov	bx,[crc]		; BX = crc
	mov	cx,[len]		; CX = len
	lds	si,[buf]		; DS:SI = buf
	les	di,[crctab]		; ES:DI = crctab
	cld

	shr	cx,1
	jnc	@@even

	lodsb				; get single byte
	mov	ah,bl			; AH = BL
	mov	bl,bh			; BL = BH
	xor	bl,al			; BL ^= AL
	sub	bh,bh			; BH = 0
	shl	bx,1			; BX *= 2 (mul for word array)
	mov	bx,[es:di + bx] 	; BX = crctab[BX]
	xor	bh,ah			; BH ^= AH

@@even: jcxz	@@fini			; if (!len) goto @@fini

@@itop: lodsw				; get next two bytes
	xor	ah,bl			; already merge with second byte
	mov	bl,bh
	xor	bl,al			; first byte
	sub	bh,bh
	shl	bx,1
	mov	bx,[es:di + bx]
	xor	bh,ah			; second byte
	mov	ah,bl
	mov	bl,bh
	sub	bh,bh
	shl	bx,1
	mov	bx,[es:di + bx]
	xor	bh,ah

	loop	@@itop			; if (--CX != 0) goto @@itop

@@fini: mov	ax,bx
	ret
crc16rblock ENDP


; -----------------------------------------------------------------------------
crc32init PROC
	ARG	crctab:DWORD
	ARG	polyl:WORD
	ARG	polyh:WORD
	USES	si,di

	sub	si,si			; i = 0
	les	di,[crctab]		; ES:DI = crctab
	mov	bx,[polyl]		; BX = poly (low)
	mov	bp,[polyh]		; BP = poly (high)

@@itop: mov	ax,si			; crc (low)  = i
	sub	dx,dx			; crc (high) = 0
	mov	cx,8			; j = 8
@@jtop: shr	dx,1			; crc (high) >>= 1
	rcr	ax,1			; crc (low)  >>= 1
	jnc	@@jbit			; if (!carry)
	xor	ax,bx			;    crc (low)	^= polyl
	xor	dx,bp			;    crc (high) ^= polyh
@@jbit: loop	@@jtop			; if (--CX != 0) goto @@jtop
	stosw				; crctab[i] = crc (low)
	mov	ax,dx
	stosw				; crctab[i] = crc (high)

	inc	si
	cmp	si,255			; if (++i <= 255)
	jle	@@itop			;    goto @@itop

	ret
crc32init ENDP


; -----------------------------------------------------------------------------
crc32block PROC
	ARG	crc32tab:DWORD
	ARG	crcl:WORD
	ARG	crch:WORD
	ARG	buf:DWORD
	ARG	len:WORD
	USES	ds,si,di

	mov	ax,[crcl]		; AX = crc (low)
	mov	dx,[crch]		; DX = crc (high)
	mov	cx,[len]		; CX = len
	lds	si,[buf]		; DS:SI = buf
	les	di,[crctab]		; ES:DI = crctab
	cld

	shr	cx,1
	jnc	@@even

	mov	bl,al			; BL = AL
	lodsb				; get single byte
	xor	bl,al			; BL ^= AL
	mov	al,ah			; AL = AH
	mov	ah,dl			; AH = DL
	mov	dl,dh			; DL = DH
	sub	dh,dh			; DH = 0
	sub	bh,bh			; BH = 0
	shl	bx,1			; BX *= 4 (mul for dword array)
	shl	bx,1
	xor	ax,[es:di + bx] 	; AX ^= crctab[BX]
	add	bx,2			; BX += 2 (add for second word)
	xor	dx,[es:di + bx] 	; DX ^= crctab[BX]

@@even: jcxz	@@fini			; if (!len) goto @@fini

@@itop: mov	bx,ax
	lodsw				; get next to bytes
	xor	bl,al
	mov	al,bh
	xor	al,ah			; already merge with second byte
	mov	ah,dl
	mov	dl,dh
	sub	dh,dh
	sub	bh,bh
	shl	bx,1
	shl	bx,1
	xor	ax,[es:di + bx]
	add	bx,2
	xor	dx,[es:di + bx]
	mov	bl,al
	mov	al,ah
	mov	ah,dl
	mov	dl,dh
	sub	dh,dh
	sub	bh,bh
	shl	bx,1
	shl	bx,1
	xor	ax,[es:di + bx]
	add	bx,2
	xor	dx,[es:di + bx]

	loop	@@itop			; if (--CX != 0) goto @@itop

@@fini: ret
crc32block ENDP


	END

; end of crc.asm --------------------------------------------------------------
