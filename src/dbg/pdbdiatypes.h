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
    DiaSymbolType type = DiaSymbolType::ANY;
    uint64_t virtualAddress = 0;
    uint64_t size = 0;
    uint32_t offset = 0;
    uint32_t disp = 0;
    uint32_t segment = 0;
    DiaReachableType reachable = DiaReachableType::UNKNOWN;
    DiaReturnableType returnable = DiaReturnableType::UNKNOWN;
    DiaCallingConvention convention = DiaCallingConvention::UNKNOWN;
    bool perfectSize = false;
    bool publicSymbol = false;
    std::string name;
    std::string undecoratedName;
};

struct DiaLineInfo_t
{
    DWORD sourceFileId;
    DWORD lineNumber;
    DWORD rva;
};

#endif // PDBDIATYPES_H_