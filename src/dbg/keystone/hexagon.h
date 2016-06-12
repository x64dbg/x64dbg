/* Keystone Assembler Engine */
/* By Nguyen Anh Quynh, 2016 */

#ifndef KEYSTONE_HEXAGON_H
#define KEYSTONE_HEXAGON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "keystone.h"

typedef enum ks_err_asm_hexagon
{
    KS_ERR_ASM_HEXAGON_INVALIDOPERAND = KS_ERR_ASM_ARCH,
    KS_ERR_ASM_HEXAGON_MISSINGFEATURE,
    KS_ERR_ASM_HEXAGON_MNEMONICFAIL,
} ks_err_asm_hexagon;


#ifdef __cplusplus
}
#endif

#endif
