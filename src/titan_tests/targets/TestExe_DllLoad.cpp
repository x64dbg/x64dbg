// TestExe_DllLoad.cpp - Test target for DLL load/unload event testing
// Loads and unloads TestDll dynamically for TitanEngine testing

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Export macros for symbols
#ifdef _WIN64
#define EXPORT_SYMBOL(name) __pragma(comment(linker, "/EXPORT:" #name))
#define TEST_DLL_NAME "TestDll_x64.dll"
#else
#define EXPORT_SYMBOL(name) __pragma(comment(linker, "/EXPORT:_" #name))
#define TEST_DLL_NAME "TestDll_x32.dll"
#endif

// ============================================================================
// Global State
// ============================================================================

extern "C" {
    __declspec(dllexport) volatile HMODULE g_loaded_dll = NULL;
    __declspec(dllexport) volatile DWORD g_load_count = 0;
    __declspec(dllexport) volatile DWORD g_unload_count = 0;
}

// Function pointer types for TestDll exports
typedef DWORD(__cdecl *dll_test_function_t)(DWORD);
typedef BOOL(__cdecl *dll_is_initialized_t)();
typedef DWORD(__cdecl *dll_compute_t)(DWORD, DWORD, DWORD);

// ============================================================================
// DLL Loading Functions
// ============================================================================

// Load the test DLL
extern "C" __declspec(dllexport) HMODULE __cdecl load_test_dll()
{
    if (g_loaded_dll)
    {
        return g_loaded_dll;  // Already loaded
    }

    g_loaded_dll = LoadLibraryA(TEST_DLL_NAME);
    if (g_loaded_dll)
    {
        g_load_count++;
    }

    return g_loaded_dll;
}
EXPORT_SYMBOL(load_test_dll)

// Load DLL by path
extern "C" __declspec(dllexport) HMODULE __cdecl load_dll_by_path(const char* path)
{
    HMODULE module = LoadLibraryA(path);
    if (module)
    {
        g_load_count++;
    }
    return module;
}
EXPORT_SYMBOL(load_dll_by_path)

// Unload the test DLL
extern "C" __declspec(dllexport) BOOL __cdecl unload_test_dll()
{
    if (!g_loaded_dll)
    {
        return FALSE;  // Not loaded
    }

    BOOL result = FreeLibrary(g_loaded_dll);
    if (result)
    {
        g_loaded_dll = NULL;
        g_unload_count++;
    }

    return result;
}
EXPORT_SYMBOL(unload_test_dll)

// Unload DLL by handle
extern "C" __declspec(dllexport) BOOL __cdecl unload_dll_by_handle(HMODULE module)
{
    BOOL result = FreeLibrary(module);
    if (result)
    {
        g_unload_count++;
    }
    return result;
}
EXPORT_SYMBOL(unload_dll_by_handle)

// ============================================================================
// DLL Function Calling
// ============================================================================

// Call a function from the loaded DLL
extern "C" __declspec(dllexport) DWORD __cdecl call_dll_test_function(DWORD value)
{
    if (!g_loaded_dll)
    {
        return 0xFFFFFFFF;
    }

    dll_test_function_t func = (dll_test_function_t)GetProcAddress(g_loaded_dll, "dll_test_function");
    if (!func)
    {
        return 0xFFFFFFFE;
    }

    return func(value);
}
EXPORT_SYMBOL(call_dll_test_function)

// Check if DLL is initialized
extern "C" __declspec(dllexport) BOOL __cdecl check_dll_initialized()
{
    if (!g_loaded_dll)
    {
        return FALSE;
    }

    dll_is_initialized_t func = (dll_is_initialized_t)GetProcAddress(g_loaded_dll, "dll_is_initialized");
    if (!func)
    {
        return FALSE;
    }

    return func();
}
EXPORT_SYMBOL(check_dll_initialized)

// ============================================================================
// Load/Unload Cycle Testing
// ============================================================================

// Load and immediately unload
extern "C" __declspec(dllexport) BOOL __cdecl load_unload_cycle()
{
    HMODULE module = LoadLibraryA(TEST_DLL_NAME);
    if (!module)
    {
        return FALSE;
    }
    g_load_count++;

    BOOL result = FreeLibrary(module);
    if (result)
    {
        g_unload_count++;
    }

    return result;
}
EXPORT_SYMBOL(load_unload_cycle)

// Multiple load/unload cycles
extern "C" __declspec(dllexport) DWORD __cdecl load_unload_multiple(DWORD count)
{
    DWORD success_count = 0;

    for (DWORD i = 0; i < count; i++)
    {
        if (load_unload_cycle())
        {
            success_count++;
        }
    }

    return success_count;
}
EXPORT_SYMBOL(load_unload_multiple)

// ============================================================================
// Multiple DLL Loading
// ============================================================================

static HMODULE g_system_dlls[10] = {0};
static DWORD g_system_dll_count = 0;

// Load multiple system DLLs
extern "C" __declspec(dllexport) DWORD __cdecl load_system_dlls()
{
    const char* dlls[] = {
        "user32.dll",
        "gdi32.dll",
        "advapi32.dll",
        "shell32.dll",
        "ole32.dll"
    };

    g_system_dll_count = 0;

    for (int i = 0; i < 5; i++)
    {
        g_system_dlls[i] = LoadLibraryA(dlls[i]);
        if (g_system_dlls[i])
        {
            g_system_dll_count++;
        }
    }

    return g_system_dll_count;
}
EXPORT_SYMBOL(load_system_dlls)

// Unload system DLLs
extern "C" __declspec(dllexport) DWORD __cdecl unload_system_dlls()
{
    DWORD unloaded = 0;

    for (DWORD i = 0; i < g_system_dll_count; i++)
    {
        if (g_system_dlls[i])
        {
            if (FreeLibrary(g_system_dlls[i]))
            {
                unloaded++;
            }
            g_system_dlls[i] = NULL;
        }
    }

    g_system_dll_count = 0;
    return unloaded;
}
EXPORT_SYMBOL(unload_system_dlls)

// ============================================================================
// Delayed DLL Loading
// ============================================================================

// Load DLL with delay (for testing load events during execution)
extern "C" __declspec(dllexport) HMODULE __cdecl load_dll_delayed(DWORD delay_ms)
{
    Sleep(delay_ms);
    return load_test_dll();
}
EXPORT_SYMBOL(load_dll_delayed)

// ============================================================================
// Entry Point Breakpoint Target
// ============================================================================

// This function is called before any DLL operations - good for initial breakpoint
extern "C" __declspec(dllexport) void __cdecl before_dll_operations()
{
    // Empty function - breakpoint target
    volatile DWORD dummy = 0;
    (void)dummy;
}
EXPORT_SYMBOL(before_dll_operations)

// This function is called after all DLL operations
extern "C" __declspec(dllexport) void __cdecl after_dll_operations()
{
    // Empty function - breakpoint target
    volatile DWORD dummy = 0;
    (void)dummy;
}
EXPORT_SYMBOL(after_dll_operations)

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[])
{
    before_dll_operations();

    // Default behavior: load, call function, unload
    HMODULE module = load_test_dll();
    if (module)
    {
        DWORD result = call_dll_test_function(42);
        (void)result;

        BOOL initialized = check_dll_initialized();
        (void)initialized;

        unload_test_dll();
    }

    // If --loop argument, do continuous load/unload
    if (argc > 1 && strcmp(argv[1], "--loop") == 0)
    {
        int iterations = 10;
        if (argc > 2)
        {
            iterations = atoi(argv[2]);
        }

        for (int i = 0; i < iterations; i++)
        {
            load_unload_cycle();
            Sleep(100);
        }
    }

    // If --system argument, load system DLLs
    if (argc > 1 && strcmp(argv[1], "--system") == 0)
    {
        load_system_dlls();
        Sleep(1000);
        unload_system_dlls();
    }

    after_dll_operations();

    return 0;
}
