// Raw clone threads whose first instruction is the breakpoint site, so the child's trap
// follows its initial stop within one instruction and can reach waitpid before the
// parent's clone event.
#include <sched.h>
#include <sys/mman.h>

extern "C"
{
    volatile int ct_done = 0;
    // Breakpoint site: the instruction both the parent and the new child return to.
    void ct_site();
    void ct_spawn(void* stackTop);
}

asm(R"(
    .text

    .globl ct_spawn
    .type  ct_spawn, @function
ct_spawn:
    movq    %rdi, %rsi
    movq    $0x50f00, %rdi
    xorq    %rdx, %rdx
    xorq    %r10, %r10
    xorq    %r8, %r8
    movq    $56, %rax
    syscall
    .globl ct_site
ct_site:
    testq   %rax, %rax
    jnz     1f
    lock incl ct_done(%rip)
    movq    $60, %rax
    xorq    %rdi, %rdi
    syscall
1:
    ret
)");

int main()
{
    for(int i = 0; i < 32; ++i)
    {
        const long size = 64 * 1024;
        void* stack = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
        if(stack == MAP_FAILED)
            return 1;
        ct_spawn(static_cast<char*>(stack) + size);
        while(ct_done <= i)
            sched_yield();
    }
    return 0;
}
