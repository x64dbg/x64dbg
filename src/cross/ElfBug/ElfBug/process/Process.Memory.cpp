#include <ElfBug/process/Process.h>
#include <sys/uio.h>
#include <unistd.h>
#include <cstring>
#include <shared_mutex>
#include <vector>

namespace ElfBug
{
    bool Process::MemRead(const ptr address, void* buffer, const ptr size, ptr* bytesRead) const
    {
        ptr read = 0;
        bool complete = false;
        {
            std::shared_lock lock(mBreakpointMutex);
            complete = MemReadRaw(address, buffer, size, &read);
            unpatchBreakpointBytesLocked(address, buffer, read);
        }

        if(bytesRead)
            *bytesRead = read;
        return complete;
    }

    bool Process::MemReadRaw(const ptr address, void* buffer, const ptr size, ptr* bytesRead) const
    {
        if(!buffer || !size)
            return false;

        iovec local{};
        local.iov_base = buffer;
        local.iov_len = size;

        iovec remote{};
        remote.iov_base = reinterpret_cast<void*>(address);
        remote.iov_len = size;

        ssize_t result = process_vm_readv(pid, &local, 1, &remote, 1, 0);
        if(result == -1)
        {
            result = memPread(buffer, size, static_cast<off_t>(address));
            if(result == -1)
            {
                memset(buffer, 0, size);
                return false;
            }
        }

        if(bytesRead)
            *bytesRead = static_cast<ptr>(result);
        if(static_cast<size_t>(result) < size)
            memset(static_cast<char*>(buffer) + result, 0, size - result);
        return static_cast<size_t>(result) == size;
    }

    void Process::unpatchBreakpointBytesLocked(const ptr address, void* buffer, const ptr size) const
    {
        if(!buffer || !size)
            return;

        auto* bytes = static_cast<uint8*>(buffer);
        for(auto it = mBreakpoints.lower_bound(address); it != mBreakpoints.end() && it->first - address < size; ++it)
        {
            if(it->second.info.armed)
                bytes[it->first - address] = it->second.info.savedByte;
        }
    }

    // A write over an armed breakpoint keeps the trap; the new byte becomes the restore byte.
    bool Process::MemWrite(const ptr address, const void* buffer, const ptr size, ptr* bytesWritten)
    {
        if(!buffer || !size)
            return false;

        std::unique_lock lock(mBreakpointMutex);
        const auto first = mBreakpoints.lower_bound(address);
        const auto inRange = [&](const SoftwareBreakpointMap::iterator & it, const ptr limit)
        {
            return it != mBreakpoints.end() && it->first - address < limit;
        };
        if(!inRange(first, size))
            return MemWriteRaw(address, buffer, size, bytesWritten);

        std::vector<uint8> patched(static_cast<size_t>(size));
        memcpy(patched.data(), buffer, static_cast<size_t>(size));
        for(auto it = first; inRange(it, size); ++it)
        {
            if(it->second.info.armed)
                patched[it->first - address] = kInt3;
        }

        ptr written = 0;
        const bool complete = MemWriteRaw(address, patched.data(), size, &written);

        for(auto it = first; inRange(it, written); ++it)
            it->second.info.savedByte = static_cast<const uint8*>(buffer)[it->first - address];

        if(bytesWritten)
            *bytesWritten = written;
        return complete;
    }

    bool Process::MemWriteRaw(const ptr address, const void* buffer, const ptr size, ptr* bytesWritten)
    {
        if(!buffer || !size)
            return false;

        iovec local{};
        local.iov_base = const_cast<void*>(buffer);
        local.iov_len = size;

        iovec remote{};
        remote.iov_base = reinterpret_cast<void*>(address);
        remote.iov_len = size;

        ssize_t result = process_vm_writev(pid, &local, 1, &remote, 1, 0);
        if(result == -1)
        {
            result = memPwrite(buffer, size, static_cast<off_t>(address));
            if(result == -1)
                return false;
        }

        if(bytesWritten)
            *bytesWritten = static_cast<ptr>(result);
        return static_cast<size_t>(result) == size;
    }

    bool Process::MemIsValidPtr(const ptr address) const
    {
        uint8 byte;
        return MemRead(address, &byte, 1);
    }

    bool Process::MemProtect(const ptr address, const ptr size, const uint32 newProtect, const uint32* oldProtect)
    {
        // TODO: implement via ptrace or /proc/pid/mem mprotect
        (void)address;
        (void)size;
        (void)newProtect;
        (void)oldProtect;
        return false;
    }
}
