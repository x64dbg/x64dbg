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

    int Process::memFd() const
    {
        std::lock_guard lock(mMemFdMutex);
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
