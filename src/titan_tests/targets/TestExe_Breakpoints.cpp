// TestExe_Breakpoints.cpp - Test target for SW/HW/Memory breakpoint tests
// Exports functions that can have breakpoints set on them for TitanEngine testing

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <string.h>

// Define QWORD if not defined
typedef unsigned __int64 QWORD;

// Export macros for symbols
#ifdef _WIN64
#define EXPORT_SYMBOL(name) __pragma(comment(linker, "/EXPORT:" #name))
#else
#define EXPORT_SYMBOL(name) __pragma(comment(linker, "/EXPORT:_" #name))
#endif

// Global volatile variables for memory breakpoint testing
// These should be accessed by the bp_memory_* functions
extern "C" {
    __declspec(dllexport) volatile DWORD g_memory_read_target = 0x12345678;
    __declspec(dllexport) volatile DWORD g_memory_write_target = 0;
    __declspec(dllexport) volatile BYTE g_memory_byte_target = 0x42;
    __declspec(dllexport) volatile QWORD g_memory_qword_target = 0xDEADBEEFCAFEBABEull;
}

// Counter for loop breakpoint testing
extern "C" __declspec(dllexport) volatile DWORD g_loop_counter = 0;

// ============================================================================
// Software Breakpoint Targets (INT3, LONG_INT3, UD2)
// ============================================================================

// Simple target for INT3 (0xCC) software breakpoint
extern "C" __declspec(dllexport) DWORD __cdecl bp_target_sw1(DWORD value)
{
    // Simple function - breakpoint on entry
    return value * 2;
}
EXPORT_SYMBOL(bp_target_sw1)

// Target for LONG_INT3 (0xCD03) software breakpoint
extern "C" __declspec(dllexport) DWORD __cdecl bp_target_sw2(DWORD value)
{
    // Another simple function
    return value + 100;
}
EXPORT_SYMBOL(bp_target_sw2)

// Target for UD2 (0x0F0B) software breakpoint
extern "C" __declspec(dllexport) DWORD __cdecl bp_target_sw3(DWORD value)
{
    // Third simple function
    return value ^ 0xDEADBEEF;
}
EXPORT_SYMBOL(bp_target_sw3)

// Target for SW-04: Delete breakpoint test
extern "C" __declspec(dllexport) DWORD __cdecl bp_target_sw4(DWORD value)
{
    return value + 1;
}
EXPORT_SYMBOL(bp_target_sw4)

// Target for SW-05: Multiple BPs on same address
extern "C" __declspec(dllexport) DWORD __cdecl bp_target_sw5(DWORD value)
{
    return value - 1;
}
EXPORT_SYMBOL(bp_target_sw5)

// Target for SW-06: Loop body for persistent BP testing
// This is the inner function called in a loop
extern "C" __declspec(dllexport) void __cdecl bp_target_sw6_loop()
{
    g_loop_counter++;
}
EXPORT_SYMBOL(bp_target_sw6_loop)

// Target for SW-07: Delete BP during callback test
extern "C" __declspec(dllexport) DWORD __cdecl bp_target_sw7(DWORD value)
{
    return value * 3;
}
EXPORT_SYMBOL(bp_target_sw7)

// Target for SW-08: IsBPXEnabled accuracy test
extern "C" __declspec(dllexport) DWORD __cdecl bp_target_sw8(DWORD value)
{
    return value / 2;
}
EXPORT_SYMBOL(bp_target_sw8)

// Targets for SW-09: RemoveAllBreakPoints test
extern "C" __declspec(dllexport) DWORD __cdecl bp_target_sw9a(DWORD value)
{
    return value & 0xFF;
}
EXPORT_SYMBOL(bp_target_sw9a)

extern "C" __declspec(dllexport) DWORD __cdecl bp_target_sw9b(DWORD value)
{
    return value | 0xFF00;
}
EXPORT_SYMBOL(bp_target_sw9b)

// ============================================================================
// Loop Target for Breakpoint Hit Counting
// ============================================================================

// This function runs a loop - useful for testing breakpoint hit counts
extern "C" __declspec(dllexport) void __cdecl bp_loop_func(DWORD iterations)
{
    for (DWORD i = 0; i < iterations; i++)
    {
        g_loop_counter++;
        // The body of the loop can have a breakpoint set
        volatile DWORD dummy = g_loop_counter * 2;
        (void)dummy;
    }
}
EXPORT_SYMBOL(bp_loop_func)

// ============================================================================
// Hardware Breakpoint Targets
// ============================================================================

// Target for hardware execution breakpoint
extern "C" __declspec(dllexport) DWORD __cdecl bp_target_hw(DWORD a, DWORD b)
{
    return a + b;
}
EXPORT_SYMBOL(bp_target_hw)

// Additional hardware breakpoint targets
extern "C" __declspec(dllexport) DWORD __cdecl bp_target_hw2(DWORD value)
{
    return value << 1;
}
EXPORT_SYMBOL(bp_target_hw2)

extern "C" __declspec(dllexport) DWORD __cdecl bp_target_hw3(DWORD value)
{
    return value >> 1;
}
EXPORT_SYMBOL(bp_target_hw3)

extern "C" __declspec(dllexport) DWORD __cdecl bp_target_hw4(DWORD value)
{
    return ~value;
}
EXPORT_SYMBOL(bp_target_hw4)

// ============================================================================
// Memory Breakpoint Targets
// ============================================================================

// Function that reads from global memory - for read breakpoints
extern "C" __declspec(dllexport) DWORD __cdecl bp_memory_read()
{
    return g_memory_read_target;
}
EXPORT_SYMBOL(bp_memory_read)

// Function that writes to global memory - for write breakpoints
extern "C" __declspec(dllexport) void __cdecl bp_memory_write(DWORD value)
{
    g_memory_write_target = value;
}
EXPORT_SYMBOL(bp_memory_write)

// Function that does read-modify-write
extern "C" __declspec(dllexport) DWORD __cdecl bp_memory_readwrite(DWORD addValue)
{
    DWORD old = g_memory_write_target;
    g_memory_write_target = old + addValue;
    return old;
}
EXPORT_SYMBOL(bp_memory_readwrite)

// Byte-sized memory access
extern "C" __declspec(dllexport) BYTE __cdecl bp_memory_byte_read()
{
    return g_memory_byte_target;
}
EXPORT_SYMBOL(bp_memory_byte_read)

extern "C" __declspec(dllexport) void __cdecl bp_memory_byte_write(BYTE value)
{
    g_memory_byte_target = value;
}
EXPORT_SYMBOL(bp_memory_byte_write)

// QWORD-sized memory access
extern "C" __declspec(dllexport) QWORD __cdecl bp_memory_qword_read()
{
    return g_memory_qword_target;
}
EXPORT_SYMBOL(bp_memory_qword_read)

extern "C" __declspec(dllexport) void __cdecl bp_memory_qword_write(QWORD value)
{
    g_memory_qword_target = value;
}
EXPORT_SYMBOL(bp_memory_qword_write)

// ============================================================================
// Additional Utility Functions
// ============================================================================

// Simple nop-sled function for testing address ranges
extern "C" __declspec(dllexport) void __cdecl bp_nop_sled()
{
    __nop(); __nop(); __nop(); __nop();
    __nop(); __nop(); __nop(); __nop();
    __nop(); __nop(); __nop(); __nop();
    __nop(); __nop(); __nop(); __nop();
}
EXPORT_SYMBOL(bp_nop_sled)

// Function with multiple instruction types for testing
extern "C" __declspec(dllexport) DWORD __cdecl bp_mixed_ops(DWORD a, DWORD b, DWORD c)
{
    DWORD result = a;
    result += b;          // ADD
    result ^= c;          // XOR
    result = (result << 3) | (result >> 29);  // ROL
    result *= 7;          // IMUL
    return result;
}
EXPORT_SYMBOL(bp_mixed_ops)

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[])
{
    // Default behavior: call each function once to verify they work
    volatile DWORD result = 0;

    // Test software breakpoint targets (SW-01 through SW-09)
    result += bp_target_sw1(10);
    result += bp_target_sw2(20);
    result += bp_target_sw3(30);
    result += bp_target_sw4(40);
    result += bp_target_sw5(50);
    // SW-06: Call the loop function multiple times to test persistent BPs
    for (int i = 0; i < 5; i++)
    {
        bp_target_sw6_loop();
    }
    result += bp_target_sw7(60);
    result += bp_target_sw8(70);
    result += bp_target_sw9a(80);
    result += bp_target_sw9b(90);

    // Test hardware breakpoint target
    result += bp_target_hw(5, 7);
    result += bp_target_hw2(100);
    result += bp_target_hw3(200);
    result += bp_target_hw4(300);

    // Test loop function
    bp_loop_func(10);
    result += g_loop_counter;

    // Test memory access functions
    result += bp_memory_read();
    bp_memory_write(0xCAFEBABE);
    result += bp_memory_readwrite(100);

    result += bp_memory_byte_read();
    bp_memory_byte_write(0xFF);

    // Test nop sled
    bp_nop_sled();

    // Test mixed ops
    result += bp_mixed_ops(1, 2, 3);

    // If command line argument provided, loop forever (for attach testing)
    if (argc > 1 && strcmp(argv[1], "--loop") == 0)
    {
        while (true)
        {
            bp_loop_func(1);
            Sleep(100);
        }
    }

    return (int)(result & 0xFF);
}
