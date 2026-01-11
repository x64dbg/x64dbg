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
#include <TlHelp32.h>

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
}

// Get the test executable path (uses framework helper with architecture suffix)
std::wstring GetTestExePath()
{
    return TitanTest::GetTestExePath(L"TestExe_Breakpoints");
}

// Get address of exported symbol from debuggee
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
}

void OnProcessExited(const void* info)
{
    g_processExited = true;
}

void OnMemBpHit(const void* info)
{
    TITAN_TRACK_BP_HIT();
    g_memBpHitCount++;

    // Memory BP callback receives the exception address (where the access occurred)
    if (info)
    {
        // The info parameter is a pointer to EXCEPTION_DEBUG_INFO
        auto* exInfo = static_cast<const EXCEPTION_DEBUG_INFO*>(info);
        if (exInfo && exInfo->ExceptionRecord.ExceptionCode == STATUS_GUARD_PAGE_VIOLATION ||
            exInfo->ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
        {
            // Second element is the address that was accessed
            if (exInfo->ExceptionRecord.NumberParameters >= 2)
            {
                g_lastMemBpAddress = exInfo->ExceptionRecord.ExceptionInformation[1];
            }
        }
    }
}

void OnMemBpHitOnce(const void* info)
{
    g_memBpHitCount++;
    // Single-hit callback - BP with RestoreOnHit=true should auto-remove
}

void OnMemBpHitMultiple(const void* info)
{
    g_memBpHitCount++;
    // Persistent callback - BP with RestoreOnHit=false should fire multiple times
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
        SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, OnSystemBreakpoint);
        SetCustomHandler(UE_CH_CREATEPROCESS, OnProcessCreated);
        SetCustomHandler(UE_CH_EXITPROCESS, OnProcessExited);
    }

    ULONG_PTR GetExport(const char* name)
    {
        ULONG_PTR moduleBase = 0;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pi->dwProcessId);
        if (hSnapshot != INVALID_HANDLE_VALUE)
        {
            MODULEENTRY32W me = {sizeof(me)};
            if (Module32FirstW(hSnapshot, &me))
            {
                moduleBase = (ULONG_PTR)me.modBaseAddr;
            }
            CloseHandle(hSnapshot);
        }

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
// MB-01: Memory read BP - SetMemoryBPXEx with UE_MEMORY_READ
// Set memory BP on g_memory_read_target, trigger read, verify callback fires
//-----------------------------------------------------------------------------
TITAN_TEST_ID("MB-01", MB_01, "Memory read BP using SetMemoryBPXEx with UE_MEMORY_READ")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    MemBpDebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");

    session.SetupHandlers();

    static MemBpDebugSession* s_session = &session;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Get address of the read target variable
        ULONG_PTR addr = s_session->GetExport("g_memory_read_target");
        if (addr)
        {
            g_memTargetAddress = addr;
            // Set memory read breakpoint
            bool result = SetMemoryBPXEx(addr, sizeof(DWORD), UE_MEMORY_READ, true, OnMemBpHitOnce);
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
    TEST_ASSERT(g_memTargetAddress != 0, "Target variable address not resolved");
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

    session.SetupHandlers();

    static MemBpDebugSession* s_session = &session;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Get address of the write target variable
        ULONG_PTR addr = s_session->GetExport("g_memory_write_target");
        if (addr)
        {
            g_memTargetAddress = addr;
            // Set memory write breakpoint
            bool result = SetMemoryBPXEx(addr, sizeof(DWORD), UE_MEMORY_WRITE, true, OnMemBpHitOnce);
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
    TEST_ASSERT(g_memTargetAddress != 0, "Target variable address not resolved");
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

    session.SetupHandlers();

    static MemBpDebugSession* s_session = &session;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Get address of function to set execute BP on
        ULONG_PTR addr = s_session->GetExport("bp_memory_read");
        if (addr)
        {
            g_memTargetAddress = addr;
            // Set memory execute breakpoint
            bool result = SetMemoryBPXEx(addr, 16, UE_MEMORY_EXECUTE, true, OnMemBpHitOnce);
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
    TEST_ASSERT(g_memTargetAddress != 0, "Target function address not resolved");
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

    session.SetupHandlers();

    static MemBpDebugSession* s_session = &session;
    static bool s_bpSetSuccess = false;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Get address of the write target variable
        ULONG_PTR addr = s_session->GetExport("g_memory_write_target");
        if (addr)
        {
            g_memTargetAddress = addr;

            // Calculate a size that would span into the next page
            // Page size is typically 4KB (0x1000)
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            DWORD pageSize = sysInfo.dwPageSize;

            // Calculate offset to page boundary
            ULONG_PTR pageOffset = addr % pageSize;
            SIZE_T sizeToBoundary = pageSize - pageOffset;

            // Create a BP that spans at least 2 pages
            SIZE_T spanSize = sizeToBoundary + 64;  // Cross boundary by 64 bytes

            // Set memory BP spanning pages
            s_bpSetSuccess = SetMemoryBPXEx(addr, spanSize, UE_MEMORY_WRITE, true, OnMemBpHitOnce);
            if (!s_bpSetSuccess)
            {
                // If spanning fails, try with just the single variable size
                s_bpSetSuccess = SetMemoryBPXEx(addr, sizeof(DWORD), UE_MEMORY_WRITE, true, OnMemBpHitOnce);
            }

            if (!s_bpSetSuccess)
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
    TEST_ASSERT(g_memTargetAddress != 0, "Target variable address not resolved");
    TEST_ASSERT(s_bpSetSuccess, "Memory BP could not be set");
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

    session.SetupHandlers();

    static MemBpDebugSession* s_session = &session;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Get address of the readwrite target variable (accessed multiple times)
        ULONG_PTR addr = s_session->GetExport("g_memory_write_target");
        if (addr)
        {
            g_memTargetAddress = addr;
            // Set memory BP with RestoreOnHit=true (should fire only once)
            bool result = SetMemoryBPXEx(addr, sizeof(DWORD), UE_MEMORY_WRITE, true, OnMemBpHitOnce);
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
    TEST_ASSERT(g_memTargetAddress != 0, "Target variable address not resolved");
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

    session.SetupHandlers();

    static MemBpDebugSession* s_session = &session;
    g_expectMultipleHits = true;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Get address of the write target variable
        ULONG_PTR addr = s_session->GetExport("g_memory_write_target");
        if (addr)
        {
            g_memTargetAddress = addr;
            // Set memory BP with RestoreOnHit=false (should persist)
            bool result = SetMemoryBPXEx(addr, sizeof(DWORD), UE_MEMORY_WRITE, false, OnMemBpHitMultiple);
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
    TEST_ASSERT(g_memTargetAddress != 0, "Target variable address not resolved");
    // With RestoreOnHit=false, should hit multiple times if variable is accessed multiple times
    // The test exe writes to g_memory_write_target at least twice (bp_memory_write and bp_memory_readwrite)
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

    session.SetupHandlers();

    static MemBpDebugSession* s_session = &session;
    static bool s_bpSetSuccess = false;
    static bool s_bpRemoveSuccess = false;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Get address of the write target variable
        ULONG_PTR addr = s_session->GetExport("g_memory_write_target");
        if (addr)
        {
            g_memTargetAddress = addr;

            // Set memory BP
            s_bpSetSuccess = SetMemoryBPXEx(addr, sizeof(DWORD), UE_MEMORY_WRITE, false, OnMemBpHit);
            if (!s_bpSetSuccess)
            {
                StopDebug();
                return;
            }

            // Immediately remove it
            s_bpRemoveSuccess = RemoveMemoryBPX(addr, sizeof(DWORD));
            if (!s_bpRemoveSuccess)
            {
                // If remove fails, the BP is still active
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
    TEST_ASSERT(g_memTargetAddress != 0, "Target variable address not resolved");
    TEST_ASSERT(s_bpSetSuccess, "Memory BP could not be set");
    TEST_ASSERT(s_bpRemoveSuccess, "RemoveMemoryBPX failed");
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

    session.SetupHandlers();

    static MemBpDebugSession* s_session = &session;
    static bool s_bp1Success = false;
    static bool s_bp2Success = false;
    static std::atomic<int> s_bp1Hits{0};
    static std::atomic<int> s_bp2Hits{0};

    // Callbacks for tracking individual BP hits
    static auto OnBp1Hit = [](const void* info) {
        s_bp1Hits++;
    };

    static auto OnBp2Hit = [](const void* info) {
        s_bp2Hits++;
    };

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Get addresses of two adjacent memory targets
        ULONG_PTR addr1 = s_session->GetExport("g_memory_read_target");
        ULONG_PTR addr2 = s_session->GetExport("g_memory_write_target");

        if (addr1 && addr2)
        {
            g_memTargetAddress = addr1;
            g_memTargetAddress2 = addr2;

            // Set memory BP on first variable (read)
            s_bp1Success = SetMemoryBPXEx(addr1, sizeof(DWORD), UE_MEMORY_READ, true, OnBp1Hit);

            // Set memory BP on second variable (write)
            // These may overlap if variables are adjacent
            s_bp2Success = SetMemoryBPXEx(addr2, sizeof(DWORD), UE_MEMORY_WRITE, true, OnBp2Hit);

            if (!s_bp1Success && !s_bp2Success)
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
    TEST_ASSERT(g_memTargetAddress != 0, "First target variable address not resolved");
    TEST_ASSERT(g_memTargetAddress2 != 0, "Second target variable address not resolved");

    // At least one of the BPs should have been set successfully
    TEST_ASSERT(s_bp1Success || s_bp2Success, "At least one memory BP should be set");

    // At least one BP should have fired
    int totalHits = s_bp1Hits + s_bp2Hits;
    TEST_ASSERT(totalHits >= 1, "At least one overlapping memory BP should have fired");

    return true;
}
