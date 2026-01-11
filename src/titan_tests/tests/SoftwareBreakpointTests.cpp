/**
 * Software Breakpoint Tests (SW-01 through SW-10)
 *
 * Tests for TitanEngine software breakpoint functionality.
 * These tests use the TestExe_Breakpoints target executable which exports
 * functions like bp_target_sw1, bp_target_sw2, etc.
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

// Path to test executable (set by test setup)
std::wstring g_testExePath;

// Atomic counters for callback verification
std::atomic<int> g_bpHitCount{0};
std::atomic<ULONG_PTR> g_lastBpAddress{0};
std::atomic<bool> g_systemBpHit{false};
std::atomic<bool> g_processCreated{false};
std::atomic<bool> g_processExited{false};
std::atomic<bool> g_dllLoaded{false};
std::atomic<ULONG_PTR> g_dllBase{0};

// Address of exported function to set breakpoint on
ULONG_PTR g_targetAddress = 0;

// For delete-during-callback test
std::atomic<bool> g_deleteBpDuringCallback{false};

// Reset all test state
void ResetTestState()
{
    g_bpHitCount = 0;
    g_lastBpAddress = 0;
    g_systemBpHit = false;
    g_processCreated = false;
    g_processExited = false;
    g_dllLoaded = false;
    g_dllBase = 0;
    g_targetAddress = 0;
    g_deleteBpDuringCallback = false;
    TitanTest::ResetDebuggeeImageBase();
}

// Get the test executable path (uses framework helper with architecture suffix)
std::wstring GetTestExePath()
{
    return TitanTest::GetTestExePath(L"TestExe_Breakpoints");
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
        // Read name RVA
        DWORD nameRVA;
        if (!MemoryReadSafe(hProcess, (LPVOID)(namesAddr + i * sizeof(DWORD)), &nameRVA, sizeof(nameRVA), nullptr))
            continue;

        // Read name
        char name[256] = {0};
        if (!MemoryReadSafe(hProcess, (LPVOID)(moduleBase + nameRVA), name, sizeof(name) - 1, nullptr))
            continue;

        if (strcmp(name, exportName) == 0)
        {
            // Read ordinal
            WORD ordinal;
            if (!MemoryReadSafe(hProcess, (LPVOID)(ordinalsAddr + i * sizeof(WORD)), &ordinal, sizeof(ordinal), nullptr))
                return 0;

            // Read function RVA
            DWORD funcRVA;
            if (!MemoryReadSafe(hProcess, (LPVOID)(functionsAddr + ordinal * sizeof(DWORD)), &funcRVA, sizeof(funcRVA), nullptr))
                return 0;

            return moduleBase + funcRVA;
        }
    }

    return 0;
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
    // Cache the image base to avoid toolhelp deadlock later
    // Use GetDebugData() to access the CREATE_PROCESS_DEBUG_INFO
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent && dbgEvent->dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
    {
        TitanTest::g_debuggeeImageBase = (ULONG_PTR)dbgEvent->u.CreateProcessInfo.lpBaseOfImage;
    }
}

void OnProcessExited(const void* info)
{
    g_processExited = true;
}

void OnDllLoaded(const void* info)
{
    auto* loadInfo = static_cast<const LOAD_DLL_DEBUG_INFO*>(info);
    if (loadInfo)
    {
        g_dllLoaded = true;
        g_dllBase = (ULONG_PTR)loadInfo->lpBaseOfDll;
    }
}

void OnBpHit()
{
    TITAN_TRACK_BP_HIT();
    g_bpHitCount++;
    g_lastBpAddress = GetContextDataEx(GetCurrentThread(), UE_CIP);
}

void OnBpHitAndDelete()
{
    g_bpHitCount++;
    g_lastBpAddress = GetContextDataEx(GetCurrentThread(), UE_CIP);

    // Delete the breakpoint from within the callback
    if (g_deleteBpDuringCallback)
    {
        DeleteBPX(g_lastBpAddress);
    }
}

void OnBpHitSingleshoot()
{
    g_bpHitCount++;
    g_lastBpAddress = GetContextDataEx(GetCurrentThread(), UE_CIP);
    // Single-shot BP should auto-delete, so we just continue
}

void OnBpHitPersistent()
{
    g_bpHitCount++;
    g_lastBpAddress = GetContextDataEx(GetCurrentThread(), UE_CIP);
    // Persistent BP should fire multiple times
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
        return true;
    }

    void SetupHandlers()
    {
        SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, OnSystemBreakpoint);
        SetCustomHandler(UE_CH_CREATEPROCESS, OnProcessCreated);
        SetCustomHandler(UE_CH_EXITPROCESS, OnProcessExited);
        SetCustomHandler(UE_CH_LOADDLL, OnDllLoaded);
    }

    ULONG_PTR GetModuleBase()
    {
        if (imageBase != 0)
            return imageBase;

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pi->dwProcessId);
        if (hSnapshot != INVALID_HANDLE_VALUE)
        {
            MODULEENTRY32W me = {sizeof(me)};
            if (Module32FirstW(hSnapshot, &me))
            {
                imageBase = (ULONG_PTR)me.modBaseAddr;
            }
            CloseHandle(hSnapshot);
        }
        return imageBase;
    }

    ULONG_PTR GetExport(const char* name)
    {
        ULONG_PTR moduleBase = GetModuleBase();
        if (moduleBase == 0)
            return 0;

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
// SW-01: INT3 breakpoint hit verification
// Set BP using SetBPX with UE_BREAKPOINT_TYPE_INT3, verify callback fires
//-----------------------------------------------------------------------------
TITAN_TEST_ID("SW-01", SW_01, "INT3 breakpoint hit verification")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    // We need to set up the BP after the system breakpoint
    // Use a custom system BP handler that sets up our test BP
    static DebugSession* s_session = &session;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Get the target function address
        ULONG_PTR addr = s_session->GetExport("bp_target_sw1");
        if (addr)
        {
            g_targetAddress = addr;
            // Set INT3 breakpoint (singleshoot)
            bool result = SetBPX(addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, OnBpHitSingleshoot);
            if (!result)
            {
                StopDebug();
            }
        }
        else
        {
            // Export not found, stop debugging
            StopDebug();
        }
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_bpHitCount == 1, "INT3 breakpoint was not hit exactly once");
    TEST_ASSERT(g_lastBpAddress == g_targetAddress, "Breakpoint hit at wrong address");

    return true;
}

//-----------------------------------------------------------------------------
// SW-02: LONG_INT3 breakpoint verification
// Use UE_BREAKPOINT_TYPE_LONG_INT3
//-----------------------------------------------------------------------------
TITAN_TEST_ID("SW-02", SW_02, "LONG_INT3 (CD 03) breakpoint verification")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        ULONG_PTR addr = s_session->GetExport("bp_target_sw2");
        if (addr)
        {
            g_targetAddress = addr;
            bool result = SetBPX(addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_LONG_INT3, OnBpHitSingleshoot);
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

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_bpHitCount == 1, "LONG_INT3 breakpoint was not hit exactly once");

    return true;
}

//-----------------------------------------------------------------------------
// SW-03: UD2 breakpoint verification
// Use UE_BREAKPOINT_TYPE_UD2
//-----------------------------------------------------------------------------
TITAN_TEST_ID("SW-03", SW_03, "UD2 (0F 0B) breakpoint verification")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        ULONG_PTR addr = s_session->GetExport("bp_target_sw3");
        if (addr)
        {
            g_targetAddress = addr;
            bool result = SetBPX(addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_UD2, OnBpHitSingleshoot);
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

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_bpHitCount == 1, "UD2 breakpoint was not hit exactly once");

    return true;
}

//-----------------------------------------------------------------------------
// SW-04: Delete breakpoint, verify no hit
// SetBPX then DeleteBPX, continue, verify no callback
//-----------------------------------------------------------------------------
TITAN_TEST_ID("SW-04", SW_04, "Delete breakpoint, verify no hit")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_bpAddress = 0;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        ULONG_PTR addr = s_session->GetExport("bp_target_sw4");
        if (addr)
        {
            g_targetAddress = addr;
            s_bpAddress = addr;

            // Set the breakpoint
            bool setResult = SetBPX(addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, OnBpHit);
            if (!setResult)
            {
                StopDebug();
                return;
            }

            // Immediately delete it
            bool delResult = DeleteBPX(addr);
            if (!delResult)
            {
                StopDebug();
                return;
            }
        }
        else
        {
            StopDebug();
        }
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_bpHitCount == 0, "Breakpoint was hit after deletion (should not have been)");

    return true;
}

//-----------------------------------------------------------------------------
// SW-05: Multiple BPs on same address
// Test setting multiple BPs on same address
//-----------------------------------------------------------------------------
TITAN_TEST_ID("SW-05", SW_05, "Multiple breakpoints on same address")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static bool s_firstBpSet = false;
    static bool s_secondBpSet = false;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        ULONG_PTR addr = s_session->GetExport("bp_target_sw5");
        if (addr)
        {
            g_targetAddress = addr;

            // Set first breakpoint
            s_firstBpSet = SetBPX(addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, OnBpHit);

            // Try to set second breakpoint on same address
            // This should either replace the first one or fail
            s_secondBpSet = SetBPX(addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, OnBpHit);
        }
        else
        {
            StopDebug();
        }
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(s_firstBpSet, "First breakpoint could not be set");
    // The second BP setting behavior depends on TitanEngine implementation
    // It may replace the existing BP or fail - either is acceptable
    TEST_ASSERT(g_bpHitCount == 1, "Breakpoint should fire exactly once");

    return true;
}

//-----------------------------------------------------------------------------
// SW-06: BP in loop, count hits
// Set singleshoot=false BP in loop, count callback invocations
//-----------------------------------------------------------------------------
TITAN_TEST_ID("SW-06", SW_06, "Persistent BP in loop, count hits")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static int s_expectedHits = 5; // Expected number of loop iterations in test exe

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        ULONG_PTR addr = s_session->GetExport("bp_target_sw6_loop");
        if (addr)
        {
            g_targetAddress = addr;
            // Set persistent breakpoint (UE_BREAKPOINT, not UE_SINGLESHOOT)
            bool result = SetBPX(addr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitPersistent);
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

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_bpHitCount >= s_expectedHits, "Persistent BP did not hit expected number of times");

    return true;
}

//-----------------------------------------------------------------------------
// SW-07: Delete BP during callback
// Call DeleteBPX from within BP callback
//-----------------------------------------------------------------------------
TITAN_TEST_ID("SW-07", SW_07, "Delete breakpoint during callback")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;
    g_deleteBpDuringCallback = true;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        ULONG_PTR addr = s_session->GetExport("bp_target_sw7");
        if (addr)
        {
            g_targetAddress = addr;
            // Set persistent breakpoint that will delete itself
            bool result = SetBPX(addr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, OnBpHitAndDelete);
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

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_bpHitCount == 1, "BP should hit once before self-delete");

    // Verify the BP is no longer enabled
    TEST_ASSERT(!IsBPXEnabled(g_targetAddress), "BP should be disabled after callback delete");

    return true;
}

//-----------------------------------------------------------------------------
// SW-08: IsBPXEnabled accuracy
// Verify IsBPXEnabled returns correct state
//-----------------------------------------------------------------------------
TITAN_TEST_ID("SW-08", SW_08, "IsBPXEnabled accuracy")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static bool s_enabledBeforeSet = false;
    static bool s_enabledAfterSet = false;
    static bool s_enabledAfterDelete = false;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        ULONG_PTR addr = s_session->GetExport("bp_target_sw8");
        if (addr)
        {
            g_targetAddress = addr;

            // Check before setting
            s_enabledBeforeSet = IsBPXEnabled(addr);

            // Set breakpoint
            SetBPX(addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, OnBpHit);

            // Check after setting
            s_enabledAfterSet = IsBPXEnabled(addr);

            // Delete breakpoint
            DeleteBPX(addr);

            // Check after deleting
            s_enabledAfterDelete = IsBPXEnabled(addr);
        }
        else
        {
            StopDebug();
        }
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(!s_enabledBeforeSet, "IsBPXEnabled should return false before BP is set");
    TEST_ASSERT(s_enabledAfterSet, "IsBPXEnabled should return true after BP is set");
    TEST_ASSERT(!s_enabledAfterDelete, "IsBPXEnabled should return false after BP is deleted");

    return true;
}

//-----------------------------------------------------------------------------
// SW-09: RemoveAllBreakPoints
// Test RemoveAllBreakPoints(UE_OPTION_REMOVEALL)
//-----------------------------------------------------------------------------
TITAN_TEST_ID("SW-09", SW_09, "RemoveAllBreakPoints clears all BPs")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_addr1 = 0;
    static ULONG_PTR s_addr2 = 0;
    static bool s_bp1Enabled = false;
    static bool s_bp2Enabled = false;
    static bool s_bp1EnabledAfterRemoveAll = false;
    static bool s_bp2EnabledAfterRemoveAll = false;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        s_addr1 = s_session->GetExport("bp_target_sw9a");
        s_addr2 = s_session->GetExport("bp_target_sw9b");

        if (s_addr1 && s_addr2)
        {
            // Set two breakpoints
            SetBPX(s_addr1, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, OnBpHit);
            SetBPX(s_addr2, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, OnBpHit);

            // Verify both are set
            s_bp1Enabled = IsBPXEnabled(s_addr1);
            s_bp2Enabled = IsBPXEnabled(s_addr2);

            // Remove all breakpoints
            RemoveAllBreakPoints(UE_OPTION_REMOVEALL);

            // Verify both are removed
            s_bp1EnabledAfterRemoveAll = IsBPXEnabled(s_addr1);
            s_bp2EnabledAfterRemoveAll = IsBPXEnabled(s_addr2);
        }
        else
        {
            StopDebug();
        }
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(s_addr1 != 0 && s_addr2 != 0, "Target function addresses not resolved");
    TEST_ASSERT(s_bp1Enabled, "BP1 should be enabled after setting");
    TEST_ASSERT(s_bp2Enabled, "BP2 should be enabled after setting");
    TEST_ASSERT(!s_bp1EnabledAfterRemoveAll, "BP1 should be disabled after RemoveAllBreakPoints");
    TEST_ASSERT(!s_bp2EnabledAfterRemoveAll, "BP2 should be disabled after RemoveAllBreakPoints");
    TEST_ASSERT(g_bpHitCount == 0, "No BPs should have hit after RemoveAllBreakPoints");

    return true;
}

//-----------------------------------------------------------------------------
// SW-10: BP on DLL function
// Set BP on function in loaded DLL
//-----------------------------------------------------------------------------
TITAN_TEST_ID("SW-10", SW_10, "Breakpoint on DLL function")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static bool s_bpSetOnDll = false;

    // Set up DLL load handler to set BP on DLL function
    SetCustomHandler(UE_CH_LOADDLL, [](const void* info) {
        auto* loadInfo = static_cast<const LOAD_DLL_DEBUG_INFO*>(info);
        if (!loadInfo)
            return;

        g_dllLoaded = true;
        g_dllBase = (ULONG_PTR)loadInfo->lpBaseOfDll;

        // Try to get an export from the loaded DLL
        // Look for "DllBpTarget" or similar export
        ULONG_PTR dllFunc = GetExportAddress(s_session->hProcess, g_dllBase, "DllBpTarget");
        if (dllFunc)
        {
            g_targetAddress = dllFunc;
            s_bpSetOnDll = SetBPX(dllFunc, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, OnBpHit);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
        // Let the process continue - it should load a DLL and call the target function
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");

    // This test may skip if the test exe doesn't load any DLLs with the expected export
    if (!g_dllLoaded)
    {
        TEST_SKIP("No DLL with expected export was loaded");
    }

    if (g_targetAddress == 0)
    {
        TEST_SKIP("DLL export 'DllBpTarget' not found");
    }

    TEST_ASSERT(s_bpSetOnDll, "Could not set breakpoint on DLL function");
    TEST_ASSERT(g_bpHitCount == 1, "DLL breakpoint was not hit");

    return true;
}
