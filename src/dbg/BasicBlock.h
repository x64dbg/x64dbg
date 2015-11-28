#pragma once

#include "_global.h"

enum BasicBlockFlags : duint
{
    BASIC_BLOCK_FLAG_NONE = 0,                 // No flag

    BASIC_BLOCK_FLAG_FUNCTION = (1 << 1),      // Scanned; also part of a known function
    BASIC_BLOCK_FLAG_CUTOFF = (1 << 3),        // Ends prematurely because of another JMP to location
    BASIC_BLOCK_FLAG_DELETE = (1 << 4),        // Delete element at the next possible time

    BASIC_BLOCK_FLAG_CALL = (1 << 5),          // The block ends with a call
    BASIC_BLOCK_FLAG_RET = (1 << 6),           // The block ends with a retn
    BASIC_BLOCK_FLAG_ABSJMP = (1 << 7),        // Branch is absolute
    BASIC_BLOCK_FLAG_INDIRECT = (1 << 8),      // This block ends with an indirect branch; memory or register
    BASIC_BLOCK_FLAG_INDIRPTR = (1 << 9),      // This block ends with an indirect branch; pointer known

    BASIC_BLOCK_FLAG_CALL_TARGET = (1 << 10),  // Block is pointed to by a call instruction
    //BASIC_BLOCK_FLAG_JMP_TARGET = (1 << 11),

    BASIC_BLOCK_FLAG_PREPAD = (1 << 12),    // Block ends because there was padding afterwards
    BASIC_BLOCK_FLAG_PAD = (1 << 13),       // Block is only a series of padding instructions
};

struct BasicBlock
{
    duint VirtualStart;  // Inclusive
    duint VirtualEnd;    // Inclusive
    duint Flags;
    duint Target;

    duint InstrCount;   // Number of instructions in block

    __forceinline bool GetFlag(duint Flag)
    {
        return (Flags & Flag) == Flag;
    }

    __forceinline void SetFlag(duint Flag)
    {
        Flags |= Flag;
    }

    __forceinline duint Size()
    {
        return VirtualEnd - VirtualStart;
    }

    bool operator< (const BasicBlock & b) const
    {
        return VirtualStart < b.VirtualStart;
    }

    bool operator== (const BasicBlock & b) const
    {
        return VirtualStart == b.VirtualStart;
    }
};

struct FunctionDef
{
    duint VirtualStart;  // Inclusive
    duint VirtualEnd;    // Inclusive

    duint BBlockStart;   // Index of first basic block
    duint BBlockEnd;     // Index of last basic block

    duint InstrCount;   // Number of instructions in function

    bool operator< (const FunctionDef & b) const
    {
        if(VirtualStart == b.VirtualStart)
            return VirtualEnd > b.VirtualEnd;

        return VirtualStart < b.VirtualStart;
    }

    bool operator== (const FunctionDef & b) const
    {
        return VirtualStart == b.VirtualStart;
    }
};

typedef std::vector<BasicBlock> BBlockArray;
typedef std::vector<FunctionDef> FuncDefArray;