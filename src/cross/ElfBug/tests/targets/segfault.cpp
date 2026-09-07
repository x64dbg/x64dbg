// Faults at a known symbol so a test can put a breakpoint on the faulting instruction.
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
    xorl    %eax, %eax
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
