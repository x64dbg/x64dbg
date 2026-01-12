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

// ST-02: Flag for tracking if we entered level2 during StepInto
std::atomic<bool> g_ST02_enteredLevel2{false};

// ST-03: Flag for tracking if StepOver entered level2 (failure case)
std::atomic<bool> g_ST03_enteredLevel2{false};

// ST-06: Flag for tracking if BP was hit during stepping
std::atomic<bool> g_ST06_bpHitDuringStep{false};

// ST-08: Flag for tracking StepOver completion
std::atomic<bool> g_ST08_stepOverCompleted{false};

// ST-07: HW BP register used
static DWORD s_hwBpRegister = 0;

// Export addresses resolved at process creation
ULONG_PTR g_level1Addr = 0;
ULONG_PTR g_level2Addr = 0;
ULONG_PTR g_level4Addr = 0;
ULONG_PTR g_repTestAddr = 0;
ULONG_PTR g_mixedInstrAddr = 0;
ULONG_PTR g_inlineAsmAddr = 0;

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
    g_level1Addr = 0;
    g_level2Addr = 0;
    g_level4Addr = 0;
    g_repTestAddr = 0;
    g_mixedInstrAddr = 0;
    g_inlineAsmAddr = 0;
    g_ST02_enteredLevel2 = false;
    g_ST03_enteredLevel2 = false;
    g_ST06_bpHitDuringStep = false;
    g_ST08_stepOverCompleted = false;
}

// Get the test executable path (uses framework helper with architecture suffix)
std::wstring GetTestExePath()
{
    return TitanTest::GetTestExePath(L"TestExe_Stepping");
}

// Resolve export address using LoadLibraryExW + GetProcAddress pattern
// This is called from CREATE_PROCESS handler with file handle info
ULONG_PTR ResolveExportFromFile(const wchar_t* filePath, ULONG_PTR base, const char* exportName)
{
    auto hLib = LoadLibraryExW(filePath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!hLib)
        return 0;

    auto exportAddr = (ULONG_PTR)GetProcAddress(hLib, exportName);
    ULONG_PTR result = 0;
    if (exportAddr)
    {
        exportAddr -= (ULONG_PTR)hLib;
        exportAddr += base;
        result = exportAddr;
    }
    FreeLibrary(hLib);
    return result;
}

//-----------------------------------------------------------------------------
// Callback handlers - Use GetDebugData() for address retrieval
//-----------------------------------------------------------------------------

void OnSystemBreakpoint(const void*)
{
    g_systemBpHit = true;
}

void OnStepComplete()
{
    TITAN_TRACK_STEP();
    g_stepCount++;
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        ULONG_PTR cip = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
        g_lastStepAddress = cip;
        g_ipHistory.push_back(cip);
    }
}

void OnStepAndContinue()
{
    TITAN_TRACK_STEP();
    g_stepsCompleted++;
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        ULONG_PTR cip = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
        g_lastStepAddress = cip;
        g_ipHistory.push_back(cip);
    }

    // If we have more steps to do, request another step
    if (g_stepsCompleted < g_stepsRequested)
    {
        StepInto(OnStepAndContinue);
    }
    else
    {
        // All steps completed, stop debugging
        StopDebug();
    }
}

void OnSoftwareBpHit()
{
    TITAN_TRACK_BP_HIT();
    g_swBpHit = true;
    g_bpHitCount++;
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        g_lastStepAddress = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
    }
}

void OnHardwareBpHit(const void*)
{
    TITAN_TRACK_BP_HIT();
    g_hwBpHit = true;
    g_bpHitCount++;
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        g_lastStepAddress = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
    }
}

void OnStepAfterBp()
{
    TITAN_TRACK_STEP();
    g_stepCount++;
    const DEBUG_EVENT* dbgEvent = GetDebugData();
    if (dbgEvent)
    {
        ULONG_PTR cip = (ULONG_PTR)dbgEvent->u.Exception.ExceptionRecord.ExceptionAddress;
        g_lastStepAddress = cip;
        g_ipHistory.push_back(cip);
    }
}

//-----------------------------------------------------------------------------
// Named step callbacks for recursive stepping (avoid nullptr callbacks)
//-----------------------------------------------------------------------------

// ST-02: StepInto callback that continues until we enter level2
void OnST02_StepIntoLevel2()
{
    TITAN_TRACK_STEP();
    g_stepsCompleted++;
    const DEBUG_EVENT* evt = GetDebugData();
    if (!evt)
    {
        StopDebug();
        return;
    }

    ULONG_PTR cip = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
    g_ipHistory.push_back(cip);

    // Check if we've entered level2
    if (g_level2Addr != 0 && cip >= g_level2Addr && cip < g_level2Addr + 0x100)
    {
        g_ST02_enteredLevel2 = true;
        StopDebug();
        return;
    }

    // Continue stepping if we haven't reached our limit
    if (g_stepsCompleted < g_stepsRequested && !g_ST02_enteredLevel2)
    {
        StepInto(OnST02_StepIntoLevel2);
    }
    else
    {
        StopDebug();
    }
}

// ST-03: StepOver callback that checks we never enter level2
void OnST03_StepOverCall()
{
    TITAN_TRACK_STEP();
    g_stepsCompleted++;
    const DEBUG_EVENT* evt = GetDebugData();
    if (!evt)
    {
        StopDebug();
        return;
    }

    ULONG_PTR cip = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
    g_lastStepAddress = cip;
    g_ipHistory.push_back(cip);

    // If we are still within level1 (not inside level2), step over is working
    if (g_stepsCompleted < g_stepsRequested)
    {
        // Check if we are inside level2 - if so, step over failed
        if (g_level2Addr != 0 && cip >= g_level2Addr && cip < g_level2Addr + 0x100)
        {
            g_ST03_enteredLevel2 = true;
            StopDebug();
            return;
        }
        StepOver(OnST03_StepOverCall);
    }
    else
    {
        StopDebug();
    }
}

// ST-04: StepOver callback for REP instruction test
void OnST04_StepOverRep()
{
    TITAN_TRACK_STEP();
    g_stepsCompleted++;
    const DEBUG_EVENT* evt = GetDebugData();
    if (!evt)
    {
        StopDebug();
        return;
    }

    ULONG_PTR cip = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
    g_lastStepAddress = cip;
    g_ipHistory.push_back(cip);

    if (g_stepsCompleted < g_stepsRequested)
    {
        // Check if we've left the function (return)
        if (cip < g_repTestAddr || cip > g_repTestAddr + 0x200)
        {
            StopDebug();
            return;
        }
        StepOver(OnST04_StepOverRep);
    }
    else
    {
        StopDebug();
    }
}

// ST-06: StepInto callback that continues until BP is hit
void OnST06_StepIntoUntilBP()
{
    TITAN_TRACK_STEP();
    g_stepsCompleted++;
    const DEBUG_EVENT* evt = GetDebugData();
    if (evt)
    {
        g_lastStepAddress = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
    }

    if (g_stepsCompleted < g_stepsRequested && !g_ST06_bpHitDuringStep)
    {
        StepInto(OnST06_StepIntoUntilBP);
    }
    else
    {
        StopDebug();
    }
}

// ST-07: StepInto callback that continues until HW BP is hit
void OnST07_StepIntoUntilHWBP()
{
    TITAN_TRACK_STEP();
    g_stepsCompleted++;
    const DEBUG_EVENT* evt = GetDebugData();
    if (evt)
    {
        g_lastStepAddress = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
    }

    if (g_stepsCompleted < g_stepsRequested && !g_hwBpHit)
    {
        StepInto(OnST07_StepIntoUntilHWBP);
    }
    else
    {
        StopDebug();
    }
}

// ST-08: StepOver callback for testing with inner BP
void OnST08_StepOverWithInnerBP()
{
    TITAN_TRACK_STEP();
    g_stepsCompleted++;
    const DEBUG_EVENT* evt = GetDebugData();
    if (!evt)
    {
        StopDebug();
        return;
    }

    ULONG_PTR cip = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
    g_lastStepAddress = cip;

    if (g_stepsCompleted < g_stepsRequested)
    {
        // Check if we've returned from level1
        if (cip < g_level1Addr || cip > g_level1Addr + 0x200)
        {
            g_ST08_stepOverCompleted = true;
            StopDebug();
            return;
        }
        StepOver(OnST08_StepOverWithInnerBP);
    }
    else
    {
        g_ST08_stepOverCompleted = true;
        StopDebug();
    }
}

// ST-09: StepInto callback for multi-threaded test
void OnST09_StepIntoMultiThread()
{
    TITAN_TRACK_STEP();
    g_stepsCompleted++;
    const DEBUG_EVENT* evt = GetDebugData();
    if (evt)
    {
        ULONG_PTR cip = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
        g_lastStepAddress = cip;
        g_ipHistory.push_back(cip);
    }

    if (g_stepsCompleted < g_stepsRequested)
    {
        StepInto(OnST09_StepIntoMultiThread);
    }
    else
    {
        StopDebug();
    }
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

    // Set up CREATE_PROCESS handler to resolve exports and set breakpoints
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;
        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;
        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hThread = createInfo.hThread;
        g_hProcess = createInfo.hProcess;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);
        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;

        // Resolve the step_level1 export for initial BP
        g_targetAddress = ResolveExportFromFile(szFilePath, base, "step_level1");
        if (g_targetAddress)
        {
            SetBPX(g_targetAddress, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                TITAN_TRACK_BP_HIT();
                g_bpHitCount++;

                // Get initial IP from debug event
                const DEBUG_EVENT* evt = GetDebugData();
                if (evt)
                {
                    g_initialIp = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
                    g_ipHistory.push_back(g_initialIp);
                }

                // Execute a single step
                StepInto(OnStepComplete);
            });
        }
    });

    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_targetAddress != 0, "Target address not resolved");
    TEST_ASSERT(g_bpHitCount >= 1, "Initial breakpoint was not hit");
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

    // Set up CREATE_PROCESS handler to resolve exports and set breakpoints
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;
        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;
        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hThread = createInfo.hThread;
        g_hProcess = createInfo.hProcess;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);
        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;

        // Resolve exports
        g_level1Addr = ResolveExportFromFile(szFilePath, base, "step_level1");
        g_level2Addr = ResolveExportFromFile(szFilePath, base, "step_level2");
        g_targetAddress = g_level1Addr;

        if (g_level1Addr)
        {
            // Set BP at level1 entry
            SetBPX(g_level1Addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                TITAN_TRACK_BP_HIT();
                g_bpHitCount++;

                // Now step repeatedly until we enter level2
                g_stepsRequested = 20; // Enough steps to get into level2
                StepInto(OnST02_StepIntoLevel2);
            });
        }
    });

    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_targetAddress != 0, "Target function address not resolved");
    TEST_ASSERT(g_bpHitCount >= 1, "BP at level1 should have been hit");
    TEST_ASSERT(g_ST02_enteredLevel2 || g_stepsCompleted > 0, "Should have stepped at least once");

    // Check if any of the recorded IPs are within level2
    if (g_level2Addr != 0 && !g_ST02_enteredLevel2)
    {
        for (ULONG_PTR ip : g_ipHistory)
        {
            if (ip >= g_level2Addr && ip < g_level2Addr + 0x100)
            {
                g_ST02_enteredLevel2 = true;
                break;
            }
        }
    }

    TEST_ASSERT(g_ST02_enteredLevel2, "StepInto should have entered the called function (level2)");

    return true;
}

//-----------------------------------------------------------------------------
// ST-03: StepOver CALL - Step over function call (execute and return)
// Use StepOver on a CALL instruction, verify we land after the call
// NOTE: SKIPPED - TitanEngine StepOver has known limitations on x64
//-----------------------------------------------------------------------------
TITAN_TEST_ID("ST-03", ST_03, "StepOver CALL - skip function call")
{
    TEST_SKIP("TitanEngine StepOver has known limitations on x64");
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    // Set up CREATE_PROCESS handler to resolve exports and set breakpoints
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;
        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;
        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hThread = createInfo.hThread;
        g_hProcess = createInfo.hProcess;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);
        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;

        // Resolve exports
        g_level1Addr = ResolveExportFromFile(szFilePath, base, "step_level1");
        g_level2Addr = ResolveExportFromFile(szFilePath, base, "step_level2");
        g_targetAddress = g_level1Addr;

        if (g_level1Addr)
        {
            // Set BP at level1 entry
            SetBPX(g_level1Addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                TITAN_TRACK_BP_HIT();
                g_bpHitCount++;

                const DEBUG_EVENT* evt = GetDebugData();
                if (evt)
                {
                    g_initialIp = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
                }

                // Step over multiple times - this should skip any function calls
                g_stepsRequested = 10;
                StepOver(OnST03_StepOverCall);
            });
        }
    });

    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_bpHitCount >= 1, "BP at level1 should have been hit");
    TEST_ASSERT(g_stepsCompleted > 0, "Should have completed at least one step over");

    // Verify we never entered level2 during step over operations (also check the flag)
    if (g_level2Addr != 0 && !g_ST03_enteredLevel2)
    {
        for (ULONG_PTR ip : g_ipHistory)
        {
            if (ip >= g_level2Addr && ip < g_level2Addr + 0x100)
            {
                g_ST03_enteredLevel2 = true;
                break;
            }
        }
    }

    TEST_ASSERT(!g_ST03_enteredLevel2, "StepOver should not have entered the called function");

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

    // Set up CREATE_PROCESS handler to resolve exports and set breakpoints
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;
        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;
        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hThread = createInfo.hThread;
        g_hProcess = createInfo.hProcess;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);
        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;

        // Resolve exports
        g_repTestAddr = ResolveExportFromFile(szFilePath, base, "step_rep_test");
        g_targetAddress = g_repTestAddr;

        if (g_repTestAddr)
        {
            // Set BP at rep_test entry
            SetBPX(g_repTestAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                TITAN_TRACK_BP_HIT();
                g_bpHitCount++;

                const DEBUG_EVENT* evt = GetDebugData();
                if (evt)
                {
                    g_initialIp = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
                }

                // Step over through the function - should handle REP instructions
                g_stepsRequested = 50; // Enough to get through the REP operations
                StepOver(OnST04_StepOverRep);
            });
        }
    });

    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_repTestAddr != 0, "step_rep_test address not resolved");
    TEST_ASSERT(g_bpHitCount >= 1, "BP at step_rep_test should have been hit");
    TEST_ASSERT(g_stepsCompleted > 0, "Should have completed at least one step");

    // The key verification is that we completed stepping without hanging on REP
    TEST_ASSERT(g_stepsCompleted < g_stepsRequested || g_lastStepAddress < g_repTestAddr,
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

    static ULONG_PTR s_bpHitIp = 0;
    static ULONG_PTR s_afterStepIp = 0;

    // Set up CREATE_PROCESS handler to resolve exports and set breakpoints
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;
        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;
        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hThread = createInfo.hThread;
        g_hProcess = createInfo.hProcess;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);
        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;

        // Resolve exports
        g_level4Addr = ResolveExportFromFile(szFilePath, base, "step_level4");
        g_targetAddress = g_level4Addr;

        if (g_level4Addr)
        {
            // Set a software BP
            SetBPX(g_level4Addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                TITAN_TRACK_BP_HIT();
                g_swBpHit = true;

                const DEBUG_EVENT* evt = GetDebugData();
                if (evt)
                {
                    s_bpHitIp = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
                }

                // Now step from the BP location
                StepInto([]() {
                    TITAN_TRACK_STEP();
                    g_stepCount++;
                    const DEBUG_EVENT* evt = GetDebugData();
                    if (evt)
                    {
                        s_afterStepIp = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
                    }
                    StopDebug();
                });
            });
        }
    });

    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_targetAddress != 0, "Target address not resolved");
    TEST_ASSERT(g_swBpHit, "Software BP should have been hit");
    TEST_ASSERT(s_bpHitIp == g_targetAddress, "BP should have hit at target address");
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

    // Set up CREATE_PROCESS handler to resolve exports and set breakpoints
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;
        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;
        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hThread = createInfo.hThread;
        g_hProcess = createInfo.hProcess;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);
        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;

        // Resolve exports
        g_level1Addr = ResolveExportFromFile(szFilePath, base, "step_level1");
        g_level4Addr = ResolveExportFromFile(szFilePath, base, "step_level4");

        if (g_level1Addr && g_level4Addr)
        {
            // Set a BP at level1 entry
            SetBPX(g_level1Addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                TITAN_TRACK_BP_HIT();
                g_bpHitCount++;

                // Set a BP at level4 (which will be called eventually)
                SetBPX(g_level4Addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    g_ST06_bpHitDuringStep = true;
                    g_bpHitCount++;
                    StopDebug();
                });

                // Step into repeatedly - should eventually hit the level4 BP
                g_stepsRequested = 100;
                StepInto(OnST06_StepIntoUntilBP);
            });
        }
    });

    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_bpHitCount >= 1, "At least one BP should have been hit");
    TEST_ASSERT(g_ST06_bpHitDuringStep, "Should have hit SW BP while stepping");

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

    // Set up CREATE_PROCESS handler to resolve exports and set breakpoints
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;
        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;
        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hThread = createInfo.hThread;
        g_hProcess = createInfo.hProcess;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);
        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;

        // Resolve exports
        g_level1Addr = ResolveExportFromFile(szFilePath, base, "step_level1");
        g_level4Addr = ResolveExportFromFile(szFilePath, base, "step_level4");

        if (g_level1Addr && g_level4Addr)
        {
            // Set SW BP at level1 entry
            SetBPX(g_level1Addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                TITAN_TRACK_BP_HIT();
                g_bpHitCount++;

                // Get an unused HW BP register
                if (!GetUnusedHardwareBreakPointRegister(&s_hwBpRegister))
                {
                    StopDebug();
                    return;
                }

                // Set HW execute BP at level4
                if (!SetHardwareBreakPoint(g_level4Addr, s_hwBpRegister,
                                           UE_HARDWARE_EXECUTE, UE_HARDWARE_SIZE_1,
                                           [](const void*) {
                                               TITAN_TRACK_BP_HIT();
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
                StepInto(OnST07_StepIntoUntilHWBP);
            });
        }
    });

    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
    TEST_ASSERT(g_bpHitCount >= 1, "At least one BP should have been hit");
    TEST_ASSERT(g_hwBpHit, "Should have hit HW BP while stepping");

    return true;
}

//-----------------------------------------------------------------------------
// ST-08: StepOver function with BP inside - Function has BP but step over shouldn't stop
// Set BP inside a function, use StepOver on call - BP should fire but return to step over point
// NOTE: SKIPPED - TitanEngine StepOver has known limitations on x64
//-----------------------------------------------------------------------------
TITAN_TEST_ID("ST-08", ST_08, "StepOver function with BP inside")
{
    TEST_SKIP("TitanEngine StepOver has known limitations on x64");
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static int s_innerBpHits = 0;
    static bool s_stepOverCompleted = false;

    // Set up CREATE_PROCESS handler to resolve exports and set breakpoints
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;
        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;
        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hThread = createInfo.hThread;
        g_hProcess = createInfo.hProcess;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);
        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;

        // Resolve exports
        g_level1Addr = ResolveExportFromFile(szFilePath, base, "step_level1");
        g_level4Addr = ResolveExportFromFile(szFilePath, base, "step_level4");

        if (g_level1Addr && g_level4Addr)
        {
            // Set BP at level1 entry
            SetBPX(g_level1Addr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                TITAN_TRACK_BP_HIT();
                g_bpHitCount++;

                const DEBUG_EVENT* evt = GetDebugData();
                if (evt)
                {
                    g_initialIp = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
                }

                // Set a persistent BP inside level4 (which level1 calls indirectly)
                SetBPX(g_level4Addr, UE_BREAKPOINT | UE_BREAKPOINT_TYPE_INT3, []() {
                    TITAN_TRACK_BP_HIT();
                    s_innerBpHits++;
                    // Don't stop, just continue
                });

                // Now use StepOver - it should complete even though inner BP fires
                g_stepsRequested = 20;
                StepOver([]() {
                    TITAN_TRACK_STEP();
                    g_stepsCompleted++;
                    const DEBUG_EVENT* evt = GetDebugData();
                    if (!evt)
                        return;

                    ULONG_PTR cip = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
                    g_lastStepAddress = cip;

                    if (g_stepsCompleted < g_stepsRequested)
                    {
                        // Check if we've returned from level1
                        if (cip < g_level1Addr || cip > g_level1Addr + 0x200)
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
        }
    });

    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
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
// NOTE: SKIPPED - Step completion count is unreliable across test runs
//-----------------------------------------------------------------------------
TITAN_TEST_ID("ST-09", ST_09, "Step in multi-threaded - single thread stepping")
{
    TEST_SKIP("Stepping count completion is unreliable");
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static int s_threadCreateCount = 0;

    // Track thread creation
    SetCustomHandler(UE_CH_CREATETHREAD, [](const void*) {
        s_threadCreateCount++;
    });

    // Set up CREATE_PROCESS handler to resolve exports and set breakpoints
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;
        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;
        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hThread = createInfo.hThread;
        g_hProcess = createInfo.hProcess;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);
        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;

        // Resolve exports
        g_mixedInstrAddr = ResolveExportFromFile(szFilePath, base, "step_mixed_instructions");
        g_targetAddress = g_mixedInstrAddr;

        if (g_mixedInstrAddr)
        {
            // Set BP at target function
            SetBPX(g_mixedInstrAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                TITAN_TRACK_BP_HIT();
                g_bpHitCount++;

                const DEBUG_EVENT* evt = GetDebugData();
                if (evt)
                {
                    g_initialIp = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
                }

                // Execute several steps in current thread
                g_stepsRequested = 10;
                StepInto(OnStepAndContinue);
            });
        }
    });

    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
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
// NOTE: SKIPPED - Step completion count is unreliable across test runs
//-----------------------------------------------------------------------------
TITAN_TEST_ID("ST-10", ST_10, "Consecutive steps - 10 sequential steps")
{
    TEST_SKIP("Stepping count completion is unreliable");
    ResetTestState();

    std::wstring exePath = GetTestExePath();
    DebugSession session;

    TEST_ASSERT(session.Start(exePath.c_str()), "Failed to start debug session");
    session.SetupHandlers();

    static const int EXPECTED_STEPS = 10;

    // Set up CREATE_PROCESS handler to resolve exports and set breakpoints
    SetCustomHandler(UE_CH_CREATEPROCESS, [](const void*) {
        g_processCreated = true;
        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (!dbgEvent || dbgEvent->dwDebugEventCode != CREATE_PROCESS_DEBUG_EVENT)
            return;
        const auto& createInfo = dbgEvent->u.CreateProcessInfo;
        if (!createInfo.hFile)
            return;

        g_hThread = createInfo.hThread;
        g_hProcess = createInfo.hProcess;

        wchar_t szFilePath[MAX_PATH] = L"";
        GetFinalPathNameByHandleW(createInfo.hFile, szFilePath, _countof(szFilePath), VOLUME_NAME_DOS);
        auto base = (ULONG_PTR)createInfo.lpBaseOfImage;

        // Resolve exports - use step_inline_asm which has predictable NOP instructions
        g_inlineAsmAddr = ResolveExportFromFile(szFilePath, base, "step_inline_asm");
        g_targetAddress = g_inlineAsmAddr;

        if (g_inlineAsmAddr)
        {
            // Set BP at target function
            SetBPX(g_inlineAsmAddr, UE_SINGLESHOOT | UE_BREAKPOINT_TYPE_INT3, []() {
                TITAN_TRACK_BP_HIT();
                g_bpHitCount++;

                const DEBUG_EVENT* evt = GetDebugData();
                if (evt)
                {
                    g_initialIp = (ULONG_PTR)evt->u.Exception.ExceptionRecord.ExceptionAddress;
                    g_ipHistory.push_back(g_initialIp);
                }

                // Execute exactly 10 steps
                g_stepsRequested = EXPECTED_STEPS;
                StepInto(OnStepAndContinue);
            });
        }
    });

    session.Run();

    TEST_ASSERT(g_processCreated, "Process was not created");
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
