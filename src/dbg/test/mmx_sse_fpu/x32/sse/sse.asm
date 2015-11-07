global _start
 
section .data
        align 16
        v1:     dd 1.1, 2.2, 3.3, 4.4
        v2:     dd 5.5, 6.6, 7.7, 8.8
        v3:     dd 9.9, 10.10, 11.11, 12.12
        v4:     dd 13.13, 14.14, 15.15, 16.16
        v5:     dd 17.17, 18.18, 19.19, 20.20
        v6:     dd 21.21, 22.22, 23.23, 24.24
        v7:     dd 25.25, 26.26, 27.27, 28.28
        v8:     dd 29.29, 30.30, 31.31, 32.32
 
section .bss
        mask1:  resd 1
        mask2:  resd 1
        mask3:  resd 1
        mask4:  resd 1
 
section .text
        _start:
        movaps  xmm0, [v1]
        movaps  xmm1, [v2]
        movaps  xmm2, [v3]
        movaps  xmm3, [v4]
        movaps  xmm4, [v5]
        movaps  xmm5, [v6]
        movaps  xmm6, [v7]
        movaps  xmm7, [v8]
	ret