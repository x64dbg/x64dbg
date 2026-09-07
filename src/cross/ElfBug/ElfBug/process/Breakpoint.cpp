#include <ElfBug/process/Process.h>

namespace ElfBug
{
    BreakpointInfo* Process::findSoftwareBreakpoint(const ptr address)
    {
        const auto it = softwareBreakpointReferences.find(address);
        if(it == softwareBreakpointReferences.end())
            return nullptr;
        return &it->second->second;
    }

    // ptrace pokes only work while the leader is the stopped task, so go through memory.
    bool Process::pokeByte(const ptr address, const uint8 byte) const
    {
        return MemWriteRaw(address, &byte, 1);
    }

    bool Process::SetBreakpoint(const ptr address, bool singleshot, const SoftwareType type)
    {
        std::unique_lock lock(mBreakpointMutex);
        return setBreakpointLocked(address, singleshot, type);
    }

    bool Process::setBreakpointLocked(const ptr address, bool singleshot, const SoftwareType type)
    {
        BreakpointKey key{BreakpointType::Software, address};
        if(breakpoints.count(key))
            return false;

        uint8_t origByte = 0;
        if(!MemReadRaw(address, &origByte, 1))
            return false;

        if(!pokeByte(address, 0xCC))
            return false;

        BreakpointInfo info;
        info.address = address;
        info.singleshot = singleshot;
        info.armed = true;
        info.type = BreakpointType::Software;
        info.internal.software.type = type;
        info.internal.software.oldbytes[0] = origByte;
        info.internal.software.newbytes[0] = 0xCC;
        info.internal.software.size = 1;

        const auto it = breakpoints.emplace(key, info).first;
        softwareBreakpointReferences[address] = it;

        return true;
    }

    bool Process::SetBreakpoint(const ptr address, const BreakpointCallback & cbBreakpoint, bool singleshot, const SoftwareType type)
    {
        std::unique_lock lock(mBreakpointMutex);
        if(!setBreakpointLocked(address, singleshot, type))
            return false;

        const BreakpointKey key{BreakpointType::Software, address};
        breakpointCallbacks[key] = cbBreakpoint;
        return true;
    }

    bool Process::DeleteBreakpoint(const ptr address)
    {
        std::unique_lock lock(mBreakpointMutex);
        const BreakpointKey key{BreakpointType::Software, address};
        const auto it = breakpoints.find(key);
        if(it == breakpoints.end())
            return false;

        if(it->second.armed && !pokeByte(address, it->second.internal.software.oldbytes[0]))
            return false;

        softwareBreakpointReferences.erase(address);
        breakpointCallbacks.erase(key);
        breakpoints.erase(it);
        return true;
    }

    bool Process::DisarmBreakpointByte(const ptr address)
    {
        std::unique_lock lock(mBreakpointMutex);
        BreakpointInfo* info = findSoftwareBreakpoint(address);
        if(!info || !info->armed)
            return false;

        if(!pokeByte(address, info->internal.software.oldbytes[0]))
            return false;

        info->armed = false;
        return true;
    }

    bool Process::RearmBreakpointByte(const ptr address)
    {
        std::unique_lock lock(mBreakpointMutex);
        BreakpointInfo* info = findSoftwareBreakpoint(address);
        if(!info || info->armed)
            return false;

        if(!pokeByte(address, info->internal.software.newbytes[0]))
            return false;

        info->armed = true;
        return true;
    }

    bool Process::ForgetBreakpoint(const ptr address)
    {
        std::unique_lock lock(mBreakpointMutex);
        const auto ref = softwareBreakpointReferences.find(address);
        if(ref == softwareBreakpointReferences.end())
            return false;

        const BreakpointKey key{BreakpointType::Software, address};
        softwareBreakpointReferences.erase(ref);
        breakpoints.erase(key);
        breakpointCallbacks.erase(key);
        return true;
    }

    bool Process::TakeBreakpointDispatch(const ptr address, BreakpointInfo & info,
                                         BreakpointCallback & callback) const
    {
        std::shared_lock lock(mBreakpointMutex);
        const auto ref = softwareBreakpointReferences.find(address);
        if(ref == softwareBreakpointReferences.end())
            return false;

        info = ref->second->second;

        const BreakpointKey key{BreakpointType::Software, address};
        const auto cbIt = breakpointCallbacks.find(key);
        callback = cbIt != breakpointCallbacks.end() ? cbIt->second : BreakpointCallback();
        return true;
    }

    bool Process::HasBreakpoint(const ptr address) const
    {
        std::shared_lock lock(mBreakpointMutex);
        return softwareBreakpointReferences.find(address) != softwareBreakpointReferences.end();
    }

    bool Process::SetMemoryBreakpoint(const ptr address, const ptr size, const MemoryType type, bool singleshot)
    {
        (void)address;
        (void)size;
        (void)type;
        (void)singleshot;
        return false;
    }

    bool Process::SetMemoryBreakpoint(const ptr address, const ptr size, const BreakpointCallback & cbBreakpoint, const MemoryType type, bool singleshot)
    {
        (void)address;
        (void)size;
        (void)cbBreakpoint;
        (void)type;
        (void)singleshot;
        return false;
    }

    bool Process::DeleteMemoryBreakpoint(ptr const address)
    {
        (void)address;
        return false;
    }

}
