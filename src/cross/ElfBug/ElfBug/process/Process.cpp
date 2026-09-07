#include <ElfBug/process/Process.h>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

namespace ElfBug
{
    Process::Process(const pid_t pid)
        : pid(pid)
    {
    }

    Process::~Process()
    {
        if(mMemFd != -1)
            close(mMemFd);
    }

    int Process::memFdLocked() const
    {
        if(mMemFd == -1)
        {
            char path[64];
            snprintf(path, sizeof(path), "/proc/%d/mem", pid);
            mMemFd = open(path, O_RDWR);
            if(mMemFd == -1)
                mMemFd = open(path, O_RDONLY);
        }
        return mMemFd;
    }

    // The lock spans the syscall: ResetMemFd would otherwise close the descriptor
    // mid-read and the number could be reused by an unrelated open.
    ssize_t Process::memPread(void* buffer, const size_t size, const off_t offset) const
    {
        std::lock_guard lock(mMemFdMutex);
        const int fd = memFdLocked();
        if(fd == -1)
            return -1;
        return pread(fd, buffer, size, offset);
    }

    ssize_t Process::memPwrite(const void* buffer, const size_t size, const off_t offset) const
    {
        std::lock_guard lock(mMemFdMutex);
        const int fd = memFdLocked();
        if(fd == -1)
            return -1;
        return pwrite(fd, buffer, size, offset);
    }

    void Process::ResetMemFd() const
    {
        std::lock_guard lock(mMemFdMutex);
        if(mMemFd != -1)
        {
            close(mMemFd);
            mMemFd = -1;
        }
    }
}
