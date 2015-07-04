#pragma once

#include "_global.h"

enum BasicBlockFlags : uint
{
    BASIC_BLOCK_FLAG_NONE = 0,              // No flag
    BASIC_BLOCK_FLAG_FUNCTION = (1 << 1),   // Scanned; also part of a known function
    BASIC_BLOCK_FLAG_ORPHANED = (1 << 2),   // No targets ever reach this block
    BASIC_BLOCK_FLAG_CALL = (1 << 3),       // The block ends with a call
    BASIC_BLOCK_FLAG_RET = (1 << 4),        // The block ends with a retn
    BASIC_BLOCK_FLAG_INDIRECT = (1 << 5),   // This block ends with an indirect branch
    BASIC_BLOCK_FLAG_INDIRPTR = (1 << 6),   // This block ends with an indirect branch; pointer known
    BASIC_BLOCK_FLAG_PREINT3 = (1 << 7),    // Block ends because there was an INT3 afterwards
    BASIC_BLOCK_FLAG_INT3 = (1 << 8),       // Block is only a series of INT3
    BASIC_BLOCK_FLAG_CUTOFF = (1 << 9),     // Ends prematurely because of another JMP to location
    BASIC_BLOCK_FLAG_DELETE = (1 << 10),    // Delete element at the next possible time
};

struct BasicBlock
{
    uint VirtualStart;  // Inclusive
    uint VirtualEnd;    // Exclusive
    uint Flags;
    uint Target;

    inline bool GetFlag(uint Flag)
    {
        return (Flags & Flag) == Flag;
    }

    inline void SetFlag(uint Flag)
    {
        Flags |= Flag;
    }

    inline uint Size()
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
    uint VirtualStart;  // Inclusive
    uint VirtualEnd;    // Exclusive

    uint BBlockStart;   // Index of first basic block
    uint BBlockEnd;     // Index of last basic block

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