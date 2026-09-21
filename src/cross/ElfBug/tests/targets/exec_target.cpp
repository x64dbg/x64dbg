// Re-execs itself once from a call site at a known symbol. ASLR is off and personality
// survives execve, so eo_exec_site is the same address on both passes.
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
    eo_call_exec();
    return 7;
}
