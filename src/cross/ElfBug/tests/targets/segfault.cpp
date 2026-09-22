// Faults at a known symbol and a known address, so a test can breakpoint the faulting
// instruction and check the address the debugger reports for it.
#include "TargetUtil.h"

static_assert(kSegfaultAddress == 0xdead0000, "keep in sync with the immediate below");

extern "C"
{
    void sf_fault();
    void sf_fault_site();
}

asm(R"(
    .text

    .globl sf_fault
    .type  sf_fault, @function
sf_fault:
    movl    $0xdead0000, %eax
    .globl sf_fault_site
sf_fault_site:
    movl    $42, (%rax)
    ret
)");

int main()
{
    sf_fault();
    return 0;
}
