#ifndef CAPSTONE_M68K_H
#define CAPSTONE_M68K_H

/* Capstone Disassembly Engine */
/* By Daniel Collin <daniel@collin.com>, 2015-2016 */

#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4201)
#endif

#define M68K_OPERAND_COUNT 4

//> M68K registers and special registers
typedef enum m68k_reg
{
    M68K_REG_INVALID = 0,

    M68K_REG_D0,
    M68K_REG_D1,
    M68K_REG_D2,
    M68K_REG_D3,
    M68K_REG_D4,
    M68K_REG_D5,
    M68K_REG_D6,
    M68K_REG_D7,

    M68K_REG_A0,
    M68K_REG_A1,
    M68K_REG_A2,
    M68K_REG_A3,
    M68K_REG_A4,
    M68K_REG_A5,
    M68K_REG_A6,
    M68K_REG_A7,

    M68K_REG_FP0,
    M68K_REG_FP1,
    M68K_REG_FP2,
    M68K_REG_FP3,
    M68K_REG_FP4,
    M68K_REG_FP5,
    M68K_REG_FP6,
    M68K_REG_FP7,

    M68K_REG_PC,

    M68K_REG_SR,
    M68K_REG_CCR,
    M68K_REG_SFC,
    M68K_REG_DFC,
    M68K_REG_USP,
    M68K_REG_VBR,
    M68K_REG_CACR,
    M68K_REG_CAAR,
    M68K_REG_MSP,
    M68K_REG_ISP,
    M68K_REG_TC,
    M68K_REG_ITT0,
    M68K_REG_ITT1,
    M68K_REG_DTT0,
    M68K_REG_DTT1,
    M68K_REG_MMUSR,
    M68K_REG_URP,
    M68K_REG_SRP,

    M68K_REG_FPCR,
    M68K_REG_FPSR,
    M68K_REG_FPIAR,

    M68K_REG_ENDING,   // <-- mark the end of the list of registers
} m68k_reg;

//> M68K Addressing Modes
typedef enum m68k_address_mode
{
    M68K_AM_NONE = 0,           // No address mode.

    M68K_AM_REG_DIRECT_DATA,        // Register Direct - Data
    M68K_AM_REG_DIRECT_ADDR,        // Register Direct - Address

    M68K_AM_REGI_ADDR,              // Register Indirect - Address
    M68K_AM_REGI_ADDR_POST_INC,     // Register Indirect - Address with Postincrement
    M68K_AM_REGI_ADDR_PRE_DEC,      // Register Indirect - Address with Predecrement
    M68K_AM_REGI_ADDR_DISP,         // Register Indirect - Address with Displacement

    M68K_AM_AREGI_INDEX_8_BIT_DISP, // Address Register Indirect With Index- 8-bit displacement
    M68K_AM_AREGI_INDEX_BASE_DISP,  // Address Register Indirect With Index- Base displacement

    M68K_AM_MEMI_POST_INDEX,        // Memory indirect - Postindex
    M68K_AM_MEMI_PRE_INDEX,         // Memory indirect - Preindex

    M68K_AM_PCI_DISP,               // Program Counter Indirect - with Displacement

    M68K_AM_PCI_INDEX_8_BIT_DISP,   // Program Counter Indirect with Index - with 8-Bit Displacement
    M68K_AM_PCI_INDEX_BASE_DISP,    // Program Counter Indirect with Index - with Base Displacement

    M68K_AM_PC_MEMI_POST_INDEX,     // Program Counter Memory Indirect - Postindexed
    M68K_AM_PC_MEMI_PRE_INDEX,      // Program Counter Memory Indirect - Preindexed

    M68K_AM_ABSOLUTE_DATA_SHORT,    // Absolute Data Addressing  - Short
    M68K_AM_ABSOLUTE_DATA_LONG,     // Absolute Data Addressing  - Long
    M68K_AM_IMMEDIATE,              // Immediate value
} m68k_address_mode;

//> Operand type for instruction's operands
typedef enum m68k_op_type
{
    M68K_OP_INVALID = 0, // = CS_OP_INVALID (Uninitialized).
    M68K_OP_REG,         // = CS_OP_REG (Register operand).
    M68K_OP_IMM,         // = CS_OP_IMM (Immediate operand).
    M68K_OP_MEM,         // = CS_OP_MEM (Memory operand).
    M68K_OP_FP_SINGLE,   // single precision Floating-Point operand
    M68K_OP_FP_DOUBLE,   // double precision Floating-Point operand
    M68K_OP_REG_BITS,    // Register bits move
    M68K_OP_REG_PAIR,    // Register pair in the same op (upper 4 bits for first reg, lower for second)
} m68k_op_type;

// Instruction's operand referring to memory
// This is associated with M68K_OP_MEM operand type above
typedef struct m68k_op_mem
{
    m68k_reg base_reg;      // base register (or M68K_REG_INVALID if irrelevant)
    m68k_reg index_reg;     // index register (or M68K_REG_INVALID if irrelevant)
    m68k_reg in_base_reg;   // indirect base register (or M68K_REG_INVALID if irrelevant)
    uint32_t in_disp;       // indirect displacement
    uint32_t out_disp;      // other displacement
    uint16_t disp;          // displacement value
    uint8_t scale;          // scale for index register
    uint8_t bitfield;       // set to true if the two values below should be used
    uint8_t width;          // used for bf* instructions
    uint8_t offset;         // used for bf* instructions
    uint8_t index_size;     // 0 = w, 1 = l
} m68k_op_mem;

// Instruction operand
typedef struct cs_m68k_op
{
    union
    {
        uint64_t imm;               // immediate value for IMM operand
        double dimm;            // double imm
        float simm;             // float imm
        m68k_reg reg;           // register value for REG operand
        struct              // register pair in one operand
        {
            m68k_reg reg_0;
            m68k_reg reg_1;
        } reg_pair;
        m68k_op_mem mem;        // data when operand is targeting memory
        uint32_t register_bits; // register bits for movem etc. (always in d0-d7, a0-a7, fp0 - fp7 order)
    };
    m68k_op_type type;
    m68k_address_mode address_mode; // M68K addressing mode for this op
} cs_m68k_op;

// Operation size of the CPU instructions
typedef enum m68k_cpu_size
{
    M68K_CPU_SIZE_NONE = 0,     // unsized or unspecified
    M68K_CPU_SIZE_BYTE = 1,     // 1 byte in size
    M68K_CPU_SIZE_WORD = 2,     // 2 bytes in size
    M68K_CPU_SIZE_LONG = 4,     // 4 bytes in size
} m68k_cpu_size;

// Operation size of the FPU instructions (Notice that FPU instruction can also use CPU sizes if needed)
typedef enum m68k_fpu_size
{
    M68K_FPU_SIZE_NONE = 0,     // unsized like fsave/frestore
    M68K_FPU_SIZE_SINGLE = 4,       // 4 byte in size (single float)
    M68K_FPU_SIZE_DOUBLE = 8,       // 8 byte in size (double)
    M68K_FPU_SIZE_EXTENDED = 12,    // 12 byte in size (extended real format)
} m68k_fpu_size;

// Type of size that is being used for the current instruction
typedef enum m68k_size_type
{
    M68K_SIZE_TYPE_INVALID = 0,

    M68K_SIZE_TYPE_CPU,
    M68K_SIZE_TYPE_FPU,
} m68k_size_type;

// Operation size of the current instruction (NOT the actually size of instruction)
typedef struct m68k_op_size
{
    m68k_size_type type;
    union
    {
        m68k_cpu_size cpu_size;
        m68k_fpu_size fpu_size;
    };
} m68k_op_size;

// The M68K instruction and it's operands
typedef struct cs_m68k
{
    // Number of operands of this instruction or 0 when instruction has no operand.
    cs_m68k_op operands[M68K_OPERAND_COUNT]; // operands for this instruction.
    m68k_op_size op_size;   // size of data operand works on in bytes (.b, .w, .l, etc)
    uint8_t op_count; // number of operands for the instruction
} cs_m68k;

//> M68K instruction
typedef enum m68k_insn
{
    M68K_INS_INVALID = 0,

    M68K_INS_ABCD,
    M68K_INS_ADD,
    M68K_INS_ADDA,
    M68K_INS_ADDI,
    M68K_INS_ADDQ,
    M68K_INS_ADDX,
    M68K_INS_AND,
    M68K_INS_ANDI,
    M68K_INS_ASL,
    M68K_INS_ASR,
    M68K_INS_BHS,
    M68K_INS_BLO,
    M68K_INS_BHI,
    M68K_INS_BLS,
    M68K_INS_BCC,
    M68K_INS_BCS,
    M68K_INS_BNE,
    M68K_INS_BEQ,
    M68K_INS_BVC,
    M68K_INS_BVS,
    M68K_INS_BPL,
    M68K_INS_BMI,
    M68K_INS_BGE,
    M68K_INS_BLT,
    M68K_INS_BGT,
    M68K_INS_BLE,
    M68K_INS_BRA,
    M68K_INS_BSR,
    M68K_INS_BCHG,
    M68K_INS_BCLR,
    M68K_INS_BSET,
    M68K_INS_BTST,
    M68K_INS_BFCHG,
    M68K_INS_BFCLR,
    M68K_INS_BFEXTS,
    M68K_INS_BFEXTU,
    M68K_INS_BFFFO,
    M68K_INS_BFINS,
    M68K_INS_BFSET,
    M68K_INS_BFTST,
    M68K_INS_BKPT,
    M68K_INS_CALLM,
    M68K_INS_CAS,
    M68K_INS_CAS2,
    M68K_INS_CHK,
    M68K_INS_CHK2,
    M68K_INS_CLR,
    M68K_INS_CMP,
    M68K_INS_CMPA,
    M68K_INS_CMPI,
    M68K_INS_CMPM,
    M68K_INS_CMP2,
    M68K_INS_CINVL,
    M68K_INS_CINVP,
    M68K_INS_CINVA,
    M68K_INS_CPUSHL,
    M68K_INS_CPUSHP,
    M68K_INS_CPUSHA,
    M68K_INS_DBT,
    M68K_INS_DBF,
    M68K_INS_DBHI,
    M68K_INS_DBLS,
    M68K_INS_DBCC,
    M68K_INS_DBCS,
    M68K_INS_DBNE,
    M68K_INS_DBEQ,
    M68K_INS_DBVC,
    M68K_INS_DBVS,
    M68K_INS_DBPL,
    M68K_INS_DBMI,
    M68K_INS_DBGE,
    M68K_INS_DBLT,
    M68K_INS_DBGT,
    M68K_INS_DBLE,
    M68K_INS_DBRA,
    M68K_INS_DIVS,
    M68K_INS_DIVSL,
    M68K_INS_DIVU,
    M68K_INS_DIVUL,
    M68K_INS_EOR,
    M68K_INS_EORI,
    M68K_INS_EXG,
    M68K_INS_EXT,
    M68K_INS_EXTB,
    M68K_INS_FABS,
    M68K_INS_FSABS,
    M68K_INS_FDABS,
    M68K_INS_FACOS,
    M68K_INS_FADD,
    M68K_INS_FSADD,
    M68K_INS_FDADD,
    M68K_INS_FASIN,
    M68K_INS_FATAN,
    M68K_INS_FATANH,
    M68K_INS_FBF,
    M68K_INS_FBEQ,
    M68K_INS_FBOGT,
    M68K_INS_FBOGE,
    M68K_INS_FBOLT,
    M68K_INS_FBOLE,
    M68K_INS_FBOGL,
    M68K_INS_FBOR,
    M68K_INS_FBUN,
    M68K_INS_FBUEQ,
    M68K_INS_FBUGT,
    M68K_INS_FBUGE,
    M68K_INS_FBULT,
    M68K_INS_FBULE,
    M68K_INS_FBNE,
    M68K_INS_FBT,
    M68K_INS_FBSF,
    M68K_INS_FBSEQ,
    M68K_INS_FBGT,
    M68K_INS_FBGE,
    M68K_INS_FBLT,
    M68K_INS_FBLE,
    M68K_INS_FBGL,
    M68K_INS_FBGLE,
    M68K_INS_FBNGLE,
    M68K_INS_FBNGL,
    M68K_INS_FBNLE,
    M68K_INS_FBNLT,
    M68K_INS_FBNGE,
    M68K_INS_FBNGT,
    M68K_INS_FBSNE,
    M68K_INS_FBST,
    M68K_INS_FCMP,
    M68K_INS_FCOS,
    M68K_INS_FCOSH,
    M68K_INS_FDBF,
    M68K_INS_FDBEQ,
    M68K_INS_FDBOGT,
    M68K_INS_FDBOGE,
    M68K_INS_FDBOLT,
    M68K_INS_FDBOLE,
    M68K_INS_FDBOGL,
    M68K_INS_FDBOR,
    M68K_INS_FDBUN,
    M68K_INS_FDBUEQ,
    M68K_INS_FDBUGT,
    M68K_INS_FDBUGE,
    M68K_INS_FDBULT,
    M68K_INS_FDBULE,
    M68K_INS_FDBNE,
    M68K_INS_FDBT,
    M68K_INS_FDBSF,
    M68K_INS_FDBSEQ,
    M68K_INS_FDBGT,
    M68K_INS_FDBGE,
    M68K_INS_FDBLT,
    M68K_INS_FDBLE,
    M68K_INS_FDBGL,
    M68K_INS_FDBGLE,
    M68K_INS_FDBNGLE,
    M68K_INS_FDBNGL,
    M68K_INS_FDBNLE,
    M68K_INS_FDBNLT,
    M68K_INS_FDBNGE,
    M68K_INS_FDBNGT,
    M68K_INS_FDBSNE,
    M68K_INS_FDBST,
    M68K_INS_FDIV,
    M68K_INS_FSDIV,
    M68K_INS_FDDIV,
    M68K_INS_FETOX,
    M68K_INS_FETOXM1,
    M68K_INS_FGETEXP,
    M68K_INS_FGETMAN,
    M68K_INS_FINT,
    M68K_INS_FINTRZ,
    M68K_INS_FLOG10,
    M68K_INS_FLOG2,
    M68K_INS_FLOGN,
    M68K_INS_FLOGNP1,
    M68K_INS_FMOD,
    M68K_INS_FMOVE,
    M68K_INS_FSMOVE,
    M68K_INS_FDMOVE,
    M68K_INS_FMOVECR,
    M68K_INS_FMOVEM,
    M68K_INS_FMUL,
    M68K_INS_FSMUL,
    M68K_INS_FDMUL,
    M68K_INS_FNEG,
    M68K_INS_FSNEG,
    M68K_INS_FDNEG,
    M68K_INS_FNOP,
    M68K_INS_FREM,
    M68K_INS_FRESTORE,
    M68K_INS_FSAVE,
    M68K_INS_FSCALE,
    M68K_INS_FSGLDIV,
    M68K_INS_FSGLMUL,
    M68K_INS_FSIN,
    M68K_INS_FSINCOS,
    M68K_INS_FSINH,
    M68K_INS_FSQRT,
    M68K_INS_FSSQRT,
    M68K_INS_FDSQRT,
    M68K_INS_FSF,
    M68K_INS_FSBEQ,
    M68K_INS_FSOGT,
    M68K_INS_FSOGE,
    M68K_INS_FSOLT,
    M68K_INS_FSOLE,
    M68K_INS_FSOGL,
    M68K_INS_FSOR,
    M68K_INS_FSUN,
    M68K_INS_FSUEQ,
    M68K_INS_FSUGT,
    M68K_INS_FSUGE,
    M68K_INS_FSULT,
    M68K_INS_FSULE,
    M68K_INS_FSNE,
    M68K_INS_FST,
    M68K_INS_FSSF,
    M68K_INS_FSSEQ,
    M68K_INS_FSGT,
    M68K_INS_FSGE,
    M68K_INS_FSLT,
    M68K_INS_FSLE,
    M68K_INS_FSGL,
    M68K_INS_FSGLE,
    M68K_INS_FSNGLE,
    M68K_INS_FSNGL,
    M68K_INS_FSNLE,
    M68K_INS_FSNLT,
    M68K_INS_FSNGE,
    M68K_INS_FSNGT,
    M68K_INS_FSSNE,
    M68K_INS_FSST,
    M68K_INS_FSUB,
    M68K_INS_FSSUB,
    M68K_INS_FDSUB,
    M68K_INS_FTAN,
    M68K_INS_FTANH,
    M68K_INS_FTENTOX,
    M68K_INS_FTRAPF,
    M68K_INS_FTRAPEQ,
    M68K_INS_FTRAPOGT,
    M68K_INS_FTRAPOGE,
    M68K_INS_FTRAPOLT,
    M68K_INS_FTRAPOLE,
    M68K_INS_FTRAPOGL,
    M68K_INS_FTRAPOR,
    M68K_INS_FTRAPUN,
    M68K_INS_FTRAPUEQ,
    M68K_INS_FTRAPUGT,
    M68K_INS_FTRAPUGE,
    M68K_INS_FTRAPULT,
    M68K_INS_FTRAPULE,
    M68K_INS_FTRAPNE,
    M68K_INS_FTRAPT,
    M68K_INS_FTRAPSF,
    M68K_INS_FTRAPSEQ,
    M68K_INS_FTRAPGT,
    M68K_INS_FTRAPGE,
    M68K_INS_FTRAPLT,
    M68K_INS_FTRAPLE,
    M68K_INS_FTRAPGL,
    M68K_INS_FTRAPGLE,
    M68K_INS_FTRAPNGLE,
    M68K_INS_FTRAPNGL,
    M68K_INS_FTRAPNLE,
    M68K_INS_FTRAPNLT,
    M68K_INS_FTRAPNGE,
    M68K_INS_FTRAPNGT,
    M68K_INS_FTRAPSNE,
    M68K_INS_FTRAPST,
    M68K_INS_FTST,
    M68K_INS_FTWOTOX,
    M68K_INS_HALT,
    M68K_INS_ILLEGAL,
    M68K_INS_JMP,
    M68K_INS_JSR,
    M68K_INS_LEA,
    M68K_INS_LINK,
    M68K_INS_LPSTOP,
    M68K_INS_LSL,
    M68K_INS_LSR,
    M68K_INS_MOVE,
    M68K_INS_MOVEA,
    M68K_INS_MOVEC,
    M68K_INS_MOVEM,
    M68K_INS_MOVEP,
    M68K_INS_MOVEQ,
    M68K_INS_MOVES,
    M68K_INS_MOVE16,
    M68K_INS_MULS,
    M68K_INS_MULU,
    M68K_INS_NBCD,
    M68K_INS_NEG,
    M68K_INS_NEGX,
    M68K_INS_NOP,
    M68K_INS_NOT,
    M68K_INS_OR,
    M68K_INS_ORI,
    M68K_INS_PACK,
    M68K_INS_PEA,
    M68K_INS_PFLUSH,
    M68K_INS_PFLUSHA,
    M68K_INS_PFLUSHAN,
    M68K_INS_PFLUSHN,
    M68K_INS_PLOADR,
    M68K_INS_PLOADW,
    M68K_INS_PLPAR,
    M68K_INS_PLPAW,
    M68K_INS_PMOVE,
    M68K_INS_PMOVEFD,
    M68K_INS_PTESTR,
    M68K_INS_PTESTW,
    M68K_INS_PULSE,
    M68K_INS_REMS,
    M68K_INS_REMU,
    M68K_INS_RESET,
    M68K_INS_ROL,
    M68K_INS_ROR,
    M68K_INS_ROXL,
    M68K_INS_ROXR,
    M68K_INS_RTD,
    M68K_INS_RTE,
    M68K_INS_RTM,
    M68K_INS_RTR,
    M68K_INS_RTS,
    M68K_INS_SBCD,
    M68K_INS_ST,
    M68K_INS_SF,
    M68K_INS_SHI,
    M68K_INS_SLS,
    M68K_INS_SCC,
    M68K_INS_SHS,
    M68K_INS_SCS,
    M68K_INS_SLO,
    M68K_INS_SNE,
    M68K_INS_SEQ,
    M68K_INS_SVC,
    M68K_INS_SVS,
    M68K_INS_SPL,
    M68K_INS_SMI,
    M68K_INS_SGE,
    M68K_INS_SLT,
    M68K_INS_SGT,
    M68K_INS_SLE,
    M68K_INS_STOP,
    M68K_INS_SUB,
    M68K_INS_SUBA,
    M68K_INS_SUBI,
    M68K_INS_SUBQ,
    M68K_INS_SUBX,
    M68K_INS_SWAP,
    M68K_INS_TAS,
    M68K_INS_TRAP,
    M68K_INS_TRAPV,
    M68K_INS_TRAPT,
    M68K_INS_TRAPF,
    M68K_INS_TRAPHI,
    M68K_INS_TRAPLS,
    M68K_INS_TRAPCC,
    M68K_INS_TRAPHS,
    M68K_INS_TRAPCS,
    M68K_INS_TRAPLO,
    M68K_INS_TRAPNE,
    M68K_INS_TRAPEQ,
    M68K_INS_TRAPVC,
    M68K_INS_TRAPVS,
    M68K_INS_TRAPPL,
    M68K_INS_TRAPMI,
    M68K_INS_TRAPGE,
    M68K_INS_TRAPLT,
    M68K_INS_TRAPGT,
    M68K_INS_TRAPLE,
    M68K_INS_TST,
    M68K_INS_UNLK,
    M68K_INS_UNPK,
    M68K_INS_ENDING,   // <-- mark the end of the list of instructions

} m68k_insn;

//> Group of M68K instructions
typedef enum m68k_group_type
{
    M68K_GRP_INVALID = 0,  // CS_GRUP_INVALID
    M68K_GRP_JUMP,  // = CS_GRP_JUMP
    M68K_GRP_RET = 3,  // = CS_GRP_RET
    M68K_GRP_IRET = 5, // = CS_GRP_IRET

    M68K_GRP_ENDING,// <-- mark the end of the list of groups
} m68k_group_type;

#ifdef __cplusplus
}
#endif

#endif
