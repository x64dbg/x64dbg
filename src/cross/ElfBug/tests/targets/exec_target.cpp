// Re-execs itself exactly once, from a call site at a known symbol.
//
// The debugger disables ASLR for the tracee and personality() survives execve, so the
// second image lands at the same base as the first. That makes eo_exec_site the same
// address on both passes, which is what lets a test observe whether the debugger poked
// a breakpoint byte into an image that no longer exists.
#include <cstdlib>
#include <unistd.h>

namespace
{
    constexpr const char* kMarker = "ELFBUG_EXEC_TARGET_REEXEC";
}

extern "C"
{
    void eo_exec_once();
    void eo_call_exec();
    void eo_exec_site();
}

extern "C" void eo_exec_once()
{
    if(getenv(kMarker))
        return;

    setenv(kMarker, "1", 1);
    char self[] = "/proc/self/exe";
    char* argv[] = {self, nullptr};
    execv(self, argv);
}

// Hand-written so eo_exec_site is the exact address of the call, not of a prologue.
asm(R"(
    .text

    .globl eo_call_exec
    .type  eo_call_exec, @function
eo_call_exec:
    pushq   %rbp
    movq    %rsp, %rbp
    .globl eo_exec_site
eo_exec_site:
    call    eo_exec_once
    popq    %rbp
    ret
)");

int main()
{
    // Both passes run this call. A stray byte left at eo_exec_site by the first pass is
    // therefore executed by the second.
    eo_call_exec();
    return 7;
}
