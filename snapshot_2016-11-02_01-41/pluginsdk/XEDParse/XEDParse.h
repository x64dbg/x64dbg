#ifndef _XEDPARSE_H
#define _XEDPARSE_H

#include <windows.h>

//XEDParse defines
#ifdef XEDPARSE_BUILD
#define XEDPARSE_EXPORT __declspec(dllexport)
#else
#define XEDPARSE_EXPORT __declspec(dllimport)
#endif //XEDPARSE_BUILD

#define XEDPARSE_CALL //calling convention

#define XEDPARSE_MAXBUFSIZE 256
#define XEDPARSE_MAXASMSIZE 16

//typedefs
typedef bool (XEDPARSE_CALL* CBXEDPARSE_UNKNOWN)(const char* text, ULONGLONG* value);

//XEDParse enums
enum XEDPARSE_STATUS
{
    XEDPARSE_ERROR = 0,
    XEDPARSE_OK = 1
};

//XEDParse structs
#pragma pack(push,8)
struct XEDPARSE
{
    bool x64; // use 64-bit instructions
    ULONGLONG cip; //instruction pointer (for relative addressing)
    unsigned int dest_size; //destination size (returned by XEDParse)
    CBXEDPARSE_UNKNOWN cbUnknown; //unknown operand callback
    unsigned char dest[XEDPARSE_MAXASMSIZE]; //destination buffer
    char instr[XEDPARSE_MAXBUFSIZE]; //instruction text
    char error[XEDPARSE_MAXBUFSIZE]; //error text (in case of an error)
};
#pragma pack(pop)

#ifdef __cplusplus
extern "C"
{
#endif

XEDPARSE_EXPORT XEDPARSE_STATUS XEDPARSE_CALL XEDParseAssemble(XEDPARSE* XEDParse);

#ifdef __cplusplus
}
#endif

#endif // _XEDPARSE_H
