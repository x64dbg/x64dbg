// TestExe_Threading.cpp - Test target for multi-threaded debugging scenarios
// Creates multiple threads with synchronization primitives for controlled testing

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Export macros for symbols
#ifdef _WIN64
#define EXPORT_SYMBOL(name) __pragma(comment(linker, "/EXPORT:" #name))
#else
#define EXPORT_SYMBOL(name) __pragma(comment(linker, "/EXPORT:_" #name))
#endif

// ============================================================================
// Global State for Thread Coordination
// ============================================================================

extern "C" {
    __declspec(dllexport) volatile LONG g_thread_counter = 0;
    __declspec(dllexport) volatile LONG g_threads_ready = 0;
    __declspec(dllexport) volatile LONG g_threads_running = 0;
    __declspec(dllexport) volatile BOOL g_stop_threads = FALSE;
}

// Synchronization primitives
static HANDLE g_start_event = NULL;      // Signaled when all threads should start
static HANDLE g_thread_ready_event = NULL; // Signaled when a thread is ready
static CRITICAL_SECTION g_cs;

// ============================================================================
// Thread Target Functions - Breakpoints can be set on these
// ============================================================================

// Simple function executed by threads - good breakpoint target
extern "C" __declspec(dllexport) DWORD __cdecl thread_bp_target(DWORD thread_id)
{
    InterlockedIncrement(&g_thread_counter);
    return thread_id * 2;
}
EXPORT_SYMBOL(thread_bp_target)

// Function with critical section - tests breakpoints in synchronized code
extern "C" __declspec(dllexport) void __cdecl thread_critical_section_work(DWORD value)
{
    EnterCriticalSection(&g_cs);
    volatile DWORD local = g_thread_counter;
    local += value;
    Sleep(1); // Small delay to allow thread interleaving
    g_thread_counter = local;
    LeaveCriticalSection(&g_cs);
}
EXPORT_SYMBOL(thread_critical_section_work)

// Function that threads execute in a loop
extern "C" __declspec(dllexport) void __cdecl thread_loop_work()
{
    while (!g_stop_threads)
    {
        thread_bp_target(GetCurrentThreadId());
        Sleep(50);
    }
}
EXPORT_SYMBOL(thread_loop_work)

// ============================================================================
// Thread Entry Points
// ============================================================================

// Worker thread that waits for start signal
extern "C" __declspec(dllexport) DWORD WINAPI thread_worker(LPVOID param)
{
    DWORD thread_num = (DWORD)(ULONG_PTR)param;

    // Signal that this thread is ready
    InterlockedIncrement(&g_threads_ready);

    // Wait for start signal
    WaitForSingleObject(g_start_event, INFINITE);

    // Mark as running
    InterlockedIncrement(&g_threads_running);

    // Do work - this is a good breakpoint location
    DWORD result = thread_bp_target(thread_num);

    // Do some synchronized work
    thread_critical_section_work(thread_num);

    // Mark as no longer running
    InterlockedDecrement(&g_threads_running);

    return result;
}
EXPORT_SYMBOL(thread_worker)

// Long-running worker thread
extern "C" __declspec(dllexport) DWORD WINAPI thread_long_worker(LPVOID param)
{
    DWORD thread_num = (DWORD)(ULONG_PTR)param;

    // Signal ready
    InterlockedIncrement(&g_threads_ready);

    // Wait for start
    WaitForSingleObject(g_start_event, INFINITE);

    // Mark as running
    InterlockedIncrement(&g_threads_running);

    // Loop until told to stop
    thread_loop_work();

    // Mark as no longer running
    InterlockedDecrement(&g_threads_running);

    return thread_num;
}
EXPORT_SYMBOL(thread_long_worker)

// ============================================================================
// Thread Management Functions
// ============================================================================

// Create multiple worker threads
extern "C" __declspec(dllexport) void __cdecl create_worker_threads(DWORD count, HANDLE* handles)
{
    for (DWORD i = 0; i < count; i++)
    {
        handles[i] = CreateThread(
            NULL,
            0,
            thread_worker,
            (LPVOID)(ULONG_PTR)(i + 1),
            0,
            NULL
        );
    }
}
EXPORT_SYMBOL(create_worker_threads)

// Signal all threads to start
extern "C" __declspec(dllexport) void __cdecl start_all_threads()
{
    SetEvent(g_start_event);
}
EXPORT_SYMBOL(start_all_threads)

// Wait for all threads to complete
extern "C" __declspec(dllexport) void __cdecl wait_for_threads(HANDLE* handles, DWORD count)
{
    WaitForMultipleObjects(count, handles, TRUE, INFINITE);
    for (DWORD i = 0; i < count; i++)
    {
        CloseHandle(handles[i]);
    }
}
EXPORT_SYMBOL(wait_for_threads)

// Stop all long-running threads
extern "C" __declspec(dllexport) void __cdecl stop_threads()
{
    g_stop_threads = TRUE;
}
EXPORT_SYMBOL(stop_threads)

// ============================================================================
// Initialization
// ============================================================================

extern "C" __declspec(dllexport) void __cdecl init_threading()
{
    InitializeCriticalSection(&g_cs);
    g_start_event = CreateEventW(NULL, TRUE, FALSE, NULL);  // Manual reset
    g_thread_ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);  // Auto reset
    g_thread_counter = 0;
    g_threads_ready = 0;
    g_threads_running = 0;
    g_stop_threads = FALSE;
}
EXPORT_SYMBOL(init_threading)

extern "C" __declspec(dllexport) void __cdecl cleanup_threading()
{
    DeleteCriticalSection(&g_cs);
    if (g_start_event) CloseHandle(g_start_event);
    if (g_thread_ready_event) CloseHandle(g_thread_ready_event);
}
EXPORT_SYMBOL(cleanup_threading)

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[])
{
    const DWORD NUM_THREADS = 4;
    HANDLE threads[NUM_THREADS];

    // Initialize
    init_threading();

    // Create worker threads
    create_worker_threads(NUM_THREADS, threads);

    // Wait for all threads to be ready
    while (g_threads_ready < (LONG)NUM_THREADS)
    {
        Sleep(10);
    }

    // Start all threads
    start_all_threads();

    // Wait for all threads to complete
    wait_for_threads(threads, NUM_THREADS);

    // Verify counter
    DWORD expected = 0;
    for (DWORD i = 1; i <= NUM_THREADS; i++)
    {
        expected += i;  // Each thread adds its number
    }

    // Counter should be NUM_THREADS (from thread_bp_target) + sum of 1..NUM_THREADS (from critical section)

    // Cleanup
    cleanup_threading();

    // If --loop argument, create long-running threads
    if (argc > 1 && strcmp(argv[1], "--loop") == 0)
    {
        init_threading();

        for (DWORD i = 0; i < NUM_THREADS; i++)
        {
            threads[i] = CreateThread(
                NULL,
                0,
                thread_long_worker,
                (LPVOID)(ULONG_PTR)(i + 1),
                0,
                NULL
            );
        }

        // Wait for all to be ready
        while (g_threads_ready < (LONG)NUM_THREADS)
        {
            Sleep(10);
        }

        // Start all
        start_all_threads();

        // Wait forever (or until Ctrl+C)
        while (true)
        {
            Sleep(1000);
        }
    }

    return 0;
}
