#ifndef _DBG_TYPES_H_
#define _DBG_TYPES_H_

/***************************************************************/
//
// This file declares common types to be used
// throughout the project. Originally duint, int_t,
// or size_t were used to represent pointers and addresses.
//
// The purpose is to use a single type as the representation.
//
/***************************************************************/
#undef COMPILE_X64
#undef COMPILE_x86

#ifdef _WIN64
#define COMPILE_X64 1    // Program is being compiled as 64-bit
#else
#define COMPILE_x86 1    // Program is being compiled as 32-bit
#endif // _WIN64

//
// Define types
//
#ifdef COMPILE_X64
typedef unsigned long long  duint;
typedef signed long long    dsint;
#else
typedef unsigned long __w64 duint;
typedef signed long __w64   dsint;
#endif // COMPILE_X64

typedef short int16;
typedef unsigned short uint16;

typedef int int32;
typedef unsigned int uint32;

typedef long long int64;
typedef unsigned long long uint64;

typedef unsigned char byte_t;
#endif //_DBG_TYPES_H_
