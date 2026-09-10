#include <ElfBug/thread/Thread.h>
#include <sys/ptrace.h>

namespace ElfBug
{
    Thread::Thread(const pid_t tid)
        : tid(tid)
        , registers(tid)
    {
    }

    bool Thread::StepInto(const int signal)
    {
        if(ptrace(PTRACE_SINGLESTEP, tid, nullptr,
                  reinterpret_cast<void*>(static_cast<uintptr_t>(signal))) == -1)
            return false;
        mIsSingleStepping = true;
        mAtBreakpoint = false;
        return true;
    }
}
