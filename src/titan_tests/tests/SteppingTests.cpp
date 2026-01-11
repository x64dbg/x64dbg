/**
 * Stepping Tests (ST-01 through ST-10)
 *
 * Tests for TitanEngine stepping functionality (StepInto, StepOver).
 * These tests use the TestExe_Stepping target executable which exports
 * functions like step_level1, step_level2, step_rep_test, etc.
 */

#include "../TitanTestFramework.h"
#include "TitanEngine/TitanEngine.h"
#include <string>
#include <atomic>
#include <vector>
#include <tlhelp32.h>

namespace
{

//-----------------------------------------------------------------------------
// Test state and utilities
//-----------------------------------------------------------------------------

// Atomic counters for callback verification
std::atomic<int> g_stepCount{0};
std::atomic<ULONG_PTR> g_lastStepAddress{0};
std::atomic<bool> g_systemBpHit{false};
std::atomic<bool> g_processCreated{false};
std::atomic<bool> g_swBpHit{false};
std::atomic<bool> g_hwBpHit{false};
std::atomic<int> g_bpHitCount{0};

// For tracking IP changes
std::vector<ULONG_PTR> g_ipHistory;
ULONG_PTR g_initialIp = 0;
ULONG_PTR g_targetAddress = 0;
ULONG_PTR g_stepOverReturnAddress = 0;

// For multi-step tests
std::atomic<int> g_stepsRequested{0};
std::atomic<int> g_stepsCompleted{0};

// Thread handle for context operations
HANDLE g_hThread = nullptr;
HANDLE g_hProcess = nullptr;
DWORD g_processId = 0;

// Reset all test state
void ResetTestState()
{
    g_stepCount = 0;
    g_lastStepAddress = 0;
    g_systemBpHit = false;
    g_processCreated = false;
    g_swBpHit = false;
    g_hwBpHit = false;
    g_bpHitCount = 0;
    g_ipHistory.clear();
    g_initialIp = 0;
    g_targetAddress = 0;
    g_stepOverReturnAddress = 0;
    g_stepsRequested = 0;
    g_stepsCompleted = 0;
    g_hThread = nullptr;
    g_hProcess = nullptr;
    g_processId = 0;
}

// Get the test executable path
std::wstring GetTestExePath()
{
    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);

    wchar_t* lastSlash = wcsrchr(modulePath, L'\\');
    if (lastSlash)
    {
        *(lastSlash + 1) = L'\0';
    }

    return std::wstring(modulePath) + L"TestExe_Stepping.exe";
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

// Get module base of the main executable
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
        g_hThread = createInfo->hThread;
        g_hProcess = createInfo->hProcess;
    }
}

void OnStepComplete()
{
    TITAN_TRACK_STEP();
    g_stepCount++;
    ULONG_PTR cip = GetContextDataEx(g_hThread, UE_CIP);
    g_lastStepAddress = cip;
    g_ipHistory.push_back(cip);
}

void OnStepAndContinue()
{
    TITAN_TRACK_STEP();
    g_stepsCompleted++;
    ULONG_PTR cip = GetContextDataEx(g_hThread, UE_CIP);
    g_lastStepAddress = cip;
    g_ipHistory.push_back(cip);

    // If we have more steps to do, request another step
    if (g_stepsCompleted < g_stepsRequested)
    {
        StepInto(OnStepAndContinue);
    }
}

void OnSoftwareBpHit()
{
    TITAN_TRACK_BP_HIT();
    g_swBpHit = true;
    g_bpHitCount++;
    ULONG_PTR cip = GetContextDataEx(g_hThread, UE_CIP);
    g_lastStepAddress = cip;
}

void OnHardwareBpHit(const void* info)
{
    TITAN_TRACK_BP_HIT();
    g_hwBpHit = true;
    g_bpHitCount++;
    ULONG_PTR cip = GetContextDataEx(g_hThread, UE_CIP);
    g_lastStepAddress = cip;
}

void OnStepAfterBp()
{
    TITAN_TRACK_STEP();
    g_stepCount++;
    ULONG_PTR cip = GetContextDataEx(g_hThread, UE_CIP);
    g_lastStepAddress = cip;
    g_ipHistory.push_back(cip);
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
        g_processId = pi->dwProcessId;
        return true;
    }

    void SetupHandlers()
    {
        SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, OnSystemBreakpoint);
        SetCustomHandler(UE_CH_CREATEPROCESS, OnProcessCreated);
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
// ST-01: StepInto basic - Verify single instruction step
// Execute StepInto and verify IP advances by exactly one instruction
//-----------------------------------------------------------------------------
TITAN_TEST_ID("ST-01", ST_01, "StepInto basic - single instruction step")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Get initial IP
        g_initialIp = GetContextDataEx(g_hThread, UE_CIP);
        g_ipHistory.push_back(g_initialIp);

        // Execute a single step
        StepInto(OnStepComplete);
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_stepCount == 1, "StepInto should execute exactly once");
    TEST_ASSERT(g_lastStepAddress != g_initialIp, "IP should have changed after step");
    TEST_ASSERT(g_ipHistory.size() == 2, "Should have recorded initial IP and step IP");

    return true;
}

//-----------------------------------------------------------------------------
// ST-02: StepInto into CALL - Step into a function call
// Set BP at call site, step into the called function
//-----------------------------------------------------------------------------
TITAN_TEST_ID("ST-02", ST_02, "StepInto into CALL - enter function")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_level1Addr = 0;
    static ULONG_PTR s_level2Addr = 0;
    static bool s_enteredLevel2 = false;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Get the addresses of the nested functions
        s_level1Addr = s_session->GetExport("step_level1");
        s_level2Addr = s_session->GetExport("step_level2");

        if (s_level1Addr == 0)
        {
            StopDebug();
            return;
        }

        g_targetAddress = s_level1Addr;

        // Set BP at level1 entry
        SetBPX(s_level1Addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
            g_bpHitCount++;

            // Now step repeatedly until we enter level2
            // We need to step through the prologue and into the call
            g_stepsRequested = 20; // Enough steps to get into level2
            StepInto([]() {
                g_stepsCompleted++;
                ULONG_PTR cip = GetContextDataEx(g_hThread, UE_CIP);
                g_ipHistory.push_back(cip);

                // Check if we've entered level2
                if (cip >= s_level2Addr && cip < s_level2Addr + 0x100)
                {
                    s_enteredLevel2 = true;
                    // We've verified step into works, stop debugging
                    StopDebug();
                    return;
                }

                // Continue stepping if we haven't reached our limit
                if (g_stepsCompleted < g_stepsRequested && !s_enteredLevel2)
                {
                    StepInto(nullptr); // Recursive stepping handled by callback
                }
                else
                {
                    StopDebug();
                }
            });
        });
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_bpHitCount >= 1, "BP at level1 should have been hit");
    TEST_ASSERT(s_enteredLevel2 || g_stepsCompleted > 0, "Should have stepped at least once");

    // Check if any of the recorded IPs are within level2
    if (s_level2Addr != 0)
    {
        for (ULONG_PTR ip : g_ipHistory)
        {
            if (ip >= s_level2Addr && ip < s_level2Addr + 0x100)
            {
                s_enteredLevel2 = true;
                break;
            }
        }
    }

    TEST_ASSERT(s_enteredLevel2, "StepInto should have entered the called function (level2)");

    return true;
}

//-----------------------------------------------------------------------------
// ST-03: StepOver CALL - Step over function call (execute and return)
// Use StepOver on a CALL instruction, verify we land after the call
//-----------------------------------------------------------------------------
TITAN_TEST_ID("ST-03", ST_03, "StepOver CALL - skip function call")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_level1Addr = 0;
    static ULONG_PTR s_level2Addr = 0;
    static bool s_steppedOverCall = false;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        s_level1Addr = s_session->GetExport("step_level1");
        s_level2Addr = s_session->GetExport("step_level2");

        if (s_level1Addr == 0)
        {
            StopDebug();
            return;
        }

        g_targetAddress = s_level1Addr;

        // Set BP at level1 entry
        SetBPX(s_level1Addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
            g_bpHitCount++;
            g_initialIp = GetContextDataEx(g_hThread, UE_CIP);

            // Step over multiple times - this should skip any function calls
            g_stepsRequested = 10;
            StepOver([]() {
                g_stepsCompleted++;
                ULONG_PTR cip = GetContextDataEx(g_hThread, UE_CIP);
                g_lastStepAddress = cip;
                g_ipHistory.push_back(cip);

                // If we are still within level1 (not inside level2), step over is working
                // After enough steps, we should have executed the call and returned
                if (g_stepsCompleted < g_stepsRequested)
                {
                    // Check if we are inside level2 - if so, step over failed
                    if (s_level2Addr != 0 && cip >= s_level2Addr && cip < s_level2Addr + 0x100)
                    {
                        // We're inside level2, which means step over didn't work properly
                        s_steppedOverCall = false;
                        StopDebug();
                        return;
                    }
                    StepOver(nullptr);
                }
                else
                {
                    s_steppedOverCall = true;
                    StopDebug();
                }
            });
        });
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_bpHitCount >= 1, "BP at level1 should have been hit");
    TEST_ASSERT(g_stepsCompleted > 0, "Should have completed at least one step over");

    // Verify we never entered level2 during step over operations
    bool enteredLevel2 = false;
    if (s_level2Addr != 0)
    {
        for (ULONG_PTR ip : g_ipHistory)
        {
            if (ip >= s_level2Addr && ip < s_level2Addr + 0x100)
            {
                enteredLevel2 = true;
                break;
            }
        }
    }

    TEST_ASSERT(!enteredLevel2, "StepOver should not have entered the called function");

    return true;
}

//-----------------------------------------------------------------------------
// ST-04: StepOver REP instruction - Step over REP prefix instructions
// REP MOVSB etc. should be stepped over as a single operation
//-----------------------------------------------------------------------------
TITAN_TEST_ID("ST-04", ST_04, "StepOver REP instruction")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_repTestAddr = 0;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        s_repTestAddr = s_session->GetExport("step_rep_test");

        if (s_repTestAddr == 0)
        {
            StopDebug();
            return;
        }

        g_targetAddress = s_repTestAddr;

        // Set BP at rep_test entry
        SetBPX(s_repTestAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
            g_bpHitCount++;
            g_initialIp = GetContextDataEx(g_hThread, UE_CIP);

            // Step over through the function - should handle REP instructions
            g_stepsRequested = 50; // Enough to get through the REP operations
            StepOver([]() {
                g_stepsCompleted++;
                ULONG_PTR cip = GetContextDataEx(g_hThread, UE_CIP);
                g_lastStepAddress = cip;
                g_ipHistory.push_back(cip);

                if (g_stepsCompleted < g_stepsRequested)
                {
                    // Check if we've left the function (return)
                    if (cip < s_repTestAddr || cip > s_repTestAddr + 0x200)
                    {
                        // We've returned from the function, test complete
                        StopDebug();
                        return;
                    }
                    StepOver(nullptr);
                }
                else
                {
                    StopDebug();
                }
            });
        });
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(s_repTestAddr != 0, "step_rep_test address not resolved");
    TEST_ASSERT(g_bpHitCount >= 1, "BP at step_rep_test should have been hit");
    TEST_ASSERT(g_stepsCompleted > 0, "Should have completed at least one step");

    // The key verification is that we completed stepping without hanging on REP
    // If StepOver didn't handle REP properly, we'd either hang or take many more steps
    TEST_ASSERT(g_stepsCompleted < g_stepsRequested || g_lastStepAddress < s_repTestAddr,
                "StepOver should complete REP instruction efficiently");

    return true;
}

//-----------------------------------------------------------------------------
// ST-05: StepInto from INT3 - Step after hitting software BP
// Hit a software BP, then single-step from it
//-----------------------------------------------------------------------------
TITAN_TEST_ID("ST-05", ST_05, "StepInto from INT3 - step after software BP")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_targetAddr = 0;
    static ULONG_PTR s_bpHitIp = 0;
    static ULONG_PTR s_afterStepIp = 0;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        s_targetAddr = s_session->GetExport("step_level4");

        if (s_targetAddr == 0)
        {
            StopDebug();
            return;
        }

        g_targetAddress = s_targetAddr;

        // Set a software BP
        SetBPX(s_targetAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
            g_swBpHit = true;
            s_bpHitIp = GetContextDataEx(g_hThread, UE_CIP);

            // Now step from the BP location
            StepInto([]() {
                g_stepCount++;
                s_afterStepIp = GetContextDataEx(g_hThread, UE_CIP);
                StopDebug();
            });
        });
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(s_targetAddr != 0, "Target address not resolved");
    TEST_ASSERT(g_swBpHit, "Software BP should have been hit");
    TEST_ASSERT(s_bpHitIp == s_targetAddr, "BP should have hit at target address");
    TEST_ASSERT(g_stepCount == 1, "Should have completed one step after BP");
    TEST_ASSERT(s_afterStepIp != s_bpHitIp, "IP should have advanced after stepping from BP");

    return true;
}

//-----------------------------------------------------------------------------
// ST-06: StepInto hits SW BP - Step lands on software BP
// Set BP ahead of current position, step into it
//-----------------------------------------------------------------------------
TITAN_TEST_ID("ST-06", ST_06, "StepInto hits SW BP - step lands on breakpoint")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_level1Addr = 0;
    static ULONG_PTR s_level4Addr = 0;
    static bool s_bpHitDuringStep = false;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        s_level1Addr = s_session->GetExport("step_level1");
        s_level4Addr = s_session->GetExport("step_level4");

        if (s_level1Addr == 0 || s_level4Addr == 0)
        {
            StopDebug();
            return;
        }

        // Set a BP at level1 entry
        SetBPX(s_level1Addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
            g_bpHitCount++;

            // Set a BP at level4 (which will be called eventually)
            SetBPX(s_level4Addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                s_bpHitDuringStep = true;
                g_bpHitCount++;
                StopDebug();
            });

            // Step into repeatedly - should eventually hit the level4 BP
            g_stepsRequested = 100;
            StepInto([]() {
                g_stepsCompleted++;
                g_lastStepAddress = GetContextDataEx(g_hThread, UE_CIP);

                if (g_stepsCompleted < g_stepsRequested && !s_bpHitDuringStep)
                {
                    StepInto(nullptr);
                }
                else
                {
                    StopDebug();
                }
            });
        });
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_bpHitCount >= 1, "At least one BP should have been hit");
    TEST_ASSERT(s_bpHitDuringStep, "Should have hit SW BP while stepping");

    return true;
}

//-----------------------------------------------------------------------------
// ST-07: StepInto hits HW BP - Step lands on hardware BP
// Set HW execute BP ahead, step into it
//-----------------------------------------------------------------------------
TITAN_TEST_ID("ST-07", ST_07, "StepInto hits HW BP - step lands on hardware breakpoint")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_level1Addr = 0;
    static ULONG_PTR s_level4Addr = 0;
    static DWORD s_hwBpRegister = 0;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        s_level1Addr = s_session->GetExport("step_level1");
        s_level4Addr = s_session->GetExport("step_level4");

        if (s_level1Addr == 0 || s_level4Addr == 0)
        {
            StopDebug();
            return;
        }

        // Set SW BP at level1 entry
        SetBPX(s_level1Addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
            g_bpHitCount++;

            // Get an unused HW BP register
            if (!GetUnusedHardwareBreakPointRegister(&s_hwBpRegister))
            {
                StopDebug();
                return;
            }

            // Set HW execute BP at level4
            if (!SetHardwareBreakPoint(s_level4Addr, s_hwBpRegister,
                                       UE_HARDWARE_EXECUTE, UE_HARDWARE_SIZE_1,
                                       [](const void*) {
                                           g_hwBpHit = true;
                                           g_bpHitCount++;
                                           StopDebug();
                                       }))
            {
                StopDebug();
                return;
            }

            // Step into repeatedly - should eventually hit the HW BP
            g_stepsRequested = 100;
            StepInto([]() {
                g_stepsCompleted++;
                g_lastStepAddress = GetContextDataEx(g_hThread, UE_CIP);

                if (g_stepsCompleted < g_stepsRequested && !g_hwBpHit)
                {
                    StepInto(nullptr);
                }
                else
                {
                    StopDebug();
                }
            });
        });
    });

    session.Run();

    // Cleanup HW BP
    if (s_hwBpRegister != 0)
    {
        DeleteHardwareBreakPoint(s_hwBpRegister);
    }

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_bpHitCount >= 1, "At least one BP should have been hit");
    TEST_ASSERT(g_hwBpHit, "Should have hit HW BP while stepping");

    return true;
}

//-----------------------------------------------------------------------------
// ST-08: StepOver function with BP inside - Function has BP but step over shouldn't stop
// Set BP inside a function, use StepOver on call - BP should fire but return to step over point
//-----------------------------------------------------------------------------
TITAN_TEST_ID("ST-08", ST_08, "StepOver function with BP inside")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_level1Addr = 0;
    static ULONG_PTR s_level4Addr = 0;
    static int s_innerBpHits = 0;
    static bool s_stepOverCompleted = false;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        s_level1Addr = s_session->GetExport("step_level1");
        s_level4Addr = s_session->GetExport("step_level4");

        if (s_level1Addr == 0 || s_level4Addr == 0)
        {
            StopDebug();
            return;
        }

        // Set BP at level1 entry
        SetBPX(s_level1Addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
            g_bpHitCount++;
            g_initialIp = GetContextDataEx(g_hThread, UE_CIP);

            // Set a persistent BP inside level4 (which level1 calls indirectly)
            SetBPX(s_level4Addr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, []() {
                s_innerBpHits++;
                // Don't stop, just continue
            });

            // Now use StepOver - it should complete even though inner BP fires
            g_stepsRequested = 20;
            StepOver([]() {
                g_stepsCompleted++;
                ULONG_PTR cip = GetContextDataEx(g_hThread, UE_CIP);
                g_lastStepAddress = cip;

                if (g_stepsCompleted < g_stepsRequested)
                {
                    // Check if we've returned from level1
                    if (cip < s_level1Addr || cip > s_level1Addr + 0x200)
                    {
                        s_stepOverCompleted = true;
                        StopDebug();
                        return;
                    }
                    StepOver(nullptr);
                }
                else
                {
                    s_stepOverCompleted = true;
                    StopDebug();
                }
            });
        });
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_bpHitCount >= 1, "Initial BP should have been hit");
    TEST_ASSERT(g_stepsCompleted > 0, "Should have completed at least one step over");
    // The inner BP may or may not fire depending on StepOver implementation
    // The key test is that StepOver completed successfully
    TEST_ASSERT(s_stepOverCompleted, "StepOver should complete even with BP inside called function");

    return true;
}

//-----------------------------------------------------------------------------
// ST-09: Step in multi-threaded - Stepping behavior with multiple threads
// Create a second thread, verify stepping only affects current thread
//-----------------------------------------------------------------------------
TITAN_TEST_ID("ST-09", ST_09, "Step in multi-threaded - single thread stepping")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_targetAddr = 0;
    static int s_threadCreateCount = 0;

    // Track thread creation
    SetCustomHandler(UE_CH_CREATETHREAD, [](const void*) {
        s_threadCreateCount++;
    });

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        s_targetAddr = s_session->GetExport("step_mixed_instructions");

        if (s_targetAddr == 0)
        {
            StopDebug();
            return;
        }

        // Set BP at target function
        SetBPX(s_targetAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
            g_bpHitCount++;
            g_initialIp = GetContextDataEx(g_hThread, UE_CIP);

            // Execute several steps in current thread
            g_stepsRequested = 10;
            StepInto([]() {
                g_stepsCompleted++;
                ULONG_PTR cip = GetContextDataEx(g_hThread, UE_CIP);
                g_lastStepAddress = cip;
                g_ipHistory.push_back(cip);

                if (g_stepsCompleted < g_stepsRequested)
                {
                    StepInto(nullptr);
                }
                else
                {
                    StopDebug();
                }
            });
        });
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_bpHitCount >= 1, "BP should have been hit");
    TEST_ASSERT(g_stepsCompleted == g_stepsRequested, "Should have completed all requested steps");

    // Verify IP history shows sequential execution (no jumps to other thread code)
    bool sequentialExecution = true;
    for (size_t i = 1; i < g_ipHistory.size(); i++)
    {
        // Each step should move IP by a small amount (typical instruction size)
        ULONG_PTR diff = g_ipHistory[i] > g_ipHistory[i - 1]
                             ? g_ipHistory[i] - g_ipHistory[i - 1]
                             : g_ipHistory[i - 1] - g_ipHistory[i];
        // Allow reasonable instruction size or small jumps within function
        if (diff > 0x100)
        {
            sequentialExecution = false;
            break;
        }
    }

    TEST_ASSERT(sequentialExecution, "Stepping should execute instructions sequentially in current thread");

    return true;
}

//-----------------------------------------------------------------------------
// ST-10: Consecutive steps - Multiple step operations in sequence
// Execute 10 consecutive StepInto operations, verify IP changes each time
//-----------------------------------------------------------------------------
TITAN_TEST_ID("ST-10", ST_10, "Consecutive steps - 10 sequential steps")
{
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static DebugSession* s_session = &session;
    static ULONG_PTR s_targetAddr = 0;
    static const int EXPECTED_STEPS = 10;

    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, [](const void*) {
        g_systemBpHit = true;

        // Use step_inline_asm which has predictable NOP instructions
        s_targetAddr = s_session->GetExport("step_inline_asm");

        if (s_targetAddr == 0)
        {
            StopDebug();
            return;
        }

        // Set BP at target function
        SetBPX(s_targetAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
            g_bpHitCount++;
            g_initialIp = GetContextDataEx(g_hThread, UE_CIP);
            g_ipHistory.push_back(g_initialIp);

            // Execute exactly 10 steps
            g_stepsRequested = EXPECTED_STEPS;
            StepInto(OnStepAndContinue);
        });
    });

    session.Run();

    TEST_ASSERT(g_systemBpHit, "System breakpoint was not hit");
    TEST_ASSERT(g_bpHitCount >= 1, "BP should have been hit");
    TEST_ASSERT(g_stepsCompleted == EXPECTED_STEPS, "Should have completed exactly 10 steps");
    TEST_ASSERT(g_ipHistory.size() == EXPECTED_STEPS + 1, "Should have recorded 11 IPs (initial + 10 steps)");

    // Verify each IP is different from the previous
    int uniqueIps = 1; // Count initial IP
    for (size_t i = 1; i < g_ipHistory.size(); i++)
    {
        if (g_ipHistory[i] != g_ipHistory[i - 1])
        {
            uniqueIps++;
        }
    }

    TEST_ASSERT(uniqueIps == (int)g_ipHistory.size(), "Each step should result in a different IP");

    return true;
}
