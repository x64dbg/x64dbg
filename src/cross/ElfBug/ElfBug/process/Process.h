#pragma once

#include <sys/types.h>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <ElfBug/types/ElfBug.h>
#include <ElfBug/types/Global.h>
#include <ElfBug/thread/Thread.h>

namespace ElfBug
{
    class Process
    {
    public:
        pid_t pid;
        Arch arch = Arch::Unknown;
        std::unordered_map<pid_t, std::unique_ptr<Thread>> threads;

        BreakpointMap breakpoints;
        BreakpointCallbackMap breakpointCallbacks;
        SoftwareBreakpointMap softwareBreakpointReferences;
        MemoryBreakpointSet memoryBreakpointRanges;
        MemoryBreakpointMap memoryBreakpointPages;

        explicit Process(pid_t pid);
        ~Process();

        Process(const Process &) = delete;
        Process & operator=(const Process &) = delete;
        Process(Process &&) = delete;
        Process & operator=(Process &&) = delete;

        bool MemRead(ptr address, void* buffer, ptr size, ptr* bytesRead = nullptr) const;
        bool MemReadRaw(ptr address, void* buffer, ptr size, ptr* bytesRead = nullptr) const;
        bool MemWrite(ptr address, const void* buffer, ptr size, ptr* bytesWritten = nullptr);
        bool MemWriteRaw(ptr address, const void* buffer, ptr size, ptr* bytesWritten = nullptr) const;
        bool MemIsValidPtr(ptr address) const;
        bool MemProtect(ptr address, ptr size, uint32 newProtect, const uint32* oldProtect = nullptr);

        bool SetBreakpoint(ptr address, bool singleshot = false, SoftwareType type = SoftwareType::ShortInt3);
        bool SetBreakpoint(ptr address, const BreakpointCallback & cbBreakpoint, bool singleshot = false, SoftwareType type = SoftwareType::ShortInt3);
        bool DeleteBreakpoint(ptr address);

        bool DisarmBreakpointByte(ptr address);
        bool RearmBreakpointByte(ptr address);
        // Drops the record without touching tracee memory (post-exec cleanup).
        bool ForgetBreakpoint(ptr address);

        // TODO: implement via mprotect + SIGSEGV handling
        bool SetMemoryBreakpoint(ptr address, ptr size, MemoryType type = MemoryType::Access, bool singleshot = true);
        bool SetMemoryBreakpoint(ptr address, ptr size, const BreakpointCallback & cbBreakpoint, MemoryType type = MemoryType::Access, bool singleshot = true);
        bool DeleteMemoryBreakpoint(ptr address);

        [[nodiscard]] bool HasBreakpoint(ptr address) const;
        // Copies out what a hit needs so the callback runs without holding the lock.
        bool TakeBreakpointDispatch(ptr address, BreakpointInfo & info, BreakpointCallback & callback) const;
        [[nodiscard]] StepOverKind ClassifyStepOverAt(ptr rip, ptr & nextAddr) const;

        // Drops the /proc/pid/mem descriptor; it is reopened lazily. Needed after execve.
        void ResetMemFd() const;

    private:
        // Guards breakpoints, breakpointCallbacks and softwareBreakpointReferences.
        // Callers mutate them from any thread while the tracee is paused; MemRead
        // unpatches from any thread at any time.
        mutable std::shared_mutex mBreakpointMutex;
        bool setBreakpointLocked(ptr address, bool singleshot, SoftwareType type);
        bool pokeByte(ptr address, uint8 byte) const;
        BreakpointInfo* findSoftwareBreakpoint(ptr address);
        void unpatchBreakpointBytesLocked(ptr address, void* buffer, ptr size) const;
        // Callers must hold mMemFdMutex; the descriptor is closed on execve.
        int memFdLocked() const;
        ssize_t memPread(void* buffer, size_t size, off_t offset) const;
        ssize_t memPwrite(const void* buffer, size_t size, off_t offset) const;
        mutable std::mutex mMemFdMutex;
        mutable int mMemFd = -1;
    };
}
