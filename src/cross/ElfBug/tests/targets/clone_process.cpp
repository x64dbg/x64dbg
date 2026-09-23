// A clone that shares the address space but starts its own thread group, which the kernel
// still reports through PTRACE_EVENT_CLONE. Every other clone target passes CLONE_THREAD.
#include <sched.h>
#include <sys/mman.h>
#include "TargetUtil.h"

extern "C"
{
    volatile int cp_child_ran = 0;
    volatile int cp_stop = 0;
}

namespace
{
    constexpr std::size_t kStackSize = 128 * 1024;

    int child(void*)
    {
        while(cp_stop == 0)
        {
            cp_child_ran++;
            nap(1000000);
        }
        return 0;
    }
}

int main()
{
    void* stack = mmap(nullptr, kStackSize, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if(stack == MAP_FAILED)
        return 1;

    char* stackTop = static_cast<char*>(stack) + kStackSize;
    if(clone(child, stackTop, CLONE_VM | CLONE_FS | CLONE_FILES, nullptr) == -1)
        return 1;

    while(cp_stop == 0)
        nap(1000000);

    return 0;
}
