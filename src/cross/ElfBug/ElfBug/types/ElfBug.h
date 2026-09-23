#pragma once

#include <cstdint>

namespace ElfBug
{
    using uint8 = uint8_t;
    using uint16 = uint16_t;
    using uint32 = uint32_t;
    using uint64 = uint64_t;

    using ptr = uint64;

    enum class Arch : uint32
    {
        Unknown = 0,
        X86_64 = 1,
        I386 = 2,
    };
}
