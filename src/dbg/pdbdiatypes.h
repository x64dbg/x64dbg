#ifndef PDBDIATYPES_H_
#define PDBDIATYPES_H_
#pragma once

#include <string>
#include <stdint.h>
#include <set>
#include <vector>
#include <deque>
#include <map>
#include <windows.h>
#include <functional>
#include <string>

enum class DiaSymbolType
{
    ANY = -1,
    FUNCTION = 0,
    CODE,
    DATA,
    LABEL,
    THUNK,
    BLOCK,
    PUBLIC,
    SECTIONCONTRIB,
};

enum class DiaReachableType
{
    UNKNOWN,
    REACHABLE,
    NOTREACHABLE,
};

enum class DiaReturnableType
{
    UNKNOWN,
    RETURNABLE,
    NOTRETURNABLE,
};

enum class DiaCallingConvention
{
    UNKNOWN,
    CALL_NEAR_C,
    CALL_NEAR_FAST,
    CALL_NEAR_STD,
    CALL_NEAR_SYS,
    CALL_THISCALL,
};

struct DiaValidationData_t
{
    GUID guid;
    uint32_t signature;
    uint32_t age;
};

struct DiaSymbol_t
{
    DiaSymbolType type;
    uint64_t virtualAddress;
    uint64_t size;
    uint32_t offset;
    uint32_t disp;
    uint32_t segment;
    DiaReachableType reachable;
    DiaReturnableType returnable;
    DiaCallingConvention convention;
    bool perfectSize;
    bool publicSymbol;
    std::string name;
    std::string undecoratedName;
};

struct DiaLineInfo_t
{
    std::string fileName;
    DWORD lineNumber;
    uint32_t offset;
    uint32_t segment;
    uint64_t virtualAddress;
};

#endif // PDBDIATYPES_H_