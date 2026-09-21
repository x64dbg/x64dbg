#include <ElfBug/process/Process.h>

namespace ElfBug
{
    BreakpointInfo* Process::findSoftwareBreakpoint(const ptr address)
    {
        const auto it = mSoftwareBreakpointReferences.find(address);
        if(it == mSoftwareBreakpointReferences.end())
            return nullptr;
        return &it->second->second;
    }

    bool Process::pokeByte(const ptr address, const uint8 byte)
    {
        return MemWriteRaw(address, &byte, 1);
    }

    bool Process::SetBreakpoint(const ptr address, const bool singleshot, const SoftwareType type)
    {
        std::unique_lock lock(mBreakpointMutex);
        return setBreakpointLocked(address, singleshot, type);
    }

    bool Process::setBreakpointLocked(const ptr address, const bool singleshot, const SoftwareType type)
    {
        BreakpointKey key{BreakpointType::Software, address};
        if(mBreakpoints.count(key))
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

        const auto it = mBreakpoints.emplace(key, info).first;
        mSoftwareBreakpointReferences[address] = it;

        return true;
    }

    bool Process::SetBreakpoint(const ptr address, const BreakpointCallback & cbBreakpoint, const bool singleshot, const SoftwareType type)
    {
        std::unique_lock lock(mBreakpointMutex);
        if(!setBreakpointLocked(address, singleshot, type))
            return false;

        const BreakpointKey key{BreakpointType::Software, address};
        mBreakpointCallbacks[key] = cbBreakpoint;
        return true;
    }

    bool Process::DeleteBreakpoint(const ptr address)
    {
        std::unique_lock lock(mBreakpointMutex);
        const BreakpointKey key{BreakpointType::Software, address};
        const auto it = mBreakpoints.find(key);
        if(it == mBreakpoints.end())
            return false;

        if(it->second.armed && !pokeByte(address, it->second.internal.software.oldbytes[0]))
            return false;

        mSoftwareBreakpointReferences.erase(address);
        mBreakpointCallbacks.erase(key);
        mBreakpoints.erase(it);
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

    bool Process::DisarmAllBreakpointBytes()
    {
        std::unique_lock lock(mBreakpointMutex);
        bool all = true;
        for(auto & [key, info] : mBreakpoints)
        {
            if(key.first != BreakpointType::Software || !info.armed)
                continue;
            if(pokeByte(key.second, info.internal.software.oldbytes[0]))
                info.armed = false;
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

        if(!pokeByte(address, info->internal.software.newbytes[0]))
            return false;

        info->armed = true;
        return true;
    }

    bool Process::ForgetBreakpoint(const ptr address)
    {
        std::unique_lock lock(mBreakpointMutex);
        const auto ref = mSoftwareBreakpointReferences.find(address);
        if(ref == mSoftwareBreakpointReferences.end())
            return false;

        const BreakpointKey key{BreakpointType::Software, address};
        mSoftwareBreakpointReferences.erase(ref);
        mBreakpoints.erase(key);
        mBreakpointCallbacks.erase(key);
        return true;
    }

    void Process::ReseatBreakpointsAfterExec()
    {
        std::unique_lock lock(mBreakpointMutex);
        for(auto & [key, info] : mBreakpoints)
        {
            if(key.first != BreakpointType::Software)
                continue;

            info.armed = false;
            info.internal.software.oldbytes[0] = 0;

            uint8 origByte = 0;
            if(!MemReadRaw(key.second, &origByte, 1))
                continue;
            if(!pokeByte(key.second, info.internal.software.newbytes[0]))
                continue;

            info.internal.software.oldbytes[0] = origByte;
            info.armed = true;
        }
    }

    bool Process::TakeBreakpointDispatch(const ptr address, BreakpointInfo & info,
                                         BreakpointCallback & callback) const
    {
        std::shared_lock lock(mBreakpointMutex);
        const auto ref = mSoftwareBreakpointReferences.find(address);
        if(ref == mSoftwareBreakpointReferences.end())
            return false;

        info = ref->second->second;

        const BreakpointKey key{BreakpointType::Software, address};
        const auto cbIt = mBreakpointCallbacks.find(key);
        callback = cbIt != mBreakpointCallbacks.end() ? cbIt->second : BreakpointCallback();
        return true;
    }

    bool Process::HasBreakpoint(const ptr address) const
    {
        std::shared_lock lock(mBreakpointMutex);
        return mSoftwareBreakpointReferences.find(address) != mSoftwareBreakpointReferences.end();
    }

    bool Process::HasBreakpointCallback(const ptr address) const
    {
        std::shared_lock lock(mBreakpointMutex);
        const BreakpointKey key{BreakpointType::Software, address};
        return mBreakpointCallbacks.find(key) != mBreakpointCallbacks.end();
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
