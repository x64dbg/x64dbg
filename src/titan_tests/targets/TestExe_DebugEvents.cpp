// TestExe_DebugEvents.cpp - Test target for debug event tests
// Creates threads, loads/unloads DLLs, outputs debug strings for TitanEngine testing

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
    __declspec(dllexport) volatile LONG g_thread_created_count = 0;
    __declspec(dllexport) volatile LONG g_thread_exited_count = 0;
    __declspec(dllexport) volatile LONG g_dll_loaded_count = 0;
    __declspec(dllexport) volatile LONG g_dll_unloaded_count = 0;
    __declspec(dllexport) volatile LONG g_debug_string_count = 0;
}

// ============================================================================
// Output Debug String Functions
// ============================================================================

// Output a simple debug string
extern "C" __declspec(dllexport) void __cdecl output_debug_string(const char* message)
{
    OutputDebugStringA(message);
    InterlockedIncrement(&g_debug_string_count);
}
EXPORT_SYMBOL(output_debug_string)

// Output a unicode debug string
extern "C" __declspec(dllexport) void __cdecl output_debug_string_w(const wchar_t* message)
{
    OutputDebugStringW(message);
    InterlockedIncrement(&g_debug_string_count);
}
EXPORT_SYMBOL(output_debug_string_w)

// Output a known test debug string
extern "C" __declspec(dllexport) void __cdecl output_test_debug_string()
{
    OutputDebugStringA("TITAN_TEST_DEBUG_STRING");
    InterlockedIncrement(&g_debug_string_count);
}
EXPORT_SYMBOL(output_test_debug_string)

// ============================================================================
// Thread Creation/Exit Functions
// ============================================================================

// Simple thread entry point
static DWORD WINAPI simple_thread_func(LPVOID param)
{
    DWORD duration = (DWORD)(ULONG_PTR)param;
    if (duration > 0)
    {
        Sleep(duration);
    }
    return 42;
}

// Create a thread that exits immediately
extern "C" __declspec(dllexport) HANDLE __cdecl create_short_thread()
{
    HANDLE hThread = CreateThread(NULL, 0, simple_thread_func, (LPVOID)0, 0, NULL);
    if (hThread)
    {
        InterlockedIncrement(&g_thread_created_count);
    }
    return hThread;
}
EXPORT_SYMBOL(create_short_thread)

// Create a thread that runs for a specified duration
extern "C" __declspec(dllexport) HANDLE __cdecl create_timed_thread(DWORD durationMs)
{
    HANDLE hThread = CreateThread(NULL, 0, simple_thread_func, (LPVOID)(ULONG_PTR)durationMs, 0, NULL);
    if (hThread)
    {
        InterlockedIncrement(&g_thread_created_count);
    }
    return hThread;
}
EXPORT_SYMBOL(create_timed_thread)

// Wait for a thread to exit
extern "C" __declspec(dllexport) DWORD __cdecl wait_thread(HANDLE hThread)
{
    DWORD exitCode = 0;
    WaitForSingleObject(hThread, INFINITE);
    GetExitCodeThread(hThread, &exitCode);
    CloseHandle(hThread);
    InterlockedIncrement(&g_thread_exited_count);
    return exitCode;
}
EXPORT_SYMBOL(wait_thread)

// Create and wait for a thread (triggers both CREATE_THREAD and EXIT_THREAD)
extern "C" __declspec(dllexport) void __cdecl create_and_wait_thread()
{
    HANDLE hThread = create_short_thread();
    if (hThread)
    {
        wait_thread(hThread);
    }
}
EXPORT_SYMBOL(create_and_wait_thread)

// ============================================================================
// DLL Load/Unload Functions
// ============================================================================

// Load kernel32.dll (always present, safe to load)
extern "C" __declspec(dllexport) HMODULE __cdecl load_system_dll()
{
    // Load a system DLL that we know exists
    HMODULE hMod = LoadLibraryW(L"version.dll");
    if (hMod)
    {
        InterlockedIncrement(&g_dll_loaded_count);
    }
    return hMod;
}
EXPORT_SYMBOL(load_system_dll)

// Unload a DLL
extern "C" __declspec(dllexport) BOOL __cdecl unload_dll(HMODULE hMod)
{
    BOOL result = FreeLibrary(hMod);
    if (result)
    {
        InterlockedIncrement(&g_dll_unloaded_count);
    }
    return result;
}
EXPORT_SYMBOL(unload_dll)

// Load and unload a DLL (triggers both LOAD_DLL and UNLOAD_DLL events)
extern "C" __declspec(dllexport) void __cdecl load_and_unload_dll()
{
    HMODULE hMod = load_system_dll();
    if (hMod)
    {
        unload_dll(hMod);
    }
}
EXPORT_SYMBOL(load_and_unload_dll)

// Load a specific DLL by name
extern "C" __declspec(dllexport) HMODULE __cdecl load_dll_by_name(const wchar_t* name)
{
    HMODULE hMod = LoadLibraryW(name);
    if (hMod)
    {
        InterlockedIncrement(&g_dll_loaded_count);
    }
    return hMod;
}
EXPORT_SYMBOL(load_dll_by_name)

// ============================================================================
// Combined Test Functions
// ============================================================================

// Run all debug event tests in sequence
extern "C" __declspec(dllexport) void __cdecl run_all_debug_events()
{
    // Output debug strings
    output_test_debug_string();
    output_debug_string("Test message 1");
    output_debug_string("Test message 2");

    // Create and exit threads
    create_and_wait_thread();
    create_and_wait_thread();

    // Load and unload DLLs
    load_and_unload_dll();
}
EXPORT_SYMBOL(run_all_debug_events)

// ============================================================================
// Entry Point Marker (for system breakpoint testing)
// ============================================================================

extern "C" __declspec(dllexport) void __cdecl entry_point_marker()
{
    // This function can be used to verify we hit the entry point
    volatile int x = 1;
    (void)x;
}
EXPORT_SYMBOL(entry_point_marker)

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[])
{
    // Mark entry point
    entry_point_marker();

    // Default behavior: run all debug events
    if (argc == 1)
    {
        run_all_debug_events();
        return 0;
    }

    // Parse command line for specific tests
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--debug-string") == 0)
        {
            output_test_debug_string();
        }
        else if (strcmp(argv[i], "--thread") == 0)
        {
            create_and_wait_thread();
        }
        else if (strcmp(argv[i], "--dll") == 0)
        {
            load_and_unload_dll();
        }
        else if (strcmp(argv[i], "--all") == 0)
        {
            run_all_debug_events();
        }
        else if (strcmp(argv[i], "--loop") == 0)
        {
            // Run in a loop for attach testing
            while (true)
            {
                output_debug_string("Loop iteration");
                Sleep(1000);
            }
        }
    }

    return (int)(g_thread_created_count + g_dll_loaded_count + g_debug_string_count);
}
