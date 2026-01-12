/**
 * Attach/Detach Tests (AD-01 through AD-07)
 *
 * Tests for TitanEngine attach/detach functionality.
 * These tests use the TestExe_Attach target executable which runs
 * as a long-running process that can be attached to.
 *
 * Key TitanEngine functions tested:
 * - AttachDebugger(pid, killOnExit, debugInfo, callback)
 * - DetachDebuggerEx(pid)
 * - IsFileBeingDebugged()
 */

#include "../TitanTestFramework.h"
#include "TitanEngine/TitanEngine.h"
#include <string>
#include <atomic>
#include <TlHelp32.h>

namespace
{

//-----------------------------------------------------------------------------
// Test state and utilities
//-----------------------------------------------------------------------------

// Atomic counters and flags
std::atomic<bool> g_attachCallbackCalled{false};
std::atomic<bool> g_systemBpHit{false};
std::atomic<int> g_bpHitCount{0};
std::atomic<ULONG_PTR> g_lastBpAddress{0};
std::atomic<int> g_threadCount{0};
std::atomic<DWORD> g_attachedPid{0};

// Target function address for breakpoints after attach
ULONG_PTR g_targetAddress = 0;

// Module base address of attached process (set in handlers)
ULONG_PTR g_moduleBase = 0;

// Global to store executable path for attach scenarios
// (handlers can't capture local variables)
std::wstring g_attachExePath;

// Reset all test state
void ResetTestState()
{
    g_attachCallbackCalled = false;
    g_systemBpHit = false;
    g_bpHitCount = 0;
    g_lastBpAddress = 0;
    g_threadCount = 0;
    g_attachedPid = 0;
    g_targetAddress = 0;
    g_moduleBase = 0;
    g_attachExePath.clear();
}

// Get the test executable path (uses framework helper with architecture suffix)
std::wstring GetTestExePath()
{
    return TitanTest::GetTestExePath(L"TestExe_Attach");
}

// Start an external process and return its info
PROCESS_INFORMATION StartExternalProcess(const wchar_t* exePath, const wchar_t* args = nullptr)
{
    PROCESS_INFORMATION pi = {0};
    STARTUPINFOW si = {sizeof(si)};

    std::wstring cmdLine = exePath;
    if (args)
    {
        cmdLine += L" ";
        cmdLine += args;
    }

    // Non-const buffer for CreateProcessW
    wchar_t cmdLineBuf[MAX_PATH * 2];
    wcscpy_s(cmdLineBuf, cmdLine.c_str());

    BOOL created = CreateProcessW(
        nullptr,
        cmdLineBuf,
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!created)
    {
        pi.dwProcessId = 0;
    }

    return pi;
}

// Wait for process to be ready (signaled by event)
bool WaitForProcessReady(DWORD pid, DWORD timeout = 5000)
{
    // Wait a short time for the process to create its ready event
    Sleep(100);

    // Try to open the ready event
    HANDLE hEvent = OpenEventW(SYNCHRONIZE, FALSE, L"TestExeAttachReady");
    if (!hEvent)
    {
        // Event doesn't exist yet, wait a bit and retry
        Sleep(200);
        hEvent = OpenEventW(SYNCHRONIZE, FALSE, L"TestExeAttachReady");
        if (!hEvent)
        {
            // Fall back to just waiting
            Sleep(500);
            return true;
        }
    }

    DWORD result = WaitForSingleObject(hEvent, timeout);
    CloseHandle(hEvent);

    return (result == WAIT_OBJECT_0);
}

// Check if process is still running
bool IsProcessRunning(DWORD pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess)
        return false;

    DWORD exitCode;
    BOOL result = GetExitCodeProcess(hProcess, &exitCode);
    CloseHandle(hProcess);

    return result && (exitCode == STILL_ACTIVE);
}

// Kill process by PID
void KillProcess(DWORD pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess)
    {
        TerminateProcess(hProcess, 1);
        CloseHandle(hProcess);
    }
}

// Resolve export address from a known executable path
// Uses LoadLibraryExW + GetProcAddress pattern instead of toolhelp which can deadlock
ULONG_PTR ResolveExportFromPath(const wchar_t* exePath, ULONG_PTR remoteBase, const char* exportName)
{
    HMODULE hLib = LoadLibraryExW(exePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!hLib)
        return 0;

    ULONG_PTR result = 0;
    auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, exportName);
    if (exportAddr)
    {
        // Convert from local address to remote address
        result = exportAddr - (ULONG_PTR)hLib + remoteBase;
    }

    FreeLibrary(hLib);
    return result;
}

// Get module base from attached process using NtQueryInformationProcess
// This avoids toolhelp snapshot which can deadlock when the target is suspended
ULONG_PTR GetModuleBaseFromPEB(HANDLE hProcess)
{
    // Define structures we need
    typedef struct _PROCESS_BASIC_INFORMATION {
        PVOID Reserved1;
        PVOID PebBaseAddress;
        PVOID Reserved2[2];
        ULONG_PTR UniqueProcessId;
        PVOID Reserved3;
    } PROCESS_BASIC_INFORMATION;

    typedef NTSTATUS (NTAPI *NtQueryInformationProcessFn)(
        HANDLE ProcessHandle,
        ULONG ProcessInformationClass,
        PVOID ProcessInformation,
        ULONG ProcessInformationLength,
        PULONG ReturnLength
    );

    static auto NtQueryInformationProcess = (NtQueryInformationProcessFn)
        GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess");
    if (!NtQueryInformationProcess)
        return 0;

    PROCESS_BASIC_INFORMATION pbi = {};
    NTSTATUS status = NtQueryInformationProcess(hProcess, 0 /* ProcessBasicInformation */, &pbi, sizeof(pbi), nullptr);
    if (status != 0 || !pbi.PebBaseAddress)
        return 0;

    // Read PEB to get ImageBaseAddress
    // PEB structure offset for ImageBaseAddress: 0x10 on x64, 0x08 on x86
#ifdef _WIN64
    const SIZE_T imageBaseOffset = 0x10;
#else
    const SIZE_T imageBaseOffset = 0x08;
#endif

    ULONG_PTR imageBase = 0;
    if (!ReadProcessMemory(hProcess, (PBYTE)pbi.PebBaseAddress + imageBaseOffset, &imageBase, sizeof(imageBase), nullptr))
        return 0;

    return imageBase;
}

// Count threads in a process
int CountProcessThreads(DWORD pid)
{
    int count = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32 te = {sizeof(te)};
        if (Thread32First(hSnapshot, &te))
        {
            do
            {
                if (te.th32OwnerProcessID == pid)
                {
                    count++;
                }
            } while (Thread32Next(hSnapshot, &te));
        }
        CloseHandle(hSnapshot);
    }
    return count;
}

//-----------------------------------------------------------------------------
// Callback handlers
//-----------------------------------------------------------------------------

void OnAttachCallback()
{
    g_attachCallbackCalled = true;
    g_attachedPid = GetDebugData()->dwProcessId;
}

void OnSystemBreakpoint(const void*)
{
    g_systemBpHit = true;
}

void OnBpHit()
{
    TITAN_TRACK_BP_HIT();
    g_bpHitCount++;
    // Get BP address from debug event (not GetContextDataEx which uses wrong thread handle)
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        g_lastBpAddress = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
    }
}

void OnThreadCreated(const void* info)
{
    g_threadCount++;
}

} // anonymous namespace

//-----------------------------------------------------------------------------
// AD-01: AttachDebugger to running process
// Start external process, then attach debugger
//-----------------------------------------------------------------------------
TITAN_TEST_ID("AD-01", AD_01, "AttachDebugger to running process")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();

    // Start the external process
    PROCESS_INFORMATION pi = StartExternalProcess(exePath.c_str());
    TEST_ASSERT(pi.dwProcessId != 0, "Failed to start external process");

    // Wait for process to be ready
    bool ready = WaitForProcessReady(pi.dwProcessId);
    TEST_ASSERT(ready, "Process did not become ready in time");
    TEST_ASSERT(IsProcessRunning(pi.dwProcessId), "Process exited prematurely");

    // Set up attach callback
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, OnSystemBreakpoint);

    // Attach to the running process
    bool attached = AttachDebugger(pi.dwProcessId, true, nullptr, OnAttachCallback);
    TEST_ASSERT(attached, "AttachDebugger failed");

    // Run debug loop briefly - the attach callback should fire
    // Set a short timeout by stopping after the callback
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
        // After system breakpoint, we can stop
        StopDebug();
    });

    DebugLoop();

    // Clean up
    if (IsProcessRunning(pi.dwProcessId))
    {
        KillProcess(pi.dwProcessId);
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    TEST_ASSERT(g_attachCallbackCalled, "Attach callback was not called");

    return true;
}

//-----------------------------------------------------------------------------
// AD-02: AttachDebugger KillOnExit=true
// Attach with killOnExit=true, detach (or stop), verify process dies
//-----------------------------------------------------------------------------
TITAN_TEST_ID("AD-02", AD_02, "AttachDebugger KillOnExit=true - process dies when debugger exits")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();

    // Start the external process
    PROCESS_INFORMATION pi = StartExternalProcess(exePath.c_str());
    TEST_ASSERT(pi.dwProcessId != 0, "Failed to start external process");

    bool ready = WaitForProcessReady(pi.dwProcessId);
    TEST_ASSERT(ready, "Process did not become ready in time");

    DWORD targetPid = pi.dwProcessId;

    // Attach with killOnExit=true
    bool attached = AttachDebugger(targetPid, true, nullptr, OnAttachCallback);
    TEST_ASSERT(attached, "AttachDebugger failed");

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
        // Stop debugging - this should kill the process because killOnExit=true
        StopDebug();
    });

    DebugLoop();

    // Wait a moment for the process to terminate
    Sleep(500);

    // The process should be dead
    bool stillRunning = IsProcessRunning(targetPid);

    // Clean up handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    TEST_ASSERT(!stillRunning, "Process should be terminated when killOnExit=true");

    return true;
}

//-----------------------------------------------------------------------------
// AD-03: AttachDebugger KillOnExit=false
// Attach with killOnExit=false, detach, verify process continues
//-----------------------------------------------------------------------------
TITAN_TEST_ID("AD-03", AD_03, "AttachDebugger KillOnExit=false - process continues after detach")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();

    // Start the external process
    PROCESS_INFORMATION pi = StartExternalProcess(exePath.c_str());
    TEST_ASSERT(pi.dwProcessId != 0, "Failed to start external process");

    bool ready = WaitForProcessReady(pi.dwProcessId);
    TEST_ASSERT(ready, "Process did not become ready in time");

    DWORD targetPid = pi.dwProcessId;

    // Attach with killOnExit=false
    bool attached = AttachDebugger(targetPid, false, nullptr, OnAttachCallback);
    TEST_ASSERT(attached, "AttachDebugger failed");

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
        // Detach cleanly
        DetachDebuggerEx(g_attachedPid);
    });

    DebugLoop();

    // Wait a moment
    Sleep(500);

    // The process should still be running
    bool stillRunning = IsProcessRunning(targetPid);

    // Clean up - kill the process since we detached
    if (stillRunning)
    {
        KillProcess(targetPid);
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    TEST_ASSERT(stillRunning, "Process should continue running when killOnExit=false");

    return true;
}

//-----------------------------------------------------------------------------
// AD-04: DetachDebuggerEx
// Attach, then cleanly detach using DetachDebuggerEx
//-----------------------------------------------------------------------------
TITAN_TEST_ID("AD-04", AD_04, "DetachDebuggerEx - clean detach from process")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();

    // Start the external process
    PROCESS_INFORMATION pi = StartExternalProcess(exePath.c_str());
    TEST_ASSERT(pi.dwProcessId != 0, "Failed to start external process");

    bool ready = WaitForProcessReady(pi.dwProcessId);
    TEST_ASSERT(ready, "Process did not become ready in time");

    DWORD targetPid = pi.dwProcessId;
    static bool s_detachSuccess = false;

    // Attach with killOnExit=false (so we can verify detach works)
    bool attached = AttachDebugger(targetPid, false, nullptr, OnAttachCallback);
    TEST_ASSERT(attached, "AttachDebugger failed");

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
        // Try to detach
        s_detachSuccess = DetachDebuggerEx(g_attachedPid);
    });

    DebugLoop();

    // Process should still be running after detach
    Sleep(200);
    bool stillRunning = IsProcessRunning(targetPid);

    // Clean up
    if (stillRunning)
    {
        KillProcess(targetPid);
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    TEST_ASSERT(s_detachSuccess, "DetachDebuggerEx returned false");
    TEST_ASSERT(stillRunning, "Process should continue running after detach");

    return true;
}

//-----------------------------------------------------------------------------
// AD-05: Set BP after attach
// Attach to process, then set breakpoints
// Uses LoadLibraryExW + GetProcAddress for export resolution instead of toolhelp
//-----------------------------------------------------------------------------
TITAN_TEST_ID("AD-05", AD_05, "Set breakpoint after attach")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    g_attachExePath = exePath;  // Store for handler access

    // Start the external process
    PROCESS_INFORMATION pi = StartExternalProcess(exePath.c_str());
    TEST_ASSERT(pi.dwProcessId != 0, "Failed to start external process");

    bool ready = WaitForProcessReady(pi.dwProcessId);
    TEST_ASSERT(ready, "Process did not become ready in time");

    DWORD targetPid = pi.dwProcessId;
    static HANDLE s_hProcess = nullptr;
    static bool s_bpSet = false;

    // Attach
    bool attached = AttachDebugger(targetPid, true, nullptr, OnAttachCallback);
    TEST_ASSERT(attached, "AttachDebugger failed");

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Get process handle
        s_hProcess = TitanOpenProcess(PROCESS_ALL_ACCESS, FALSE, g_attachedPid);
        if (!s_hProcess)
        {
            StopDebug();
            return;
        }

        // Get module base from PEB (avoids toolhelp snapshot deadlock)
        ULONG_PTR moduleBase = GetModuleBaseFromPEB(s_hProcess);
        if (!moduleBase)
        {
            StopDebug();
            return;
        }
        g_moduleBase = moduleBase;

        // Resolve export using LoadLibraryExW + GetProcAddress pattern
        ULONG_PTR addr = ResolveExportFromPath(g_attachExePath.c_str(), moduleBase, "attach_periodic_work");
        if (addr)
        {
            g_targetAddress = addr;
            // Set breakpoint on the function
            s_bpSet = SetBPX(addr, UE_SINGLESHOOT, OnBpHit);
        }

        if (!s_bpSet)
        {
            StopDebug();
        }
    });

    // After BP hits, stop
    // The test process calls attach_periodic_work repeatedly, so BP should hit
    DebugLoop();

    // Clean up
    if (s_hProcess)
    {
        CloseHandle(s_hProcess);
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function not found");
    TEST_ASSERT(s_bpSet, "Failed to set breakpoint");
    TEST_ASSERT(g_bpHitCount >= 1, "Breakpoint was not hit after attach");

    return true;
}

//-----------------------------------------------------------------------------
// AD-06: Attach, detach, re-attach
// Multiple attach cycles on same process
//-----------------------------------------------------------------------------
TITAN_TEST_ID("AD-06", AD_06, "Attach, detach, re-attach - multiple cycles")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();

    // Start the external process
    PROCESS_INFORMATION pi = StartExternalProcess(exePath.c_str());
    TEST_ASSERT(pi.dwProcessId != 0, "Failed to start external process");

    bool ready = WaitForProcessReady(pi.dwProcessId);
    TEST_ASSERT(ready, "Process did not become ready in time");

    DWORD targetPid = pi.dwProcessId;
    static int s_attachCount = 0;

    // First attach cycle
    {
        ResetTestState();

        bool attached = AttachDebugger(targetPid, false, nullptr, OnAttachCallback);
        TEST_ASSERT(attached, "First AttachDebugger failed");

        SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
            g_systemBpHit = true;
            s_attachCount++;
            // Detach
            DetachDebuggerEx(g_attachedPid);
        });

        DebugLoop();

        TEST_ASSERT(g_attachCallbackCalled, "First attach callback not called");
        TEST_ASSERT(g_systemBpHit, "First system BP not hit");
    }

    Sleep(500);
    TEST_ASSERT(IsProcessRunning(targetPid), "Process died after first detach");

    // Second attach cycle
    {
        ResetTestState();

        bool attached = AttachDebugger(targetPid, false, nullptr, OnAttachCallback);
        TEST_ASSERT(attached, "Second AttachDebugger failed");

        SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
            g_systemBpHit = true;
            s_attachCount++;
            // Detach
            DetachDebuggerEx(g_attachedPid);
        });

        DebugLoop();

        TEST_ASSERT(g_attachCallbackCalled, "Second attach callback not called");
        TEST_ASSERT(g_systemBpHit, "Second system BP not hit");
    }

    Sleep(500);
    TEST_ASSERT(IsProcessRunning(targetPid), "Process died after second detach");

    // Third attach cycle (with killOnExit=true to clean up)
    {
        ResetTestState();

        bool attached = AttachDebugger(targetPid, true, nullptr, OnAttachCallback);
        TEST_ASSERT(attached, "Third AttachDebugger failed");

        SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
            g_systemBpHit = true;
            s_attachCount++;
            StopDebug();  // This will kill due to killOnExit=true
        });

        DebugLoop();

        TEST_ASSERT(g_attachCallbackCalled, "Third attach callback not called");
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    TEST_ASSERT(s_attachCount == 3, "Expected 3 successful attach cycles");

    return true;
}

//-----------------------------------------------------------------------------
// AD-07: Attach to multi-threaded process
// Attach to process with multiple threads running
// Uses LoadLibraryExW + GetProcAddress for export resolution instead of toolhelp
//-----------------------------------------------------------------------------
TITAN_TEST_ID("AD-07", AD_07, "Attach to multi-threaded process")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    g_attachExePath = exePath;  // Store for handler access

    // Start the external process with --worker flag to create additional threads
    PROCESS_INFORMATION pi = StartExternalProcess(exePath.c_str(), L"--worker");
    TEST_ASSERT(pi.dwProcessId != 0, "Failed to start external process");

    bool ready = WaitForProcessReady(pi.dwProcessId);
    TEST_ASSERT(ready, "Process did not become ready in time");

    // Give the worker thread time to start
    Sleep(500);

    DWORD targetPid = pi.dwProcessId;

    // Count threads before attach (using toolhelp is safe here - process not suspended)
    int threadsBefore = CountProcessThreads(targetPid);
    TEST_ASSERT(threadsBefore > 1, "Process should have multiple threads");

    static int s_threadsSeenDuringDebug = 0;
    static HANDLE s_hProcess = nullptr;

    // Attach
    bool attached = AttachDebugger(targetPid, true, nullptr, OnAttachCallback);
    TEST_ASSERT(attached, "AttachDebugger failed");

    // Track thread creation events
    SetCustomHandler(UE_CH_CREATETHREAD, [](const void*) {
        g_threadCount++;
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Count threads via snapshot (safe since we're in debug break)
        s_threadsSeenDuringDebug = CountProcessThreads(g_attachedPid);

        // Get process handle
        s_hProcess = TitanOpenProcess(PROCESS_ALL_ACCESS, FALSE, g_attachedPid);
        if (s_hProcess)
        {
            // Get module base from PEB (avoids toolhelp snapshot deadlock)
            ULONG_PTR moduleBase = GetModuleBaseFromPEB(s_hProcess);
            if (moduleBase)
            {
                g_moduleBase = moduleBase;
                // Resolve export using LoadLibraryExW + GetProcAddress pattern
                ULONG_PTR addr = ResolveExportFromPath(g_attachExePath.c_str(), moduleBase, "attach_periodic_work");
                if (addr)
                {
                    g_targetAddress = addr;
                    SetBPX(addr, UE_SINGLESHOOT, OnBpHit);
                }
            }
        }
    });

    DebugLoop();

    // Clean up
    if (s_hProcess)
    {
        CloseHandle(s_hProcess);
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(s_threadsSeenDuringDebug > 1, "Should see multiple threads after attach");
    // Note: We may or may not see CREATE_THREAD events depending on when we attach
    // The important thing is that we can attach and debug a multi-threaded process

    return true;
}
