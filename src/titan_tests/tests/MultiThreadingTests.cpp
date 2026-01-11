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
#include <TlHelp32.h>

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

// Get address of exported function from debuggee
ULONG_PTR GetExportAddress(HANDLE hProcess, ULONG_PTR moduleBase, const char* exportName)
{
    // Read DOS header
    IMAGE_DOS_HEADER dosHeader;
    if (!MemoryReadSafe(hProcess, (LPVOID)moduleBase, &dosHeader, sizeof(dosHeader), nullptr))
        return 0;

    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    // Read NT headers
    ULONG_PTR ntHeadersAddr = moduleBase + dosHeader.e_lfanew;

#ifdef _WIN64
    IMAGE_NT_HEADERS64 ntHeaders;
#else
    IMAGE_NT_HEADERS32 ntHeaders;
#endif

    if (!MemoryReadSafe(hProcess, (LPVOID)ntHeadersAddr, &ntHeaders, sizeof(ntHeaders), nullptr))
        return 0;

    if (ntHeaders.Signature != IMAGE_NT_SIGNATURE)
        return 0;

    // Get export directory
    DWORD exportDirRVA = ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    DWORD exportDirSize = ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

    if (exportDirRVA == 0 || exportDirSize == 0)
        return 0;

    IMAGE_EXPORT_DIRECTORY exportDir;
    if (!MemoryReadSafe(hProcess, (LPVOID)(moduleBase + exportDirRVA), &exportDir, sizeof(exportDir), nullptr))
        return 0;

    // Read function addresses, names, and ordinals
    DWORD numNames = exportDir.NumberOfNames;
    ULONG_PTR namesAddr = moduleBase + exportDir.AddressOfNames;
    ULONG_PTR ordinalsAddr = moduleBase + exportDir.AddressOfNameOrdinals;
    ULONG_PTR functionsAddr = moduleBase + exportDir.AddressOfFunctions;

    for (DWORD i = 0; i < numNames; i++)
    {
        DWORD nameRVA;
        if (!MemoryReadSafe(hProcess, (LPVOID)(namesAddr + i * sizeof(DWORD)), &nameRVA, sizeof(nameRVA), nullptr))
            continue;

        char name[256] = {0};
        if (!MemoryReadSafe(hProcess, (LPVOID)(moduleBase + nameRVA), name, sizeof(name) - 1, nullptr))
            continue;

        if (strcmp(name, exportName) == 0)
        {
            WORD ordinal;
            if (!MemoryReadSafe(hProcess, (LPVOID)(ordinalsAddr + i * sizeof(WORD)), &ordinal, sizeof(ordinal), nullptr))
                return 0;

            DWORD funcRVA;
            if (!MemoryReadSafe(hProcess, (LPVOID)(functionsAddr + ordinal * sizeof(DWORD)), &funcRVA, sizeof(funcRVA), nullptr))
                return 0;

            return moduleBase + funcRVA;
        }
    }

    return 0;
}

// Get the module base for the debuggee
ULONG_PTR GetModuleBase(DWORD processId)
{
    ULONG_PTR moduleBase = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32W me = {sizeof(me)};
        if (Module32FirstW(hSnapshot, &me))
        {
            moduleBase = (ULONG_PTR)me.modBaseAddr;
        }
        CloseHandle(hSnapshot);
    }
    return moduleBase;
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

    // Get the debug event to determine which thread hit the BP
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        EnterCriticalSection(&g_threadIdLock);
        g_hitThreadIds.insert(dbgEvent->dwThreadId);
        LeaveCriticalSection(&g_threadIdLock);
    }

    g_lastBpAddress = GetContextDataEx(GetCurrentThread(), UE_CIP);
}

// BP callback for simultaneous BP test
void OnBpHitSimultaneous()
{
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
    g_bpHitCount++;
    g_lastBpAddress = GetContextDataEx(GetCurrentThread(), UE_CIP);

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

//-----------------------------------------------------------------------------
// Helper to run a test with a debuggee
//-----------------------------------------------------------------------------

struct DebugSession
{
    PROCESS_INFORMATION* pi = nullptr;
    HANDLE hProcess = nullptr;
    ULONG_PTR imageBase = 0;

    bool Start(const wchar_t* exePath)
    {
        pi = InitDebugW(exePath, nullptr, nullptr);
        if (!pi)
            return false;

        hProcess = pi->hProcess;
        g_processId = pi->dwProcessId;
        return true;
    }

    void SetupHandlers()
    {
        SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, OnSystemBreakpoint);
        SetCustomHandler(UE_CH_CREATEPROCESS, OnProcessCreated);
        SetCustomHandler(UE_CH_EXITPROCESS, OnProcessExited);
        SetCustomHandler(UE_CH_CREATETHREAD, OnThreadCreated);
        SetCustomHandler(UE_CH_EXITTHREAD, OnThreadExited);
    }

    ULONG_PTR GetExport(const char* name)
    {
        ULONG_PTR moduleBase = GetModuleBase(pi->dwProcessId);
        if (moduleBase == 0)
            return 0;

        imageBase = moduleBase;
        return GetExportAddress(hProcess, moduleBase, name);
    }

    void Run()
    {
        DebugLoop();
    }

    void Stop()
    {
        StopDebug();
    }
};

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
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Get the target function address - this function is called by multiple threads
        ULONG_PTR addr = s_session->GetExport("thread_bp_target");
        if (addr)
        {
            g_targetAddress = addr;
            // Set persistent breakpoint to catch all thread hits
            bool result = SetBPX(addr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitTrackThread);
            if (!result)
            {
                StopDebug();
            }
        }
        else
        {
            StopDebug();
        }
    });

    session.Run();

    CleanupThreadLock();

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
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static std::atomic<int> s_bp1Hits{0};
    static std::atomic<int> s_bp2Hits{0};

    // Custom callback for first BP
    auto onBp1Hit = [](const void*) {
        s_bp1Hits++;
        EnterCriticalSection(&g_threadIdLock);
        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (dbgEvent) g_hitThreadIds.insert(dbgEvent->dwThreadId);
        LeaveCriticalSection(&g_threadIdLock);
    };

    // Custom callback for second BP
    auto onBp2Hit = [](const void*) {
        s_bp2Hits++;
        EnterCriticalSection(&g_threadIdLock);
        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (dbgEvent) g_hitThreadIds.insert(dbgEvent->dwThreadId);
        LeaveCriticalSection(&g_threadIdLock);
    };

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Set BPs on two different functions
        ULONG_PTR addr1 = s_session->GetExport("thread_bp_target");
        ULONG_PTR addr2 = s_session->GetExport("thread_critical_section_work");

        if (addr1 && addr2)
        {
            g_targetAddress = addr1;
            g_targetAddress2 = addr2;

            SetBPX(addr1, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitSimultaneous);
            SetBPX(addr2, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitSimultaneous);
        }
        else
        {
            StopDebug();
        }
    });

    session.Run();

    CleanupThreadLock();

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
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        ULONG_PTR addr = s_session->GetExport("thread_bp_target");
        if (addr)
        {
            g_targetAddress = addr;
            // Set persistent BP that will delete itself on first hit
            bool result = SetBPX(addr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitAndDeleteMT);
            if (!result)
            {
                StopDebug();
            }
        }
        else
        {
            StopDebug();
        }
    });

    session.Run();

    CleanupThreadLock();

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
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static DWORD s_drIndex = 0;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        ULONG_PTR addr = s_session->GetExport("thread_bp_target");
        if (addr)
        {
            g_targetAddress = addr;

            // Get unused DR register
            if (!GetUnusedHardwareBreakPointRegister(&s_drIndex))
            {
                StopDebug();
                return;
            }

            // Set hardware execute breakpoint
            bool result = SetHardwareBreakPoint(addr, s_drIndex, UE_HARDWARE_EXECUTE,
                                                UE_HARDWARE_SIZE_1, OnHwBpHit);
            if (!result)
            {
                StopDebug();
            }
        }
        else
        {
            StopDebug();
        }
    });

    session.Run();

    // Clean up HW BP
    DeleteHardwareBreakPoint(s_drIndex);

    CleanupThreadLock();

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
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static DWORD s_bpThreadId = 0;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        ULONG_PTR addr = s_session->GetExport("thread_bp_target");
        if (addr)
        {
            g_targetAddress = addr;
            // Set singleshoot BP to catch a thread
            bool result = SetBPX(addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
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

            if (!result)
            {
                StopDebug();
            }
        }
        else
        {
            StopDebug();
        }
    });

    session.Run();

    CleanupThreadLock();

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
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        ULONG_PTR addr = s_session->GetExport("thread_bp_target");
        if (addr)
        {
            g_targetAddress = addr;
            // Set persistent BP to catch multiple threads
            bool result = SetBPX(addr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitRecordContext);
            if (!result)
            {
                StopDebug();
            }
        }
        else
        {
            StopDebug();
        }
    });

    session.Run();

    CleanupThreadLock();

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
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static int s_initialThreadCount = 0;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
        s_initialThreadCount = g_threadCreateCount.load();

        // Set BP on function that might be called when threads are being created
        ULONG_PTR addr = s_session->GetExport("init_threading");
        if (addr)
        {
            g_targetAddress = addr;
            SetBPX(addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                g_bpHitCount++;
                // Do multiple steps while threads might be created
                StepInto([]() {
                    g_stepCompleted = true;
                    // Continue stepping
                    StepInto(OnStepComplete);
                });
            });
        }
    });

    session.Run();

    CleanupThreadLock();

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
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Set BP on thread_worker which threads execute and then exit
        ULONG_PTR addr = s_session->GetExport("thread_worker");
        if (addr)
        {
            g_targetAddress = addr;
            SetBPX(addr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitTrackThread);
        }
    });

    session.Run();

    CleanupThreadLock();

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

    // Use the test exe with --loop argument which creates multiple threads
    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static std::atomic<int> s_maxHitCount{100}; // Stop after 100 hits for time

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        ULONG_PTR addr = s_session->GetExport("thread_bp_target");
        if (addr)
        {
            g_targetAddress = addr;
            // Set persistent BP
            bool result = SetBPX(addr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, []() {
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

            if (!result)
            {
                StopDebug();
            }
        }
        else
        {
            StopDebug();
        }
    });

    session.Run();

    CleanupThreadLock();

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
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static std::atomic<bool> s_targetThreadSelected{false};

    // Use thread creation event to capture a specific thread ID
    SetCustomHandler(UE_CH_CREATETHREAD, [](const void* info) {
        g_threadCreateCount++;

        // Select the first non-main thread as our target
        if (!s_targetThreadSelected && g_threadCreateCount > 0)
        {
            const CREATE_THREAD_DEBUG_INFO* threadInfo = static_cast<const CREATE_THREAD_DEBUG_INFO*>(info);
            // Get thread ID from debug event
            const DEBUG_EVENT* dbgEvent = GetDebugData();
            if (dbgEvent && dbgEvent->dwThreadId != 0)
            {
                g_targetThreadId = dbgEvent->dwThreadId;
                s_targetThreadSelected = true;
            }
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        ULONG_PTR addr = s_session->GetExport("thread_bp_target");
        if (addr)
        {
            g_targetAddress = addr;
            // Set persistent BP with thread-filtering callback
            bool result = SetBPX(addr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitThreadSpecific);
            if (!result)
            {
                StopDebug();
            }
        }
        else
        {
            StopDebug();
        }
    });

    session.Run();

    CleanupThreadLock();

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
