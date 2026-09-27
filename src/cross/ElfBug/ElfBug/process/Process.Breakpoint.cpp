#include <ElfBug/process/Process.h>

namespace ElfBug
{
    BreakpointInfo* Process::findSoftwareBreakpoint(const ptr address)
    {
        const auto it = mBreakpoints.find(address);
        return it == mBreakpoints.end() ? nullptr : &it->second.info;
    }

    bool Process::pokeByte(const ptr address, const uint8 byte)
    {
        return MemWriteRaw(address, &byte, 1);
    }

    bool Process::SetBreakpoint(const ptr address, const bool singleshot, const SoftwareType type)
    {
        (void)type;
        std::unique_lock lock(mBreakpointMutex);
        return setBreakpointLocked(address, singleshot) != nullptr;
    }

    SoftwareBreakpoint* Process::setBreakpointLocked(const ptr address, const bool singleshot)
    {
        if(mBreakpoints.count(address))
            return nullptr;

        uint8 savedByte = 0;
        if(!MemReadRaw(address, &savedByte, 1))
            return nullptr;

        if(!pokeByte(address, kInt3))
            return nullptr;

        SoftwareBreakpoint & bp = mBreakpoints[address];
        bp.info.address = address;
        bp.info.singleshot = singleshot;
        bp.info.armed = true;
        bp.info.type = BreakpointType::Software;
        bp.info.savedByte = savedByte;
        return &bp;
    }

    bool Process::SetBreakpoint(const ptr address, const BreakpointCallback & cbBreakpoint, const bool singleshot, const SoftwareType type)
    {
        (void)type;
        std::unique_lock lock(mBreakpointMutex);
        SoftwareBreakpoint* bp = setBreakpointLocked(address, singleshot);
        if(!bp)
            return false;

        bp->callback = cbBreakpoint;
        return true;
    }

    bool Process::DeleteBreakpoint(const ptr address)
    {
        std::unique_lock lock(mBreakpointMutex);
        const auto it = mBreakpoints.find(address);
        if(it == mBreakpoints.end())
            return false;

        const BreakpointInfo & info = it->second.info;
        if(info.armed && !pokeByte(address, info.savedByte))
            return false;

        mBreakpoints.erase(it);
        return true;
    }

    bool Process::DisarmBreakpointByte(const ptr address)
    {
        std::unique_lock lock(mBreakpointMutex);
        BreakpointInfo* info = findSoftwareBreakpoint(address);
        if(!info || !info->armed)
            return false;

        if(!pokeByte(address, info->savedByte))
            return false;

        info->armed = false;
        return true;
    }

    bool Process::DisarmAllBreakpointBytes()
    {
        std::unique_lock lock(mBreakpointMutex);
        bool all = true;
        for(auto & [address, bp] : mBreakpoints)
        {
            if(!bp.info.armed)
                continue;
            if(pokeByte(address, bp.info.savedByte))
                bp.info.armed = false;
            else
                all = false;
        }
        return all;
    }

    bool Process::RearmBreakpointByte(const ptr address)
    {
        std::unique_lock lock(mBreakpointMutex);
        BreakpointInfo* info = findSoftwareBreakpoint(address);
        if(!info || info->armed)
            return false;

        if(!pokeByte(address, kInt3))
            return false;

        info->armed = true;
        return true;
    }

    bool Process::ForgetBreakpoint(const ptr address)
    {
        std::unique_lock lock(mBreakpointMutex);
        return mBreakpoints.erase(address) != 0;
    }

    void Process::ReseatBreakpointsAfterExec()
    {
        std::unique_lock lock(mBreakpointMutex);
        for(auto & [address, bp] : mBreakpoints)
        {
            bp.info.armed = false;
            bp.info.savedByte = 0;

            uint8 savedByte = 0;
            if(!MemReadRaw(address, &savedByte, 1))
                continue;
            if(!pokeByte(address, kInt3))
                continue;

            bp.info.savedByte = savedByte;
            bp.info.armed = true;
        }
    }

    void Process::ForgetBreakpointsAfterExec()
    {
        std::unique_lock lock(mBreakpointMutex);
        mBreakpoints.clear();
    }

    bool Process::TakeBreakpointDispatch(const ptr address, BreakpointInfo & info,
                                         BreakpointCallback & callback) const
    {
        std::shared_lock lock(mBreakpointMutex);
        const auto it = mBreakpoints.find(address);
        if(it == mBreakpoints.end())
            return false;

        info = it->second.info;
        callback = it->second.callback;
        return true;
    }

    bool Process::HasBreakpoint(const ptr address) const
    {
        std::shared_lock lock(mBreakpointMutex);
        return mBreakpoints.count(address) != 0;
    }

    bool Process::HasBreakpointCallback(const ptr address) const
    {
        std::shared_lock lock(mBreakpointMutex);
        const auto it = mBreakpoints.find(address);
        return it != mBreakpoints.end() && it->second.callback;
    }

    bool Process::SetMemoryBreakpoint(const ptr address, const ptr size, const MemoryType type, const bool singleshot)
    {
        (void)address;
        (void)size;
        (void)type;
        (void)singleshot;
        return false;
    }

    bool Process::SetMemoryBreakpoint(const ptr address, const ptr size, const BreakpointCallback & cbBreakpoint, const MemoryType type, const bool singleshot)
    {
        (void)address;
        (void)size;
        (void)cbBreakpoint;
        (void)type;
        (void)singleshot;
        return false;
    }

    bool Process::DeleteMemoryBreakpoint(const ptr address)
    {
        (void)address;
        return false;
    }

}
