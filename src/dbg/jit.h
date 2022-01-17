#ifndef _JIT_H
#define _JIT_H

#include "_global.h"

#define ATTACH_CMD_LINE     "\" -a %ld -e %ld"
#define JIT_REG_KEY         L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug"

#define JIT_ENTRY_MAX_SIZE  512
#define JIT_ENTRY_DEF_SIZE  (MAX_PATH + sizeof(ATTACH_CMD_LINE) + 2)

typedef enum
{
    ERROR_RW = 0,
    ERROR_RW_FILE_NOT_FOUND,
    ERROR_RW_NOTWOW64,
    ERROR_RW_NOTADMIN
} readwritejitkey_error_t;

enum arch
{
    notfound,
    x32,
    x64,
};

bool dbggetjit(char jit_entry[JIT_ENTRY_MAX_SIZE], arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out);
bool dbgsetjit(const char* jit_cmd, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out);
bool dbggetjitauto(bool* auto_on, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out);
bool dbgsetjitauto(bool auto_on, arch arch_in, arch* arch_out, readwritejitkey_error_t* rw_error_out);
bool dbggetdefjit(char* jit_entry);

#endif // _JIT_H