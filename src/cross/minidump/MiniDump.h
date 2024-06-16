#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Disassembler/Architecture.h"

namespace MiniDump
{
    struct MemoryRegion
    {
        uint64_t BaseAddress = 0;
        uint64_t RegionSize = 0;
        std::string State;
        uint64_t AllocationBase = -1;
        std::string AllocationProtect;
        std::string Protect;
        std::string Type;
        std::string Info;
    };

    struct AbstractParser
    {
        static std::unique_ptr<AbstractParser> Create(const uint8_t* begin, const uint8_t* end, std::string& error);

        virtual ~AbstractParser() = default;
        virtual bool disasm64() = 0;
        virtual std::vector<MemoryRegion> MemoryRegions() const = 0;
    };

    Architecture* Architecture();
}
