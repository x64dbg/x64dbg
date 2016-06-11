/* Keystone Assembler Engine */
/* By Nguyen Anh Quynh, 2016 */

#ifndef KEYSTONE_SYSTEMZ_H
#define KEYSTONE_SYSTEMZ_H

#ifdef __cplusplus
extern "C" {
#endif

#include "keystone.h"

typedef enum ks_err_asm_systemz
{
    KS_ERR_ASM_SYSTEMZ_INVALIDOPERAND = KS_ERR_ASM_ARCH,
    KS_ERR_ASM_SYSTEMZ_MISSINGFEATURE,
    KS_ERR_ASM_SYSTEMZ_MNEMONICFAIL,
} ks_err_asm_systemz;


#ifdef __cplusplus
}
#endif

#endif
