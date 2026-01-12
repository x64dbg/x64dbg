// TestDll.cpp - Simple DLL for DLL load/unload event testing
// Provides exported functions and DllMain for testing

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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
    __declspec(dllexport) volatile DWORD g_dll_load_count = 0;
    __declspec(dllexport) volatile DWORD g_dll_unload_count = 0;
    __declspec(dllexport) volatile DWORD g_thread_attach_count = 0;
    __declspec(dllexport) volatile DWORD g_thread_detach_count = 0;
    __declspec(dllexport) volatile BOOL g_dll_initialized = FALSE;
    __declspec(dllexport) volatile HMODULE g_dll_module = NULL;
}

// ============================================================================
// Exported Functions
// ============================================================================

// Simple function that can be called after DLL load
extern "C" __declspec(dllexport) DWORD __cdecl dll_test_function(DWORD value)
{
    return value * 3;
}
EXPORT_SYMBOL(dll_test_function)

// Function that returns DLL state
extern "C" __declspec(dllexport) BOOL __cdecl dll_is_initialized()
{
    return g_dll_initialized;
}
EXPORT_SYMBOL(dll_is_initialized)

// Function with side effects
extern "C" __declspec(dllexport) DWORD __cdecl dll_increment_counter(volatile DWORD* counter)
{
    return InterlockedIncrement((LONG*)counter);
}
EXPORT_SYMBOL(dll_increment_counter)

// Function that returns the DLL's module handle
extern "C" __declspec(dllexport) HMODULE __cdecl dll_get_module()
{
    return g_dll_module;
}
EXPORT_SYMBOL(dll_get_module)

// Function with multiple parameters for testing
extern "C" __declspec(dllexport) DWORD __cdecl dll_compute(DWORD a, DWORD b, DWORD c)
{
    return (a + b) * c;
}
EXPORT_SYMBOL(dll_compute)

// Target for breakpoint testing in DLL
extern "C" __declspec(dllexport) volatile DWORD g_DllCounter = 0;

extern "C" __declspec(dllexport) DWORD __cdecl DllBpTarget(DWORD value)
{
    g_DllCounter++;
    return value + 10;
}
EXPORT_SYMBOL(DllBpTarget)

// Callback function type for testing
typedef void (CALLBACK *DLL_CALLBACK)(DWORD value);

// Function that invokes a callback
extern "C" __declspec(dllexport) void __cdecl dll_with_callback(DLL_CALLBACK callback, DWORD value)
{
    if (callback)
    {
        callback(value);
    }
}
EXPORT_SYMBOL(dll_with_callback)

// ============================================================================
// DllMain - Entry Point for DLL Events
// ============================================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    (void)lpReserved;

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        g_dll_module = hModule;
        g_dll_load_count++;
        g_dll_initialized = TRUE;
        // Disable thread notifications for performance
        // (Comment out to test thread attach/detach)
        // DisableThreadLibraryCalls(hModule);
        break;

    case DLL_PROCESS_DETACH:
        g_dll_unload_count++;
        g_dll_initialized = FALSE;
        g_dll_module = NULL;
        break;

    case DLL_THREAD_ATTACH:
        g_thread_attach_count++;
        break;

    case DLL_THREAD_DETACH:
        g_thread_detach_count++;
        break;
    }

    return TRUE;
}
