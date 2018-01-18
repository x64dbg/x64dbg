/* Keystone Assembler Engine (www.keystone-engine.org) */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2016 */

#ifndef KEYSTONE_ENGINE_H
#define KEYSTONE_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef _MSC_VER     // MSVC compiler
#pragma warning(disable:4201)
#pragma warning(disable:4100)
#define KEYSTONE_EXPORT __declspec(dllexport)
#else
#ifdef __GNUC__
#define KEYSTONE_EXPORT __attribute__((visibility("default")))
#else
#define KEYSTONE_EXPORT
#endif
#endif


struct ks_struct;
typedef struct ks_struct ks_engine;

// Keystone API version
#define KS_API_MAJOR 0
#define KS_API_MINOR 9

/*
  Macro to create combined version which can be compared to
  result of ks_version() API.
*/
#define KS_MAKE_VERSION(major, minor) ((major << 8) + minor)

// Architecture type
typedef enum ks_arch
{
    KS_ARCH_ARM = 1,    // ARM architecture (including Thumb, Thumb-2)
    KS_ARCH_ARM64,      // ARM-64, also called AArch64
    KS_ARCH_MIPS,       // Mips architecture
    KS_ARCH_X86,        // X86 architecture (including x86 & x86-64)
    KS_ARCH_PPC,        // PowerPC architecture (currently unsupported)
    KS_ARCH_SPARC,      // Sparc architecture
    KS_ARCH_SYSTEMZ,    // SystemZ architecture (S390X)
    KS_ARCH_HEXAGON,    // Hexagon architecture
    KS_ARCH_MAX,
} ks_arch;

// Mode type
typedef enum ks_mode
{
    KS_MODE_LITTLE_ENDIAN = 0,    // little-endian mode (default mode)
    KS_MODE_BIG_ENDIAN = 1 << 30, // big-endian mode
    // arm / arm64
    KS_MODE_ARM = 1 << 0,              // ARM mode
    KS_MODE_THUMB = 1 << 4,       // THUMB mode (including Thumb-2)
    KS_MODE_V8 = 1 << 6,          // ARMv8 A32 encodings for ARM
    // mips
    KS_MODE_MICRO = 1 << 4,       // MicroMips mode
    KS_MODE_MIPS3 = 1 << 5,       // Mips III ISA
    KS_MODE_MIPS32R6 = 1 << 6,    // Mips32r6 ISA
    KS_MODE_MIPS32 = 1 << 2,      // Mips32 ISA
    KS_MODE_MIPS64 = 1 << 3,      // Mips64 ISA
    // x86 / x64
    KS_MODE_16 = 1 << 1,          // 16-bit mode
    KS_MODE_32 = 1 << 2,          // 32-bit mode
    KS_MODE_64 = 1 << 3,          // 64-bit mode
    // ppc
    KS_MODE_PPC32 = 1 << 2,       // 32-bit mode
    KS_MODE_PPC64 = 1 << 3,       // 64-bit mode
    KS_MODE_QPX = 1 << 4,         // Quad Processing eXtensions mode
    // sparc
    KS_MODE_SPARC32 = 1 << 2,     // 32-bit mode
    KS_MODE_SPARC64 = 1 << 3,     // 64-bit mode
    KS_MODE_V9 = 1 << 4,          // SparcV9 mode
} ks_mode;

// All generic errors related to input assembly >= KS_ERR_ASM
#define KS_ERR_ASM 128

// All architecture-specific errors related to input assembly >= KS_ERR_ASM_ARCH
#define KS_ERR_ASM_ARCH 512

// All type of errors encountered by Keystone API.
typedef enum ks_err
{
    KS_ERR_OK = 0,   // No error: everything was fine
    KS_ERR_NOMEM,      // Out-Of-Memory error: ks_open(), ks_emulate()
    KS_ERR_ARCH,     // Unsupported architecture: ks_open()
    KS_ERR_HANDLE,   // Invalid handle
    KS_ERR_MODE,     // Invalid/unsupported mode: ks_open()
    KS_ERR_VERSION,  // Unsupported version (bindings)
    KS_ERR_OPT_INVALID,  // Unsupported option

    // generic input assembly errors - parser specific
    KS_ERR_ASM_EXPR_TOKEN = KS_ERR_ASM,    // unknown token in expression
    KS_ERR_ASM_DIRECTIVE_VALUE_RANGE,   // literal value out of range for directive
    KS_ERR_ASM_DIRECTIVE_ID,    // expected identifier in directive
    KS_ERR_ASM_DIRECTIVE_TOKEN, // unexpected token in directive
    KS_ERR_ASM_DIRECTIVE_STR,   // expected string in directive
    KS_ERR_ASM_DIRECTIVE_COMMA, // expected comma in directive
    KS_ERR_ASM_DIRECTIVE_RELOC_NAME, // expected relocation name in directive
    KS_ERR_ASM_DIRECTIVE_RELOC_TOKEN, // unexpected token in .reloc directive
    KS_ERR_ASM_DIRECTIVE_FPOINT,    // invalid floating point in directive
    KS_ERR_ASM_DIRECTIVE_UNKNOWN,    // unknown directive
    KS_ERR_ASM_DIRECTIVE_EQU,   // invalid equal directive
    KS_ERR_ASM_DIRECTIVE_INVALID,   // (generic) invalid directive
    KS_ERR_ASM_VARIANT_INVALID, // invalid variant
    KS_ERR_ASM_EXPR_BRACKET,    // brackets expression not supported on this target
    KS_ERR_ASM_SYMBOL_MODIFIER, // unexpected symbol modifier following '@'
    KS_ERR_ASM_SYMBOL_REDEFINED, // invalid symbol redefinition
    KS_ERR_ASM_SYMBOL_MISSING,  // cannot find a symbol
    KS_ERR_ASM_RPAREN,          // expected ')' in parentheses expression
    KS_ERR_ASM_STAT_TOKEN,      // unexpected token at start of statement
    KS_ERR_ASM_UNSUPPORTED,     // unsupported token yet
    KS_ERR_ASM_MACRO_TOKEN,     // unexpected token in macro instantiation
    KS_ERR_ASM_MACRO_PAREN,     // unbalanced parentheses in macro argument
    KS_ERR_ASM_MACRO_EQU,       // expected '=' after formal parameter identifier
    KS_ERR_ASM_MACRO_ARGS,      // too many positional arguments
    KS_ERR_ASM_MACRO_LEVELS_EXCEED, // macros cannot be nested more than 20 levels deep
    KS_ERR_ASM_MACRO_STR,    // invalid macro string
    KS_ERR_ASM_MACRO_INVALID,    // invalid macro (generic error)
    KS_ERR_ASM_ESC_BACKSLASH,   // unexpected backslash at end of escaped string
    KS_ERR_ASM_ESC_OCTAL,       // invalid octal escape sequence  (out of range)
    KS_ERR_ASM_ESC_SEQUENCE,         // invalid escape sequence (unrecognized character)
    KS_ERR_ASM_ESC_STR,         // broken escape string
    KS_ERR_ASM_TOKEN_INVALID,   // invalid token
    KS_ERR_ASM_INSN_UNSUPPORTED,   // this instruction is unsupported in this mode
    KS_ERR_ASM_FIXUP_INVALID,   // invalid fixup
    KS_ERR_ASM_LABEL_INVALID,   // invalid label
    KS_ERR_ASM_FRAGMENT_INVALID,   // invalid fragment

    // generic input assembly errors - architecture specific
    KS_ERR_ASM_INVALIDOPERAND = KS_ERR_ASM_ARCH,
    KS_ERR_ASM_MISSINGFEATURE,
    KS_ERR_ASM_MNEMONICFAIL,
} ks_err;

// Resolver callback to provide value for a missing symbol in @symbol.
// To handle a symbol, the resolver must put value of the symbol in @value,
// then returns True.
// If we do not resolve a missing symbol, this function must return False.
// In that case, ks_asm() would eventually return with error KS_ERR_ASM_SYMBOL_MISSING.

// To register the resolver, pass its function address to ks_option(), using
// option KS_OPT_SYM_RESOLVER. For example, see samples/sample.c.
typedef bool (*ks_sym_resolver)(const char* symbol, uint64_t* value);

// Runtime option for the Keystone engine
typedef enum ks_opt_type
{
    KS_OPT_SYNTAX = 1,    // Choose syntax for input assembly
    KS_OPT_SYM_RESOLVER,  // Set symbol resolver callback
} ks_opt_type;


// Runtime option value (associated with ks_opt_type above)
typedef enum ks_opt_value
{
    KS_OPT_SYNTAX_INTEL =   1 << 0, // X86 Intel syntax - default on X86 (KS_OPT_SYNTAX).
    KS_OPT_SYNTAX_ATT   =   1 << 1, // X86 ATT asm syntax (KS_OPT_SYNTAX).
    KS_OPT_SYNTAX_NASM  =   1 << 2, // X86 Nasm syntax (KS_OPT_SYNTAX).
    KS_OPT_SYNTAX_MASM  =   1 << 3, // X86 Masm syntax (KS_OPT_SYNTAX) - unsupported yet.
    KS_OPT_SYNTAX_GAS   =   1 << 4, // X86 GNU GAS syntax (KS_OPT_SYNTAX).
    KS_OPT_SYNTAX_RADIX16 = 1 << 5, // All immediates are in hex format (i.e 12 is 0x12)
} ks_opt_value;


#include "arm64.h"
#include "arm.h"
#include "hexagon.h"
#include "mips.h"
#include "ppc.h"
#include "sparc.h"
#include "systemz.h"
#include "x86.h"

/*
 Return combined API version & major and minor version numbers.

 @major: major number of API version
 @minor: minor number of API version

 @return hexical number as (major << 8 | minor), which encodes both
     major & minor versions.
     NOTE: This returned value can be compared with version number made
     with macro KS_MAKE_VERSION

 For example, second API version would return 1 in @major, and 1 in @minor
 The return value would be 0x0101

 NOTE: if you only care about returned value, but not major and minor values,
 set both @major & @minor arguments to NULL.
*/
KEYSTONE_EXPORT
unsigned int ks_version(unsigned int* major, unsigned int* minor);


/*
 Determine if the given architecture is supported by this library.

 @arch: architecture type (KS_ARCH_*)

 @return True if this library supports the given arch.
*/
KEYSTONE_EXPORT
bool ks_arch_supported(ks_arch arch);


/*
 Create new instance of Keystone engine.

 @arch: architecture type (KS_ARCH_*)
 @mode: hardware mode. This is combined of KS_MODE_*
 @ks: pointer to ks_engine, which will be updated at return time

 @return KS_ERR_OK on success, or other value on failure (refer to ks_err enum
   for detailed error).
*/
KEYSTONE_EXPORT
ks_err ks_open(ks_arch arch, int mode, ks_engine** ks);


/*
 Close KS instance: MUST do to release the handle when it is not used anymore.
 NOTE: this must be called only when there is no longer usage of Keystone.
 The reason is the this API releases some cached memory, thus access to any
 Keystone API after ks_close() might crash your application.
 After this, @ks is invalid, and nolonger usable.

 @ks: pointer to a handle returned by ks_open()

 @return KS_ERR_OK on success, or other value on failure (refer to ks_err enum
   for detailed error).
*/
KEYSTONE_EXPORT
ks_err ks_close(ks_engine* ks);


/*
 Report the last error number when some API function fail.
 Like glibc's errno, ks_errno might not retain its old error once accessed.

 @ks: handle returned by ks_open()

 @return: error code of ks_err enum type (KS_ERR_*, see above)
*/
KEYSTONE_EXPORT
ks_err ks_errno(ks_engine* ks);


/*
 Return a string describing given error code.

 @code: error code (see KS_ERR_* above)

 @return: returns a pointer to a string that describes the error code
   passed in the argument @code
 */
KEYSTONE_EXPORT
const char* ks_strerror(ks_err code);


/*
 Set option for Keystone engine at runtime

 @ks: handle returned by ks_open()
 @type: type of option to be set. See ks_opt_type
 @value: option value corresponding with @type

 @return: KS_ERR_OK on success, or other value on failure.
 Refer to ks_err enum for detailed error.
*/
KEYSTONE_EXPORT
ks_err ks_option(ks_engine* ks, ks_opt_type type, size_t value);


/*
 Assemble a string given its the buffer, size, start address and number
 of instructions to be decoded.
 This API dynamically allocate memory to contain assembled instruction.
 Resulted array of bytes containing the machine code  is put into @*encoding

 NOTE 1: this API will automatically determine memory needed to contain
 output bytes in *encoding.

 NOTE 2: caller must free the allocated memory itself to avoid memory leaking.

 @ks: handle returned by ks_open()
 @str: NULL-terminated assembly string. Use ; or \n to separate statements.
 @address: address of the first assembly instruction, or 0 to ignore.
 @encoding: array of bytes containing encoding of input assembly string.
       NOTE: *encoding will be allocated by this function, and should be freed
       with ks_free() function.
 @encoding_size: size of *encoding
 @stat_count: number of statements successfully processed

 @return: 0 on success, or -1 on failure.

 On failure, call ks_errno() for error code.
*/
KEYSTONE_EXPORT
int ks_asm(ks_engine* ks,
           const char* string,
           uint64_t address,
           unsigned char** encoding, size_t* encoding_size,
           size_t* stat_count);


/*
 Free memory allocated by ks_asm()

 @p: memory allocated in @encoding argument of ks_asm()
*/
KEYSTONE_EXPORT
void ks_free(unsigned char* p);


#ifdef __cplusplus
}
#endif

#endif
