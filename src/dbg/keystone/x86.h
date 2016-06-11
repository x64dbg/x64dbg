/* Keystone Assembler Engine */
/* By Nguyen Anh Quynh, 2016 */

#ifndef KEYSTONE_X86_H
#define KEYSTONE_X86_H

#ifdef __cplusplus
extern "C" {
#endif

#include "keystone.h"

typedef enum ks_err_asm_x86
{
    KS_ERR_ASM_X86_INVALIDOPERAND = KS_ERR_ASM_ARCH,
    KS_ERR_ASM_X86_MISSINGFEATURE,
    KS_ERR_ASM_X86_MNEMONICFAIL,
} ks_err_asm_x86;

#ifdef __cplusplus
}
#endif

#endif
