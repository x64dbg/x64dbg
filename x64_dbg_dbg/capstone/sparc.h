#ifndef CAPSTONE_SPARC_H
#define CAPSTONE_SPARC_H

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2014 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "platform.h"

// GCC SPARC toolchain has a default macro called "sparc" which breaks
// compilation
#undef sparc

#ifdef _MSC_VER
#pragma warning(disable:4201)
#endif

//> Enums corresponding to Sparc condition codes, both icc's and fcc's.
typedef enum sparc_cc
{
    SPARC_CC_INVALID = 0,   // invalid CC (default)
    //> Integer condition codes
    SPARC_CC_ICC_A   =  8 + 256, // Always
    SPARC_CC_ICC_N   =  0 + 256, // Never
    SPARC_CC_ICC_NE  =  9 + 256, // Not Equal
    SPARC_CC_ICC_E   =  1 + 256, // Equal
    SPARC_CC_ICC_G   = 10 + 256, // Greater
    SPARC_CC_ICC_LE  =  2 + 256, // Less or Equal
    SPARC_CC_ICC_GE  = 11 + 256, // Greater or Equal
    SPARC_CC_ICC_L   =  3 + 256, // Less
    SPARC_CC_ICC_GU  = 12 + 256, // Greater Unsigned
    SPARC_CC_ICC_LEU =  4 + 256, // Less or Equal Unsigned
    SPARC_CC_ICC_CC  = 13 + 256, // Carry Clear/Great or Equal Unsigned
    SPARC_CC_ICC_CS  =  5 + 256, // Carry Set/Less Unsigned
    SPARC_CC_ICC_POS = 14 + 256, // Positive
    SPARC_CC_ICC_NEG =  6 + 256, // Negative
    SPARC_CC_ICC_VC  = 15 + 256, // Overflow Clear
    SPARC_CC_ICC_VS  =  7 + 256, // Overflow Set

    //> Floating condition codes
    SPARC_CC_FCC_A   =  8 + 16 + 256, // Always
    SPARC_CC_FCC_N   =  0 + 16 + 256, // Never
    SPARC_CC_FCC_U   =  7 + 16 + 256, // Unordered
    SPARC_CC_FCC_G   =  6 + 16 + 256, // Greater
    SPARC_CC_FCC_UG  =  5 + 16 + 256, // Unordered or Greater
    SPARC_CC_FCC_L   =  4 + 16 + 256, // Less
    SPARC_CC_FCC_UL  =  3 + 16 + 256, // Unordered or Less
    SPARC_CC_FCC_LG  =  2 + 16 + 256, // Less or Greater
    SPARC_CC_FCC_NE  =  1 + 16 + 256, // Not Equal
    SPARC_CC_FCC_E   =  9 + 16 + 256, // Equal
    SPARC_CC_FCC_UE  = 10 + 16 + 256, // Unordered or Equal
    SPARC_CC_FCC_GE  = 11 + 16 + 256, // Greater or Equal
    SPARC_CC_FCC_UGE = 12 + 16 + 256, // Unordered or Greater or Equal
    SPARC_CC_FCC_LE  = 13 + 16 + 256, // Less or Equal
    SPARC_CC_FCC_ULE = 14 + 16 + 256, // Unordered or Less or Equal
    SPARC_CC_FCC_O   = 15 + 16 + 256, // Ordered
} sparc_cc;

//> Branch hint
typedef enum sparc_hint
{
    SPARC_HINT_INVALID = 0, // no hint
    SPARC_HINT_A    = 1 << 0,   // annul delay slot instruction
    SPARC_HINT_PT   = 1 << 1,   // branch taken
    SPARC_HINT_PN   = 1 << 2,   // branch NOT taken
} sparc_hint;

//> Operand type for instruction's operands
typedef enum sparc_op_type
{
    SPARC_OP_INVALID = 0, // = CS_OP_INVALID (Uninitialized).
    SPARC_OP_REG, // = CS_OP_REG (Register operand).
    SPARC_OP_IMM, // = CS_OP_IMM (Immediate operand).
    SPARC_OP_MEM, // = CS_OP_MEM (Memory operand).
} sparc_op_type;

// Instruction's operand referring to memory
// This is associated with SPARC_OP_MEM operand type above
typedef struct sparc_op_mem
{
    uint8_t base;   // base register
    uint8_t index;  // index register
    int32_t disp;   // displacement/offset value
} sparc_op_mem;

// Instruction operand
typedef struct cs_sparc_op
{
    sparc_op_type type; // operand type
    union
    {
        unsigned int reg;   // register value for REG operand
        int32_t imm;        // immediate value for IMM operand
        sparc_op_mem mem;       // base/disp value for MEM operand
    };
} cs_sparc_op;

// Instruction structure
typedef struct cs_sparc
{
    sparc_cc cc;    // code condition for this insn
    sparc_hint hint;    // branch hint: encoding as bitwise OR of sparc_hint.
    // Number of operands of this instruction,
    // or 0 when instruction has no operand.
    uint8_t op_count;
    cs_sparc_op operands[4]; // operands for this instruction.
} cs_sparc;

//> SPARC registers
typedef enum sparc_reg
{
    SPARC_REG_INVALID = 0,

    SPARC_REG_F0,
    SPARC_REG_F1,
    SPARC_REG_F2,
    SPARC_REG_F3,
    SPARC_REG_F4,
    SPARC_REG_F5,
    SPARC_REG_F6,
    SPARC_REG_F7,
    SPARC_REG_F8,
    SPARC_REG_F9,
    SPARC_REG_F10,
    SPARC_REG_F11,
    SPARC_REG_F12,
    SPARC_REG_F13,
    SPARC_REG_F14,
    SPARC_REG_F15,
    SPARC_REG_F16,
    SPARC_REG_F17,
    SPARC_REG_F18,
    SPARC_REG_F19,
    SPARC_REG_F20,
    SPARC_REG_F21,
    SPARC_REG_F22,
    SPARC_REG_F23,
    SPARC_REG_F24,
    SPARC_REG_F25,
    SPARC_REG_F26,
    SPARC_REG_F27,
    SPARC_REG_F28,
    SPARC_REG_F29,
    SPARC_REG_F30,
    SPARC_REG_F31,
    SPARC_REG_F32,
    SPARC_REG_F34,
    SPARC_REG_F36,
    SPARC_REG_F38,
    SPARC_REG_F40,
    SPARC_REG_F42,
    SPARC_REG_F44,
    SPARC_REG_F46,
    SPARC_REG_F48,
    SPARC_REG_F50,
    SPARC_REG_F52,
    SPARC_REG_F54,
    SPARC_REG_F56,
    SPARC_REG_F58,
    SPARC_REG_F60,
    SPARC_REG_F62,
    SPARC_REG_FCC0, // Floating condition codes
    SPARC_REG_FCC1,
    SPARC_REG_FCC2,
    SPARC_REG_FCC3,
    SPARC_REG_FP,
    SPARC_REG_G0,
    SPARC_REG_G1,
    SPARC_REG_G2,
    SPARC_REG_G3,
    SPARC_REG_G4,
    SPARC_REG_G5,
    SPARC_REG_G6,
    SPARC_REG_G7,
    SPARC_REG_I0,
    SPARC_REG_I1,
    SPARC_REG_I2,
    SPARC_REG_I3,
    SPARC_REG_I4,
    SPARC_REG_I5,
    SPARC_REG_I7,
    SPARC_REG_ICC,  // Integer condition codes
    SPARC_REG_L0,
    SPARC_REG_L1,
    SPARC_REG_L2,
    SPARC_REG_L3,
    SPARC_REG_L4,
    SPARC_REG_L5,
    SPARC_REG_L6,
    SPARC_REG_L7,
    SPARC_REG_O0,
    SPARC_REG_O1,
    SPARC_REG_O2,
    SPARC_REG_O3,
    SPARC_REG_O4,
    SPARC_REG_O5,
    SPARC_REG_O7,
    SPARC_REG_SP,
    SPARC_REG_Y,

    // special register
    SPARC_REG_XCC,

    SPARC_REG_ENDING,   // <-- mark the end of the list of registers

    // extras
    SPARC_REG_O6 = SPARC_REG_SP,
    SPARC_REG_I6 = SPARC_REG_FP,
} sparc_reg;

//> SPARC instruction
typedef enum sparc_insn
{
    SPARC_INS_INVALID = 0,

    SPARC_INS_ADDCC,
    SPARC_INS_ADDX,
    SPARC_INS_ADDXCC,
    SPARC_INS_ADDXC,
    SPARC_INS_ADDXCCC,
    SPARC_INS_ADD,
    SPARC_INS_ALIGNADDR,
    SPARC_INS_ALIGNADDRL,
    SPARC_INS_ANDCC,
    SPARC_INS_ANDNCC,
    SPARC_INS_ANDN,
    SPARC_INS_AND,
    SPARC_INS_ARRAY16,
    SPARC_INS_ARRAY32,
    SPARC_INS_ARRAY8,
    SPARC_INS_B,
    SPARC_INS_JMP,
    SPARC_INS_BMASK,
    SPARC_INS_FB,
    SPARC_INS_BRGEZ,
    SPARC_INS_BRGZ,
    SPARC_INS_BRLEZ,
    SPARC_INS_BRLZ,
    SPARC_INS_BRNZ,
    SPARC_INS_BRZ,
    SPARC_INS_BSHUFFLE,
    SPARC_INS_CALL,
    SPARC_INS_CASX,
    SPARC_INS_CAS,
    SPARC_INS_CMASK16,
    SPARC_INS_CMASK32,
    SPARC_INS_CMASK8,
    SPARC_INS_CMP,
    SPARC_INS_EDGE16,
    SPARC_INS_EDGE16L,
    SPARC_INS_EDGE16LN,
    SPARC_INS_EDGE16N,
    SPARC_INS_EDGE32,
    SPARC_INS_EDGE32L,
    SPARC_INS_EDGE32LN,
    SPARC_INS_EDGE32N,
    SPARC_INS_EDGE8,
    SPARC_INS_EDGE8L,
    SPARC_INS_EDGE8LN,
    SPARC_INS_EDGE8N,
    SPARC_INS_FABSD,
    SPARC_INS_FABSQ,
    SPARC_INS_FABSS,
    SPARC_INS_FADDD,
    SPARC_INS_FADDQ,
    SPARC_INS_FADDS,
    SPARC_INS_FALIGNDATA,
    SPARC_INS_FAND,
    SPARC_INS_FANDNOT1,
    SPARC_INS_FANDNOT1S,
    SPARC_INS_FANDNOT2,
    SPARC_INS_FANDNOT2S,
    SPARC_INS_FANDS,
    SPARC_INS_FCHKSM16,
    SPARC_INS_FCMPD,
    SPARC_INS_FCMPEQ16,
    SPARC_INS_FCMPEQ32,
    SPARC_INS_FCMPGT16,
    SPARC_INS_FCMPGT32,
    SPARC_INS_FCMPLE16,
    SPARC_INS_FCMPLE32,
    SPARC_INS_FCMPNE16,
    SPARC_INS_FCMPNE32,
    SPARC_INS_FCMPQ,
    SPARC_INS_FCMPS,
    SPARC_INS_FDIVD,
    SPARC_INS_FDIVQ,
    SPARC_INS_FDIVS,
    SPARC_INS_FDMULQ,
    SPARC_INS_FDTOI,
    SPARC_INS_FDTOQ,
    SPARC_INS_FDTOS,
    SPARC_INS_FDTOX,
    SPARC_INS_FEXPAND,
    SPARC_INS_FHADDD,
    SPARC_INS_FHADDS,
    SPARC_INS_FHSUBD,
    SPARC_INS_FHSUBS,
    SPARC_INS_FITOD,
    SPARC_INS_FITOQ,
    SPARC_INS_FITOS,
    SPARC_INS_FLCMPD,
    SPARC_INS_FLCMPS,
    SPARC_INS_FLUSHW,
    SPARC_INS_FMEAN16,
    SPARC_INS_FMOVD,
    SPARC_INS_FMOVQ,
    SPARC_INS_FMOVRDGEZ,
    SPARC_INS_FMOVRQGEZ,
    SPARC_INS_FMOVRSGEZ,
    SPARC_INS_FMOVRDGZ,
    SPARC_INS_FMOVRQGZ,
    SPARC_INS_FMOVRSGZ,
    SPARC_INS_FMOVRDLEZ,
    SPARC_INS_FMOVRQLEZ,
    SPARC_INS_FMOVRSLEZ,
    SPARC_INS_FMOVRDLZ,
    SPARC_INS_FMOVRQLZ,
    SPARC_INS_FMOVRSLZ,
    SPARC_INS_FMOVRDNZ,
    SPARC_INS_FMOVRQNZ,
    SPARC_INS_FMOVRSNZ,
    SPARC_INS_FMOVRDZ,
    SPARC_INS_FMOVRQZ,
    SPARC_INS_FMOVRSZ,
    SPARC_INS_FMOVS,
    SPARC_INS_FMUL8SUX16,
    SPARC_INS_FMUL8ULX16,
    SPARC_INS_FMUL8X16,
    SPARC_INS_FMUL8X16AL,
    SPARC_INS_FMUL8X16AU,
    SPARC_INS_FMULD,
    SPARC_INS_FMULD8SUX16,
    SPARC_INS_FMULD8ULX16,
    SPARC_INS_FMULQ,
    SPARC_INS_FMULS,
    SPARC_INS_FNADDD,
    SPARC_INS_FNADDS,
    SPARC_INS_FNAND,
    SPARC_INS_FNANDS,
    SPARC_INS_FNEGD,
    SPARC_INS_FNEGQ,
    SPARC_INS_FNEGS,
    SPARC_INS_FNHADDD,
    SPARC_INS_FNHADDS,
    SPARC_INS_FNOR,
    SPARC_INS_FNORS,
    SPARC_INS_FNOT1,
    SPARC_INS_FNOT1S,
    SPARC_INS_FNOT2,
    SPARC_INS_FNOT2S,
    SPARC_INS_FONE,
    SPARC_INS_FONES,
    SPARC_INS_FOR,
    SPARC_INS_FORNOT1,
    SPARC_INS_FORNOT1S,
    SPARC_INS_FORNOT2,
    SPARC_INS_FORNOT2S,
    SPARC_INS_FORS,
    SPARC_INS_FPACK16,
    SPARC_INS_FPACK32,
    SPARC_INS_FPACKFIX,
    SPARC_INS_FPADD16,
    SPARC_INS_FPADD16S,
    SPARC_INS_FPADD32,
    SPARC_INS_FPADD32S,
    SPARC_INS_FPADD64,
    SPARC_INS_FPMERGE,
    SPARC_INS_FPSUB16,
    SPARC_INS_FPSUB16S,
    SPARC_INS_FPSUB32,
    SPARC_INS_FPSUB32S,
    SPARC_INS_FQTOD,
    SPARC_INS_FQTOI,
    SPARC_INS_FQTOS,
    SPARC_INS_FQTOX,
    SPARC_INS_FSLAS16,
    SPARC_INS_FSLAS32,
    SPARC_INS_FSLL16,
    SPARC_INS_FSLL32,
    SPARC_INS_FSMULD,
    SPARC_INS_FSQRTD,
    SPARC_INS_FSQRTQ,
    SPARC_INS_FSQRTS,
    SPARC_INS_FSRA16,
    SPARC_INS_FSRA32,
    SPARC_INS_FSRC1,
    SPARC_INS_FSRC1S,
    SPARC_INS_FSRC2,
    SPARC_INS_FSRC2S,
    SPARC_INS_FSRL16,
    SPARC_INS_FSRL32,
    SPARC_INS_FSTOD,
    SPARC_INS_FSTOI,
    SPARC_INS_FSTOQ,
    SPARC_INS_FSTOX,
    SPARC_INS_FSUBD,
    SPARC_INS_FSUBQ,
    SPARC_INS_FSUBS,
    SPARC_INS_FXNOR,
    SPARC_INS_FXNORS,
    SPARC_INS_FXOR,
    SPARC_INS_FXORS,
    SPARC_INS_FXTOD,
    SPARC_INS_FXTOQ,
    SPARC_INS_FXTOS,
    SPARC_INS_FZERO,
    SPARC_INS_FZEROS,
    SPARC_INS_JMPL,
    SPARC_INS_LDD,
    SPARC_INS_LD,
    SPARC_INS_LDQ,
    SPARC_INS_LDSB,
    SPARC_INS_LDSH,
    SPARC_INS_LDSW,
    SPARC_INS_LDUB,
    SPARC_INS_LDUH,
    SPARC_INS_LDX,
    SPARC_INS_LZCNT,
    SPARC_INS_MEMBAR,
    SPARC_INS_MOVDTOX,
    SPARC_INS_MOV,
    SPARC_INS_MOVRGEZ,
    SPARC_INS_MOVRGZ,
    SPARC_INS_MOVRLEZ,
    SPARC_INS_MOVRLZ,
    SPARC_INS_MOVRNZ,
    SPARC_INS_MOVRZ,
    SPARC_INS_MOVSTOSW,
    SPARC_INS_MOVSTOUW,
    SPARC_INS_MULX,
    SPARC_INS_NOP,
    SPARC_INS_ORCC,
    SPARC_INS_ORNCC,
    SPARC_INS_ORN,
    SPARC_INS_OR,
    SPARC_INS_PDIST,
    SPARC_INS_PDISTN,
    SPARC_INS_POPC,
    SPARC_INS_RD,
    SPARC_INS_RESTORE,
    SPARC_INS_RETT,
    SPARC_INS_SAVE,
    SPARC_INS_SDIVCC,
    SPARC_INS_SDIVX,
    SPARC_INS_SDIV,
    SPARC_INS_SETHI,
    SPARC_INS_SHUTDOWN,
    SPARC_INS_SIAM,
    SPARC_INS_SLLX,
    SPARC_INS_SLL,
    SPARC_INS_SMULCC,
    SPARC_INS_SMUL,
    SPARC_INS_SRAX,
    SPARC_INS_SRA,
    SPARC_INS_SRLX,
    SPARC_INS_SRL,
    SPARC_INS_STBAR,
    SPARC_INS_STB,
    SPARC_INS_STD,
    SPARC_INS_ST,
    SPARC_INS_STH,
    SPARC_INS_STQ,
    SPARC_INS_STX,
    SPARC_INS_SUBCC,
    SPARC_INS_SUBX,
    SPARC_INS_SUBXCC,
    SPARC_INS_SUB,
    SPARC_INS_SWAP,
    SPARC_INS_TADDCCTV,
    SPARC_INS_TADDCC,
    SPARC_INS_T,
    SPARC_INS_TSUBCCTV,
    SPARC_INS_TSUBCC,
    SPARC_INS_UDIVCC,
    SPARC_INS_UDIVX,
    SPARC_INS_UDIV,
    SPARC_INS_UMULCC,
    SPARC_INS_UMULXHI,
    SPARC_INS_UMUL,
    SPARC_INS_UNIMP,
    SPARC_INS_FCMPED,
    SPARC_INS_FCMPEQ,
    SPARC_INS_FCMPES,
    SPARC_INS_WR,
    SPARC_INS_XMULX,
    SPARC_INS_XMULXHI,
    SPARC_INS_XNORCC,
    SPARC_INS_XNOR,
    SPARC_INS_XORCC,
    SPARC_INS_XOR,

    // alias instructions
    SPARC_INS_RET,
    SPARC_INS_RETL,

    SPARC_INS_ENDING,   // <-- mark the end of the list of instructions
} sparc_insn;

//> Group of SPARC instructions
typedef enum sparc_insn_group
{
    SPARC_GRP_INVALID = 0, // = CS_GRP_INVALID

    //> Generic groups
    // all jump instructions (conditional+direct+indirect jumps)
    SPARC_GRP_JUMP, // = CS_GRP_JUMP

    //> Architecture-specific groups
    SPARC_GRP_HARDQUAD = 128,
    SPARC_GRP_V9,
    SPARC_GRP_VIS,
    SPARC_GRP_VIS2,
    SPARC_GRP_VIS3,
    SPARC_GRP_32BIT,
    SPARC_GRP_64BIT,

    SPARC_GRP_ENDING,   // <-- mark the end of the list of groups
} sparc_insn_group;

#ifdef __cplusplus
}
#endif

#endif
