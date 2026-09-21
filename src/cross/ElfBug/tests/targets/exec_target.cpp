// Re-execs itself exactly once, from a call site at a known symbol.
//
// The debugger disables ASLR for the tracee and personality() survives execve, so the
// second image lands at the same base as the first. That makes eo_exec_site the same
// address on both passes, which is what lets a test observe whether the debugger poked
// a breakpoint byte into an image that no longer exists.
#include <sys/mman.h>
#include <cstdlib>
#include <unistd.h>

namespace
{
    constexpr const char* kMarker = "ELFBUG_EXEC_TARGET_REEXEC";
    // Far above anything the loader maps, so the second image cannot land on it.
    void* const kScratchHint = reinterpret_cast<void*>(0x700000000000ULL);
}

extern "C"
{
    void eo_exec_once();
    void eo_call_exec();
    void eo_exec_site();
    // Destroyed by execve, so a breakpoint on it outlives the address it names.
    void* eo_scratch = nullptr;
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
    void* page = mmap(kScratchHint, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    eo_scratch = page == MAP_FAILED ? nullptr : page;

    // Both passes run this call. A stray byte left at eo_exec_site by the first pass is
    // therefore executed by the second.
    eo_call_exec();
    return 7;
}
