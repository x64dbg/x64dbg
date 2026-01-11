// TestExe_Stepping.cpp - Test target for step-into/step-over tests
// Provides nested call chains, loops, and REP instructions for stepping tests

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <string.h>

// Export macros for symbols
#ifdef _WIN64
#define EXPORT_SYMBOL(name) __pragma(comment(linker, "/EXPORT:" #name))
#else
#define EXPORT_SYMBOL(name) __pragma(comment(linker, "/EXPORT:_" #name))
#endif

// ============================================================================
// Global State
// ============================================================================

extern "C" {
    __declspec(dllexport) volatile DWORD g_call_depth = 0;
    __declspec(dllexport) volatile DWORD g_step_counter = 0;
    __declspec(dllexport) volatile DWORD g_loop_iteration = 0;
}

// ============================================================================
// Nested Call Chain for Step-Into Testing
// ============================================================================

// Deepest function in the call chain
extern "C" __declspec(dllexport) DWORD __cdecl step_level4(DWORD value)
{
    g_call_depth = 4;
    g_step_counter++;
    return value * 2;
}
EXPORT_SYMBOL(step_level4)

// Third level
extern "C" __declspec(dllexport) DWORD __cdecl step_level3(DWORD value)
{
    g_call_depth = 3;
    g_step_counter++;
    DWORD result = step_level4(value + 1);
    g_call_depth = 3;  // Reset after return
    return result + 10;
}
EXPORT_SYMBOL(step_level3)

// Second level
extern "C" __declspec(dllexport) DWORD __cdecl step_level2(DWORD value)
{
    g_call_depth = 2;
    g_step_counter++;
    DWORD result = step_level3(value + 1);
    g_call_depth = 2;  // Reset after return
    return result + 100;
}
EXPORT_SYMBOL(step_level2)

// First level - entry point for nested calls
extern "C" __declspec(dllexport) DWORD __cdecl step_level1(DWORD value)
{
    g_call_depth = 1;
    g_step_counter++;
    DWORD result = step_level2(value + 1);
    g_call_depth = 1;  // Reset after return
    return result + 1000;
}
EXPORT_SYMBOL(step_level1)

// ============================================================================
// Functions with Loops for Step-Over Testing
// ============================================================================

// Simple loop - step over should execute entire loop
extern "C" __declspec(dllexport) DWORD __cdecl step_simple_loop(DWORD iterations)
{
    DWORD sum = 0;
    for (DWORD i = 0; i < iterations; i++)
    {
        g_loop_iteration = i;
        sum += i;
    }
    return sum;
}
EXPORT_SYMBOL(step_simple_loop)

// Nested loops
extern "C" __declspec(dllexport) DWORD __cdecl step_nested_loop(DWORD outer, DWORD inner)
{
    DWORD sum = 0;
    for (DWORD i = 0; i < outer; i++)
    {
        for (DWORD j = 0; j < inner; j++)
        {
            g_loop_iteration = i * inner + j;
            sum += i * j;
        }
    }
    return sum;
}
EXPORT_SYMBOL(step_nested_loop)

// While loop
extern "C" __declspec(dllexport) DWORD __cdecl step_while_loop(DWORD count)
{
    DWORD i = 0;
    DWORD sum = 0;
    while (i < count)
    {
        g_loop_iteration = i;
        sum += i;
        i++;
    }
    return sum;
}
EXPORT_SYMBOL(step_while_loop)

// Loop with function calls inside
extern "C" __declspec(dllexport) DWORD __cdecl step_loop_with_calls(DWORD iterations)
{
    DWORD sum = 0;
    for (DWORD i = 0; i < iterations; i++)
    {
        g_loop_iteration = i;
        sum += step_level4(i);  // Call inside loop
    }
    return sum;
}
EXPORT_SYMBOL(step_loop_with_calls)

// ============================================================================
// REP Instruction Patterns
// ============================================================================

// Buffer for REP operations
static char g_src_buffer[256] = "Hello, World! This is a test buffer for REP instructions.";
static char g_dst_buffer[256] = {0};

// REP MOVSB - byte-by-byte copy
extern "C" __declspec(dllexport) void __cdecl step_rep_movsb(void* dst, const void* src, size_t count)
{
    // The compiler will typically generate REP MOVSB for this
    memcpy(dst, src, count);
}
EXPORT_SYMBOL(step_rep_movsb)

// REP STOSB - fill memory with byte
extern "C" __declspec(dllexport) void __cdecl step_rep_stosb(void* dst, int value, size_t count)
{
    memset(dst, value, count);
}
EXPORT_SYMBOL(step_rep_stosb)

// REP CMPSB - compare strings
extern "C" __declspec(dllexport) int __cdecl step_rep_cmpsb(const char* s1, const char* s2)
{
    return strcmp(s1, s2);
}
EXPORT_SYMBOL(step_rep_cmpsb)

// REP SCASB - scan for character
extern "C" __declspec(dllexport) const char* __cdecl step_rep_scasb(const char* s, int c)
{
    return strchr(s, c);
}
EXPORT_SYMBOL(step_rep_scasb)

// Wrapper that uses the buffers
extern "C" __declspec(dllexport) void __cdecl step_rep_test()
{
    // Clear destination
    step_rep_stosb(g_dst_buffer, 0, sizeof(g_dst_buffer));

    // Copy source to destination
    step_rep_movsb(g_dst_buffer, g_src_buffer, strlen(g_src_buffer) + 1);

    // Compare
    volatile int cmp_result = step_rep_cmpsb(g_src_buffer, g_dst_buffer);
    (void)cmp_result;

    // Find character
    volatile const char* found = step_rep_scasb(g_dst_buffer, 'W');
    (void)found;
}
EXPORT_SYMBOL(step_rep_test)

// ============================================================================
// Conditional Branching for Step Testing
// ============================================================================

// Function with multiple branches
extern "C" __declspec(dllexport) DWORD __cdecl step_conditional(DWORD value)
{
    DWORD result = 0;

    if (value < 10)
    {
        result = value * 2;
    }
    else if (value < 50)
    {
        result = value + 100;
    }
    else if (value < 100)
    {
        result = value - 50;
    }
    else
    {
        result = value / 2;
    }

    return result;
}
EXPORT_SYMBOL(step_conditional)

// Switch statement
extern "C" __declspec(dllexport) DWORD __cdecl step_switch(DWORD selector)
{
    DWORD result = 0;

    switch (selector % 5)
    {
    case 0:
        result = 100;
        break;
    case 1:
        result = 200;
        break;
    case 2:
        result = 300;
        break;
    case 3:
        result = 400;
        break;
    case 4:
        result = 500;
        break;
    default:
        result = 0;
        break;
    }

    return result;
}
EXPORT_SYMBOL(step_switch)

// ============================================================================
// Recursive Functions for Step Testing
// ============================================================================

extern "C" __declspec(dllexport) DWORD __cdecl step_factorial(DWORD n)
{
    g_call_depth++;
    DWORD result;

    if (n <= 1)
    {
        result = 1;
    }
    else
    {
        result = n * step_factorial(n - 1);
    }

    g_call_depth--;
    return result;
}
EXPORT_SYMBOL(step_factorial)

extern "C" __declspec(dllexport) DWORD __cdecl step_fibonacci(DWORD n)
{
    g_call_depth++;
    g_step_counter++;
    DWORD result;

    if (n <= 1)
    {
        result = n;
    }
    else
    {
        result = step_fibonacci(n - 1) + step_fibonacci(n - 2);
    }

    g_call_depth--;
    return result;
}
EXPORT_SYMBOL(step_fibonacci)

// ============================================================================
// Step Target with Various Instruction Types
// ============================================================================

extern "C" __declspec(dllexport) DWORD __cdecl step_mixed_instructions(DWORD a, DWORD b)
{
    DWORD result = a;

    // Various instruction types
    result = result + b;                              // ADD
    result = result - 1;                              // SUB
    result = result ^ 0xAAAAAAAA;                     // XOR
    result = result & 0xFFFF0000;                     // AND
    result = result | 0x0000FFFF;                     // OR
    result = (result << 4) | (result >> 28);          // ROL (simulated)
    result = (result >> 4) | (result << 28);          // ROR (simulated)
    result = ~result;                                 // NOT

    // Some NOPs for easy single-step testing
    __nop();
    __nop();
    __nop();

    return result;
}
EXPORT_SYMBOL(step_mixed_instructions)

// ============================================================================
// Function with Inline Assembly (x86 only) / Intrinsics (x64)
// ============================================================================

#ifndef _WIN64
extern "C" __declspec(dllexport) DWORD __cdecl step_inline_asm(DWORD value)
{
    DWORD result;
    __asm
    {
        mov eax, value
        nop
        add eax, 10
        nop
        sub eax, 5
        nop
        shl eax, 2
        nop
        mov result, eax
    }
    return result;
}
EXPORT_SYMBOL(step_inline_asm)
#else
extern "C" __declspec(dllexport) DWORD __cdecl step_inline_asm(DWORD value)
{
    // x64 version using intrinsics
    DWORD result = value;
    __nop();
    result += 10;
    __nop();
    result -= 5;
    __nop();
    result <<= 2;
    __nop();
    return result;
}
EXPORT_SYMBOL(step_inline_asm)
#endif

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[])
{
    volatile DWORD result = 0;

    // Reset counters
    g_call_depth = 0;
    g_step_counter = 0;
    g_loop_iteration = 0;

    // Test nested calls
    result += step_level1(1);

    // Test loops
    result += step_simple_loop(10);
    result += step_nested_loop(3, 4);
    result += step_while_loop(5);
    result += step_loop_with_calls(3);

    // Test REP instructions
    step_rep_test();

    // Test conditionals
    result += step_conditional(5);
    result += step_conditional(25);
    result += step_conditional(75);
    result += step_conditional(150);

    // Test switch
    for (DWORD i = 0; i < 5; i++)
    {
        result += step_switch(i);
    }

    // Test recursion
    result += step_factorial(5);
    result += step_fibonacci(10);

    // Test mixed instructions
    result += step_mixed_instructions(100, 50);

    // Test inline asm
    result += step_inline_asm(20);

    return (int)(result & 0xFF);
}
