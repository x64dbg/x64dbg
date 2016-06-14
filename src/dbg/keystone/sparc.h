/* Keystone Assembler Engine */
/* By Nguyen Anh Quynh, 2016 */

#ifndef KEYSTONE_SPARC_H
#define KEYSTONE_SPARC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "keystone.h"

typedef enum ks_err_asm_sparc
{
    KS_ERR_ASM_SPARC_INVALIDOPERAND = KS_ERR_ASM_ARCH,
    KS_ERR_ASM_SPARC_MISSINGFEATURE,
    KS_ERR_ASM_SPARC_MNEMONICFAIL,
} ks_err_asm_sparc;


#ifdef __cplusplus
}
#endif

#endif
