// TestExe_Exceptions.cpp - Test target for exception handling tests
// Functions that trigger various exceptions for TitanEngine testing

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>

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
    __declspec(dllexport) volatile DWORD g_exception_count = 0;
    __declspec(dllexport) volatile DWORD g_last_exception_code = 0;
    __declspec(dllexport) volatile BOOL g_use_seh_handler = TRUE;
}

// ============================================================================
// ACCESS VIOLATION Triggers
// ============================================================================

// Read from NULL pointer - EXCEPTION_ACCESS_VIOLATION (read)
extern "C" __declspec(dllexport) DWORD __cdecl trigger_access_violation_read()
{
    OutputDebugStringA("[TestExe_Exceptions] trigger_access_violation_read() ENTRY\n");
    volatile DWORD* ptr = NULL;
    OutputDebugStringA("[TestExe_Exceptions] About to read from NULL\n");
    DWORD result = *ptr;  // Read from NULL - will cause access violation
    OutputDebugStringA("[TestExe_Exceptions] After read (should not reach here)\n");
    return result;
}
EXPORT_SYMBOL(trigger_access_violation_read)

// Write to NULL pointer - EXCEPTION_ACCESS_VIOLATION (write)
extern "C" __declspec(dllexport) void __cdecl trigger_access_violation_write()
{
    volatile DWORD* ptr = NULL;
    *ptr = 0x12345678;  // Write to NULL - will cause access violation
}
EXPORT_SYMBOL(trigger_access_violation_write)

// Execute at invalid address - EXCEPTION_ACCESS_VIOLATION (execute)
extern "C" __declspec(dllexport) void __cdecl trigger_access_violation_execute()
{
    typedef void (*func_ptr)();
    func_ptr invalid_func = (func_ptr)0x00000001;  // Invalid address
    invalid_func();  // Try to execute at invalid address
}
EXPORT_SYMBOL(trigger_access_violation_execute)

// Read from unmapped high address
extern "C" __declspec(dllexport) DWORD __cdecl trigger_access_violation_unmapped()
{
    volatile DWORD* ptr = (volatile DWORD*)0xDEADBEEF;
    return *ptr;
}
EXPORT_SYMBOL(trigger_access_violation_unmapped)

// ============================================================================
// Integer Exception Triggers
// ============================================================================

// Division by zero - EXCEPTION_INT_DIVIDE_BY_ZERO
extern "C" __declspec(dllexport) DWORD __cdecl trigger_div_by_zero(DWORD numerator)
{
    volatile DWORD divisor = 0;
    return numerator / divisor;  // Division by zero
}
EXPORT_SYMBOL(trigger_div_by_zero)

// Integer overflow (into trap) - EXCEPTION_INT_OVERFLOW
// Note: This is rarely triggered on modern x86 as INTO is not used
extern "C" __declspec(dllexport) int __cdecl trigger_int_overflow()
{
    // Force an integer overflow scenario
    volatile int a = 0x7FFFFFFF;
    volatile int b = 1;
    return a + b;  // Overflow (though won't trap without INTO)
}
EXPORT_SYMBOL(trigger_int_overflow)

// ============================================================================
// Illegal Instruction Triggers
// ============================================================================

// UD2 instruction - EXCEPTION_ILLEGAL_INSTRUCTION
extern "C" __declspec(dllexport) void __cdecl trigger_illegal_instruction()
{
    __ud2();  // Undefined instruction - guaranteed to raise exception
}
EXPORT_SYMBOL(trigger_illegal_instruction)

// ============================================================================
// Privileged Instruction Triggers
// ============================================================================

// CLI instruction - EXCEPTION_PRIV_INSTRUCTION
extern "C" __declspec(dllexport) void __cdecl trigger_privileged_instruction()
{
    // CLI (clear interrupts) is a privileged instruction
    // In user mode, this will cause EXCEPTION_PRIV_INSTRUCTION
#ifdef _WIN64
    // On x64, we use inline assembly isn't available, use __halt instead
    // Actually __halt is also privileged. We need to emit the bytes directly.
    // CLI = 0xFA
    __asm { cli }
#else
    __asm { cli }
#endif
}
EXPORT_SYMBOL(trigger_privileged_instruction)

// Alternative: HLT instruction
extern "C" __declspec(dllexport) void __cdecl trigger_privileged_hlt()
{
    __halt();  // HLT is privileged
}
EXPORT_SYMBOL(trigger_privileged_hlt)

// ============================================================================
// Breakpoint Exceptions
// ============================================================================

// INT 3 breakpoint - EXCEPTION_BREAKPOINT
extern "C" __declspec(dllexport) void __cdecl trigger_int3()
{
    __debugbreak();  // INT 3
}
EXPORT_SYMBOL(trigger_int3)

// ============================================================================
// Single Step Exception
// ============================================================================

// This function can be used to test single-step exception handling
// The debugger needs to set the trap flag
extern "C" __declspec(dllexport) DWORD __cdecl single_step_target(DWORD a, DWORD b)
{
    DWORD result = a;
    result += b;
    result *= 2;
    return result;
}
EXPORT_SYMBOL(single_step_target)

// ============================================================================
// SEH Handler for Testing
// ============================================================================

// Simple SEH filter that continues execution after exception
LONG WINAPI seh_filter(EXCEPTION_POINTERS* ep)
{
    g_exception_count++;
    g_last_exception_code = ep->ExceptionRecord->ExceptionCode;

    if (g_use_seh_handler)
    {
        // Skip the faulting instruction and continue
        // This is simplified - real handlers would need to know instruction length
#ifdef _WIN64
        ep->ContextRecord->Rip += 2;  // Skip past the instruction (simplified)
#else
        ep->ContextRecord->Eip += 2;  // Skip past the instruction (simplified)
#endif
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

// ============================================================================
// Wrapped Exception Triggers (with SEH)
// ============================================================================

extern "C" __declspec(dllexport) DWORD __cdecl safe_trigger_access_violation_read()
{
    __try
    {
        return trigger_access_violation_read();
    }
    __except(seh_filter(GetExceptionInformation()))
    {
        return 0xFFFFFFFF;
    }
}
EXPORT_SYMBOL(safe_trigger_access_violation_read)

extern "C" __declspec(dllexport) void __cdecl safe_trigger_access_violation_write()
{
    __try
    {
        trigger_access_violation_write();
    }
    __except(seh_filter(GetExceptionInformation()))
    {
    }
}
EXPORT_SYMBOL(safe_trigger_access_violation_write)

extern "C" __declspec(dllexport) DWORD __cdecl safe_trigger_div_by_zero()
{
    __try
    {
        return trigger_div_by_zero(100);
    }
    __except(seh_filter(GetExceptionInformation()))
    {
        return 0xFFFFFFFF;
    }
}
EXPORT_SYMBOL(safe_trigger_div_by_zero)

extern "C" __declspec(dllexport) void __cdecl safe_trigger_illegal_instruction()
{
    __try
    {
        trigger_illegal_instruction();
    }
    __except(seh_filter(GetExceptionInformation()))
    {
    }
}
EXPORT_SYMBOL(safe_trigger_illegal_instruction)

extern "C" __declspec(dllexport) void __cdecl safe_trigger_int3()
{
    __try
    {
        trigger_int3();
    }
    __except(seh_filter(GetExceptionInformation()))
    {
    }
}
EXPORT_SYMBOL(safe_trigger_int3)

// ============================================================================
// Nested Exception Testing
// ============================================================================

extern "C" __declspec(dllexport) DWORD __cdecl nested_exception_outer()
{
    __try
    {
        __try
        {
            return trigger_access_violation_read();
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            g_exception_count++;
            // Trigger another exception in the handler
            return trigger_div_by_zero(50);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        g_exception_count++;
        return 0xDEAD;
    }
}
EXPORT_SYMBOL(nested_exception_outer)

// ============================================================================
// Stack Overflow Trigger
// ============================================================================

// Infinite recursion to cause EXCEPTION_STACK_OVERFLOW
#pragma warning(push)
#pragma warning(disable: 4717) // recursive on all control paths
extern "C" __declspec(dllexport) volatile DWORD __cdecl trigger_stack_overflow(volatile DWORD depth)
{
    volatile BYTE stack_buffer[4096]; // Use some stack space
    stack_buffer[0] = (BYTE)depth;
    stack_buffer[4095] = (BYTE)(depth >> 8);
    return trigger_stack_overflow(depth + 1) + stack_buffer[0] + stack_buffer[4095];
}
#pragma warning(pop)
EXPORT_SYMBOL(trigger_stack_overflow)

// ============================================================================
// Guard Page Trigger
// ============================================================================

// Access a guard page to cause STATUS_GUARD_PAGE_VIOLATION
extern "C" __declspec(dllexport) DWORD __cdecl trigger_guard_page()
{
    // Allocate memory with PAGE_GUARD protection
    LPVOID guardPage = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE | PAGE_GUARD);
    if (!guardPage)
        return 0;

    // Read from the guard page - this triggers STATUS_GUARD_PAGE_VIOLATION
    volatile DWORD* ptr = (volatile DWORD*)guardPage;
    DWORD value = *ptr;

    VirtualFree(guardPage, 0, MEM_RELEASE);
    return value;
}
EXPORT_SYMBOL(trigger_guard_page)

// ============================================================================
// Single Step Trigger
// ============================================================================

// Trigger EXCEPTION_SINGLE_STEP by setting the trap flag
extern "C" __declspec(dllexport) void __cdecl trigger_single_step()
{
    // Set the Trap Flag (TF) in EFLAGS/RFLAGS to trigger EXCEPTION_SINGLE_STEP
    // TF is bit 8 of EFLAGS
#ifdef _WIN64
    // x64: Cannot use inline asm, use intrinsic to read/modify flags
    // We'll use pushfq/popfq pattern through a function pointer trick
    // Actually, on x64 with MSVC we need to emit raw bytes
    BYTE code[] = {
        0x9C,                   // pushfq
        0x48, 0x81, 0x0C, 0x24, // or qword [rsp], 0x100
        0x00, 0x01, 0x00, 0x00,
        0x9D,                   // popfq
        0x90,                   // nop (will single-step here)
        0xC3                    // ret
    };
    LPVOID execMem = VirtualAlloc(NULL, sizeof(code), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (execMem)
    {
        memcpy(execMem, code, sizeof(code));
        typedef void (*TriggerFunc)();
        TriggerFunc func = (TriggerFunc)execMem;
        func();
        VirtualFree(execMem, 0, MEM_RELEASE);
    }
#else
    __asm
    {
        pushfd
        or dword ptr [esp], 0x100  // Set trap flag
        popfd
        nop  // Single step will occur after this instruction
    }
#endif
}
EXPORT_SYMBOL(trigger_single_step)

// ============================================================================
// Vectored Exception Handler Testing
// ============================================================================

static PVOID g_veh_handle = NULL;

LONG CALLBACK vectored_handler(EXCEPTION_POINTERS* ep)
{
    g_exception_count++;
    g_last_exception_code = ep->ExceptionRecord->ExceptionCode;
    return EXCEPTION_CONTINUE_SEARCH;  // Let other handlers process it
}

extern "C" __declspec(dllexport) void __cdecl install_veh()
{
    if (!g_veh_handle)
    {
        g_veh_handle = AddVectoredExceptionHandler(1, vectored_handler);
    }
}
EXPORT_SYMBOL(install_veh)

extern "C" __declspec(dllexport) void __cdecl remove_veh()
{
    if (g_veh_handle)
    {
        RemoveVectoredExceptionHandler(g_veh_handle);
        g_veh_handle = NULL;
    }
}
EXPORT_SYMBOL(remove_veh)

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[])
{
    g_use_seh_handler = TRUE;

    // Debug: output argc/argv to verify command line parsing
    OutputDebugStringA("[TestExe_Exceptions] main() called\n");
    char buf[256];
    wsprintfA(buf, "[TestExe_Exceptions] argc=%d\n", argc);
    OutputDebugStringA(buf);
    for (int i = 0; i < argc; i++)
    {
        wsprintfA(buf, "[TestExe_Exceptions] argv[%d]=%s\n", i, argv[i]);
        OutputDebugStringA(buf);
    }

    // Check for specific exception trigger arguments FIRST
    // This avoids having safe_* first-chance exceptions interfere with testing
    if (argc > 1)
    {
        g_use_seh_handler = FALSE;
        OutputDebugStringA("[TestExe_Exceptions] Has arguments, calling trigger function\n");

        // Debug: show exact argv[1] content
        wsprintfA(buf, "[TestExe_Exceptions] argv[1] len=%d, strcmp result=%d\n",
                  (int)strlen(argv[1]), strcmp(argv[1], "--read"));
        OutputDebugStringA(buf);

        if (strcmp(argv[1], "--read") == 0)
        {
            OutputDebugStringA("[TestExe_Exceptions] Triggering read AV\n");
            trigger_access_violation_read();
        }
        else if (strcmp(argv[1], "--write") == 0)
        {
            trigger_access_violation_write();
        }
        else if (strcmp(argv[1], "--execute") == 0)
        {
            trigger_access_violation_execute();
        }
        else if (strcmp(argv[1], "--divzero") == 0)
        {
            trigger_div_by_zero(100);
        }
        else if (strcmp(argv[1], "--illegal") == 0)
        {
            trigger_illegal_instruction();
        }
        else if (strcmp(argv[1], "--privileged") == 0)
        {
            trigger_privileged_instruction();
        }
        else if (strcmp(argv[1], "--int3") == 0)
        {
            trigger_int3();
        }
        else if (strcmp(argv[1], "--singlestep") == 0)
        {
            trigger_single_step();
        }
        else if (strcmp(argv[1], "--stackoverflow") == 0)
        {
            trigger_stack_overflow(0);
        }
        else if (strcmp(argv[1], "--guardpage") == 0)
        {
            trigger_guard_page();
        }
        else if (strcmp(argv[1], "--nested") == 0)
        {
            nested_exception_outer();
        }
        // Exit after the specific test - don't run safe_* functions
        return (int)g_exception_count;
    }

    // If no arguments, run safe self-test versions (with SEH handlers)
    safe_trigger_access_violation_read();
    safe_trigger_access_violation_write();
    safe_trigger_div_by_zero();
    safe_trigger_illegal_instruction();
    safe_trigger_int3();

    // Test nested exceptions
    nested_exception_outer();

    return (int)g_exception_count;
}
