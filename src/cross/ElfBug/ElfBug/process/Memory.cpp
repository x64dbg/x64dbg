#include <ElfBug/process/Process.h>
#include <shared_mutex>
#include <sys/uio.h>
#include <unistd.h>
#include <cstring>
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
        if(!buffer || !size || softwareBreakpointReferences.empty())
            return;

        auto* bytes = static_cast<uint8*>(buffer);
        for(const auto & [bpAddress, it] : softwareBreakpointReferences)
        {
            const BreakpointInfo & info = it->second;
            if(!info.armed || bpAddress < address || bpAddress - address >= size)
                continue;

            bytes[bpAddress - address] = info.internal.software.oldbytes[0];
        }
    }

    // A write over an armed breakpoint keeps the trap; the new byte becomes the restore byte.
    bool Process::MemWrite(const ptr address, const void* buffer, const ptr size, ptr* bytesWritten)
    {
        if(!buffer || !size)
            return false;

        std::unique_lock lock(mBreakpointMutex);
        if(softwareBreakpointReferences.empty())
        {
            // Still under the lock
            return MemWriteRaw(address, buffer, size, bytesWritten);
        }

        std::vector<uint8> patched(static_cast<size_t>(size));
        memcpy(patched.data(), buffer, static_cast<size_t>(size));
        for(const auto & [bpAddress, it] : softwareBreakpointReferences)
        {
            const BreakpointInfo & info = it->second;
            if(info.armed && bpAddress >= address && bpAddress - address < size)
                patched[bpAddress - address] = info.internal.software.newbytes[0];
        }

        ptr written = 0;
        const bool complete = MemWriteRaw(address, patched.data(), size, &written);

        for(const auto & [bpAddress, it] : softwareBreakpointReferences)
        {
            BreakpointInfo & info = it->second;
            if(bpAddress >= address && bpAddress - address < written)
                info.internal.software.oldbytes[0] = static_cast<const uint8*>(buffer)[bpAddress - address];
        }

        if(bytesWritten)
            *bytesWritten = written;
        return complete;
    }

    bool Process::MemWriteRaw(const ptr address, const void* buffer, const ptr size, ptr* bytesWritten) const
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

    bool Process::MemProtect(ptr address, ptr size, uint32 newProtect, const uint32* oldProtect)
    {
        // TODO: implement via ptrace or /proc/pid/mem mprotect
        (void)address;
        (void)size;
        (void)newProtect;
        (void)oldProtect;
        return false;
    }
}
