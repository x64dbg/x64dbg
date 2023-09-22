#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

// TODO: do something cross platform
using duint = uint64_t;
using dsint = int64_t;

#ifndef _WIN32
template<size_t Count, class... Args>
int sprintf_s(char (&Dest)[Count], const char* fmt, Args... args)
{
    return snprintf(Dest, Count, fmt, args...);
}

inline size_t strcpy_s(char* dst, size_t size, const char* src)
{
    return strlcpy(dst, src, size);
}
#endif // _WIN32

// These types are needed in very deep UI components
typedef enum
{
    enc_unknown,  //must be 0
    enc_byte,     //1 byte
    enc_word,     //2 bytes
    enc_dword,    //4 bytes
    enc_fword,    //6 bytes
    enc_qword,    //8 bytes
    enc_tbyte,    //10 bytes
    enc_oword,    //16 bytes
    enc_mmword,   //8 bytes
    enc_xmmword,  //16 bytes
    enc_ymmword,  //32 bytes
    enc_zmmword,  //64 bytes avx512 not supported
    enc_real4,    //4 byte float
    enc_real8,    //8 byte double
    enc_real10,   //10 byte decimal
    enc_ascii,    //ascii sequence
    enc_unicode,  //unicode sequence
    enc_code,     //start of code
    enc_junk,     //junk code
    enc_middle    //middle of data
} ENCODETYPE;

typedef enum
{
    initialized,
    paused,
    running,
    stopped
} DBGSTATE;

typedef enum
{
    XREF_NONE,
    XREF_DATA,
    XREF_JMP,
    XREF_CALL
} XREFTYPE;

typedef struct
{
    duint addr;
    XREFTYPE type;
} XREF_RECORD;

typedef struct
{
    duint refcount;
    XREF_RECORD* references;
} XREF_INFO;
