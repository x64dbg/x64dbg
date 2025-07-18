EXTERN inspect_state:PROC

.CODE

PUBLIC test_avx512

test_avx512 PROC
    xor rax, rax
    not rax
    kmovq k0, rax
    vmovdqa64 zmm0, zmmword ptr [pattern_all_01]
    vmovdqa64 zmm1, zmmword ptr [pattern_all_ones]
    vmovdqa64 zmm2, zmmword ptr [pattern_inc_byte]
    vmovdqa64 zmm3, zmmword ptr [pattern_inc_dword]
    vmovdqa64 zmm4, zmmword ptr [pattern_checker_55]
    vmovdqa64 zmm5, zmmword ptr [pattern_checker_AA]
    vmovdqa64 zmm6, zmmword ptr [pattern_floats]
    vmovdqa64 zmm7, zmmword ptr [pattern_ascii]
    vmovdqa64 zmm8, zmmword ptr [pattern_all_01]
    vmovdqa64 zmm9, zmmword ptr [pattern_all_ones]
    vmovdqa64 zmm10, zmmword ptr [pattern_inc_byte]
    vmovdqa64 zmm11, zmmword ptr [pattern_inc_dword]
    vmovdqa64 zmm12, zmmword ptr [pattern_checker_55]
    vmovdqa64 zmm13, zmmword ptr [pattern_checker_AA]
    vmovdqa64 zmm14, zmmword ptr [pattern_floats]
    vmovdqa64 zmm15, zmmword ptr [pattern_ascii]
    vmovdqa64 zmm16, zmmword ptr [pattern_all_01]
    vmovdqa64 zmm17, zmmword ptr [pattern_all_ones]
    vmovdqa64 zmm18, zmmword ptr [pattern_inc_byte]
    vmovdqa64 zmm19, zmmword ptr [pattern_inc_dword]
    vmovdqa64 zmm20, zmmword ptr [pattern_checker_55]
    vmovdqa64 zmm21, zmmword ptr [pattern_checker_AA]
    vmovdqa64 zmm22, zmmword ptr [pattern_floats]
    vmovdqa64 zmm23, zmmword ptr [pattern_ascii]
    vmovdqa64 zmm24, zmmword ptr [pattern_all_01]
    vmovdqa64 zmm25, zmmword ptr [pattern_all_ones]
    vmovdqa64 zmm26, zmmword ptr [pattern_inc_byte]
    vmovdqa64 zmm27, zmmword ptr [pattern_inc_dword]
    vmovdqa64 zmm28, zmmword ptr [pattern_checker_55]
    vmovdqa64 zmm29, zmmword ptr [pattern_checker_AA]
    vmovdqa64 zmm30, zmmword ptr [pattern_floats]
    vmovdqa64 zmm31, zmmword ptr [pattern_ascii]
    call inspect_state
    ret

test_avx512 ENDP

.DATA

pattern_all_01 DB 64 DUP(01h)

pattern_all_ones DB 64 DUP(0FFh)

pattern_inc_byte DB 00h, 01h, 02h, 03h, 04h, 05h, 06h, 07h, 08h, 09h, 0Ah, 0Bh, 0Ch, 0Dh, 0Eh, 0Fh
                 DB 10h, 11h, 12h, 13h, 14h, 15h, 16h, 17h, 18h, 19h, 1Ah, 1Bh, 1Ch, 1Dh, 1Eh, 1Fh
                 DB 20h, 21h, 22h, 23h, 24h, 25h, 26h, 27h, 28h, 29h, 2Ah, 2Bh, 2Ch, 2Dh, 2Eh, 2Fh
                 DB 30h, 31h, 32h, 33h, 34h, 35h, 36h, 37h, 38h, 39h, 3Ah, 3Bh, 3Ch, 3Dh, 3Eh, 3Fh

pattern_inc_dword DD 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15

pattern_checker_55 DB 64 DUP(55h)

pattern_checker_AA DB 64 DUP(0AAh)

pattern_floats REAL4 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0

pattern_ascii DB 'This is a recognizable ASCII pattern for the ZMM register demo!!'

END