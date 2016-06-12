/* Keystone Assembler Engine */
/* By Nguyen Anh Quynh, 2016 */

#ifndef KEYSTONE_MIPS_H
#define KEYSTONE_MIPS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "keystone.h"

typedef enum ks_err_asm_mips
{
    KS_ERR_ASM_MIPS_INVALIDOPERAND = KS_ERR_ASM_ARCH,
    KS_ERR_ASM_MIPS_MISSINGFEATURE,
    KS_ERR_ASM_MIPS_MNEMONICFAIL,
} ks_err_asm_mips;

#ifdef __cplusplus
}
#endif

#endif
