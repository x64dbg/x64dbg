#pragma once

#include <functional>
#include <map>
#include <unordered_map>
#include <set>

#include <ElfBug/types/ElfBug.h>
#include <ElfBug/types/Breakpoint.h>

namespace ElfBug
{
    struct BreakpointInternalInfo
    {
        union
        {
            struct
            {
                SoftwareType type;
                ptr size;
                uint8 newbytes[2];
                uint8 oldbytes[2];
            } software;

            struct
            {
                HardwareSlot slot;
                HardwareType type;
                HardwareSize size;
                bool enabled;
            } hardware;

            struct
            {
                MemoryType type;
                ptr size;
            } memory;
        };
    };

    struct BreakpointInfo
    {
        ptr address = 0;
        bool singleshot = false;
        // False while the patch byte is temporarily lifted.
        bool armed = false;
        BreakpointType type = BreakpointType::Software;
        BreakpointInternalInfo internal = {};
    };

    using BreakpointCallback = std::function<void(const BreakpointInfo &)>;
    using StepCallback = std::function<void()>;
    using BreakpointKey = std::pair<BreakpointType, ptr>;
    using BreakpointMap = std::map<BreakpointKey, BreakpointInfo>;
    using BreakpointCallbackMap = std::map<BreakpointKey, BreakpointCallback>;
    using SoftwareBreakpointMap = std::unordered_map<ptr, BreakpointMap::iterator>;

    struct MemoryBreakpointData
    {
        uint32 Refcount = 0;
        uint32 Type = 0;
        uint32 OldProtect = 0;
        uint32 NewProtect = 0;
    };

    struct Range
    {
        ptr start = 0;
        ptr end = 0;
    };

    struct RangeCompare
    {
        bool operator()(const Range & a, const Range & b) const
        {
            return a.end < b.start;
        }
    };

    using MemoryBreakpointSet = std::set<Range, RangeCompare>;
    using MemoryBreakpointMap = std::unordered_map<ptr, MemoryBreakpointData>;
}
