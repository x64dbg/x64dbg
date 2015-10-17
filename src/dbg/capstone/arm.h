#ifndef CAPSTONE_ARM_H
#define CAPSTONE_ARM_H

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2014 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4201)
#endif

//> ARM shift type
typedef enum arm_shifter
{
    ARM_SFT_INVALID = 0,
    ARM_SFT_ASR,    // shift with immediate const
    ARM_SFT_LSL,    // shift with immediate const
    ARM_SFT_LSR,    // shift with immediate const
    ARM_SFT_ROR,    // shift with immediate const
    ARM_SFT_RRX,    // shift with immediate const
    ARM_SFT_ASR_REG,    // shift with register
    ARM_SFT_LSL_REG,    // shift with register
    ARM_SFT_LSR_REG,    // shift with register
    ARM_SFT_ROR_REG,    // shift with register
    ARM_SFT_RRX_REG,    // shift with register
} arm_shifter;

//> ARM condition code
typedef enum arm_cc
{
    ARM_CC_INVALID = 0,
    ARM_CC_EQ,            // Equal                      Equal
    ARM_CC_NE,            // Not equal                  Not equal, or unordered
    ARM_CC_HS,            // Carry set                  >, ==, or unordered
    ARM_CC_LO,            // Carry clear                Less than
    ARM_CC_MI,            // Minus, negative            Less than
    ARM_CC_PL,            // Plus, positive or zero     >, ==, or unordered
    ARM_CC_VS,            // Overflow                   Unordered
    ARM_CC_VC,            // No overflow                Not unordered
    ARM_CC_HI,            // Unsigned higher            Greater than, or unordered
    ARM_CC_LS,            // Unsigned lower or same     Less than or equal
    ARM_CC_GE,            // Greater than or equal      Greater than or equal
    ARM_CC_LT,            // Less than                  Less than, or unordered
    ARM_CC_GT,            // Greater than               Greater than
    ARM_CC_LE,            // Less than or equal         <, ==, or unordered
    ARM_CC_AL             // Always (unconditional)     Always (unconditional)
} arm_cc;

typedef enum arm_sysreg
{
    //> Special registers for MSR
    ARM_SYSREG_INVALID = 0,

    // SPSR* registers can be OR combined
    ARM_SYSREG_SPSR_C = 1,
    ARM_SYSREG_SPSR_X = 2,
    ARM_SYSREG_SPSR_S = 4,
    ARM_SYSREG_SPSR_F = 8,

    // CPSR* registers can be OR combined
    ARM_SYSREG_CPSR_C = 16,
    ARM_SYSREG_CPSR_X = 32,
    ARM_SYSREG_CPSR_S = 64,
    ARM_SYSREG_CPSR_F = 128,

    // independent registers
    ARM_SYSREG_APSR = 256,
    ARM_SYSREG_APSR_G,
    ARM_SYSREG_APSR_NZCVQ,
    ARM_SYSREG_APSR_NZCVQG,

    ARM_SYSREG_IAPSR,
    ARM_SYSREG_IAPSR_G,
    ARM_SYSREG_IAPSR_NZCVQG,

    ARM_SYSREG_EAPSR,
    ARM_SYSREG_EAPSR_G,
    ARM_SYSREG_EAPSR_NZCVQG,

    ARM_SYSREG_XPSR,
    ARM_SYSREG_XPSR_G,
    ARM_SYSREG_XPSR_NZCVQG,

    ARM_SYSREG_IPSR,
    ARM_SYSREG_EPSR,
    ARM_SYSREG_IEPSR,

    ARM_SYSREG_MSP,
    ARM_SYSREG_PSP,
    ARM_SYSREG_PRIMASK,
    ARM_SYSREG_BASEPRI,
    ARM_SYSREG_BASEPRI_MAX,
    ARM_SYSREG_FAULTMASK,
    ARM_SYSREG_CONTROL,
} arm_sysreg;

//> The memory barrier constants map directly to the 4-bit encoding of
//> the option field for Memory Barrier operations.
typedef enum arm_mem_barrier
{
    ARM_MB_INVALID = 0,
    ARM_MB_RESERVED_0,
    ARM_MB_OSHLD,
    ARM_MB_OSHST,
    ARM_MB_OSH,
    ARM_MB_RESERVED_4,
    ARM_MB_NSHLD,
    ARM_MB_NSHST,
    ARM_MB_NSH,
    ARM_MB_RESERVED_8,
    ARM_MB_ISHLD,
    ARM_MB_ISHST,
    ARM_MB_ISH,
    ARM_MB_RESERVED_12,
    ARM_MB_LD,
    ARM_MB_ST,
    ARM_MB_SY,
} arm_mem_barrier;

//> Operand type for instruction's operands
typedef enum arm_op_type
{
    ARM_OP_INVALID = 0, // = CS_OP_INVALID (Uninitialized).
    ARM_OP_REG, // = CS_OP_REG (Register operand).
    ARM_OP_IMM, // = CS_OP_IMM (Immediate operand).
    ARM_OP_MEM, // = CS_OP_MEM (Memory operand).
    ARM_OP_FP,  // = CS_OP_FP (Floating-Point operand).
    ARM_OP_CIMM = 64, // C-Immediate (coprocessor registers)
    ARM_OP_PIMM, // P-Immediate (coprocessor registers)
    ARM_OP_SETEND,  // operand for SETEND instruction
    ARM_OP_SYSREG,  // MSR/MSR special register operand
} arm_op_type;

//> Operand type for SETEND instruction
typedef enum arm_setend_type
{
    ARM_SETEND_INVALID = 0, // Uninitialized.
    ARM_SETEND_BE,  // BE operand.
    ARM_SETEND_LE, // LE operand
} arm_setend_type;

typedef enum arm_cpsmode_type
{
    ARM_CPSMODE_INVALID = 0,
    ARM_CPSMODE_IE = 2,
    ARM_CPSMODE_ID = 3
} arm_cpsmode_type;

//> Operand type for SETEND instruction
typedef enum arm_cpsflag_type
{
    ARM_CPSFLAG_INVALID = 0,
    ARM_CPSFLAG_F = 1,
    ARM_CPSFLAG_I = 2,
    ARM_CPSFLAG_A = 4,
    ARM_CPSFLAG_NONE = 16,  // no flag
} arm_cpsflag_type;

//> Data type for elements of vector instructions.
typedef enum arm_vectordata_type
{
    ARM_VECTORDATA_INVALID = 0,

    // Integer type
    ARM_VECTORDATA_I8,
    ARM_VECTORDATA_I16,
    ARM_VECTORDATA_I32,
    ARM_VECTORDATA_I64,

    // Signed integer type
    ARM_VECTORDATA_S8,
    ARM_VECTORDATA_S16,
    ARM_VECTORDATA_S32,
    ARM_VECTORDATA_S64,

    // Unsigned integer type
    ARM_VECTORDATA_U8,
    ARM_VECTORDATA_U16,
    ARM_VECTORDATA_U32,
    ARM_VECTORDATA_U64,

    // Data type for VMUL/VMULL
    ARM_VECTORDATA_P8,

    // Floating type
    ARM_VECTORDATA_F32,
    ARM_VECTORDATA_F64,

    // Convert float <-> float
    ARM_VECTORDATA_F16F64,  // f16.f64
    ARM_VECTORDATA_F64F16,  // f64.f16
    ARM_VECTORDATA_F32F16,  // f32.f16
    ARM_VECTORDATA_F16F32,  // f32.f16
    ARM_VECTORDATA_F64F32,  // f64.f32
    ARM_VECTORDATA_F32F64,  // f32.f64

    // Convert integer <-> float
    ARM_VECTORDATA_S32F32,  // s32.f32
    ARM_VECTORDATA_U32F32,  // u32.f32
    ARM_VECTORDATA_F32S32,  // f32.s32
    ARM_VECTORDATA_F32U32,  // f32.u32
    ARM_VECTORDATA_F64S16,  // f64.s16
    ARM_VECTORDATA_F32S16,  // f32.s16
    ARM_VECTORDATA_F64S32,  // f64.s32
    ARM_VECTORDATA_S16F64,  // s16.f64
    ARM_VECTORDATA_S16F32,  // s16.f64
    ARM_VECTORDATA_S32F64,  // s32.f64
    ARM_VECTORDATA_U16F64,  // u16.f64
    ARM_VECTORDATA_U16F32,  // u16.f32
    ARM_VECTORDATA_U32F64,  // u32.f64
    ARM_VECTORDATA_F64U16,  // f64.u16
    ARM_VECTORDATA_F32U16,  // f32.u16
    ARM_VECTORDATA_F64U32,  // f64.u32
} arm_vectordata_type;

// Instruction's operand referring to memory
// This is associated with ARM_OP_MEM operand type above
typedef struct arm_op_mem
{
    unsigned int base;  // base register
    unsigned int index; // index register
    int scale;  // scale for index register (can be 1, or -1)
    int disp;   // displacement/offset value
} arm_op_mem;

// Instruction operand
typedef struct cs_arm_op
{
    int vector_index;   // Vector Index for some vector operands (or -1 if irrelevant)
    struct
    {
        arm_shifter type;
        unsigned int value;
    } shift;
    arm_op_type type;   // operand type
    union
    {
        unsigned int reg;   // register value for REG/SYSREG operand
        int32_t imm;            // immediate value for C-IMM, P-IMM or IMM operand
        double fp;          // floating point value for FP operand
        arm_op_mem mem;     // base/index/scale/disp value for MEM operand
        arm_setend_type setend; // SETEND instruction's operand type
    };
    // in some instructions, an operand can be subtracted or added to
    // the base register,
    bool subtracted; // if TRUE, this operand is subtracted. otherwise, it is added.
} cs_arm_op;

// Instruction structure
typedef struct cs_arm
{
    bool usermode;  // User-mode registers to be loaded (for LDM/STM instructions)
    int vector_size;    // Scalar size for vector instructions
    arm_vectordata_type vector_data; // Data type for elements of vector instructions
    arm_cpsmode_type cps_mode;  // CPS mode for CPS instruction
    arm_cpsflag_type cps_flag;  // CPS mode for CPS instruction
    arm_cc cc;          // conditional code for this insn
    bool update_flags;  // does this insn update flags?
    bool writeback;     // does this insn write-back?
    arm_mem_barrier mem_barrier;    // Option for some memory barrier instructions

    // Number of operands of this instruction,
    // or 0 when instruction has no operand.
    uint8_t op_count;

    cs_arm_op operands[36]; // operands for this instruction.
} cs_arm;

//> ARM registers
typedef enum arm_reg
{
    ARM_REG_INVALID = 0,
    ARM_REG_APSR,
    ARM_REG_APSR_NZCV,
    ARM_REG_CPSR,
    ARM_REG_FPEXC,
    ARM_REG_FPINST,
    ARM_REG_FPSCR,
    ARM_REG_FPSCR_NZCV,
    ARM_REG_FPSID,
    ARM_REG_ITSTATE,
    ARM_REG_LR,
    ARM_REG_PC,
    ARM_REG_SP,
    ARM_REG_SPSR,
    ARM_REG_D0,
    ARM_REG_D1,
    ARM_REG_D2,
    ARM_REG_D3,
    ARM_REG_D4,
    ARM_REG_D5,
    ARM_REG_D6,
    ARM_REG_D7,
    ARM_REG_D8,
    ARM_REG_D9,
    ARM_REG_D10,
    ARM_REG_D11,
    ARM_REG_D12,
    ARM_REG_D13,
    ARM_REG_D14,
    ARM_REG_D15,
    ARM_REG_D16,
    ARM_REG_D17,
    ARM_REG_D18,
    ARM_REG_D19,
    ARM_REG_D20,
    ARM_REG_D21,
    ARM_REG_D22,
    ARM_REG_D23,
    ARM_REG_D24,
    ARM_REG_D25,
    ARM_REG_D26,
    ARM_REG_D27,
    ARM_REG_D28,
    ARM_REG_D29,
    ARM_REG_D30,
    ARM_REG_D31,
    ARM_REG_FPINST2,
    ARM_REG_MVFR0,
    ARM_REG_MVFR1,
    ARM_REG_MVFR2,
    ARM_REG_Q0,
    ARM_REG_Q1,
    ARM_REG_Q2,
    ARM_REG_Q3,
    ARM_REG_Q4,
    ARM_REG_Q5,
    ARM_REG_Q6,
    ARM_REG_Q7,
    ARM_REG_Q8,
    ARM_REG_Q9,
    ARM_REG_Q10,
    ARM_REG_Q11,
    ARM_REG_Q12,
    ARM_REG_Q13,
    ARM_REG_Q14,
    ARM_REG_Q15,
    ARM_REG_R0,
    ARM_REG_R1,
    ARM_REG_R2,
    ARM_REG_R3,
    ARM_REG_R4,
    ARM_REG_R5,
    ARM_REG_R6,
    ARM_REG_R7,
    ARM_REG_R8,
    ARM_REG_R9,
    ARM_REG_R10,
    ARM_REG_R11,
    ARM_REG_R12,
    ARM_REG_S0,
    ARM_REG_S1,
    ARM_REG_S2,
    ARM_REG_S3,
    ARM_REG_S4,
    ARM_REG_S5,
    ARM_REG_S6,
    ARM_REG_S7,
    ARM_REG_S8,
    ARM_REG_S9,
    ARM_REG_S10,
    ARM_REG_S11,
    ARM_REG_S12,
    ARM_REG_S13,
    ARM_REG_S14,
    ARM_REG_S15,
    ARM_REG_S16,
    ARM_REG_S17,
    ARM_REG_S18,
    ARM_REG_S19,
    ARM_REG_S20,
    ARM_REG_S21,
    ARM_REG_S22,
    ARM_REG_S23,
    ARM_REG_S24,
    ARM_REG_S25,
    ARM_REG_S26,
    ARM_REG_S27,
    ARM_REG_S28,
    ARM_REG_S29,
    ARM_REG_S30,
    ARM_REG_S31,

    ARM_REG_ENDING,     // <-- mark the end of the list or registers

    //> alias registers
    ARM_REG_R13 = ARM_REG_SP,
    ARM_REG_R14 = ARM_REG_LR,
    ARM_REG_R15 = ARM_REG_PC,

    ARM_REG_SB = ARM_REG_R9,
    ARM_REG_SL = ARM_REG_R10,
    ARM_REG_FP = ARM_REG_R11,
    ARM_REG_IP = ARM_REG_R12,
} arm_reg;

//> ARM instruction
typedef enum arm_insn
{
    ARM_INS_INVALID = 0,

    ARM_INS_ADC,
    ARM_INS_ADD,
    ARM_INS_ADR,
    ARM_INS_AESD,
    ARM_INS_AESE,
    ARM_INS_AESIMC,
    ARM_INS_AESMC,
    ARM_INS_AND,
    ARM_INS_BFC,
    ARM_INS_BFI,
    ARM_INS_BIC,
    ARM_INS_BKPT,
    ARM_INS_BL,
    ARM_INS_BLX,
    ARM_INS_BX,
    ARM_INS_BXJ,
    ARM_INS_B,
    ARM_INS_CDP,
    ARM_INS_CDP2,
    ARM_INS_CLREX,
    ARM_INS_CLZ,
    ARM_INS_CMN,
    ARM_INS_CMP,
    ARM_INS_CPS,
    ARM_INS_CRC32B,
    ARM_INS_CRC32CB,
    ARM_INS_CRC32CH,
    ARM_INS_CRC32CW,
    ARM_INS_CRC32H,
    ARM_INS_CRC32W,
    ARM_INS_DBG,
    ARM_INS_DMB,
    ARM_INS_DSB,
    ARM_INS_EOR,
    ARM_INS_VMOV,
    ARM_INS_FLDMDBX,
    ARM_INS_FLDMIAX,
    ARM_INS_VMRS,
    ARM_INS_FSTMDBX,
    ARM_INS_FSTMIAX,
    ARM_INS_HINT,
    ARM_INS_HLT,
    ARM_INS_ISB,
    ARM_INS_LDA,
    ARM_INS_LDAB,
    ARM_INS_LDAEX,
    ARM_INS_LDAEXB,
    ARM_INS_LDAEXD,
    ARM_INS_LDAEXH,
    ARM_INS_LDAH,
    ARM_INS_LDC2L,
    ARM_INS_LDC2,
    ARM_INS_LDCL,
    ARM_INS_LDC,
    ARM_INS_LDMDA,
    ARM_INS_LDMDB,
    ARM_INS_LDM,
    ARM_INS_LDMIB,
    ARM_INS_LDRBT,
    ARM_INS_LDRB,
    ARM_INS_LDRD,
    ARM_INS_LDREX,
    ARM_INS_LDREXB,
    ARM_INS_LDREXD,
    ARM_INS_LDREXH,
    ARM_INS_LDRH,
    ARM_INS_LDRHT,
    ARM_INS_LDRSB,
    ARM_INS_LDRSBT,
    ARM_INS_LDRSH,
    ARM_INS_LDRSHT,
    ARM_INS_LDRT,
    ARM_INS_LDR,
    ARM_INS_MCR,
    ARM_INS_MCR2,
    ARM_INS_MCRR,
    ARM_INS_MCRR2,
    ARM_INS_MLA,
    ARM_INS_MLS,
    ARM_INS_MOV,
    ARM_INS_MOVT,
    ARM_INS_MOVW,
    ARM_INS_MRC,
    ARM_INS_MRC2,
    ARM_INS_MRRC,
    ARM_INS_MRRC2,
    ARM_INS_MRS,
    ARM_INS_MSR,
    ARM_INS_MUL,
    ARM_INS_MVN,
    ARM_INS_ORR,
    ARM_INS_PKHBT,
    ARM_INS_PKHTB,
    ARM_INS_PLDW,
    ARM_INS_PLD,
    ARM_INS_PLI,
    ARM_INS_QADD,
    ARM_INS_QADD16,
    ARM_INS_QADD8,
    ARM_INS_QASX,
    ARM_INS_QDADD,
    ARM_INS_QDSUB,
    ARM_INS_QSAX,
    ARM_INS_QSUB,
    ARM_INS_QSUB16,
    ARM_INS_QSUB8,
    ARM_INS_RBIT,
    ARM_INS_REV,
    ARM_INS_REV16,
    ARM_INS_REVSH,
    ARM_INS_RFEDA,
    ARM_INS_RFEDB,
    ARM_INS_RFEIA,
    ARM_INS_RFEIB,
    ARM_INS_RSB,
    ARM_INS_RSC,
    ARM_INS_SADD16,
    ARM_INS_SADD8,
    ARM_INS_SASX,
    ARM_INS_SBC,
    ARM_INS_SBFX,
    ARM_INS_SDIV,
    ARM_INS_SEL,
    ARM_INS_SETEND,
    ARM_INS_SHA1C,
    ARM_INS_SHA1H,
    ARM_INS_SHA1M,
    ARM_INS_SHA1P,
    ARM_INS_SHA1SU0,
    ARM_INS_SHA1SU1,
    ARM_INS_SHA256H,
    ARM_INS_SHA256H2,
    ARM_INS_SHA256SU0,
    ARM_INS_SHA256SU1,
    ARM_INS_SHADD16,
    ARM_INS_SHADD8,
    ARM_INS_SHASX,
    ARM_INS_SHSAX,
    ARM_INS_SHSUB16,
    ARM_INS_SHSUB8,
    ARM_INS_SMC,
    ARM_INS_SMLABB,
    ARM_INS_SMLABT,
    ARM_INS_SMLAD,
    ARM_INS_SMLADX,
    ARM_INS_SMLAL,
    ARM_INS_SMLALBB,
    ARM_INS_SMLALBT,
    ARM_INS_SMLALD,
    ARM_INS_SMLALDX,
    ARM_INS_SMLALTB,
    ARM_INS_SMLALTT,
    ARM_INS_SMLATB,
    ARM_INS_SMLATT,
    ARM_INS_SMLAWB,
    ARM_INS_SMLAWT,
    ARM_INS_SMLSD,
    ARM_INS_SMLSDX,
    ARM_INS_SMLSLD,
    ARM_INS_SMLSLDX,
    ARM_INS_SMMLA,
    ARM_INS_SMMLAR,
    ARM_INS_SMMLS,
    ARM_INS_SMMLSR,
    ARM_INS_SMMUL,
    ARM_INS_SMMULR,
    ARM_INS_SMUAD,
    ARM_INS_SMUADX,
    ARM_INS_SMULBB,
    ARM_INS_SMULBT,
    ARM_INS_SMULL,
    ARM_INS_SMULTB,
    ARM_INS_SMULTT,
    ARM_INS_SMULWB,
    ARM_INS_SMULWT,
    ARM_INS_SMUSD,
    ARM_INS_SMUSDX,
    ARM_INS_SRSDA,
    ARM_INS_SRSDB,
    ARM_INS_SRSIA,
    ARM_INS_SRSIB,
    ARM_INS_SSAT,
    ARM_INS_SSAT16,
    ARM_INS_SSAX,
    ARM_INS_SSUB16,
    ARM_INS_SSUB8,
    ARM_INS_STC2L,
    ARM_INS_STC2,
    ARM_INS_STCL,
    ARM_INS_STC,
    ARM_INS_STL,
    ARM_INS_STLB,
    ARM_INS_STLEX,
    ARM_INS_STLEXB,
    ARM_INS_STLEXD,
    ARM_INS_STLEXH,
    ARM_INS_STLH,
    ARM_INS_STMDA,
    ARM_INS_STMDB,
    ARM_INS_STM,
    ARM_INS_STMIB,
    ARM_INS_STRBT,
    ARM_INS_STRB,
    ARM_INS_STRD,
    ARM_INS_STREX,
    ARM_INS_STREXB,
    ARM_INS_STREXD,
    ARM_INS_STREXH,
    ARM_INS_STRH,
    ARM_INS_STRHT,
    ARM_INS_STRT,
    ARM_INS_STR,
    ARM_INS_SUB,
    ARM_INS_SVC,
    ARM_INS_SWP,
    ARM_INS_SWPB,
    ARM_INS_SXTAB,
    ARM_INS_SXTAB16,
    ARM_INS_SXTAH,
    ARM_INS_SXTB,
    ARM_INS_SXTB16,
    ARM_INS_SXTH,
    ARM_INS_TEQ,
    ARM_INS_TRAP,
    ARM_INS_TST,
    ARM_INS_UADD16,
    ARM_INS_UADD8,
    ARM_INS_UASX,
    ARM_INS_UBFX,
    ARM_INS_UDF,
    ARM_INS_UDIV,
    ARM_INS_UHADD16,
    ARM_INS_UHADD8,
    ARM_INS_UHASX,
    ARM_INS_UHSAX,
    ARM_INS_UHSUB16,
    ARM_INS_UHSUB8,
    ARM_INS_UMAAL,
    ARM_INS_UMLAL,
    ARM_INS_UMULL,
    ARM_INS_UQADD16,
    ARM_INS_UQADD8,
    ARM_INS_UQASX,
    ARM_INS_UQSAX,
    ARM_INS_UQSUB16,
    ARM_INS_UQSUB8,
    ARM_INS_USAD8,
    ARM_INS_USADA8,
    ARM_INS_USAT,
    ARM_INS_USAT16,
    ARM_INS_USAX,
    ARM_INS_USUB16,
    ARM_INS_USUB8,
    ARM_INS_UXTAB,
    ARM_INS_UXTAB16,
    ARM_INS_UXTAH,
    ARM_INS_UXTB,
    ARM_INS_UXTB16,
    ARM_INS_UXTH,
    ARM_INS_VABAL,
    ARM_INS_VABA,
    ARM_INS_VABDL,
    ARM_INS_VABD,
    ARM_INS_VABS,
    ARM_INS_VACGE,
    ARM_INS_VACGT,
    ARM_INS_VADD,
    ARM_INS_VADDHN,
    ARM_INS_VADDL,
    ARM_INS_VADDW,
    ARM_INS_VAND,
    ARM_INS_VBIC,
    ARM_INS_VBIF,
    ARM_INS_VBIT,
    ARM_INS_VBSL,
    ARM_INS_VCEQ,
    ARM_INS_VCGE,
    ARM_INS_VCGT,
    ARM_INS_VCLE,
    ARM_INS_VCLS,
    ARM_INS_VCLT,
    ARM_INS_VCLZ,
    ARM_INS_VCMP,
    ARM_INS_VCMPE,
    ARM_INS_VCNT,
    ARM_INS_VCVTA,
    ARM_INS_VCVTB,
    ARM_INS_VCVT,
    ARM_INS_VCVTM,
    ARM_INS_VCVTN,
    ARM_INS_VCVTP,
    ARM_INS_VCVTT,
    ARM_INS_VDIV,
    ARM_INS_VDUP,
    ARM_INS_VEOR,
    ARM_INS_VEXT,
    ARM_INS_VFMA,
    ARM_INS_VFMS,
    ARM_INS_VFNMA,
    ARM_INS_VFNMS,
    ARM_INS_VHADD,
    ARM_INS_VHSUB,
    ARM_INS_VLD1,
    ARM_INS_VLD2,
    ARM_INS_VLD3,
    ARM_INS_VLD4,
    ARM_INS_VLDMDB,
    ARM_INS_VLDMIA,
    ARM_INS_VLDR,
    ARM_INS_VMAXNM,
    ARM_INS_VMAX,
    ARM_INS_VMINNM,
    ARM_INS_VMIN,
    ARM_INS_VMLA,
    ARM_INS_VMLAL,
    ARM_INS_VMLS,
    ARM_INS_VMLSL,
    ARM_INS_VMOVL,
    ARM_INS_VMOVN,
    ARM_INS_VMSR,
    ARM_INS_VMUL,
    ARM_INS_VMULL,
    ARM_INS_VMVN,
    ARM_INS_VNEG,
    ARM_INS_VNMLA,
    ARM_INS_VNMLS,
    ARM_INS_VNMUL,
    ARM_INS_VORN,
    ARM_INS_VORR,
    ARM_INS_VPADAL,
    ARM_INS_VPADDL,
    ARM_INS_VPADD,
    ARM_INS_VPMAX,
    ARM_INS_VPMIN,
    ARM_INS_VQABS,
    ARM_INS_VQADD,
    ARM_INS_VQDMLAL,
    ARM_INS_VQDMLSL,
    ARM_INS_VQDMULH,
    ARM_INS_VQDMULL,
    ARM_INS_VQMOVUN,
    ARM_INS_VQMOVN,
    ARM_INS_VQNEG,
    ARM_INS_VQRDMULH,
    ARM_INS_VQRSHL,
    ARM_INS_VQRSHRN,
    ARM_INS_VQRSHRUN,
    ARM_INS_VQSHL,
    ARM_INS_VQSHLU,
    ARM_INS_VQSHRN,
    ARM_INS_VQSHRUN,
    ARM_INS_VQSUB,
    ARM_INS_VRADDHN,
    ARM_INS_VRECPE,
    ARM_INS_VRECPS,
    ARM_INS_VREV16,
    ARM_INS_VREV32,
    ARM_INS_VREV64,
    ARM_INS_VRHADD,
    ARM_INS_VRINTA,
    ARM_INS_VRINTM,
    ARM_INS_VRINTN,
    ARM_INS_VRINTP,
    ARM_INS_VRINTR,
    ARM_INS_VRINTX,
    ARM_INS_VRINTZ,
    ARM_INS_VRSHL,
    ARM_INS_VRSHRN,
    ARM_INS_VRSHR,
    ARM_INS_VRSQRTE,
    ARM_INS_VRSQRTS,
    ARM_INS_VRSRA,
    ARM_INS_VRSUBHN,
    ARM_INS_VSELEQ,
    ARM_INS_VSELGE,
    ARM_INS_VSELGT,
    ARM_INS_VSELVS,
    ARM_INS_VSHLL,
    ARM_INS_VSHL,
    ARM_INS_VSHRN,
    ARM_INS_VSHR,
    ARM_INS_VSLI,
    ARM_INS_VSQRT,
    ARM_INS_VSRA,
    ARM_INS_VSRI,
    ARM_INS_VST1,
    ARM_INS_VST2,
    ARM_INS_VST3,
    ARM_INS_VST4,
    ARM_INS_VSTMDB,
    ARM_INS_VSTMIA,
    ARM_INS_VSTR,
    ARM_INS_VSUB,
    ARM_INS_VSUBHN,
    ARM_INS_VSUBL,
    ARM_INS_VSUBW,
    ARM_INS_VSWP,
    ARM_INS_VTBL,
    ARM_INS_VTBX,
    ARM_INS_VCVTR,
    ARM_INS_VTRN,
    ARM_INS_VTST,
    ARM_INS_VUZP,
    ARM_INS_VZIP,
    ARM_INS_ADDW,
    ARM_INS_ASR,
    ARM_INS_DCPS1,
    ARM_INS_DCPS2,
    ARM_INS_DCPS3,
    ARM_INS_IT,
    ARM_INS_LSL,
    ARM_INS_LSR,
    ARM_INS_ASRS,
    ARM_INS_LSRS,
    ARM_INS_ORN,
    ARM_INS_ROR,
    ARM_INS_RRX,
    ARM_INS_SUBS,
    ARM_INS_SUBW,
    ARM_INS_TBB,
    ARM_INS_TBH,
    ARM_INS_CBNZ,
    ARM_INS_CBZ,
    ARM_INS_MOVS,
    ARM_INS_POP,
    ARM_INS_PUSH,

    // special instructions
    ARM_INS_NOP,
    ARM_INS_YIELD,
    ARM_INS_WFE,
    ARM_INS_WFI,
    ARM_INS_SEV,
    ARM_INS_SEVL,
    ARM_INS_VPUSH,
    ARM_INS_VPOP,

    ARM_INS_ENDING, // <-- mark the end of the list of instructions
} arm_insn;

//> Group of ARM instructions
typedef enum arm_insn_group
{
    ARM_GRP_INVALID = 0, // = CS_GRP_INVALID

    //> Generic groups
    // all jump instructions (conditional+direct+indirect jumps)
    ARM_GRP_JUMP,   // = CS_GRP_JUMP

    //> Architecture-specific groups
    ARM_GRP_CRYPTO = 128,
    ARM_GRP_DATABARRIER,
    ARM_GRP_DIVIDE,
    ARM_GRP_FPARMV8,
    ARM_GRP_MULTPRO,
    ARM_GRP_NEON,
    ARM_GRP_T2EXTRACTPACK,
    ARM_GRP_THUMB2DSP,
    ARM_GRP_TRUSTZONE,
    ARM_GRP_V4T,
    ARM_GRP_V5T,
    ARM_GRP_V5TE,
    ARM_GRP_V6,
    ARM_GRP_V6T2,
    ARM_GRP_V7,
    ARM_GRP_V8,
    ARM_GRP_VFP2,
    ARM_GRP_VFP3,
    ARM_GRP_VFP4,
    ARM_GRP_ARM,
    ARM_GRP_MCLASS,
    ARM_GRP_NOTMCLASS,
    ARM_GRP_THUMB,
    ARM_GRP_THUMB1ONLY,
    ARM_GRP_THUMB2,
    ARM_GRP_PREV8,
    ARM_GRP_FPVMLX,
    ARM_GRP_MULOPS,
    ARM_GRP_CRC,
    ARM_GRP_DPVFP,
    ARM_GRP_V6M,

    ARM_GRP_ENDING,
} arm_insn_group;

#ifdef __cplusplus
}
#endif

#endif
