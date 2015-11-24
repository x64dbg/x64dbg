global _start
 
section .data
	a: dq 1.1
	b: dq 2.2
	c: dq 3.3
	d: dq 4.4
	e: dq 5.5
	f: dq 6.6
	g: dq 7.7
	h: dq 8.8
 
section .text
	_start:
	fld qword [a]
	fld qword [b]
	fld qword [c]
	fld qword [d]
	fld qword [e]
	fld qword [f]
	fld qword [g]
	fld qword [h]
	ret