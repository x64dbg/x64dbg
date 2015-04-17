#ifndef CAPSTONE_ARM64_H
#define CAPSTONE_ARM64_H

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

//> ARM64 shift type
typedef enum arm64_shifter
{
    ARM64_SFT_INVALID = 0,
    ARM64_SFT_LSL = 1,
    ARM64_SFT_MSL = 2,
    ARM64_SFT_LSR = 3,
    ARM64_SFT_ASR = 4,
    ARM64_SFT_ROR = 5,
} arm64_shifter;

//> ARM64 extender type
typedef enum arm64_extender
{
    ARM64_EXT_INVALID = 0,
    ARM64_EXT_UXTB = 1,
    ARM64_EXT_UXTH = 2,
    ARM64_EXT_UXTW = 3,
    ARM64_EXT_UXTX = 4,
    ARM64_EXT_SXTB = 5,
    ARM64_EXT_SXTH = 6,
    ARM64_EXT_SXTW = 7,
    ARM64_EXT_SXTX = 8,
} arm64_extender;

//> ARM64 condition code
typedef enum arm64_cc
{
    ARM64_CC_INVALID = 0,
    ARM64_CC_EQ = 1,     // Equal
    ARM64_CC_NE = 2,     // Not equal:                 Not equal, or unordered
    ARM64_CC_HS = 3,     // Unsigned higher or same:   >, ==, or unordered
    ARM64_CC_LO = 4,     // Unsigned lower or same:    Less than
    ARM64_CC_MI = 5,     // Minus, negative:           Less than
    ARM64_CC_PL = 6,     // Plus, positive or zero:    >, ==, or unordered
    ARM64_CC_VS = 7,     // Overflow:                  Unordered
    ARM64_CC_VC = 8,     // No overflow:               Ordered
    ARM64_CC_HI = 9,     // Unsigned higher:           Greater than, or unordered
    ARM64_CC_LS = 10,     // Unsigned lower or same:    Less than or equal
    ARM64_CC_GE = 11,     // Greater than or equal:     Greater than or equal
    ARM64_CC_LT = 12,     // Less than:                 Less than, or unordered
    ARM64_CC_GT = 13,     // Signed greater than:       Greater than
    ARM64_CC_LE = 14,     // Signed less than or equal: <, ==, or unordered
    ARM64_CC_AL = 15,     // Always (unconditional):    Always (unconditional)
    ARM64_CC_NV = 16,     // Always (unconditional):   Always (unconditional)
    // Note the NV exists purely to disassemble 0b1111. Execution
    // is "always".
} arm64_cc;

//> System registers
typedef enum arm64_mrs_reg
{
    //> System registers for MRS
    ARM64_SYSREG_INVALID           = 0,
    ARM64_SYSREG_MDCCSR_EL0        = 0x9808, // 10  011  0000  0001  000
    ARM64_SYSREG_DBGDTRRX_EL0      = 0x9828, // 10  011  0000  0101  000
    ARM64_SYSREG_MDRAR_EL1         = 0x8080, // 10  000  0001  0000  000
    ARM64_SYSREG_OSLSR_EL1         = 0x808c, // 10  000  0001  0001  100
    ARM64_SYSREG_DBGAUTHSTATUS_EL1 = 0x83f6, // 10  000  0111  1110  110
    ARM64_SYSREG_PMCEID0_EL0       = 0xdce6, // 11  011  1001  1100  110
    ARM64_SYSREG_PMCEID1_EL0       = 0xdce7, // 11  011  1001  1100  111
    ARM64_SYSREG_MIDR_EL1          = 0xc000, // 11  000  0000  0000  000
    ARM64_SYSREG_CCSIDR_EL1        = 0xc800, // 11  001  0000  0000  000
    ARM64_SYSREG_CLIDR_EL1         = 0xc801, // 11  001  0000  0000  001
    ARM64_SYSREG_CTR_EL0           = 0xd801, // 11  011  0000  0000  001
    ARM64_SYSREG_MPIDR_EL1         = 0xc005, // 11  000  0000  0000  101
    ARM64_SYSREG_REVIDR_EL1        = 0xc006, // 11  000  0000  0000  110
    ARM64_SYSREG_AIDR_EL1          = 0xc807, // 11  001  0000  0000  111
    ARM64_SYSREG_DCZID_EL0         = 0xd807, // 11  011  0000  0000  111
    ARM64_SYSREG_ID_PFR0_EL1       = 0xc008, // 11  000  0000  0001  000
    ARM64_SYSREG_ID_PFR1_EL1       = 0xc009, // 11  000  0000  0001  001
    ARM64_SYSREG_ID_DFR0_EL1       = 0xc00a, // 11  000  0000  0001  010
    ARM64_SYSREG_ID_AFR0_EL1       = 0xc00b, // 11  000  0000  0001  011
    ARM64_SYSREG_ID_MMFR0_EL1      = 0xc00c, // 11  000  0000  0001  100
    ARM64_SYSREG_ID_MMFR1_EL1      = 0xc00d, // 11  000  0000  0001  101
    ARM64_SYSREG_ID_MMFR2_EL1      = 0xc00e, // 11  000  0000  0001  110
    ARM64_SYSREG_ID_MMFR3_EL1      = 0xc00f, // 11  000  0000  0001  111
    ARM64_SYSREG_ID_ISAR0_EL1      = 0xc010, // 11  000  0000  0010  000
    ARM64_SYSREG_ID_ISAR1_EL1      = 0xc011, // 11  000  0000  0010  001
    ARM64_SYSREG_ID_ISAR2_EL1      = 0xc012, // 11  000  0000  0010  010
    ARM64_SYSREG_ID_ISAR3_EL1      = 0xc013, // 11  000  0000  0010  011
    ARM64_SYSREG_ID_ISAR4_EL1      = 0xc014, // 11  000  0000  0010  100
    ARM64_SYSREG_ID_ISAR5_EL1      = 0xc015, // 11  000  0000  0010  101
    ARM64_SYSREG_ID_A64PFR0_EL1   = 0xc020, // 11  000  0000  0100  000
    ARM64_SYSREG_ID_A64PFR1_EL1   = 0xc021, // 11  000  0000  0100  001
    ARM64_SYSREG_ID_A64DFR0_EL1   = 0xc028, // 11  000  0000  0101  000
    ARM64_SYSREG_ID_A64DFR1_EL1   = 0xc029, // 11  000  0000  0101  001
    ARM64_SYSREG_ID_A64AFR0_EL1   = 0xc02c, // 11  000  0000  0101  100
    ARM64_SYSREG_ID_A64AFR1_EL1   = 0xc02d, // 11  000  0000  0101  101
    ARM64_SYSREG_ID_A64ISAR0_EL1  = 0xc030, // 11  000  0000  0110  000
    ARM64_SYSREG_ID_A64ISAR1_EL1  = 0xc031, // 11  000  0000  0110  001
    ARM64_SYSREG_ID_A64MMFR0_EL1  = 0xc038, // 11  000  0000  0111  000
    ARM64_SYSREG_ID_A64MMFR1_EL1  = 0xc039, // 11  000  0000  0111  001
    ARM64_SYSREG_MVFR0_EL1         = 0xc018, // 11  000  0000  0011  000
    ARM64_SYSREG_MVFR1_EL1         = 0xc019, // 11  000  0000  0011  001
    ARM64_SYSREG_MVFR2_EL1         = 0xc01a, // 11  000  0000  0011  010
    ARM64_SYSREG_RVBAR_EL1         = 0xc601, // 11  000  1100  0000  001
    ARM64_SYSREG_RVBAR_EL2         = 0xe601, // 11  100  1100  0000  001
    ARM64_SYSREG_RVBAR_EL3         = 0xf601, // 11  110  1100  0000  001
    ARM64_SYSREG_ISR_EL1           = 0xc608, // 11  000  1100  0001  000
    ARM64_SYSREG_CNTPCT_EL0        = 0xdf01, // 11  011  1110  0000  001
    ARM64_SYSREG_CNTVCT_EL0        = 0xdf02,  // 11  011  1110  0000  010

    // Trace registers
    ARM64_SYSREG_TRCSTATR          = 0x8818, // 10  001  0000  0011  000
    ARM64_SYSREG_TRCIDR8           = 0x8806, // 10  001  0000  0000  110
    ARM64_SYSREG_TRCIDR9           = 0x880e, // 10  001  0000  0001  110
    ARM64_SYSREG_TRCIDR10          = 0x8816, // 10  001  0000  0010  110
    ARM64_SYSREG_TRCIDR11          = 0x881e, // 10  001  0000  0011  110
    ARM64_SYSREG_TRCIDR12          = 0x8826, // 10  001  0000  0100  110
    ARM64_SYSREG_TRCIDR13          = 0x882e, // 10  001  0000  0101  110
    ARM64_SYSREG_TRCIDR0           = 0x8847, // 10  001  0000  1000  111
    ARM64_SYSREG_TRCIDR1           = 0x884f, // 10  001  0000  1001  111
    ARM64_SYSREG_TRCIDR2           = 0x8857, // 10  001  0000  1010  111
    ARM64_SYSREG_TRCIDR3           = 0x885f, // 10  001  0000  1011  111
    ARM64_SYSREG_TRCIDR4           = 0x8867, // 10  001  0000  1100  111
    ARM64_SYSREG_TRCIDR5           = 0x886f, // 10  001  0000  1101  111
    ARM64_SYSREG_TRCIDR6           = 0x8877, // 10  001  0000  1110  111
    ARM64_SYSREG_TRCIDR7           = 0x887f, // 10  001  0000  1111  111
    ARM64_SYSREG_TRCOSLSR          = 0x888c, // 10  001  0001  0001  100
    ARM64_SYSREG_TRCPDSR           = 0x88ac, // 10  001  0001  0101  100
    ARM64_SYSREG_TRCDEVAFF0        = 0x8bd6, // 10  001  0111  1010  110
    ARM64_SYSREG_TRCDEVAFF1        = 0x8bde, // 10  001  0111  1011  110
    ARM64_SYSREG_TRCLSR            = 0x8bee, // 10  001  0111  1101  110
    ARM64_SYSREG_TRCAUTHSTATUS     = 0x8bf6, // 10  001  0111  1110  110
    ARM64_SYSREG_TRCDEVARCH        = 0x8bfe, // 10  001  0111  1111  110
    ARM64_SYSREG_TRCDEVID          = 0x8b97, // 10  001  0111  0010  111
    ARM64_SYSREG_TRCDEVTYPE        = 0x8b9f, // 10  001  0111  0011  111
    ARM64_SYSREG_TRCPIDR4          = 0x8ba7, // 10  001  0111  0100  111
    ARM64_SYSREG_TRCPIDR5          = 0x8baf, // 10  001  0111  0101  111
    ARM64_SYSREG_TRCPIDR6          = 0x8bb7, // 10  001  0111  0110  111
    ARM64_SYSREG_TRCPIDR7          = 0x8bbf, // 10  001  0111  0111  111
    ARM64_SYSREG_TRCPIDR0          = 0x8bc7, // 10  001  0111  1000  111
    ARM64_SYSREG_TRCPIDR1          = 0x8bcf, // 10  001  0111  1001  111
    ARM64_SYSREG_TRCPIDR2          = 0x8bd7, // 10  001  0111  1010  111
    ARM64_SYSREG_TRCPIDR3          = 0x8bdf, // 10  001  0111  1011  111
    ARM64_SYSREG_TRCCIDR0          = 0x8be7, // 10  001  0111  1100  111
    ARM64_SYSREG_TRCCIDR1          = 0x8bef, // 10  001  0111  1101  111
    ARM64_SYSREG_TRCCIDR2          = 0x8bf7, // 10  001  0111  1110  111
    ARM64_SYSREG_TRCCIDR3          = 0x8bff, // 10  001  0111  1111  111

    // GICv3 registers
    ARM64_SYSREG_ICC_IAR1_EL1      = 0xc660, // 11  000  1100  1100  000
    ARM64_SYSREG_ICC_IAR0_EL1      = 0xc640, // 11  000  1100  1000  000
    ARM64_SYSREG_ICC_HPPIR1_EL1    = 0xc662, // 11  000  1100  1100  010
    ARM64_SYSREG_ICC_HPPIR0_EL1    = 0xc642, // 11  000  1100  1000  010
    ARM64_SYSREG_ICC_RPR_EL1       = 0xc65b, // 11  000  1100  1011  011
    ARM64_SYSREG_ICH_VTR_EL2       = 0xe659, // 11  100  1100  1011  001
    ARM64_SYSREG_ICH_EISR_EL2      = 0xe65b, // 11  100  1100  1011  011
    ARM64_SYSREG_ICH_ELSR_EL2      = 0xe65d, // 11  100  1100  1011  101
} arm64_sysreg;

typedef enum arm64_msr_reg
{
    //> System registers for MSR
    ARM64_SYSREG_DBGDTRTX_EL0      = 0x9828, // 10  011  0000  0101  000
    ARM64_SYSREG_OSLAR_EL1         = 0x8084, // 10  000  0001  0000  100
    ARM64_SYSREG_PMSWINC_EL0       = 0xdce4,  // 11  011  1001  1100  100

    // Trace Registers
    ARM64_SYSREG_TRCOSLAR          = 0x8884, // 10  001  0001  0000  100
    ARM64_SYSREG_TRCLAR            = 0x8be6, // 10  001  0111  1100  110

    // GICv3 registers
    ARM64_SYSREG_ICC_EOIR1_EL1     = 0xc661, // 11  000  1100  1100  001
    ARM64_SYSREG_ICC_EOIR0_EL1     = 0xc641, // 11  000  1100  1000  001
    ARM64_SYSREG_ICC_DIR_EL1       = 0xc659, // 11  000  1100  1011  001
    ARM64_SYSREG_ICC_SGI1R_EL1     = 0xc65d, // 11  000  1100  1011  101
    ARM64_SYSREG_ICC_ASGI1R_EL1    = 0xc65e, // 11  000  1100  1011  110
    ARM64_SYSREG_ICC_SGI0R_EL1     = 0xc65f, // 11  000  1100  1011  111
} arm64_msr_reg;

//> System PState Field (MSR instruction)
typedef enum arm64_pstate
{
    ARM64_PSTATE_INVALID = 0,
    ARM64_PSTATE_SPSEL = 0x05,
    ARM64_PSTATE_DAIFSET = 0x1e,
    ARM64_PSTATE_DAIFCLR = 0x1f
} arm64_pstate;

//> Vector arrangement specifier (for FloatingPoint/Advanced SIMD insn)
typedef enum arm64_vas
{
    ARM64_VAS_INVALID = 0,
    ARM64_VAS_8B,
    ARM64_VAS_16B,
    ARM64_VAS_4H,
    ARM64_VAS_8H,
    ARM64_VAS_2S,
    ARM64_VAS_4S,
    ARM64_VAS_1D,
    ARM64_VAS_2D,
    ARM64_VAS_1Q,
} arm64_vas;

//> Vector element size specifier
typedef enum arm64_vess
{
    ARM64_VESS_INVALID = 0,
    ARM64_VESS_B,
    ARM64_VESS_H,
    ARM64_VESS_S,
    ARM64_VESS_D,
} arm64_vess;

//> Memory barrier operands
typedef enum arm64_barrier_op
{
    ARM64_BARRIER_INVALID = 0,
    ARM64_BARRIER_OSHLD = 0x1,
    ARM64_BARRIER_OSHST = 0x2,
    ARM64_BARRIER_OSH =   0x3,
    ARM64_BARRIER_NSHLD = 0x5,
    ARM64_BARRIER_NSHST = 0x6,
    ARM64_BARRIER_NSH =   0x7,
    ARM64_BARRIER_ISHLD = 0x9,
    ARM64_BARRIER_ISHST = 0xa,
    ARM64_BARRIER_ISH =   0xb,
    ARM64_BARRIER_LD =    0xd,
    ARM64_BARRIER_ST =    0xe,
    ARM64_BARRIER_SY =    0xf
} arm64_barrier_op;

//> Operand type for instruction's operands
typedef enum arm64_op_type
{
    ARM64_OP_INVALID = 0, // = CS_OP_INVALID (Uninitialized).
    ARM64_OP_REG, // = CS_OP_REG (Register operand).
    ARM64_OP_IMM, // = CS_OP_IMM (Immediate operand).
    ARM64_OP_MEM, // = CS_OP_MEM (Memory operand).
    ARM64_OP_FP,  // = CS_OP_FP (Floating-Point operand).
    ARM64_OP_CIMM = 64, // C-Immediate
    ARM64_OP_REG_MRS, // MRS register operand.
    ARM64_OP_REG_MSR, // MSR register operand.
    ARM64_OP_PSTATE, // PState operand.
    ARM64_OP_SYS, // SYS operand for IC/DC/AT/TLBI instructions.
    ARM64_OP_PREFETCH, // Prefetch operand (PRFM).
    ARM64_OP_BARRIER, // Memory barrier operand (ISB/DMB/DSB instructions).
} arm64_op_type;

//> TLBI operations
typedef enum arm64_tlbi_op
{
    ARM64_TLBI_INVALID = 0,
    ARM64_TLBI_VMALLE1IS,
    ARM64_TLBI_VAE1IS,
    ARM64_TLBI_ASIDE1IS,
    ARM64_TLBI_VAAE1IS,
    ARM64_TLBI_VALE1IS,
    ARM64_TLBI_VAALE1IS,
    ARM64_TLBI_ALLE2IS,
    ARM64_TLBI_VAE2IS,
    ARM64_TLBI_ALLE1IS,
    ARM64_TLBI_VALE2IS,
    ARM64_TLBI_VMALLS12E1IS,
    ARM64_TLBI_ALLE3IS,
    ARM64_TLBI_VAE3IS,
    ARM64_TLBI_VALE3IS,
    ARM64_TLBI_IPAS2E1IS,
    ARM64_TLBI_IPAS2LE1IS,
    ARM64_TLBI_IPAS2E1,
    ARM64_TLBI_IPAS2LE1,
    ARM64_TLBI_VMALLE1,
    ARM64_TLBI_VAE1,
    ARM64_TLBI_ASIDE1,
    ARM64_TLBI_VAAE1,
    ARM64_TLBI_VALE1,
    ARM64_TLBI_VAALE1,
    ARM64_TLBI_ALLE2,
    ARM64_TLBI_VAE2,
    ARM64_TLBI_ALLE1,
    ARM64_TLBI_VALE2,
    ARM64_TLBI_VMALLS12E1,
    ARM64_TLBI_ALLE3,
    ARM64_TLBI_VAE3,
    ARM64_TLBI_VALE3,
} arm64_tlbi_op;

//> AT operations
typedef enum arm64_at_op
{
    ARM64_AT_S1E1R,
    ARM64_AT_S1E1W,
    ARM64_AT_S1E0R,
    ARM64_AT_S1E0W,
    ARM64_AT_S1E2R,
    ARM64_AT_S1E2W,
    ARM64_AT_S12E1R,
    ARM64_AT_S12E1W,
    ARM64_AT_S12E0R,
    ARM64_AT_S12E0W,
    ARM64_AT_S1E3R,
    ARM64_AT_S1E3W,
} arm64_at_op;

//> DC operations
typedef enum arm64_dc_op
{
    ARM64_DC_INVALID = 0,
    ARM64_DC_ZVA,
    ARM64_DC_IVAC,
    ARM64_DC_ISW,
    ARM64_DC_CVAC,
    ARM64_DC_CSW,
    ARM64_DC_CVAU,
    ARM64_DC_CIVAC,
    ARM64_DC_CISW,
} arm64_dc_op;

//> IC operations
typedef enum arm64_ic_op
{
    ARM64_IC_INVALID = 0,
    ARM64_IC_IALLUIS,
    ARM64_IC_IALLU,
    ARM64_IC_IVAU,
} arm64_ic_op;

//> Prefetch operations (PRFM)
typedef enum arm64_prefetch_op
{
    ARM64_PRFM_INVALID = 0,
    ARM64_PRFM_PLDL1KEEP = 0x00 + 1,
    ARM64_PRFM_PLDL1STRM = 0x01 + 1,
    ARM64_PRFM_PLDL2KEEP = 0x02 + 1,
    ARM64_PRFM_PLDL2STRM = 0x03 + 1,
    ARM64_PRFM_PLDL3KEEP = 0x04 + 1,
    ARM64_PRFM_PLDL3STRM = 0x05 + 1,
    ARM64_PRFM_PLIL1KEEP = 0x08 + 1,
    ARM64_PRFM_PLIL1STRM = 0x09 + 1,
    ARM64_PRFM_PLIL2KEEP = 0x0a + 1,
    ARM64_PRFM_PLIL2STRM = 0x0b + 1,
    ARM64_PRFM_PLIL3KEEP = 0x0c + 1,
    ARM64_PRFM_PLIL3STRM = 0x0d + 1,
    ARM64_PRFM_PSTL1KEEP = 0x10 + 1,
    ARM64_PRFM_PSTL1STRM = 0x11 + 1,
    ARM64_PRFM_PSTL2KEEP = 0x12 + 1,
    ARM64_PRFM_PSTL2STRM = 0x13 + 1,
    ARM64_PRFM_PSTL3KEEP = 0x14 + 1,
    ARM64_PRFM_PSTL3STRM = 0x15 + 1,
} arm64_prefetch_op;

// Instruction's operand referring to memory
// This is associated with ARM64_OP_MEM operand type above
typedef struct arm64_op_mem
{
    unsigned int base;  // base register
    unsigned int index; // index register
    int32_t disp;   // displacement/offset value
} arm64_op_mem;

// Instruction operand
typedef struct cs_arm64_op
{
    int vector_index;   // Vector Index for some vector operands (or -1 if irrelevant)
    arm64_vas vas;      // Vector Arrangement Specifier
    arm64_vess vess;    // Vector Element Size Specifier
    struct
    {
        arm64_shifter type; // shifter type of this operand
        unsigned int value; // shifter value of this operand
    } shift;
    arm64_extender ext;     // extender type of this operand
    arm64_op_type type; // operand type
    union
    {
        unsigned int reg;   // register value for REG operand
        int64_t imm;        // immediate value, or index for C-IMM or IMM operand
        double fp;          // floating point value for FP operand
        arm64_op_mem mem;       // base/index/scale/disp value for MEM operand
        arm64_pstate pstate;        // PState field of MSR instruction.
        unsigned int sys;  // IC/DC/AT/TLBI operation (see arm64_ic_op, arm64_dc_op, arm64_at_op, arm64_tlbi_op)
        arm64_prefetch_op prefetch;  // PRFM operation.
        arm64_barrier_op barrier;  // Memory barrier operation (ISB/DMB/DSB instructions).
    };
} cs_arm64_op;

// Instruction structure
typedef struct cs_arm64
{
    arm64_cc cc;    // conditional code for this insn
    bool update_flags;  // does this insn update flags?
    bool writeback; // does this insn request writeback? 'True' means 'yes'

    // Number of operands of this instruction,
    // or 0 when instruction has no operand.
    uint8_t op_count;

    cs_arm64_op operands[8]; // operands for this instruction.
} cs_arm64;

//> ARM64 registers
typedef enum arm64_reg
{
    ARM64_REG_INVALID = 0,

    ARM64_REG_X29,
    ARM64_REG_X30,
    ARM64_REG_NZCV,
    ARM64_REG_SP,
    ARM64_REG_WSP,
    ARM64_REG_WZR,
    ARM64_REG_XZR,
    ARM64_REG_B0,
    ARM64_REG_B1,
    ARM64_REG_B2,
    ARM64_REG_B3,
    ARM64_REG_B4,
    ARM64_REG_B5,
    ARM64_REG_B6,
    ARM64_REG_B7,
    ARM64_REG_B8,
    ARM64_REG_B9,
    ARM64_REG_B10,
    ARM64_REG_B11,
    ARM64_REG_B12,
    ARM64_REG_B13,
    ARM64_REG_B14,
    ARM64_REG_B15,
    ARM64_REG_B16,
    ARM64_REG_B17,
    ARM64_REG_B18,
    ARM64_REG_B19,
    ARM64_REG_B20,
    ARM64_REG_B21,
    ARM64_REG_B22,
    ARM64_REG_B23,
    ARM64_REG_B24,
    ARM64_REG_B25,
    ARM64_REG_B26,
    ARM64_REG_B27,
    ARM64_REG_B28,
    ARM64_REG_B29,
    ARM64_REG_B30,
    ARM64_REG_B31,
    ARM64_REG_D0,
    ARM64_REG_D1,
    ARM64_REG_D2,
    ARM64_REG_D3,
    ARM64_REG_D4,
    ARM64_REG_D5,
    ARM64_REG_D6,
    ARM64_REG_D7,
    ARM64_REG_D8,
    ARM64_REG_D9,
    ARM64_REG_D10,
    ARM64_REG_D11,
    ARM64_REG_D12,
    ARM64_REG_D13,
    ARM64_REG_D14,
    ARM64_REG_D15,
    ARM64_REG_D16,
    ARM64_REG_D17,
    ARM64_REG_D18,
    ARM64_REG_D19,
    ARM64_REG_D20,
    ARM64_REG_D21,
    ARM64_REG_D22,
    ARM64_REG_D23,
    ARM64_REG_D24,
    ARM64_REG_D25,
    ARM64_REG_D26,
    ARM64_REG_D27,
    ARM64_REG_D28,
    ARM64_REG_D29,
    ARM64_REG_D30,
    ARM64_REG_D31,
    ARM64_REG_H0,
    ARM64_REG_H1,
    ARM64_REG_H2,
    ARM64_REG_H3,
    ARM64_REG_H4,
    ARM64_REG_H5,
    ARM64_REG_H6,
    ARM64_REG_H7,
    ARM64_REG_H8,
    ARM64_REG_H9,
    ARM64_REG_H10,
    ARM64_REG_H11,
    ARM64_REG_H12,
    ARM64_REG_H13,
    ARM64_REG_H14,
    ARM64_REG_H15,
    ARM64_REG_H16,
    ARM64_REG_H17,
    ARM64_REG_H18,
    ARM64_REG_H19,
    ARM64_REG_H20,
    ARM64_REG_H21,
    ARM64_REG_H22,
    ARM64_REG_H23,
    ARM64_REG_H24,
    ARM64_REG_H25,
    ARM64_REG_H26,
    ARM64_REG_H27,
    ARM64_REG_H28,
    ARM64_REG_H29,
    ARM64_REG_H30,
    ARM64_REG_H31,
    ARM64_REG_Q0,
    ARM64_REG_Q1,
    ARM64_REG_Q2,
    ARM64_REG_Q3,
    ARM64_REG_Q4,
    ARM64_REG_Q5,
    ARM64_REG_Q6,
    ARM64_REG_Q7,
    ARM64_REG_Q8,
    ARM64_REG_Q9,
    ARM64_REG_Q10,
    ARM64_REG_Q11,
    ARM64_REG_Q12,
    ARM64_REG_Q13,
    ARM64_REG_Q14,
    ARM64_REG_Q15,
    ARM64_REG_Q16,
    ARM64_REG_Q17,
    ARM64_REG_Q18,
    ARM64_REG_Q19,
    ARM64_REG_Q20,
    ARM64_REG_Q21,
    ARM64_REG_Q22,
    ARM64_REG_Q23,
    ARM64_REG_Q24,
    ARM64_REG_Q25,
    ARM64_REG_Q26,
    ARM64_REG_Q27,
    ARM64_REG_Q28,
    ARM64_REG_Q29,
    ARM64_REG_Q30,
    ARM64_REG_Q31,
    ARM64_REG_S0,
    ARM64_REG_S1,
    ARM64_REG_S2,
    ARM64_REG_S3,
    ARM64_REG_S4,
    ARM64_REG_S5,
    ARM64_REG_S6,
    ARM64_REG_S7,
    ARM64_REG_S8,
    ARM64_REG_S9,
    ARM64_REG_S10,
    ARM64_REG_S11,
    ARM64_REG_S12,
    ARM64_REG_S13,
    ARM64_REG_S14,
    ARM64_REG_S15,
    ARM64_REG_S16,
    ARM64_REG_S17,
    ARM64_REG_S18,
    ARM64_REG_S19,
    ARM64_REG_S20,
    ARM64_REG_S21,
    ARM64_REG_S22,
    ARM64_REG_S23,
    ARM64_REG_S24,
    ARM64_REG_S25,
    ARM64_REG_S26,
    ARM64_REG_S27,
    ARM64_REG_S28,
    ARM64_REG_S29,
    ARM64_REG_S30,
    ARM64_REG_S31,
    ARM64_REG_W0,
    ARM64_REG_W1,
    ARM64_REG_W2,
    ARM64_REG_W3,
    ARM64_REG_W4,
    ARM64_REG_W5,
    ARM64_REG_W6,
    ARM64_REG_W7,
    ARM64_REG_W8,
    ARM64_REG_W9,
    ARM64_REG_W10,
    ARM64_REG_W11,
    ARM64_REG_W12,
    ARM64_REG_W13,
    ARM64_REG_W14,
    ARM64_REG_W15,
    ARM64_REG_W16,
    ARM64_REG_W17,
    ARM64_REG_W18,
    ARM64_REG_W19,
    ARM64_REG_W20,
    ARM64_REG_W21,
    ARM64_REG_W22,
    ARM64_REG_W23,
    ARM64_REG_W24,
    ARM64_REG_W25,
    ARM64_REG_W26,
    ARM64_REG_W27,
    ARM64_REG_W28,
    ARM64_REG_W29,
    ARM64_REG_W30,
    ARM64_REG_X0,
    ARM64_REG_X1,
    ARM64_REG_X2,
    ARM64_REG_X3,
    ARM64_REG_X4,
    ARM64_REG_X5,
    ARM64_REG_X6,
    ARM64_REG_X7,
    ARM64_REG_X8,
    ARM64_REG_X9,
    ARM64_REG_X10,
    ARM64_REG_X11,
    ARM64_REG_X12,
    ARM64_REG_X13,
    ARM64_REG_X14,
    ARM64_REG_X15,
    ARM64_REG_X16,
    ARM64_REG_X17,
    ARM64_REG_X18,
    ARM64_REG_X19,
    ARM64_REG_X20,
    ARM64_REG_X21,
    ARM64_REG_X22,
    ARM64_REG_X23,
    ARM64_REG_X24,
    ARM64_REG_X25,
    ARM64_REG_X26,
    ARM64_REG_X27,
    ARM64_REG_X28,

    ARM64_REG_V0,
    ARM64_REG_V1,
    ARM64_REG_V2,
    ARM64_REG_V3,
    ARM64_REG_V4,
    ARM64_REG_V5,
    ARM64_REG_V6,
    ARM64_REG_V7,
    ARM64_REG_V8,
    ARM64_REG_V9,
    ARM64_REG_V10,
    ARM64_REG_V11,
    ARM64_REG_V12,
    ARM64_REG_V13,
    ARM64_REG_V14,
    ARM64_REG_V15,
    ARM64_REG_V16,
    ARM64_REG_V17,
    ARM64_REG_V18,
    ARM64_REG_V19,
    ARM64_REG_V20,
    ARM64_REG_V21,
    ARM64_REG_V22,
    ARM64_REG_V23,
    ARM64_REG_V24,
    ARM64_REG_V25,
    ARM64_REG_V26,
    ARM64_REG_V27,
    ARM64_REG_V28,
    ARM64_REG_V29,
    ARM64_REG_V30,
    ARM64_REG_V31,

    ARM64_REG_ENDING,       // <-- mark the end of the list of registers

    //> alias registers

    ARM64_REG_IP1 = ARM64_REG_X16,
    ARM64_REG_IP0 = ARM64_REG_X17,
    ARM64_REG_FP = ARM64_REG_X29,
    ARM64_REG_LR = ARM64_REG_X30,
} arm64_reg;

//> ARM64 instruction
typedef enum arm64_insn
{
    ARM64_INS_INVALID = 0,

    ARM64_INS_ABS,
    ARM64_INS_ADC,
    ARM64_INS_ADDHN,
    ARM64_INS_ADDHN2,
    ARM64_INS_ADDP,
    ARM64_INS_ADD,
    ARM64_INS_ADDV,
    ARM64_INS_ADR,
    ARM64_INS_ADRP,
    ARM64_INS_AESD,
    ARM64_INS_AESE,
    ARM64_INS_AESIMC,
    ARM64_INS_AESMC,
    ARM64_INS_AND,
    ARM64_INS_ASR,
    ARM64_INS_B,
    ARM64_INS_BFM,
    ARM64_INS_BIC,
    ARM64_INS_BIF,
    ARM64_INS_BIT,
    ARM64_INS_BL,
    ARM64_INS_BLR,
    ARM64_INS_BR,
    ARM64_INS_BRK,
    ARM64_INS_BSL,
    ARM64_INS_CBNZ,
    ARM64_INS_CBZ,
    ARM64_INS_CCMN,
    ARM64_INS_CCMP,
    ARM64_INS_CLREX,
    ARM64_INS_CLS,
    ARM64_INS_CLZ,
    ARM64_INS_CMEQ,
    ARM64_INS_CMGE,
    ARM64_INS_CMGT,
    ARM64_INS_CMHI,
    ARM64_INS_CMHS,
    ARM64_INS_CMLE,
    ARM64_INS_CMLT,
    ARM64_INS_CMTST,
    ARM64_INS_CNT,
    ARM64_INS_MOV,
    ARM64_INS_CRC32B,
    ARM64_INS_CRC32CB,
    ARM64_INS_CRC32CH,
    ARM64_INS_CRC32CW,
    ARM64_INS_CRC32CX,
    ARM64_INS_CRC32H,
    ARM64_INS_CRC32W,
    ARM64_INS_CRC32X,
    ARM64_INS_CSEL,
    ARM64_INS_CSINC,
    ARM64_INS_CSINV,
    ARM64_INS_CSNEG,
    ARM64_INS_DCPS1,
    ARM64_INS_DCPS2,
    ARM64_INS_DCPS3,
    ARM64_INS_DMB,
    ARM64_INS_DRPS,
    ARM64_INS_DSB,
    ARM64_INS_DUP,
    ARM64_INS_EON,
    ARM64_INS_EOR,
    ARM64_INS_ERET,
    ARM64_INS_EXTR,
    ARM64_INS_EXT,
    ARM64_INS_FABD,
    ARM64_INS_FABS,
    ARM64_INS_FACGE,
    ARM64_INS_FACGT,
    ARM64_INS_FADD,
    ARM64_INS_FADDP,
    ARM64_INS_FCCMP,
    ARM64_INS_FCCMPE,
    ARM64_INS_FCMEQ,
    ARM64_INS_FCMGE,
    ARM64_INS_FCMGT,
    ARM64_INS_FCMLE,
    ARM64_INS_FCMLT,
    ARM64_INS_FCMP,
    ARM64_INS_FCMPE,
    ARM64_INS_FCSEL,
    ARM64_INS_FCVTAS,
    ARM64_INS_FCVTAU,
    ARM64_INS_FCVT,
    ARM64_INS_FCVTL,
    ARM64_INS_FCVTL2,
    ARM64_INS_FCVTMS,
    ARM64_INS_FCVTMU,
    ARM64_INS_FCVTNS,
    ARM64_INS_FCVTNU,
    ARM64_INS_FCVTN,
    ARM64_INS_FCVTN2,
    ARM64_INS_FCVTPS,
    ARM64_INS_FCVTPU,
    ARM64_INS_FCVTXN,
    ARM64_INS_FCVTXN2,
    ARM64_INS_FCVTZS,
    ARM64_INS_FCVTZU,
    ARM64_INS_FDIV,
    ARM64_INS_FMADD,
    ARM64_INS_FMAX,
    ARM64_INS_FMAXNM,
    ARM64_INS_FMAXNMP,
    ARM64_INS_FMAXNMV,
    ARM64_INS_FMAXP,
    ARM64_INS_FMAXV,
    ARM64_INS_FMIN,
    ARM64_INS_FMINNM,
    ARM64_INS_FMINNMP,
    ARM64_INS_FMINNMV,
    ARM64_INS_FMINP,
    ARM64_INS_FMINV,
    ARM64_INS_FMLA,
    ARM64_INS_FMLS,
    ARM64_INS_FMOV,
    ARM64_INS_FMSUB,
    ARM64_INS_FMUL,
    ARM64_INS_FMULX,
    ARM64_INS_FNEG,
    ARM64_INS_FNMADD,
    ARM64_INS_FNMSUB,
    ARM64_INS_FNMUL,
    ARM64_INS_FRECPE,
    ARM64_INS_FRECPS,
    ARM64_INS_FRECPX,
    ARM64_INS_FRINTA,
    ARM64_INS_FRINTI,
    ARM64_INS_FRINTM,
    ARM64_INS_FRINTN,
    ARM64_INS_FRINTP,
    ARM64_INS_FRINTX,
    ARM64_INS_FRINTZ,
    ARM64_INS_FRSQRTE,
    ARM64_INS_FRSQRTS,
    ARM64_INS_FSQRT,
    ARM64_INS_FSUB,
    ARM64_INS_HINT,
    ARM64_INS_HLT,
    ARM64_INS_HVC,
    ARM64_INS_INS,

    ARM64_INS_ISB,
    ARM64_INS_LD1,
    ARM64_INS_LD1R,
    ARM64_INS_LD2R,
    ARM64_INS_LD2,
    ARM64_INS_LD3R,
    ARM64_INS_LD3,
    ARM64_INS_LD4,
    ARM64_INS_LD4R,

    ARM64_INS_LDARB,
    ARM64_INS_LDARH,
    ARM64_INS_LDAR,
    ARM64_INS_LDAXP,
    ARM64_INS_LDAXRB,
    ARM64_INS_LDAXRH,
    ARM64_INS_LDAXR,
    ARM64_INS_LDNP,
    ARM64_INS_LDP,
    ARM64_INS_LDPSW,
    ARM64_INS_LDRB,
    ARM64_INS_LDR,
    ARM64_INS_LDRH,
    ARM64_INS_LDRSB,
    ARM64_INS_LDRSH,
    ARM64_INS_LDRSW,
    ARM64_INS_LDTRB,
    ARM64_INS_LDTRH,
    ARM64_INS_LDTRSB,

    ARM64_INS_LDTRSH,
    ARM64_INS_LDTRSW,
    ARM64_INS_LDTR,
    ARM64_INS_LDURB,
    ARM64_INS_LDUR,
    ARM64_INS_LDURH,
    ARM64_INS_LDURSB,
    ARM64_INS_LDURSH,
    ARM64_INS_LDURSW,
    ARM64_INS_LDXP,
    ARM64_INS_LDXRB,
    ARM64_INS_LDXRH,
    ARM64_INS_LDXR,
    ARM64_INS_LSL,
    ARM64_INS_LSR,
    ARM64_INS_MADD,
    ARM64_INS_MLA,
    ARM64_INS_MLS,
    ARM64_INS_MOVI,
    ARM64_INS_MOVK,
    ARM64_INS_MOVN,
    ARM64_INS_MOVZ,
    ARM64_INS_MRS,
    ARM64_INS_MSR,
    ARM64_INS_MSUB,
    ARM64_INS_MUL,
    ARM64_INS_MVNI,
    ARM64_INS_NEG,
    ARM64_INS_NOT,
    ARM64_INS_ORN,
    ARM64_INS_ORR,
    ARM64_INS_PMULL2,
    ARM64_INS_PMULL,
    ARM64_INS_PMUL,
    ARM64_INS_PRFM,
    ARM64_INS_PRFUM,
    ARM64_INS_RADDHN,
    ARM64_INS_RADDHN2,
    ARM64_INS_RBIT,
    ARM64_INS_RET,
    ARM64_INS_REV16,
    ARM64_INS_REV32,
    ARM64_INS_REV64,
    ARM64_INS_REV,
    ARM64_INS_ROR,
    ARM64_INS_RSHRN2,
    ARM64_INS_RSHRN,
    ARM64_INS_RSUBHN,
    ARM64_INS_RSUBHN2,
    ARM64_INS_SABAL2,
    ARM64_INS_SABAL,

    ARM64_INS_SABA,
    ARM64_INS_SABDL2,
    ARM64_INS_SABDL,
    ARM64_INS_SABD,
    ARM64_INS_SADALP,
    ARM64_INS_SADDLP,
    ARM64_INS_SADDLV,
    ARM64_INS_SADDL2,
    ARM64_INS_SADDL,
    ARM64_INS_SADDW2,
    ARM64_INS_SADDW,
    ARM64_INS_SBC,
    ARM64_INS_SBFM,
    ARM64_INS_SCVTF,
    ARM64_INS_SDIV,
    ARM64_INS_SHA1C,
    ARM64_INS_SHA1H,
    ARM64_INS_SHA1M,
    ARM64_INS_SHA1P,
    ARM64_INS_SHA1SU0,
    ARM64_INS_SHA1SU1,
    ARM64_INS_SHA256H2,
    ARM64_INS_SHA256H,
    ARM64_INS_SHA256SU0,
    ARM64_INS_SHA256SU1,
    ARM64_INS_SHADD,
    ARM64_INS_SHLL2,
    ARM64_INS_SHLL,
    ARM64_INS_SHL,
    ARM64_INS_SHRN2,
    ARM64_INS_SHRN,
    ARM64_INS_SHSUB,
    ARM64_INS_SLI,
    ARM64_INS_SMADDL,
    ARM64_INS_SMAXP,
    ARM64_INS_SMAXV,
    ARM64_INS_SMAX,
    ARM64_INS_SMC,
    ARM64_INS_SMINP,
    ARM64_INS_SMINV,
    ARM64_INS_SMIN,
    ARM64_INS_SMLAL2,
    ARM64_INS_SMLAL,
    ARM64_INS_SMLSL2,
    ARM64_INS_SMLSL,
    ARM64_INS_SMOV,
    ARM64_INS_SMSUBL,
    ARM64_INS_SMULH,
    ARM64_INS_SMULL2,
    ARM64_INS_SMULL,
    ARM64_INS_SQABS,
    ARM64_INS_SQADD,
    ARM64_INS_SQDMLAL,
    ARM64_INS_SQDMLAL2,
    ARM64_INS_SQDMLSL,
    ARM64_INS_SQDMLSL2,
    ARM64_INS_SQDMULH,
    ARM64_INS_SQDMULL,
    ARM64_INS_SQDMULL2,
    ARM64_INS_SQNEG,
    ARM64_INS_SQRDMULH,
    ARM64_INS_SQRSHL,
    ARM64_INS_SQRSHRN,
    ARM64_INS_SQRSHRN2,
    ARM64_INS_SQRSHRUN,
    ARM64_INS_SQRSHRUN2,
    ARM64_INS_SQSHLU,
    ARM64_INS_SQSHL,
    ARM64_INS_SQSHRN,
    ARM64_INS_SQSHRN2,
    ARM64_INS_SQSHRUN,
    ARM64_INS_SQSHRUN2,
    ARM64_INS_SQSUB,
    ARM64_INS_SQXTN2,
    ARM64_INS_SQXTN,
    ARM64_INS_SQXTUN2,
    ARM64_INS_SQXTUN,
    ARM64_INS_SRHADD,
    ARM64_INS_SRI,
    ARM64_INS_SRSHL,
    ARM64_INS_SRSHR,
    ARM64_INS_SRSRA,
    ARM64_INS_SSHLL2,
    ARM64_INS_SSHLL,
    ARM64_INS_SSHL,
    ARM64_INS_SSHR,
    ARM64_INS_SSRA,
    ARM64_INS_SSUBL2,
    ARM64_INS_SSUBL,
    ARM64_INS_SSUBW2,
    ARM64_INS_SSUBW,
    ARM64_INS_ST1,
    ARM64_INS_ST2,
    ARM64_INS_ST3,
    ARM64_INS_ST4,
    ARM64_INS_STLRB,
    ARM64_INS_STLRH,
    ARM64_INS_STLR,
    ARM64_INS_STLXP,
    ARM64_INS_STLXRB,
    ARM64_INS_STLXRH,
    ARM64_INS_STLXR,
    ARM64_INS_STNP,
    ARM64_INS_STP,
    ARM64_INS_STRB,
    ARM64_INS_STR,
    ARM64_INS_STRH,
    ARM64_INS_STTRB,
    ARM64_INS_STTRH,
    ARM64_INS_STTR,
    ARM64_INS_STURB,
    ARM64_INS_STUR,
    ARM64_INS_STURH,
    ARM64_INS_STXP,
    ARM64_INS_STXRB,
    ARM64_INS_STXRH,
    ARM64_INS_STXR,
    ARM64_INS_SUBHN,
    ARM64_INS_SUBHN2,
    ARM64_INS_SUB,
    ARM64_INS_SUQADD,
    ARM64_INS_SVC,
    ARM64_INS_SYSL,
    ARM64_INS_SYS,
    ARM64_INS_TBL,
    ARM64_INS_TBNZ,
    ARM64_INS_TBX,
    ARM64_INS_TBZ,
    ARM64_INS_TRN1,
    ARM64_INS_TRN2,
    ARM64_INS_UABAL2,
    ARM64_INS_UABAL,
    ARM64_INS_UABA,
    ARM64_INS_UABDL2,
    ARM64_INS_UABDL,
    ARM64_INS_UABD,
    ARM64_INS_UADALP,
    ARM64_INS_UADDLP,
    ARM64_INS_UADDLV,
    ARM64_INS_UADDL2,
    ARM64_INS_UADDL,
    ARM64_INS_UADDW2,
    ARM64_INS_UADDW,
    ARM64_INS_UBFM,
    ARM64_INS_UCVTF,
    ARM64_INS_UDIV,
    ARM64_INS_UHADD,
    ARM64_INS_UHSUB,
    ARM64_INS_UMADDL,
    ARM64_INS_UMAXP,
    ARM64_INS_UMAXV,
    ARM64_INS_UMAX,
    ARM64_INS_UMINP,
    ARM64_INS_UMINV,
    ARM64_INS_UMIN,
    ARM64_INS_UMLAL2,
    ARM64_INS_UMLAL,
    ARM64_INS_UMLSL2,
    ARM64_INS_UMLSL,
    ARM64_INS_UMOV,
    ARM64_INS_UMSUBL,
    ARM64_INS_UMULH,
    ARM64_INS_UMULL2,
    ARM64_INS_UMULL,
    ARM64_INS_UQADD,
    ARM64_INS_UQRSHL,
    ARM64_INS_UQRSHRN,
    ARM64_INS_UQRSHRN2,
    ARM64_INS_UQSHL,
    ARM64_INS_UQSHRN,
    ARM64_INS_UQSHRN2,
    ARM64_INS_UQSUB,
    ARM64_INS_UQXTN2,
    ARM64_INS_UQXTN,
    ARM64_INS_URECPE,
    ARM64_INS_URHADD,
    ARM64_INS_URSHL,
    ARM64_INS_URSHR,
    ARM64_INS_URSQRTE,
    ARM64_INS_URSRA,
    ARM64_INS_USHLL2,
    ARM64_INS_USHLL,
    ARM64_INS_USHL,
    ARM64_INS_USHR,
    ARM64_INS_USQADD,
    ARM64_INS_USRA,
    ARM64_INS_USUBL2,
    ARM64_INS_USUBL,
    ARM64_INS_USUBW2,
    ARM64_INS_USUBW,
    ARM64_INS_UZP1,
    ARM64_INS_UZP2,
    ARM64_INS_XTN2,
    ARM64_INS_XTN,
    ARM64_INS_ZIP1,
    ARM64_INS_ZIP2,

    // alias insn
    ARM64_INS_MNEG,
    ARM64_INS_UMNEGL,
    ARM64_INS_SMNEGL,
    ARM64_INS_NOP,
    ARM64_INS_YIELD,
    ARM64_INS_WFE,
    ARM64_INS_WFI,
    ARM64_INS_SEV,
    ARM64_INS_SEVL,
    ARM64_INS_NGC,
    ARM64_INS_SBFIZ,
    ARM64_INS_UBFIZ,
    ARM64_INS_SBFX,
    ARM64_INS_UBFX,
    ARM64_INS_BFI,
    ARM64_INS_BFXIL,
    ARM64_INS_CMN,
    ARM64_INS_MVN,
    ARM64_INS_TST,
    ARM64_INS_CSET,
    ARM64_INS_CINC,
    ARM64_INS_CSETM,
    ARM64_INS_CINV,
    ARM64_INS_CNEG,
    ARM64_INS_SXTB,
    ARM64_INS_SXTH,
    ARM64_INS_SXTW,
    ARM64_INS_CMP,
    ARM64_INS_UXTB,
    ARM64_INS_UXTH,
    ARM64_INS_UXTW,
    ARM64_INS_IC,
    ARM64_INS_DC,
    ARM64_INS_AT,
    ARM64_INS_TLBI,

    ARM64_INS_ENDING,  // <-- mark the end of the list of insn
} arm64_insn;

//> Group of ARM64 instructions
typedef enum arm64_insn_group
{
    ARM64_GRP_INVALID = 0, // = CS_GRP_INVALID

    //> Generic groups
    // all jump instructions (conditional+direct+indirect jumps)
    ARM64_GRP_JUMP, // = CS_GRP_JUMP

    //> Architecture-specific groups
    ARM64_GRP_CRYPTO = 128,
    ARM64_GRP_FPARMV8,
    ARM64_GRP_NEON,
    ARM64_GRP_CRC,

    ARM64_GRP_ENDING,  // <-- mark the end of the list of groups
} arm64_insn_group;

#ifdef __cplusplus
}
#endif

#endif
