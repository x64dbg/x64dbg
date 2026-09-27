#pragma once

#include <functional>
#include <map>

#include <ElfBug/types/ElfBug.h>
#include <ElfBug/types/Breakpoint.h>

namespace ElfBug
{
    constexpr uint8 kInt3 = 0xCC;

    struct BreakpointInfo
    {
        ptr address = 0;
        bool singleshot = false;
        // False while the patch byte is temporarily lifted.
        bool armed = false;
        BreakpointType type = BreakpointType::Software;
        // The tracee's own byte under the int3.
        uint8 savedByte = 0;
    };

    using BreakpointCallback = std::function<void(const BreakpointInfo &)>;

    struct SoftwareBreakpoint
    {
        BreakpointInfo info;
        BreakpointCallback callback;
    };

    // Ordered so a memory access can visit only the breakpoints inside its range.
    using SoftwareBreakpointMap = std::map<ptr, SoftwareBreakpoint>;
}
