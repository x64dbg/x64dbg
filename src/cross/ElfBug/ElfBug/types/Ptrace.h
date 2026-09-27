#pragma once

#include <cstdint>

namespace ElfBug
{
    constexpr int PtraceEvent(const int status)
    {
        return (status >> 16) & 0xffff;
    }

    inline void* PtraceData(const uintptr_t value)
    {
        return reinterpret_cast<void*>(value);
    }
}
