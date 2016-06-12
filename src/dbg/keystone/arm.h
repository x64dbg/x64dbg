/* Keystone Assembler Engine */
/* By Nguyen Anh Quynh, 2016 */

#ifndef KEYSTONE_ARM_H
#define KEYSTONE_ARM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "keystone.h"

typedef enum ks_err_asm_arm
{
    KS_ERR_ASM_ARM_INVALIDOPERAND = KS_ERR_ASM_ARCH,
    KS_ERR_ASM_ARM_MISSINGFEATURE,
    KS_ERR_ASM_ARM_MNEMONICFAIL,
} ks_err_asm_arm;

#ifdef __cplusplus
}
#endif

#endif
