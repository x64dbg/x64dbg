global _start
 
section .data
@real:
	db 0
	
section .text
	_start:
	MOV RAX,QWORD [@real]
	MOV RAX,QWORD [RAX*4+@real]
	MOV RAX,QWORD [RAX+@real]
	MOV RAX,@real
	MOV RBX,QWORD [RAX]
	MOV RBX,4
	XOR RDX,RDX
	DIV RBX
	MOV RAX,QWORD [RAX*4]
	JMP SHORT @next
@next:
	RET