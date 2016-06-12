/* Keystone Assembler Engine */
/* By Nguyen Anh Quynh, 2016 */

#ifndef KEYSTONE_PPC_H
#define KEYSTONE_PPC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "keystone.h"

typedef enum ks_err_asm_ppc
{
    KS_ERR_ASM_PPC_INVALIDOPERAND = KS_ERR_ASM_ARCH,
    KS_ERR_ASM_PPC_MISSINGFEATURE,
    KS_ERR_ASM_PPC_MNEMONICFAIL,
} ks_err_asm_ppc;


#ifdef __cplusplus
}
#endif

#endif
