// TestExe_Context.cpp - Test target for register/context testing
// Functions that set specific register values for verifying context read/write

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
// Global State for Verification
// ============================================================================

extern "C" {
    // Values that should be in registers at various points
    __declspec(dllexport) volatile DWORD g_expected_eax = 0;
    __declspec(dllexport) volatile DWORD g_expected_ebx = 0;
    __declspec(dllexport) volatile DWORD g_expected_ecx = 0;
    __declspec(dllexport) volatile DWORD g_expected_edx = 0;

#ifdef _WIN64
    __declspec(dllexport) volatile DWORD64 g_expected_rax = 0;
    __declspec(dllexport) volatile DWORD64 g_expected_rbx = 0;
    __declspec(dllexport) volatile DWORD64 g_expected_rcx = 0;
    __declspec(dllexport) volatile DWORD64 g_expected_rdx = 0;
    __declspec(dllexport) volatile DWORD64 g_expected_r8 = 0;
    __declspec(dllexport) volatile DWORD64 g_expected_r9 = 0;
    __declspec(dllexport) volatile DWORD64 g_expected_r10 = 0;
    __declspec(dllexport) volatile DWORD64 g_expected_r11 = 0;
#endif

    // Flag state
    __declspec(dllexport) volatile BOOL g_expect_cf = FALSE;
    __declspec(dllexport) volatile BOOL g_expect_zf = FALSE;
    __declspec(dllexport) volatile BOOL g_expect_sf = FALSE;
}

// ============================================================================
// Functions that Set Known Register Values (x86)
// ============================================================================

#ifndef _WIN64

// Set EAX to known value
extern "C" __declspec(dllexport) void __cdecl __declspec(naked) ctx_set_eax()
{
    __asm
    {
        mov eax, 0xDEADBEEF
        nop  ; Breakpoint here to verify EAX
        ret
    }
}
EXPORT_SYMBOL(ctx_set_eax)

// Set EBX to known value
extern "C" __declspec(dllexport) void __cdecl __declspec(naked) ctx_set_ebx()
{
    __asm
    {
        push ebx
        mov ebx, 0xCAFEBABE
        nop  ; Breakpoint here to verify EBX
        pop ebx
        ret
    }
}
EXPORT_SYMBOL(ctx_set_ebx)

// Set ECX to known value
extern "C" __declspec(dllexport) void __cdecl __declspec(naked) ctx_set_ecx()
{
    __asm
    {
        mov ecx, 0x12345678
        nop  ; Breakpoint here to verify ECX
        ret
    }
}
EXPORT_SYMBOL(ctx_set_ecx)

// Set EDX to known value
extern "C" __declspec(dllexport) void __cdecl __declspec(naked) ctx_set_edx()
{
    __asm
    {
        mov edx, 0xABCDEF00
        nop  ; Breakpoint here to verify EDX
        ret
    }
}
EXPORT_SYMBOL(ctx_set_edx)

// Set multiple registers
extern "C" __declspec(dllexport) void __cdecl __declspec(naked) ctx_set_all_gpr()
{
    __asm
    {
        push ebx
        push esi
        push edi

        mov eax, 0x11111111
        mov ebx, 0x22222222
        mov ecx, 0x33333333
        mov edx, 0x44444444
        mov esi, 0x55555555
        mov edi, 0x66666666

        nop  ; Breakpoint here to verify all GPRs

        pop edi
        pop esi
        pop ebx
        ret
    }
}
EXPORT_SYMBOL(ctx_set_all_gpr)

// Set flags to known state (CF=1)
extern "C" __declspec(dllexport) void __cdecl __declspec(naked) ctx_set_cf()
{
    __asm
    {
        stc  ; Set carry flag
        nop  ; Breakpoint here to verify CF=1
        ret
    }
}
EXPORT_SYMBOL(ctx_set_cf)

// Set flags to known state (ZF=1)
extern "C" __declspec(dllexport) void __cdecl __declspec(naked) ctx_set_zf()
{
    __asm
    {
        xor eax, eax  ; Sets ZF=1
        nop  ; Breakpoint here to verify ZF=1
        ret
    }
}
EXPORT_SYMBOL(ctx_set_zf)

// Set flags to known state (SF=1)
extern "C" __declspec(dllexport) void __cdecl __declspec(naked) ctx_set_sf()
{
    __asm
    {
        mov eax, -1  ; Will have SF=1 after operation
        test eax, eax
        nop  ; Breakpoint here to verify SF=1
        ret
    }
}
EXPORT_SYMBOL(ctx_set_sf)

// Function that uses FPU
extern "C" __declspec(dllexport) void __cdecl ctx_set_fpu()
{
    double value = 3.14159265358979323846;
    volatile double result = value * 2.0;
    (void)result;
    // Breakpoint here to verify FPU state
    __nop();
}
EXPORT_SYMBOL(ctx_set_fpu)

#else // _WIN64

// x64 versions - can't use inline asm, so we use intrinsics and patterns

// Function that sets RAX to known value
extern "C" __declspec(dllexport) DWORD64 __cdecl ctx_set_rax()
{
    g_expected_rax = 0xDEADBEEFCAFEBABEull;
    return 0xDEADBEEFCAFEBABEull;  // Return value will be in RAX
    // Caller can breakpoint after call to verify RAX
}
EXPORT_SYMBOL(ctx_set_rax)

// Function that sets RCX, RDX (first two params on x64)
extern "C" __declspec(dllexport) void __cdecl ctx_set_params(DWORD64 rcx_val, DWORD64 rdx_val)
{
    g_expected_rcx = rcx_val;
    g_expected_rdx = rdx_val;
    __nop();  // Breakpoint here - RCX and RDX will have param values
}
EXPORT_SYMBOL(ctx_set_params)

// Function that sets R8, R9 (params 3-4 on x64)
extern "C" __declspec(dllexport) void __cdecl ctx_set_r8_r9(DWORD64 a, DWORD64 b, DWORD64 r8_val, DWORD64 r9_val)
{
    (void)a; (void)b;
    g_expected_r8 = r8_val;
    g_expected_r9 = r9_val;
    __nop();  // Breakpoint here - R8 and R9 will have param values
}
EXPORT_SYMBOL(ctx_set_r8_r9)

// Function for FPU/XMM testing
extern "C" __declspec(dllexport) double __cdecl ctx_set_xmm0(double value)
{
    // On x64, floating point params/returns use XMM registers
    return value * 2.0;  // XMM0 will have the result
}
EXPORT_SYMBOL(ctx_set_xmm0)

// Set flags via comparison
extern "C" __declspec(dllexport) void __cdecl ctx_set_flags(DWORD64 a, DWORD64 b)
{
    // This comparison will set flags
    volatile BOOL result = (a == b);  // ZF if equal
    (void)result;
    __nop();  // Breakpoint here to verify flags
}
EXPORT_SYMBOL(ctx_set_flags)

#endif // _WIN64

// ============================================================================
// Platform-Independent Functions
// ============================================================================

// Function to pause for context inspection
extern "C" __declspec(dllexport) void __cdecl ctx_pause_point()
{
    __nop();
    __nop();
    __nop();
    __nop();
    // Set breakpoint here for context inspection
    __nop();
    __nop();
    __nop();
    __nop();
}
EXPORT_SYMBOL(ctx_pause_point)

// Function that modifies stack
extern "C" __declspec(dllexport) DWORD __cdecl ctx_use_stack(DWORD depth)
{
    volatile DWORD local[16];

    for (DWORD i = 0; i < 16; i++)
    {
        local[i] = i * depth;
    }

    // Breakpoint here to inspect stack
    __nop();

    DWORD sum = 0;
    for (DWORD i = 0; i < 16; i++)
    {
        sum += local[i];
    }

    if (depth > 1)
    {
        sum += ctx_use_stack(depth - 1);
    }

    return sum;
}
EXPORT_SYMBOL(ctx_use_stack)

// Function for testing context modification by debugger
extern "C" __declspec(dllexport) DWORD __cdecl ctx_verify_modification()
{
    volatile DWORD value = 0x12345678;

    // Debugger should modify 'value' via context write
    __nop();  // Breakpoint 1: modify value here

    // Read it back
    DWORD read_back = value;

    __nop();  // Breakpoint 2: verify modification

    return read_back;
}
EXPORT_SYMBOL(ctx_verify_modification)

// Function with known return value for EAX/RAX testing
extern "C" __declspec(dllexport) DWORD __cdecl ctx_return_known_value()
{
    return 0xBAADF00D;  // Known value in EAX/RAX after return
}
EXPORT_SYMBOL(ctx_return_known_value)

// ============================================================================
// SSE/AVX Context Testing
// ============================================================================

// XMM register testing with SSE
extern "C" __declspec(dllexport) void __cdecl ctx_sse_test()
{
    __m128 a = _mm_set_ps(1.0f, 2.0f, 3.0f, 4.0f);
    __m128 b = _mm_set_ps(5.0f, 6.0f, 7.0f, 8.0f);
    volatile __m128 c = _mm_add_ps(a, b);
    (void)c;

    // Breakpoint here to verify XMM registers
    __nop();
}
EXPORT_SYMBOL(ctx_sse_test)

// Multiple XMM operations
extern "C" __declspec(dllexport) void __cdecl ctx_sse_multiple()
{
    __m128 xmm0 = _mm_set_ps(1.0f, 1.0f, 1.0f, 1.0f);
    __m128 xmm1 = _mm_set_ps(2.0f, 2.0f, 2.0f, 2.0f);
    __m128 xmm2 = _mm_set_ps(3.0f, 3.0f, 3.0f, 3.0f);
    __m128 xmm3 = _mm_set_ps(4.0f, 4.0f, 4.0f, 4.0f);

    xmm0 = _mm_add_ps(xmm0, xmm1);
    xmm2 = _mm_mul_ps(xmm2, xmm3);
    volatile __m128 result = _mm_sub_ps(xmm0, xmm2);
    (void)result;

    // Breakpoint here to verify XMM state
    __nop();
}
EXPORT_SYMBOL(ctx_sse_multiple)

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;

    // Set expected values for verification
    g_expected_eax = 0xDEADBEEF;
    g_expected_ebx = 0xCAFEBABE;
    g_expected_ecx = 0x12345678;
    g_expected_edx = 0xABCDEF00;

#ifdef _WIN64
    g_expected_rax = 0xDEADBEEFCAFEBABEull;
    g_expected_r8 = 0x88888888ull;
    g_expected_r9 = 0x99999999ull;
#endif

    // Call test functions
    ctx_pause_point();

#ifndef _WIN64
    ctx_set_eax();
    ctx_set_ebx();
    ctx_set_ecx();
    ctx_set_edx();
    ctx_set_all_gpr();
    ctx_set_cf();
    ctx_set_zf();
    ctx_set_sf();
    ctx_set_fpu();
#else
    volatile DWORD64 rax = ctx_set_rax();
    (void)rax;
    ctx_set_params(0x1111111111111111ull, 0x2222222222222222ull);
    ctx_set_r8_r9(0, 0, 0x8888888888888888ull, 0x9999999999999999ull);
    volatile double xmm_result = ctx_set_xmm0(3.14159);
    (void)xmm_result;
    ctx_set_flags(100, 100);  // Will set ZF=1
    ctx_set_flags(100, 200);  // Will clear ZF
#endif

    // Stack testing
    volatile DWORD stack_result = ctx_use_stack(3);
    (void)stack_result;

    // Verification function
    volatile DWORD modified = ctx_verify_modification();
    (void)modified;

    // Known return value
    volatile DWORD known = ctx_return_known_value();
    (void)known;

    // SSE testing
    ctx_sse_test();
    ctx_sse_multiple();

    return 0;
}
