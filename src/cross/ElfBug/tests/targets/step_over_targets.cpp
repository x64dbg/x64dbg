// Hand-written asm because the tests need byte-exact instruction addresses.
#include <cstdint>

extern "C"
{
    volatile int so_callee_ran = 0;
    volatile int so_recurse_depth = 0;
    unsigned char so_src[64];
    unsigned char so_dst[64];

    void so_call_site();
    void so_recurse_site();
    void so_rep_site();
    void so_rep_insn();
    void so_pushf_site();
    void so_plain_site();
    void so_getpc_site();
    void so_run_all();
}

asm(R"(
    .text

    .globl so_callee
    .type  so_callee, @function
so_callee:
    movl    $1, so_callee_ran(%rip)
    ret

    .globl so_call_site
    .type  so_call_site, @function
so_call_site:
    call    so_callee
    ret

    .globl so_recurse
    .type  so_recurse, @function
so_recurse:
    pushq   %rbp
    movq    %rsp, %rbp
    movl    so_recurse_depth(%rip), %eax
    cmpl    $2, %eax
    jge     1f
    addl    $1, %eax
    movl    %eax, so_recurse_depth(%rip)
    .globl so_recurse_site
so_recurse_site:
    call    so_recurse
1:
    popq    %rbp
    ret

    .globl so_rep_site
    .type  so_rep_site, @function
so_rep_site:
    leaq    so_src(%rip), %rsi
    leaq    so_dst(%rip), %rdi
    movq    $64, %rcx
    cld
    .globl so_rep_insn
so_rep_insn:
    rep movsb
    ret

    .globl so_pushf_site
    .type  so_pushf_site, @function
so_pushf_site:
    pushfq
    popq    %rax
    ret

    .globl so_getpc_site
    .type  so_getpc_site, @function
so_getpc_site:
    call    1f
1:  popq    %rax
    ret

    .globl so_plain_site
    .type  so_plain_site, @function
so_plain_site:
    nop
    ret

    .globl so_run_all
    .type  so_run_all, @function
so_run_all:
    pushq   %rbp
    movq    %rsp, %rbp
    call    so_call_site
    call    so_recurse
    call    so_rep_site
    call    so_pushf_site
    call    so_plain_site
    call    so_getpc_site
    popq    %rbp
    ret
)");

int main()
{
    so_run_all();
    return 0;
}
