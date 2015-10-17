global _start
 
section .data
	a: dq 1111111122222222h
	b: dq 2222222233333333h
	c: dq 3333333344444444h
	d: dq 4444444455555555h
	e: dq 5555555566666666h
	f: dq 6666666677777777h
	g: dq 7777777788888888h
	h: dq 8888888811111111h
 
section .text
	_start:
	movq mm0,[a]
	movq mm1,[b]
	movq mm2,[c]
	movq mm3,[d]
	movq mm4,[e]
	movq mm5,[f]
	movq mm6,[g]
	movq mm7,[h]
	ret