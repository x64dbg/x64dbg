/**
 * Context Operation Tests (CX-01 through CX-10)
 *
 * Tests for TitanEngine context (register) manipulation functionality.
 * These tests use the TestExe_Context target executable which exports
 * functions for register manipulation testing.
 *
 * TitanEngine Context API:
 *   - GetContextDataEx(hThread, registerIndex) - Get register value
 *   - SetContextDataEx(hThread, registerIndex, value) - Set register value
 *   - GetFullContextDataEx(hProcess, TITAN_ENGINE_CONTEXT_t*) - Get full context
 *   - SetFullContextDataEx(hProcess, TITAN_ENGINE_CONTEXT_t*) - Set full context
 *
 * Register indices (partial list):
 *   - UE_CIP (0) - Instruction pointer (EIP/RIP)
 *   - UE_CSP (1) - Stack pointer (ESP/RSP)
 *   - UE_CFLAGS (2) - Flags register (EFLAGS/RFLAGS)
 *   - UE_EAX/UE_RAX, UE_EBX/UE_RBX, etc.
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

std::atomic<bool> g_systemBpHit{false};
std::atomic<bool> g_processExited{false};
std::atomic<bool> g_bpHit{false};

// For context tests
std::atomic<ULONG_PTR> g_savedCip{0};
std::atomic<ULONG_PTR> g_savedCsp{0};
std::atomic<ULONG_PTR> g_savedEax{0};
std::atomic<ULONG_PTR> g_savedEbx{0};
std::atomic<ULONG_PTR> g_savedEcx{0};
std::atomic<ULONG_PTR> g_savedEdx{0};
std::atomic<ULONG_PTR> g_savedEflags{0};

// For modification tests
std::atomic<bool> g_contextModified{false};
std::atomic<ULONG_PTR> g_modifiedValue{0};

// Reset all test state
void ResetTestState()
{
    g_systemBpHit = false;
    g_processExited = false;
    g_bpHit = false;
    g_savedCip = 0;
    g_savedCsp = 0;
    g_savedEax = 0;
    g_savedEbx = 0;
    g_savedEcx = 0;
    g_savedEdx = 0;
    g_savedEflags = 0;
    g_contextModified = false;
    g_modifiedValue = 0;
}

// Get the test executable path (uses framework helper with architecture suffix)
std::wstring GetTestExePath()
{
    return TitanTest::GetTestExePath(L"TestExe_Context");
}

// Get address of exported function from debuggee
ULONG_PTR GetExportAddress(HANDLE hProcess, ULONG_PTR moduleBase, const char* exportName)
{
    IMAGE_DOS_HEADER dosHeader;
    if (!MemoryReadSafe(hProcess, (LPVOID)moduleBase, &dosHeader, sizeof(dosHeader), nullptr))
        return 0;

    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

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

void OnProcessExited(const void* /*info*/)
{
    g_processExited = true;
}

void OnBpHit()
{
    TITAN_TRACK_CONTEXT();
    g_bpHit = true;
}

//-----------------------------------------------------------------------------
// Debug session helper
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
        SetCustomHandler(UE_CH_EXITPROCESS, OnProcessExited);
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
// CX-01: Get CIP (Instruction Pointer)
// Read current instruction pointer using GetContextDataEx
//-----------------------------------------------------------------------------
TITAN_TEST_ID("CX-01", CX_01, "Get instruction pointer (CIP)")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        HANDLE hThread = GetCurrentThread();
        ULONG_PTR cip = GetContextDataEx(hThread, UE_CIP);

        g_savedCip = cip;

        // CIP should be within the module's code section
        ULONG_PTR moduleBase = s_session->GetModuleBase();
        if (cip >= moduleBase && cip < moduleBase + 0x10000000)
        {
            // Valid CIP
        }
        else
        {
            // Still valid - could be in ntdll or kernel32
        }
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_savedCip != 0, "CIP should not be zero");

    return true;
}

//-----------------------------------------------------------------------------
// CX-02: Get CSP (Stack Pointer)
// Read current stack pointer using GetContextDataEx
//-----------------------------------------------------------------------------
TITAN_TEST_ID("CX-02", CX_02, "Get stack pointer (CSP)")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        HANDLE hThread = GetCurrentThread();
        ULONG_PTR csp = GetContextDataEx(hThread, UE_CSP);

        g_savedCsp = csp;
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_savedCsp != 0, "CSP should not be zero");

    // Stack pointer should be aligned (4-byte on x86, 8-byte on x64)
#ifdef _WIN64
    TEST_ASSERT((g_savedCsp & 0x7) == 0, "RSP should be 8-byte aligned");
#else
    TEST_ASSERT((g_savedCsp & 0x3) == 0, "ESP should be 4-byte aligned");
#endif

    return true;
}

//-----------------------------------------------------------------------------
// CX-03: Get General Purpose Registers
// Read EAX/RAX, EBX/RBX, ECX/RCX, EDX/RDX
//-----------------------------------------------------------------------------
TITAN_TEST_ID("CX-03", CX_03, "Get general purpose registers (EAX/RAX, EBX/RBX, ECX/RCX, EDX/RDX)")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        HANDLE hThread = GetCurrentThread();

#ifdef _WIN64
        g_savedEax = GetContextDataEx(hThread, UE_RAX);
        g_savedEbx = GetContextDataEx(hThread, UE_RBX);
        g_savedEcx = GetContextDataEx(hThread, UE_RCX);
        g_savedEdx = GetContextDataEx(hThread, UE_RDX);
#else
        g_savedEax = GetContextDataEx(hThread, UE_EAX);
        g_savedEbx = GetContextDataEx(hThread, UE_EBX);
        g_savedEcx = GetContextDataEx(hThread, UE_ECX);
        g_savedEdx = GetContextDataEx(hThread, UE_EDX);
#endif
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");

    // Just verify we can read the registers (values can be anything)
    // The fact that no exception occurred means success
    return true;
}

//-----------------------------------------------------------------------------
// CX-04: Get EFLAGS/RFLAGS
// Read flags register
//-----------------------------------------------------------------------------
TITAN_TEST_ID("CX-04", CX_04, "Get flags register (EFLAGS/RFLAGS)")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        HANDLE hThread = GetCurrentThread();
        g_savedEflags = GetContextDataEx(hThread, UE_CFLAGS);
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_savedEflags != 0, "EFLAGS should not be zero");

    // Reserved bit 1 should always be set
    TEST_ASSERT((g_savedEflags & 0x2) != 0, "EFLAGS bit 1 should be set (reserved)");

    return true;
}

//-----------------------------------------------------------------------------
// CX-05: Set CIP (Modify Instruction Pointer)
// Modify instruction pointer using SetContextDataEx
//-----------------------------------------------------------------------------
TITAN_TEST_ID("CX-05", CX_05, "Set instruction pointer (CIP)")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_targetAddr = 0;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Get target function address
        s_targetAddr = s_session->GetExport("context_test_target");
        if (!s_targetAddr)
        {
            StopDebug();
            return;
        }

        HANDLE hThread = GetCurrentThread();

        // Save original CIP
        g_savedCip = GetContextDataEx(hThread, UE_CIP);

        // Set breakpoint at target to verify we jump there
        SetBPX(s_targetAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, OnBpHit);

        // Modify CIP to jump to target function
        bool result = SetContextDataEx(hThread, UE_CIP, s_targetAddr);
        g_contextModified = result;
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(s_targetAddr != 0, "Target address not resolved");
    TEST_ASSERT(g_contextModified, "SetContextDataEx failed to modify CIP");
    TEST_ASSERT(g_bpHit, "Breakpoint at target was not hit - CIP modification failed");

    return true;
}

//-----------------------------------------------------------------------------
// CX-06: Set General Purpose Register
// Modify EAX/RAX using SetContextDataEx
//-----------------------------------------------------------------------------
TITAN_TEST_ID("CX-06", CX_06, "Set general purpose register (EAX/RAX)")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_testValue = 0xDEADBEEF;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        HANDLE hThread = GetCurrentThread();

        // Save original value
#ifdef _WIN64
        g_savedEax = GetContextDataEx(hThread, UE_RAX);
        bool result = SetContextDataEx(hThread, UE_RAX, s_testValue);
        g_contextModified = result;
        if (result)
        {
            g_modifiedValue = GetContextDataEx(hThread, UE_RAX);
        }
#else
        g_savedEax = GetContextDataEx(hThread, UE_EAX);
        bool result = SetContextDataEx(hThread, UE_EAX, s_testValue);
        g_contextModified = result;
        if (result)
        {
            g_modifiedValue = GetContextDataEx(hThread, UE_EAX);
        }
#endif
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_contextModified, "SetContextDataEx failed to modify register");
    TEST_ASSERT(g_modifiedValue == s_testValue, "Register value was not set correctly");

    return true;
}

//-----------------------------------------------------------------------------
// CX-07: Set EFLAGS
// Modify specific flags (e.g., Zero Flag, Carry Flag)
//-----------------------------------------------------------------------------
TITAN_TEST_ID("CX-07", CX_07, "Set flags register (modify ZF)")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static ULONG_PTR s_originalFlags = 0;
    static ULONG_PTR s_modifiedFlags = 0;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        HANDLE hThread = GetCurrentThread();

        // Get original flags
        s_originalFlags = GetContextDataEx(hThread, UE_CFLAGS);

        // Toggle Zero Flag (bit 6)
        ULONG_PTR newFlags = s_originalFlags ^ 0x40;  // Toggle ZF

        // Set modified flags
        bool result = SetContextDataEx(hThread, UE_CFLAGS, newFlags);
        g_contextModified = result;

        if (result)
        {
            s_modifiedFlags = GetContextDataEx(hThread, UE_CFLAGS);
        }
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_contextModified, "SetContextDataEx failed to modify flags");

    // Verify ZF was toggled
    bool originalZF = (s_originalFlags & 0x40) != 0;
    bool modifiedZF = (s_modifiedFlags & 0x40) != 0;
    TEST_ASSERT(originalZF != modifiedZF, "Zero Flag should have been toggled");

    return true;
}

//-----------------------------------------------------------------------------
// CX-08: Get/Set Context at Breakpoint
// Verify context operations work correctly at a breakpoint
//-----------------------------------------------------------------------------
TITAN_TEST_ID("CX-08", CX_08, "Get/Set context at breakpoint")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_bpAddress = 0;
    static ULONG_PTR s_contextAtBp = 0;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        s_bpAddress = s_session->GetExport("context_test_target");
        if (!s_bpAddress)
        {
            StopDebug();
            return;
        }

        SetBPX(s_bpAddress, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
            g_bpHit = true;

            HANDLE hThread = GetCurrentThread();

            // Read context at breakpoint
            s_contextAtBp = GetContextDataEx(hThread, UE_CIP);

            // Modify a register
#ifdef _WIN64
            g_contextModified = SetContextDataEx(hThread, UE_RAX, 0x12345678);
            g_modifiedValue = GetContextDataEx(hThread, UE_RAX);
#else
            g_contextModified = SetContextDataEx(hThread, UE_EAX, 0x12345678);
            g_modifiedValue = GetContextDataEx(hThread, UE_EAX);
#endif
        });
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(s_bpAddress != 0, "Target address not resolved");
    TEST_ASSERT(g_bpHit, "Breakpoint was not hit");
    TEST_ASSERT(s_contextAtBp == s_bpAddress, "CIP at breakpoint should match BP address");
    TEST_ASSERT(g_contextModified, "Failed to modify context at breakpoint");
    TEST_ASSERT(g_modifiedValue == 0x12345678, "Register modification at BP failed");

    return true;
}

//-----------------------------------------------------------------------------
// CX-09: Context Across Step Operations
// Verify context is correctly updated after single step
//-----------------------------------------------------------------------------
TITAN_TEST_ID("CX-09", CX_09, "Context updates after single step")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_cipBeforeStep = 0;
    static ULONG_PTR s_cipAfterStep = 0;
    static int s_stepCount = 0;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        ULONG_PTR targetAddr = s_session->GetExport("context_test_loop");
        if (!targetAddr)
        {
            StopDebug();
            return;
        }

        SetBPX(targetAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
            g_bpHit = true;

            HANDLE hThread = GetCurrentThread();
            s_cipBeforeStep = GetContextDataEx(hThread, UE_CIP);

            // Single step
            StepInto([]() {
                TITAN_TRACK_STEP();
                s_stepCount++;

                HANDLE hThread = GetCurrentThread();
                s_cipAfterStep = GetContextDataEx(hThread, UE_CIP);
            });
        });
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_bpHit, "Target breakpoint was not hit");
    TEST_ASSERT(s_stepCount >= 1, "Single step did not occur");
    TEST_ASSERT(s_cipBeforeStep != 0, "CIP before step should not be zero");
    TEST_ASSERT(s_cipAfterStep != 0, "CIP after step should not be zero");
    TEST_ASSERT(s_cipBeforeStep != s_cipAfterStep, "CIP should change after step");

    return true;
}

//-----------------------------------------------------------------------------
// CX-10: Get Debug Register Values
// Read DR0-DR7 debug registers
//-----------------------------------------------------------------------------
TITAN_TEST_ID("CX-10", CX_10, "Get debug register values (DR0-DR3, DR6, DR7)")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static ULONG_PTR s_dr0 = 0;
    static ULONG_PTR s_dr1 = 0;
    static ULONG_PTR s_dr2 = 0;
    static ULONG_PTR s_dr3 = 0;
    static ULONG_PTR s_dr6 = 0;
    static ULONG_PTR s_dr7 = 0;
    static bool s_drRead = false;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        HANDLE hThread = GetCurrentThread();

        // Read debug registers
        s_dr0 = GetContextDataEx(hThread, UE_DR0);
        s_dr1 = GetContextDataEx(hThread, UE_DR1);
        s_dr2 = GetContextDataEx(hThread, UE_DR2);
        s_dr3 = GetContextDataEx(hThread, UE_DR3);
        s_dr6 = GetContextDataEx(hThread, UE_DR6);
        s_dr7 = GetContextDataEx(hThread, UE_DR7);

        s_drRead = true;
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(s_drRead, "Debug registers were not read");

    // DR6 should have some default bits set
    // DR7 controls debug breakpoints - usually has some bits set
    // Just verify we could read them without error
    return true;
}
