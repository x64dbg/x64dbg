// TestExe_Attach.cpp - Test target for attach/detach testing
// Long-running process that can be attached to by a debugger

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
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
    __declspec(dllexport) volatile DWORD g_tick_count = 0;
    __declspec(dllexport) volatile DWORD g_work_count = 0;
    __declspec(dllexport) volatile BOOL g_running = TRUE;
    __declspec(dllexport) volatile BOOL g_attached = FALSE;
    __declspec(dllexport) volatile DWORD g_process_id = 0;
    __declspec(dllexport) volatile HANDLE g_main_thread = NULL;
}

// Mutex for signaling
static HANDLE g_stop_event = NULL;

// ============================================================================
// Work Functions - Breakpoint Targets After Attach
// ============================================================================

// Simple periodic work function
extern "C" __declspec(dllexport) void __cdecl attach_periodic_work()
{
    g_work_count++;
    volatile DWORD dummy = g_work_count * 2;
    (void)dummy;
}
EXPORT_SYMBOL(attach_periodic_work)

// Function that can be called to verify attach
extern "C" __declspec(dllexport) DWORD __cdecl attach_verify()
{
    g_attached = TRUE;
    return g_tick_count;
}
EXPORT_SYMBOL(attach_verify)

// Compute something - good for stepping after attach
extern "C" __declspec(dllexport) DWORD __cdecl attach_compute(DWORD value)
{
    DWORD result = value;
    result *= 7;
    result += 13;
    result ^= 0xAAAAAAAA;
    return result;
}
EXPORT_SYMBOL(attach_compute)

// ============================================================================
// Main Loop Functions
// ============================================================================

// Tight loop function - run with high CPU
extern "C" __declspec(dllexport) void __cdecl attach_tight_loop(DWORD iterations)
{
    for (DWORD i = 0; i < iterations && g_running; i++)
    {
        g_tick_count++;
        attach_periodic_work();
    }
}
EXPORT_SYMBOL(attach_tight_loop)

// Idle loop - runs with Sleep
extern "C" __declspec(dllexport) void __cdecl attach_idle_loop(DWORD interval_ms)
{
    while (g_running)
    {
        g_tick_count++;
        attach_periodic_work();
        Sleep(interval_ms);
    }
}
EXPORT_SYMBOL(attach_idle_loop)

// Event-based loop
extern "C" __declspec(dllexport) void __cdecl attach_event_loop()
{
    while (g_running)
    {
        DWORD result = WaitForSingleObject(g_stop_event, 100);

        if (result == WAIT_OBJECT_0)
        {
            // Stop event signaled
            g_running = FALSE;
            break;
        }

        // Timeout - do periodic work
        g_tick_count++;
        attach_periodic_work();
    }
}
EXPORT_SYMBOL(attach_event_loop)

// ============================================================================
// Control Functions
// ============================================================================

// Signal the process to stop
extern "C" __declspec(dllexport) void __cdecl attach_stop()
{
    g_running = FALSE;
    if (g_stop_event)
    {
        SetEvent(g_stop_event);
    }
}
EXPORT_SYMBOL(attach_stop)

// Get process ID (for external attach)
extern "C" __declspec(dllexport) DWORD __cdecl attach_get_pid()
{
    return g_process_id;
}
EXPORT_SYMBOL(attach_get_pid)

// Get main thread handle
extern "C" __declspec(dllexport) HANDLE __cdecl attach_get_main_thread()
{
    return g_main_thread;
}
EXPORT_SYMBOL(attach_get_main_thread)

// ============================================================================
// Initialization
// ============================================================================

extern "C" __declspec(dllexport) void __cdecl attach_init()
{
    g_process_id = GetCurrentProcessId();
    g_main_thread = GetCurrentThread();
    g_stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    g_running = TRUE;
    g_attached = FALSE;
    g_tick_count = 0;
    g_work_count = 0;
}
EXPORT_SYMBOL(attach_init)

extern "C" __declspec(dllexport) void __cdecl attach_cleanup()
{
    if (g_stop_event)
    {
        CloseHandle(g_stop_event);
        g_stop_event = NULL;
    }
}
EXPORT_SYMBOL(attach_cleanup)

// ============================================================================
// Secondary Thread for Multi-Thread Attach Testing
// ============================================================================

static HANDLE g_worker_thread = NULL;

DWORD WINAPI attach_worker_thread(LPVOID param)
{
    DWORD interval = (DWORD)(ULONG_PTR)param;

    while (g_running)
    {
        attach_periodic_work();
        Sleep(interval);
    }

    return 0;
}

extern "C" __declspec(dllexport) HANDLE __cdecl attach_start_worker(DWORD interval_ms)
{
    if (g_worker_thread)
    {
        return g_worker_thread;  // Already running
    }

    g_worker_thread = CreateThread(
        NULL,
        0,
        attach_worker_thread,
        (LPVOID)(ULONG_PTR)interval_ms,
        0,
        NULL
    );

    return g_worker_thread;
}
EXPORT_SYMBOL(attach_start_worker)

extern "C" __declspec(dllexport) void __cdecl attach_stop_worker()
{
    if (g_worker_thread)
    {
        // Signal stop (will be caught by g_running check)
        WaitForSingleObject(g_worker_thread, 5000);
        CloseHandle(g_worker_thread);
        g_worker_thread = NULL;
    }
}
EXPORT_SYMBOL(attach_stop_worker)

// ============================================================================
// PID File Output (for external test harness)
// ============================================================================

static void write_pid_file(const char* filename)
{
    FILE* f;
    fopen_s(&f, filename, "w");
    if (f)
    {
        fprintf(f, "%u\n", g_process_id);
        fclose(f);
    }
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[])
{
    // Initialize
    attach_init();

    printf("TestExe_Attach starting, PID: %u\n", g_process_id);
    fflush(stdout);

    // Parse arguments
    DWORD interval = 100;  // Default 100ms
    BOOL use_worker = FALSE;
    const char* pid_file = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--interval") == 0 && i + 1 < argc)
        {
            interval = (DWORD)atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--worker") == 0)
        {
            use_worker = TRUE;
        }
        else if (strcmp(argv[i], "--pidfile") == 0 && i + 1 < argc)
        {
            pid_file = argv[++i];
        }
        else if (strcmp(argv[i], "--tight") == 0)
        {
            interval = 0;  // No sleep
        }
    }

    // Write PID file if requested
    if (pid_file)
    {
        write_pid_file(pid_file);
    }

    // Start worker thread if requested
    if (use_worker)
    {
        attach_start_worker(interval);
    }

    // Main loop
    if (interval == 0)
    {
        // Tight loop (no sleep)
        printf("Running tight loop (no sleep)...\n");
        fflush(stdout);
        while (g_running)
        {
            attach_tight_loop(1000000);
        }
    }
    else
    {
        // Idle loop with sleep
        printf("Running idle loop (interval: %u ms)...\n", interval);
        fflush(stdout);
        attach_idle_loop(interval);
    }

    // Cleanup
    if (use_worker)
    {
        attach_stop_worker();
    }
    attach_cleanup();

    printf("TestExe_Attach exiting, tick_count: %u, work_count: %u\n",
           g_tick_count, g_work_count);

    return 0;
}
