global _start
 
section .data
@real:
	mov byte [l1],0b0h
	l1:	mov al,1
	ret
	
section .text
	_start:
	jmp @real