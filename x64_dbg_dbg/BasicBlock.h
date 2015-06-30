#pragma once

#include "_global.h"

enum BasicBlockFlags : uint
{
    BASIC_BLOCK_FLAG_NONE,                  // No flag
    BASIC_BLOCK_FLAG_SCANNED = (1 << 1),    // The block was scanned at least once
    BASIC_BLOCK_FLAG_ORPHANED = (1 << 2),   // No targets ever reach this block
    BASIC_BLOCK_FLAG_CALL = (1 << 3),       // The block ends with a call
    BASIC_BLOCK_FLAG_RET = (1 << 4),        // The block ends with a retn
    BASIC_BLOCK_FLAG_INDIRECT = (1 << 5),   // This block ends with an indirect branch
    BASIC_BLOCK_FLAG_PREINT3 = (1 << 6),    // Block ends because there was an INT3 afterwards
    BASIC_BLOCK_FLAG_INT3 = (1 << 7),       // Block is only a series of INT3
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

    bool operator< (const BasicBlock & b) const
    {
        return VirtualStart < b.VirtualStart;
    }

    bool operator== (const BasicBlock & b) const
    {
        return VirtualStart == b.VirtualStart;
    }
};

typedef std::vector<BasicBlock> BBlockArray;