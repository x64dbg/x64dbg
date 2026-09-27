#include <ElfBug/thread/Thread.h>
#include <ElfBug/types/Ptrace.h>
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
        if(ptrace(PTRACE_SINGLESTEP, tid, nullptr, PtraceData(signal)) == -1)
            return false;
        mIsSingleStepping = true;
        mAtBreakpoint = false;
        return true;
    }
}
