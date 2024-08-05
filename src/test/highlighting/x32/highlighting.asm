global _start
 
section .data
@real:
	db 0
	
section .text
	_start:
	MOV EAX,DWORD [@real]
	MOV EAX,DWORD [EAX*4+@real]
	MOV EAX,DWORD [EAX+@real]
	MOV EAX,@real
	MOV EBX,DWORD [EAX]
	MOV EBX,4
	XOR EDX,EDX
	DIV EBX
	MOV EAX,DWORD [EAX*4]
	JMP SHORT @next
@next:
	RET