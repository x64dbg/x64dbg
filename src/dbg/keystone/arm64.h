/* Keystone Assembler Engine */
/* By Nguyen Anh Quynh, 2016 */

#ifndef KEYSTONE_ARM64_H
#define KEYSTONE_ARM64_H

#ifdef __cplusplus
extern "C" {
#endif

#include "keystone.h"

typedef enum ks_err_asm_arm64
{
    KS_ERR_ASM_ARM64_INVALIDOPERAND = KS_ERR_ASM_ARCH,
    KS_ERR_ASM_ARM64_MISSINGFEATURE,
    KS_ERR_ASM_ARM64_MNEMONICFAIL,
} ks_err_asm_arm64;

#ifdef __cplusplus
}
#endif

#endif
