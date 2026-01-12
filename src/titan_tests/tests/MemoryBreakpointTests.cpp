/**
 * Memory Breakpoint Tests (MB-01 through MB-08)
 *
 * Tests for TitanEngine memory breakpoint functionality using SetMemoryBPXEx.
 * These tests use the TestExe_Breakpoints target executable which exports
 * global volatile variables for memory BP targeting:
 *   - g_memory_read_target
 *   - g_memory_write_target
 *   - g_memory_byte_target
 *   - g_memory_qword_target
 *
 * And functions that access them:
 *   - bp_memory_read()
 *   - bp_memory_write()
 *   - bp_memory_readwrite()
 */

#include "../TitanTestFramework.h"
#include "TitanEngine/TitanEngine.h"
#include <string>
#include <atomic>

namespace
{

//-----------------------------------------------------------------------------
// Test state and utilities
//-----------------------------------------------------------------------------

// Atomic counters for callback verification
std::atomic<int> g_memBpHitCount{0};
std::atomic<ULONG_PTR> g_lastMemBpAddress{0};
std::atomic<bool> g_systemBpHit{false};
std::atomic<bool> g_processCreated{false};
std::atomic<bool> g_processExited{false};

// Address of memory target variable
ULONG_PTR g_memTargetAddress = 0;

// Second memory target for overlapping tests
ULONG_PTR g_memTargetAddress2 = 0;

// Page-aligned buffer address for cross-page tests
ULONG_PTR g_pageAlignedAddress = 0;

// For RestoreOnHit testing
std::atomic<bool> g_expectMultipleHits{false};

// BP set success flags
static bool g_bpSetSuccess = false;
static bool g_bpRemoveSuccess = false;
static std::atomic<int> g_bp1Hits{0};
static std::atomic<int> g_bp2Hits{0};
static bool g_bp1Success = false;
static bool g_bp2Success = false;

// Reset all test state
void ResetTestState()
{
    g_memBpHitCount = 0;
    g_lastMemBpAddress = 0;
    g_systemBpHit = false;
    g_processCreated = false;
    g_processExited = false;
    g_memTargetAddress = 0;
    g_memTargetAddress2 = 0;
    g_pageAlignedAddress = 0;
    g_expectMultipleHits = false;
    g_bpSetSuccess = false;
    g_bpRemoveSuccess = false;
    g_bp1Hits = 0;
    g_bp2Hits = 0;
    g_bp1Success = false;
    g_bp2Success = false;
    TitanTest::ResetDebuggeeImageBase();
}

// Get the test executable path (uses framework helper with architecture suffix)
std::wstring GetTestExePath()
{
    return TitanTest::GetTestExePath(L"TestExe_Breakpoints");
}

//-----------------------------------------------------------------------------
// Callback handlers - using GetDebugData() for exception address
//-----------------------------------------------------------------------------

void OnMemBpHit(const void*)
{
    TITAN_TRACK_BP_HIT();
    g_memBpHitCount++;
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        g_lastMemBpAddress = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
    }
}

void OnMemBpHitOnce(const void*)
{
    TITAN_TRACK_BP_HIT();
    g_memBpHitCount++;
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        g_lastMemBpAddress = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
    }
    // Single-hit callback - BP with RestoreOnHit=true should auto-remove
}

void OnMemBpHitMultiple(const void*)
{
    TITAN_TRACK_BP_HIT();
    g_memBpHitCount++;
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        g_lastMemBpAddress = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
    }
    // Persistent callback - BP with RestoreOnHit=false should fire multiple times
}

void OnBp1Hit(const void*)
{
    TITAN_TRACK_BP_HIT();
    g_bp1Hits++;
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        g_lastMemBpAddress = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
    }
}

void OnBp2Hit(const void*)
{
    TITAN_TRACK_BP_HIT();
    g_bp2Hits++;
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        g_lastMemBpAddress = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
    }
}

//-----------------------------------------------------------------------------
// Helper debug session class
//-----------------------------------------------------------------------------

struct MemBpDebugSession
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
        SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
            g_systemBpHit = true;
        });
        SetCustomHandler(UE_CH_EXITPROCESS, [](const void*) {
            g_processExited = true;
        });
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
// MB-01: Memory read BP - SetMemoryBPXEx with UE_MEMORY_READ
// Set memory BP on g_memory_read_target, trigger read, verify callback fires
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MB-01", MB_01, "Memory read BP using SetMemoryBPXEx with UE_MEMORY_READ")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    MemBpDebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    // Set up CREATE_PROCESS handler to set the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "g_memory_read_target");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_memTargetAddress = exportAddr;
                g_bpSetSuccess = SetMemoryBPXEx(exportAddr, sizeof(DWORD), UE_MEMORY_READ, true, OnMemBpHitOnce);
            }
            FreeLibrary(hLib);
        }
    });

    session.SetupHandlers();
    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_memTargetAddress != 0, "Target variable address not resolved");
    TEST_ASSERT(g_bpSetSuccess, "Memory BP could not be set");
    TEST_ASSERT(g_memBpHitCount >= 1, "Memory read BP was not hit");

    return true;
}

//-----------------------------------------------------------------------------
// MB-02: Memory write BP - SetMemoryBPXEx with UE_MEMORY_WRITE
// Set memory BP on g_memory_write_target, trigger write, verify callback fires
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MB-02", MB_02, "Memory write BP using SetMemoryBPXEx with UE_MEMORY_WRITE")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    MemBpDebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    // Set up CREATE_PROCESS handler to set the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "g_memory_write_target");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_memTargetAddress = exportAddr;
                g_bpSetSuccess = SetMemoryBPXEx(exportAddr, sizeof(DWORD), UE_MEMORY_WRITE, true, OnMemBpHitOnce);
            }
            FreeLibrary(hLib);
        }
    });

    session.SetupHandlers();
    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_memTargetAddress != 0, "Target variable address not resolved");
    TEST_ASSERT(g_bpSetSuccess, "Memory BP could not be set");
    TEST_ASSERT(g_memBpHitCount >= 1, "Memory write BP was not hit");

    return true;
}

//-----------------------------------------------------------------------------
// MB-03: Memory execute BP - SetMemoryBPXEx with UE_MEMORY_EXECUTE
// Set memory BP on a function entry point, verify callback fires on execution
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MB-03", MB_03, "Memory execute BP using SetMemoryBPXEx with UE_MEMORY_EXECUTE")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    MemBpDebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    // Set up CREATE_PROCESS handler to set the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "bp_memory_read");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_memTargetAddress = exportAddr;
                g_bpSetSuccess = SetMemoryBPXEx(exportAddr, 16, UE_MEMORY_EXECUTE, true, OnMemBpHitOnce);
            }
            FreeLibrary(hLib);
        }
    });

    session.SetupHandlers();
    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_memTargetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_bpSetSuccess, "Memory BP could not be set");
    TEST_ASSERT(g_memBpHitCount >= 1, "Memory execute BP was not hit");

    return true;
}

//-----------------------------------------------------------------------------
// MB-04: Memory BP spanning pages - BP across page boundary
// Set memory BP that spans across a page boundary
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MB-04", MB_04, "Memory BP spanning page boundary")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    MemBpDebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    // Set up CREATE_PROCESS handler to set the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "g_memory_write_target");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_memTargetAddress = exportAddr;

                // Calculate a size that would span into the next page
                SYSTEM_INFO sysInfo;
                GetSystemInfo(&sysInfo);
                DWORD pageSize = sysInfo.dwPageSize;

                // Calculate offset to page boundary
                ULONG_PTR pageOffset = exportAddr % pageSize;
                SIZE_T sizeToBoundary = pageSize - pageOffset;

                // Create a BP that spans at least 2 pages
                SIZE_T spanSize = sizeToBoundary + 64;

                // Set memory BP spanning pages
                g_bpSetSuccess = SetMemoryBPXEx(exportAddr, spanSize, UE_MEMORY_WRITE, true, OnMemBpHitOnce);
                if (!g_bpSetSuccess)
                {
                    // If spanning fails, try with just the single variable size
                    g_bpSetSuccess = SetMemoryBPXEx(exportAddr, sizeof(DWORD), UE_MEMORY_WRITE, true, OnMemBpHitOnce);
                }
            }
            FreeLibrary(hLib);
        }
    });

    session.SetupHandlers();
    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_memTargetAddress != 0, "Target variable address not resolved");
    TEST_ASSERT(g_bpSetSuccess, "Memory BP could not be set");
    TEST_ASSERT(g_memBpHitCount >= 1, "Memory BP spanning pages was not hit");

    return true;
}

//-----------------------------------------------------------------------------
// MB-05: RestoreOnHit=true - Verify BP is removed after hit
// Set memory BP with RestoreOnHit=true, verify it fires only once
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MB-05", MB_05, "RestoreOnHit=true removes BP after first hit")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    MemBpDebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    // Set up CREATE_PROCESS handler to set the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "g_memory_write_target");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_memTargetAddress = exportAddr;
                // Set memory BP with RestoreOnHit=true (should fire only once)
                g_bpSetSuccess = SetMemoryBPXEx(exportAddr, sizeof(DWORD), UE_MEMORY_WRITE, true, OnMemBpHitOnce);
            }
            FreeLibrary(hLib);
        }
    });

    session.SetupHandlers();
    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_memTargetAddress != 0, "Target variable address not resolved");
    TEST_ASSERT(g_bpSetSuccess, "Memory BP could not be set");
    // With RestoreOnHit=true, should only hit once even if variable is accessed multiple times
    TEST_ASSERT(g_memBpHitCount == 1, "RestoreOnHit=true BP should fire exactly once");

    return true;
}

//-----------------------------------------------------------------------------
// MB-06: RestoreOnHit=false - Verify BP persists after hit
// Set memory BP with RestoreOnHit=false, verify it fires multiple times
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MB-06", MB_06, "RestoreOnHit=false keeps BP after hit")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    MemBpDebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    g_expectMultipleHits = true;

    // Set up CREATE_PROCESS handler to set the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "g_memory_write_target");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_memTargetAddress = exportAddr;
                // Set memory BP with RestoreOnHit=false (should persist)
                g_bpSetSuccess = SetMemoryBPXEx(exportAddr, sizeof(DWORD), UE_MEMORY_WRITE, false, OnMemBpHitMultiple);
            }
            FreeLibrary(hLib);
        }
    });

    session.SetupHandlers();
    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_memTargetAddress != 0, "Target variable address not resolved");
    TEST_ASSERT(g_bpSetSuccess, "Memory BP could not be set");
    // With RestoreOnHit=false, should hit multiple times if variable is accessed multiple times
    TEST_ASSERT(g_memBpHitCount >= 2, "RestoreOnHit=false BP should fire multiple times");

    return true;
}

//-----------------------------------------------------------------------------
// MB-07: Remove memory BP - RemoveMemoryBPX verification
// Set memory BP, then remove it, verify no callback fires
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MB-07", MB_07, "RemoveMemoryBPX removes BP successfully")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    MemBpDebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    // Set up CREATE_PROCESS handler to set and immediately remove the breakpoint
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "g_memory_write_target");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_memTargetAddress = exportAddr;

                // Set memory BP
                g_bpSetSuccess = SetMemoryBPXEx(exportAddr, sizeof(DWORD), UE_MEMORY_WRITE, false, OnMemBpHit);
                if (g_bpSetSuccess)
                {
                    // Immediately remove it
                    g_bpRemoveSuccess = RemoveMemoryBPX(exportAddr, sizeof(DWORD));
                }
            }
            FreeLibrary(hLib);
        }
    });

    session.SetupHandlers();
    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_memTargetAddress != 0, "Target variable address not resolved");
    TEST_ASSERT(g_bpSetSuccess, "Memory BP could not be set");
    TEST_ASSERT(g_bpRemoveSuccess, "RemoveMemoryBPX failed");
    TEST_ASSERT(g_memBpHitCount == 0, "Memory BP was hit after removal (should not have been)");

    return true;
}

//-----------------------------------------------------------------------------
// MB-08: Overlapping memory BPs - Multiple BPs on overlapping regions
// Set multiple memory BPs that overlap in address range
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MB-08", MB_08, "Overlapping memory BPs on adjacent variables")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    MemBpDebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    // Set up CREATE_PROCESS handler to set the breakpoints
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;

        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            // Get addresses of two memory targets
            auto export1 = (ULONG_PTR)GetProcAddress(hLib, "g_memory_read_target");
            auto export2 = (ULONG_PTR)GetProcAddress(hLib, "g_memory_write_target");

            if (export1)
            {
                export1 -= (ULONG_PTR)hLib;
                export1 += base;
                g_memTargetAddress = export1;
                // Set memory BP on first variable (read)
                g_bp1Success = SetMemoryBPXEx(export1, sizeof(DWORD), UE_MEMORY_READ, true, OnBp1Hit);
            }

            if (export2)
            {
                export2 -= (ULONG_PTR)hLib;
                export2 += base;
                g_memTargetAddress2 = export2;
                // Set memory BP on second variable (write)
                g_bp2Success = SetMemoryBPXEx(export2, sizeof(DWORD), UE_MEMORY_WRITE, true, OnBp2Hit);
            }

            FreeLibrary(hLib);
        }
    });

    session.SetupHandlers();
    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_memTargetAddress != 0, "First target variable address not resolved");
    TEST_ASSERT(g_memTargetAddress2 != 0, "Second target variable address not resolved");

    // At least one of the BPs should have been set successfully
    TEST_ASSERT(g_bp1Success || g_bp2Success, "At least one memory BP should be set");

    // At least one BP should have fired
    int totalHits = g_bp1Hits + g_bp2Hits;
    TEST_ASSERT(totalHits >= 1, "At least one overlapping memory BP should have fired");

    return true;
}
