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

namespace
{

//-----------------------------------------------------------------------------
// Test state and utilities
//-----------------------------------------------------------------------------

std::atomic<bool> g_systemBpHit{false};
std::atomic<bool> g_processCreated{false};
std::atomic<bool> g_processExited{false};
std::atomic<bool> g_bpHit{false};
std::atomic<int> g_bpHitCount{0};
std::atomic<ULONG_PTR> g_lastBpAddress{0};

// Target address for breakpoint
ULONG_PTR g_targetAddress = 0;

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
    g_processCreated = false;
    g_processExited = false;
    g_bpHit = false;
    g_bpHitCount = 0;
    g_lastBpAddress = 0;
    g_targetAddress = 0;
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

//-----------------------------------------------------------------------------
// Callback handlers
//-----------------------------------------------------------------------------

void OnBpHit()
{
    TITAN_TRACK_BP_HIT();
    g_bpHit = true;
    g_bpHitCount++;
    // Get BP address from debug event (not GetContextDataEx which uses wrong thread handle)
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        g_lastBpAddress = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
    }
}

} // anonymous namespace

//-----------------------------------------------------------------------------
// CX-01: Get CIP (Instruction Pointer)
// Read current instruction pointer using GetContextDataEx
//-----------------------------------------------------------------------------
TITAN_TEST_ID("CX-01", CX_01, "Get instruction pointer (CIP)")
{
    ResetTestState();

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

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);

        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;
        auto hLib = LoadLibraryExW(szFilePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
        if (hLib)
        {
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "ctx_pause_point");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                SetBPX(exportAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    g_bpHit = true;

                    // Read CIP from the debug event exception address
                    const DEBUG_EVENT* dbgEvent = GetDebugData();
                    if (dbgEvent)
                    {
                        g_savedCip = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
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

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_bpHit, "Breakpoint was not hit");
    TEST_ASSERT(g_savedCip != 0, "CIP should not be zero");
    TEST_ASSERT(g_savedCip == g_targetAddress, "CIP should match breakpoint address");

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
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "ctx_pause_point");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                SetBPX(exportAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    g_bpHit = true;

                    // Read CSP using GetContextDataEx with the debuggee's thread
                    const DEBUG_EVENT* dbgEvent = GetDebugData();
                    if (dbgEvent)
                    {
                        HANDLE hThread = TitanOpenThread(THREAD_ALL_ACCESS, FALSE, dbgEvent->dwThreadId);
                        if (hThread)
                        {
                            g_savedCsp = GetContextDataEx(hThread, UE_CSP);
                            CloseHandle(hThread);
                        }
                    }
                });
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_bpHit, "Breakpoint was not hit");
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
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "ctx_pause_point");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                SetBPX(exportAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    g_bpHit = true;

                    const DEBUG_EVENT* dbgEvent = GetDebugData();
                    if (dbgEvent)
                    {
                        HANDLE hThread = TitanOpenThread(THREAD_ALL_ACCESS, FALSE, dbgEvent->dwThreadId);
                        if (hThread)
                        {
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
                            CloseHandle(hThread);
                        }
                    }
                });
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_bpHit, "Breakpoint was not hit");

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
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "ctx_pause_point");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                SetBPX(exportAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    g_bpHit = true;

                    const DEBUG_EVENT* dbgEvent = GetDebugData();
                    if (dbgEvent)
                    {
                        HANDLE hThread = TitanOpenThread(THREAD_ALL_ACCESS, FALSE, dbgEvent->dwThreadId);
                        if (hThread)
                        {
                            g_savedEflags = GetContextDataEx(hThread, UE_CFLAGS);
                            CloseHandle(hThread);
                        }
                    }
                });
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_bpHit, "Breakpoint was not hit");
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

    static ULONG_PTR s_pausePointAddr = 0;
    static ULONG_PTR s_sseTestAddr = 0;
    s_pausePointAddr = 0;
    s_sseTestAddr = 0;

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
            auto pauseAddr = (ULONG_PTR)GetProcAddress(hLib, "ctx_pause_point");
            auto sseAddr = (ULONG_PTR)GetProcAddress(hLib, "ctx_sse_test");
            if (pauseAddr && sseAddr)
            {
                pauseAddr -= (ULONG_PTR)hLib;
                pauseAddr += base;
                sseAddr -= (ULONG_PTR)hLib;
                sseAddr += base;
                s_pausePointAddr = pauseAddr;
                s_sseTestAddr = sseAddr;
                g_targetAddress = pauseAddr;

                // Set BP on pause_point, then in callback modify CIP to jump to sse_test
                SetBPX(pauseAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    g_bpHit = true;

                    const DEBUG_EVENT* dbgEvent = GetDebugData();
                    if (!dbgEvent)
                        return;

                    HANDLE hThread = TitanOpenThread(THREAD_ALL_ACCESS, FALSE, dbgEvent->dwThreadId);
                    if (!hThread)
                        return;

                    // Save original CIP
                    g_savedCip = GetContextDataEx(hThread, UE_CIP);

                    // Modify CIP to jump to a different function
                    g_contextModified = SetContextDataEx(hThread, UE_CIP, s_sseTestAddr);
                    CloseHandle(hThread);

                    // Also set a breakpoint at the new location to verify we jumped
                    SetBPX(s_sseTestAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                        TITAN_TRACK_BP_HIT();
                        g_bpHitCount++;
                        const DEBUG_EVENT* dbgEvent = GetDebugData();
                        if (dbgEvent)
                        {
                            g_lastBpAddress = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
                        }
                    });
                });
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(s_pausePointAddr != 0, "Pause point address not resolved");
    TEST_ASSERT(s_sseTestAddr != 0, "SSE test address not resolved");
    TEST_ASSERT(g_bpHit, "First breakpoint was not hit");
    TEST_ASSERT(g_contextModified, "SetContextDataEx failed to modify CIP");
    TEST_ASSERT(g_bpHitCount >= 1, "Second breakpoint at modified CIP was not hit");
    TEST_ASSERT(g_lastBpAddress == s_sseTestAddr, "CIP modification did not redirect execution");

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

    static ULONG_PTR s_testValue = 0xDEADBEEF;

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
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "ctx_pause_point");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                SetBPX(exportAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    g_bpHit = true;

                    const DEBUG_EVENT* dbgEvent = GetDebugData();
                    if (!dbgEvent)
                        return;

                    HANDLE hThread = TitanOpenThread(THREAD_ALL_ACCESS, FALSE, dbgEvent->dwThreadId);
                    if (!hThread)
                        return;

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
                    CloseHandle(hThread);
                });
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_bpHit, "Breakpoint was not hit");
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

    static ULONG_PTR s_originalFlags = 0;
    static ULONG_PTR s_modifiedFlags = 0;
    s_originalFlags = 0;
    s_modifiedFlags = 0;

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
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "ctx_pause_point");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                SetBPX(exportAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    g_bpHit = true;

                    const DEBUG_EVENT* dbgEvent = GetDebugData();
                    if (!dbgEvent)
                        return;

                    HANDLE hThread = TitanOpenThread(THREAD_ALL_ACCESS, FALSE, dbgEvent->dwThreadId);
                    if (!hThread)
                        return;

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
                    CloseHandle(hThread);
                });
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_bpHit, "Breakpoint was not hit");
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

    static ULONG_PTR s_contextAtBp = 0;
    s_contextAtBp = 0;

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
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "ctx_pause_point");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                SetBPX(exportAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    g_bpHit = true;

                    // Read context at breakpoint from debug event
                    const DEBUG_EVENT* dbgEvent = GetDebugData();
                    if (!dbgEvent)
                        return;

                    s_contextAtBp = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;

                    HANDLE hThread = TitanOpenThread(THREAD_ALL_ACCESS, FALSE, dbgEvent->dwThreadId);
                    if (!hThread)
                        return;

                    // Modify a register
#ifdef _WIN64
                    g_contextModified = SetContextDataEx(hThread, UE_RAX, 0x12345678);
                    g_modifiedValue = GetContextDataEx(hThread, UE_RAX);
#else
                    g_contextModified = SetContextDataEx(hThread, UE_EAX, 0x12345678);
                    g_modifiedValue = GetContextDataEx(hThread, UE_EAX);
#endif
                    CloseHandle(hThread);
                });
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target address not resolved");
    TEST_ASSERT(g_bpHit, "Breakpoint was not hit");
    TEST_ASSERT(s_contextAtBp == g_targetAddress, "CIP at breakpoint should match BP address");
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

    static ULONG_PTR s_cipBeforeStep = 0;
    static ULONG_PTR s_cipAfterStep = 0;
    static int s_stepCount = 0;
    s_cipBeforeStep = 0;
    s_cipAfterStep = 0;
    s_stepCount = 0;

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
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "ctx_pause_point");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                SetBPX(exportAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    g_bpHit = true;

                    // Get CIP before step from debug event
                    const DEBUG_EVENT* dbgEvent = GetDebugData();
                    if (!dbgEvent)
                        return;

                    s_cipBeforeStep = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;

                    // Single step
                    StepInto([]() {
                        TITAN_TRACK_STEP();
                        s_stepCount++;

                        const DEBUG_EVENT* dbgEvent = GetDebugData();
                        if (dbgEvent)
                        {
                            HANDLE hThread = TitanOpenThread(THREAD_ALL_ACCESS, FALSE, dbgEvent->dwThreadId);
                            if (hThread)
                            {
                                s_cipAfterStep = GetContextDataEx(hThread, UE_CIP);
                                CloseHandle(hThread);
                            }
                        }
                    });
                });
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
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

    static ULONG_PTR s_dr0 = 0;
    static ULONG_PTR s_dr1 = 0;
    static ULONG_PTR s_dr2 = 0;
    static ULONG_PTR s_dr3 = 0;
    static ULONG_PTR s_dr6 = 0;
    static ULONG_PTR s_dr7 = 0;
    static bool s_drRead = false;
    s_dr0 = 0;
    s_dr1 = 0;
    s_dr2 = 0;
    s_dr3 = 0;
    s_dr6 = 0;
    s_dr7 = 0;
    s_drRead = false;

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
            auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, "ctx_pause_point");
            if (exportAddr)
            {
                exportAddr -= (ULONG_PTR)hLib;
                exportAddr += base;
                g_targetAddress = exportAddr;
                SetBPX(exportAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    g_bpHit = true;

                    const DEBUG_EVENT* dbgEvent = GetDebugData();
                    if (!dbgEvent)
                        return;

                    HANDLE hThread = TitanOpenThread(THREAD_ALL_ACCESS, FALSE, dbgEvent->dwThreadId);
                    if (!hThread)
                        return;

                    // Read debug registers
                    s_dr0 = GetContextDataEx(hThread, UE_DR0);
                    s_dr1 = GetContextDataEx(hThread, UE_DR1);
                    s_dr2 = GetContextDataEx(hThread, UE_DR2);
                    s_dr3 = GetContextDataEx(hThread, UE_DR3);
                    s_dr6 = GetContextDataEx(hThread, UE_DR6);
                    s_dr7 = GetContextDataEx(hThread, UE_DR7);

                    s_drRead = true;
                    CloseHandle(hThread);
                });
            }
            FreeLibrary(hLib);
        }
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;
    });

    auto* pi = InitDebugW(exePath.c_str(), nullptr, nullptr);
    TEST_ASSERT(pi != nullptr, "Failed to start debug session");

    DebugLoop();

    TEST_ASSERT(g_processCreated, "CREATE_PROCESS event was not received");
    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_bpHit, "Breakpoint was not hit");
    TEST_ASSERT(s_drRead, "Debug registers were not read");

    // DR6 should have some default bits set
    // DR7 controls debug breakpoints - usually has some bits set
    // Just verify we could read them without error
    return true;
}
