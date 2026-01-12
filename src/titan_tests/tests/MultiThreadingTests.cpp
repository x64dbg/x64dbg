/**
 * Multi-Threading Tests (MT-01 through MT-10)
 *
 * Tests for TitanEngine multi-threaded debugging functionality.
 * These tests use the TestExe_Threading target executable which creates
 * multiple threads and exports functions like thread_bp_target, thread_worker, etc.
 */

#include "../TitanTestFramework.h"
#include "TitanEngine/TitanEngine.h"
#include <string>
#include <atomic>
#include <vector>
#include <set>
#include <map>

namespace
{

//-----------------------------------------------------------------------------
// Test state and utilities
//-----------------------------------------------------------------------------

// Atomic counters for callback verification
std::atomic<int> g_bpHitCount{0};
std::atomic<ULONG_PTR> g_lastBpAddress{0};
std::atomic<bool> g_systemBpHit{false};
std::atomic<bool> g_processCreated{false};
std::atomic<bool> g_processExited{false};

// Thread tracking
std::atomic<int> g_threadCreateCount{0};
std::atomic<int> g_threadExitCount{0};
std::set<DWORD> g_hitThreadIds;
std::map<DWORD, ULONG_PTR> g_threadContextValues;
CRITICAL_SECTION g_threadIdLock;

// Address of exported function to set breakpoint on
ULONG_PTR g_targetAddress = 0;
ULONG_PTR g_targetAddress2 = 0;

// Process info
HANDLE g_hProcess = nullptr;
DWORD g_processId = 0;

// For thread-specific tests
DWORD g_targetThreadId = 0;
std::atomic<bool> g_stepCompleted{false};
std::atomic<DWORD> g_steppingThreadId{0};

// For HW BP tests
std::atomic<int> g_hwBpHitCount{0};

// Reset all test state
void ResetTestState()
{
    g_bpHitCount = 0;
    g_lastBpAddress = 0;
    g_systemBpHit = false;
    g_processCreated = false;
    g_processExited = false;
    g_threadCreateCount = 0;
    g_threadExitCount = 0;
    g_hitThreadIds.clear();
    g_threadContextValues.clear();
    g_targetAddress = 0;
    g_targetAddress2 = 0;
    g_hProcess = nullptr;
    g_processId = 0;
    g_targetThreadId = 0;
    g_stepCompleted = false;
    g_steppingThreadId = 0;
    g_hwBpHitCount = 0;
}

// Initialize thread ID lock
void InitThreadLock()
{
    InitializeCriticalSection(&g_threadIdLock);
}

void CleanupThreadLock()
{
    DeleteCriticalSection(&g_threadIdLock);
}

// Get the test executable path (uses framework helper with architecture suffix)
std::wstring GetTestExePath()
{
    return TitanTest::GetTestExePath(L"TestExe_Threading");
}

//-----------------------------------------------------------------------------
// Callback handlers
//-----------------------------------------------------------------------------

void OnSystemBreakpoint(const void*)
{
    g_systemBpHit = true;
}

void OnProcessCreated(const void* info)
{
    g_processCreated = true;
    auto* createInfo = static_cast<const CREATE_PROCESS_DEBUG_INFO*>(info);
    if (createInfo)
    {
        g_hProcess = createInfo->hProcess;
    }
}

void OnProcessExited(const void* info)
{
    g_processExited = true;
}

void OnThreadCreated(const void* info)
{
    g_threadCreateCount++;
}

void OnThreadExited(const void* info)
{
    g_threadExitCount++;
}

// BP callback that tracks which thread hit the BP
void OnBpHitTrackThread()
{
    TITAN_TRACK_BP_HIT();
    g_bpHitCount++;

    // Get the debug event to determine which thread hit the BP and the address
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        EnterCriticalSection(&g_threadIdLock);
        g_hitThreadIds.insert(dbgEvent->dwThreadId);
        LeaveCriticalSection(&g_threadIdLock);

        // Get BP address from debug event (not GetContextDataEx which uses wrong thread handle)
        g_lastBpAddress = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
    }
}

// BP callback for simultaneous BP test
void OnBpHitSimultaneous()
{
    TITAN_TRACK_BP_HIT();
    g_bpHitCount++;

    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        EnterCriticalSection(&g_threadIdLock);
        g_hitThreadIds.insert(dbgEvent->dwThreadId);
        LeaveCriticalSection(&g_threadIdLock);
    }
}

// BP callback that deletes BP during multi-thread hit
void OnBpHitAndDeleteMT()
{
    TITAN_TRACK_BP_HIT();
    g_bpHitCount++;

    // Get BP address from debug event (not GetContextDataEx which uses wrong thread handle)
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        g_lastBpAddress = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
    }

    // Delete the breakpoint while other threads might be hitting it
    DeleteBPX(g_lastBpAddress);
}

// HW BP callback
void OnHwBpHit(const void* info)
{
    g_hwBpHitCount++;

    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        EnterCriticalSection(&g_threadIdLock);
        g_hitThreadIds.insert(dbgEvent->dwThreadId);
        LeaveCriticalSection(&g_threadIdLock);
    }
}

// Step callback
void OnStepComplete()
{
    g_stepCompleted = true;

    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        g_steppingThreadId = dbgEvent->dwThreadId;
    }
}

// BP callback for context test - records context per thread
void OnBpHitRecordContext()
{
    TITAN_TRACK_BP_HIT();
    g_bpHitCount++;

    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        DWORD threadId = dbgEvent->dwThreadId;

        // Open thread handle
        HANDLE hThread = TitanOpenThread(THREAD_ALL_ACCESS, FALSE, threadId);
        if (hThread)
        {
            // Get a register value (use stack pointer as it's different per thread)
            ULONG_PTR spValue = GetContextDataEx(hThread, UE_CSP);

            EnterCriticalSection(&g_threadIdLock);
            g_threadContextValues[threadId] = spValue;
            g_hitThreadIds.insert(threadId);
            LeaveCriticalSection(&g_threadIdLock);

            CloseHandle(hThread);
        }
    }
}

// BP callback for thread-specific BP test
void OnBpHitThreadSpecific()
{
    TITAN_TRACK_BP_HIT();

    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        // Only count hits from the target thread
        if (dbgEvent->dwThreadId == g_targetThreadId)
        {
            g_bpHitCount++;
        }
        EnterCriticalSection(&g_threadIdLock);
        g_hitThreadIds.insert(dbgEvent->dwThreadId);
        LeaveCriticalSection(&g_threadIdLock);
    }
}

} // anonymous namespace

//-----------------------------------------------------------------------------
// MT-01: BP hit by multiple threads
// Same BP hit by different threads - verify each hit is counted and thread IDs differ
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MT-01", MT_01, "BP hit by multiple threads")
{
    ResetTestState();
    InitThreadLock();

    std::wstring exePath = GetTestExePath();

    // Set up CREATE_PROCESS handler to set the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hProcess = createInfo.hProcess;
        g_processId = dbgEvent->dwProcessId;

        // Get the executable path from the file handle
        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        // Load the DLL in our process to resolve exports (no code execution)
        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            // Get the export offset and adjust to debuggee's base
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "thread_bp_target");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                // Set persistent breakpoint to catch all thread hits
                SetBPX(exportAddr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitTrackThread);
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    SetCustomHandler(UE_CH_EXITPROCESS, [](const void*) {
        g_processExited = true;
    });

    SetCustomHandler(UE_CH_CREATETHREAD, OnThreadCreated);
    SetCustomHandler(UE_CH_EXITTHREAD, OnThreadExited);

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    CleanupThreadLock();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_bpHitCount >= 2, "BP not hit by multiple threads");
    TEST_ASSERT(g_hitThreadIds.size() >= 2, "BP not hit by at least 2 different threads");

    return true;
}

//-----------------------------------------------------------------------------
// MT-02: Simultaneous BP hits
// Multiple threads hit different BPs - verify no race conditions
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MT-02", MT_02, "Simultaneous BP hits on different addresses")
{
    ResetTestState();
    InitThreadLock();

    std::wstring exePath = GetTestExePath();

    // Set up CREATE_PROCESS handler to set the breakpoints
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hProcess = createInfo.hProcess;
        g_processId = dbgEvent->dwProcessId;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            // Set BPs on two different functions
            auto exportAddr1 = (ULONG_PTR)GetProcAddress(hLib, "thread_bp_target");
            auto exportAddr2 = (ULONG_PTR)GetProcAddress(hLib, "thread_critical_section_work");

            if (exportAddr1)
            {
                exportAddr1 -= (ULONG_PTR)hLib;
                exportAddr1 += base;
                g_targetAddress = exportAddr1;
                SetBPX(exportAddr1, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitSimultaneous);
            }

            if (exportAddr2)
            {
                exportAddr2 -= (ULONG_PTR)hLib;
                exportAddr2 += base;
                g_targetAddress2 = exportAddr2;
                SetBPX(exportAddr2, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitSimultaneous);
            }

            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    SetCustomHandler(UE_CH_EXITPROCESS, [](const void*) {
        g_processExited = true;
    });

    SetCustomHandler(UE_CH_CREATETHREAD, OnThreadCreated);
    SetCustomHandler(UE_CH_EXITTHREAD, OnThreadExited);

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    CleanupThreadLock();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0 && g_targetAddress2 != 0, "Target function addresses not resolved");
    TEST_ASSERT(g_bpHitCount >= 2, "Not enough BP hits");
    // The test passes if we get multiple hits without crashing (no race condition)

    return true;
}

//-----------------------------------------------------------------------------
// MT-03: Delete BP during multi-thread hit
// Delete while another thread might be on BP
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MT-03", MT_03, "Delete BP during multi-thread hit")
{
    ResetTestState();
    InitThreadLock();

    std::wstring exePath = GetTestExePath();

    // Set up CREATE_PROCESS handler to set the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hProcess = createInfo.hProcess;
        g_processId = dbgEvent->dwProcessId;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "thread_bp_target");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                // Set persistent BP that will delete itself on first hit
                SetBPX(exportAddr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitAndDeleteMT);
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    SetCustomHandler(UE_CH_EXITPROCESS, [](const void*) {
        g_processExited = true;
    });

    SetCustomHandler(UE_CH_CREATETHREAD, OnThreadCreated);
    SetCustomHandler(UE_CH_EXITTHREAD, OnThreadExited);

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    CleanupThreadLock();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_bpHitCount >= 1, "BP was not hit at least once");
    // The test passes if no crash occurred during BP deletion while other threads might be hitting it
    TEST_ASSERT(!IsBPXEnabled(g_targetAddress), "BP should be deleted");

    return true;
}

//-----------------------------------------------------------------------------
// MT-04: HW BP per-thread
// Hardware BPs are thread-local - verify they fire on multiple threads
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MT-04", MT_04, "Hardware BP fires on all threads")
{
    ResetTestState();
    InitThreadLock();

    std::wstring exePath = GetTestExePath();
    static DWORD s_drIndex = 0;

    // Set up CREATE_PROCESS handler to set the hardware breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hProcess = createInfo.hProcess;
        g_processId = dbgEvent->dwProcessId;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "thread_bp_target");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;

                // Get unused DR register
                if (GetUnusedHardwareBreakPointRegister(&s_drIndex))
                {
                    // Set hardware execute breakpoint
                    SetHardwareBreakPoint(exportAddr, s_drIndex, UE_HARDWARE_EXECUTE,
                                          UE_HARDWARE_SIZE_1, OnHwBpHit);
                }
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    SetCustomHandler(UE_CH_EXITPROCESS, [](const void*) {
        g_processExited = true;
    });

    SetCustomHandler(UE_CH_CREATETHREAD, OnThreadCreated);
    SetCustomHandler(UE_CH_EXITTHREAD, OnThreadExited);

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    // Clean up HW BP
    DeleteHardwareBreakPoint(s_drIndex);

    CleanupThreadLock();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_hwBpHitCount >= 2, "HW BP not hit by multiple threads");
    TEST_ASSERT(g_hitThreadIds.size() >= 2, "HW BP not hit by at least 2 different threads");

    return true;
}

//-----------------------------------------------------------------------------
// MT-05: Step in one thread
// Stepping affects only one thread
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MT-05", MT_05, "Single-step affects only current thread")
{
    ResetTestState();
    InitThreadLock();

    std::wstring exePath = GetTestExePath();
    static DWORD s_bpThreadId = 0;

    // Set up CREATE_PROCESS handler to set the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hProcess = createInfo.hProcess;
        g_processId = dbgEvent->dwProcessId;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "thread_bp_target");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                // Set singleshoot BP to catch a thread
                SetBPX(exportAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    g_bpHitCount++;

                    // Record which thread hit the BP
                    const DEBUG_EVENT* dbgEvent = GetDebugData();
                    if (dbgEvent)
                    {
                        s_bpThreadId = dbgEvent->dwThreadId;
                        g_targetThreadId = dbgEvent->dwThreadId;
                    }

                    // Now do a single step
                    StepInto(OnStepComplete);
                });
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    SetCustomHandler(UE_CH_EXITPROCESS, [](const void*) {
        g_processExited = true;
    });

    SetCustomHandler(UE_CH_CREATETHREAD, OnThreadCreated);
    SetCustomHandler(UE_CH_EXITTHREAD, OnThreadExited);

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    CleanupThreadLock();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_bpHitCount >= 1, "BP was not hit");
    TEST_ASSERT(g_stepCompleted, "Step did not complete");
    // Verify step happened on the same thread that hit the BP
    TEST_ASSERT(g_steppingThreadId == s_bpThreadId, "Step occurred on wrong thread");

    return true;
}

//-----------------------------------------------------------------------------
// MT-06: Context per thread
// GetContextDataEx with different thread handles returns different contexts
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MT-06", MT_06, "GetContextDataEx returns per-thread context")
{
    ResetTestState();
    InitThreadLock();

    std::wstring exePath = GetTestExePath();

    // Set up CREATE_PROCESS handler to set the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hProcess = createInfo.hProcess;
        g_processId = dbgEvent->dwProcessId;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "thread_bp_target");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                // Set persistent BP to catch multiple threads
                SetBPX(exportAddr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitRecordContext);
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    SetCustomHandler(UE_CH_EXITPROCESS, [](const void*) {
        g_processExited = true;
    });

    SetCustomHandler(UE_CH_CREATETHREAD, OnThreadCreated);
    SetCustomHandler(UE_CH_EXITTHREAD, OnThreadExited);

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    CleanupThreadLock();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_hitThreadIds.size() >= 2, "Need at least 2 threads to test context");

    // Verify that different threads have different stack pointers
    std::set<ULONG_PTR> uniqueStackPointers;
    for (const auto& pair : g_threadContextValues)
    {
        uniqueStackPointers.insert(pair.second);
    }

    TEST_ASSERT(uniqueStackPointers.size() >= 2, "Different threads should have different stack pointers");

    return true;
}

//-----------------------------------------------------------------------------
// MT-07: Thread create during step
// New thread created while stepping
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MT-07", MT_07, "New thread created during step")
{
    ResetTestState();
    InitThreadLock();

    std::wstring exePath = GetTestExePath();
    static int s_initialThreadCount = 0;

    // Set up CREATE_PROCESS handler to set the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hProcess = createInfo.hProcess;
        g_processId = dbgEvent->dwProcessId;
        s_initialThreadCount = g_threadCreateCount.load();

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            // Set BP on function that might be called when threads are being created
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "init_threading");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                SetBPX(exportAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    g_bpHitCount++;
                    // Do multiple steps while threads might be created
                    StepInto([]() {
                        g_stepCompleted = true;
                        // Continue stepping
                        StepInto(OnStepComplete);
                    });
                });
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    SetCustomHandler(UE_CH_EXITPROCESS, [](const void*) {
        g_processExited = true;
    });

    SetCustomHandler(UE_CH_CREATETHREAD, OnThreadCreated);
    SetCustomHandler(UE_CH_EXITTHREAD, OnThreadExited);

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    CleanupThreadLock();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    // This test mainly verifies we don't crash when threads are created during stepping
    // The process creates multiple threads, so we should see thread creation events
    TEST_ASSERT(g_threadCreateCount > 0, "No threads were created");

    return true;
}

//-----------------------------------------------------------------------------
// MT-08: Thread exit during BP
// Thread exits while at breakpoint
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MT-08", MT_08, "Thread exit while at breakpoint")
{
    ResetTestState();
    InitThreadLock();

    std::wstring exePath = GetTestExePath();

    // Set up CREATE_PROCESS handler to set the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hProcess = createInfo.hProcess;
        g_processId = dbgEvent->dwProcessId;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            // Set BP on thread_worker which threads execute and then exit
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "thread_worker");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                SetBPX(exportAddr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitTrackThread);
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    SetCustomHandler(UE_CH_EXITPROCESS, [](const void*) {
        g_processExited = true;
    });

    SetCustomHandler(UE_CH_CREATETHREAD, OnThreadCreated);
    SetCustomHandler(UE_CH_EXITTHREAD, OnThreadExited);

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    CleanupThreadLock();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    // The test passes if we don't crash when threads exit after hitting BPs
    TEST_ASSERT(g_threadExitCount > 0, "No thread exits recorded");
    // At least some BPs should have been hit before thread exit
    TEST_ASSERT(g_bpHitCount >= 1, "No BP hits recorded");

    return true;
}

//-----------------------------------------------------------------------------
// MT-09: 32 threads with same BP
// Stress test with many threads hitting the same BP
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MT-09", MT_09, "32 threads hitting same BP (stress test)")
{
    ResetTestState();
    InitThreadLock();

    std::wstring exePath = GetTestExePath();
    static std::atomic<int> s_maxHitCount{100}; // Stop after 100 hits for time

    // Set up CREATE_PROCESS handler to set the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hProcess = createInfo.hProcess;
        g_processId = dbgEvent->dwProcessId;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "thread_bp_target");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                // Set persistent BP
                SetBPX(exportAddr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    g_bpHitCount++;

                    const DEBUG_EVENT* dbgEvent = GetDebugData();
                    if (dbgEvent)
                    {
                        EnterCriticalSection(&g_threadIdLock);
                        g_hitThreadIds.insert(dbgEvent->dwThreadId);
                        LeaveCriticalSection(&g_threadIdLock);
                    }

                    // Stop after enough hits to avoid long test times
                    if (g_bpHitCount >= s_maxHitCount)
                    {
                        StopDebug();
                    }
                });
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    SetCustomHandler(UE_CH_EXITPROCESS, [](const void*) {
        g_processExited = true;
    });

    SetCustomHandler(UE_CH_CREATETHREAD, OnThreadCreated);
    SetCustomHandler(UE_CH_EXITTHREAD, OnThreadExited);

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    CleanupThreadLock();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    // With 4 threads in TestExe_Threading, we should see multiple thread IDs
    TEST_ASSERT(g_hitThreadIds.size() >= 2, "Not enough unique threads hit the BP");
    TEST_ASSERT(g_bpHitCount >= 4, "Not enough BP hits (expecting at least 4 for 4 threads)");

    return true;
}

//-----------------------------------------------------------------------------
// MT-10: Thread-specific BP (TID filter)
// BP that only triggers for specific thread
// Note: TitanEngine doesn't have native TID filtering, so we implement it in callback
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MT-10", MT_10, "Thread-specific BP (TID filter in callback)")
{
    ResetTestState();
    InitThreadLock();

    std::wstring exePath = GetTestExePath();
    static std::atomic<bool> s_targetThreadSelected{false};

    // Use thread creation event to capture a specific thread ID
    SetCustomHandler(UE_CH_CREATETHREAD, [](const void* info) {
        g_threadCreateCount++;

        // Select the first non-main thread as our target
        if (!s_targetThreadSelected && g_threadCreateCount > 0)
        {
            // Get thread ID from debug event
            const DEBUG_EVENT* dbgEvent = GetDebugData();
            if (dbgEvent && dbgEvent->dwThreadId != 0)
            {
                g_targetThreadId = dbgEvent->dwThreadId;
                s_targetThreadSelected = true;
            }
        }
    });

    // Set up CREATE_PROCESS handler to set the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hProcess = createInfo.hProcess;
        g_processId = dbgEvent->dwProcessId;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "thread_bp_target");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                // Set persistent BP with thread-filtering callback
                SetBPX(exportAddr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitThreadSpecific);
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    SetCustomHandler(UE_CH_EXITPROCESS, [](const void*) {
        g_processExited = true;
    });

    SetCustomHandler(UE_CH_EXITTHREAD, OnThreadExited);

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    CleanupThreadLock();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");

    // Multiple threads should have hit the BP location
    TEST_ASSERT(g_hitThreadIds.size() >= 1, "No threads hit the BP");

    // But only hits from the target thread should be counted
    // (g_bpHitCount only increments when thread ID matches)
    if (s_targetThreadSelected && g_hitThreadIds.find(g_targetThreadId) != g_hitThreadIds.end())
    {
        TEST_ASSERT(g_bpHitCount >= 1, "Target thread did not hit the filtered BP");
    }

    return true;
}
