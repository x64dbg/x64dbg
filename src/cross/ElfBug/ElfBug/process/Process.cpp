#include <ElfBug/process/Process.h>
#include <ElfBug/process/ProcFs.h>
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

    Thread* Process::FindThread(const pid_t tid) const
    {
        const auto it = threads.find(tid);
        return it != threads.end() ? it->second.get() : nullptr;
    }

    int Process::memFdLocked() const
    {
        if(mMemFd == -1)
        {
            const std::string path = procfs::Path(pid, "mem");
            mMemFd = open(path.c_str(), O_RDWR | O_CLOEXEC);
            if(mMemFd == -1)
                mMemFd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
        }
        return mMemFd;
    }

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
