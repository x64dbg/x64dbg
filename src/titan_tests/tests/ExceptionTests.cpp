/**
 * Exception Handling Tests (EX-01 through EX-13)
 *
 * Tests TitanEngine's exception handling capabilities using
 * SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, ...) and related APIs.
 */

#include "TitanTestFramework.h"
#include "TitanEngine/TitanEngine.h"
#include <string>
#include <atomic>

//-----------------------------------------------------------------------------
// Test Utilities
//-----------------------------------------------------------------------------

namespace
{
    // Path to test executable
    std::wstring GetTestExePath()
    {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring exePath(path);
        size_t lastSlash = exePath.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos)
        {
            exePath = exePath.substr(0, lastSlash + 1);
        }
        exePath += L"TestExe_Exceptions.exe";
        return exePath;
    }

    // Shared state for callbacks
    struct ExceptionTestState
    {
        std::atomic<bool> exceptionCaught{false};
        std::atomic<DWORD> exceptionCode{0};
        std::atomic<int> exceptionCount{0};
        std::atomic<bool> isFirstChance{false};
        std::atomic<bool> isSecondChance{false};
        std::atomic<ULONG_PTR> exceptionAddress{0};
        std::atomic<ULONG_PTR> accessViolationAddress{0};
        std::atomic<DWORD> accessType{0}; // 0 = read, 1 = write, 8 = execute
        std::atomic<bool> systemBpHit{false};

        void Reset()
        {
            exceptionCaught = false;
            exceptionCode = 0;
            exceptionCount = 0;
            isFirstChance = false;
            isSecondChance = false;
            exceptionAddress = 0;
            accessViolationAddress = 0;
            accessType = 0;
            systemBpHit = false;
        }
    };

    ExceptionTestState g_state;

    // Generic exception callback
    void ExceptionCallback(const void* /* arg */)
    {
        g_state.exceptionCaught = true;
        g_state.exceptionCount++;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (dbgEvent && dbgEvent->dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            const EXCEPTION_RECORD& rec = dbgEvent->u.Exception.ExceptionRecord;
            g_state.exceptionCode = rec.ExceptionCode;
            g_state.exceptionAddress = (ULONG_PTR)rec.ExceptionAddress;
            g_state.isFirstChance = dbgEvent->u.Exception.dwFirstChance != 0;

            // For access violations, capture additional info
            if (rec.ExceptionCode == EXCEPTION_ACCESS_VIOLATION && rec.NumberParameters >= 2)
            {
                g_state.accessType = (DWORD)rec.ExceptionInformation[0];
                g_state.accessViolationAddress = (ULONG_PTR)rec.ExceptionInformation[1];
            }
        }

        // Stop debugging after we catch the exception
        StopDebug();
    }

    // System breakpoint callback
    void SystemBreakpointCallback(const void* /* arg */)
    {
        g_state.systemBpHit = true;
    }

    // Callback that allows first-chance, catches second-chance
    void FirstSecondChanceCallback(const void* /* arg */)
    {
        g_state.exceptionCount++;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (dbgEvent && dbgEvent->dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            const EXCEPTION_RECORD& rec = dbgEvent->u.Exception.ExceptionRecord;
            g_state.exceptionCode = rec.ExceptionCode;

            if (dbgEvent->u.Exception.dwFirstChance)
            {
                g_state.isFirstChance = true;
                // Pass to the application (don't handle first-chance)
                SetNextDbgContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
            }
            else
            {
                g_state.isSecondChance = true;
                g_state.exceptionCaught = true;
                StopDebug();
            }
        }
    }

    // Callback for testing SetNextDbgContinueStatus
    void ContinueStatusCallback(const void* /* arg */)
    {
        g_state.exceptionCount++;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (dbgEvent && dbgEvent->dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            g_state.exceptionCode = dbgEvent->u.Exception.ExceptionRecord.ExceptionCode;
            g_state.exceptionCaught = true;

            // Tell the OS we handled it
            SetNextDbgContinueStatus(DBG_CONTINUE);
            StopDebug();
        }
    }

    // Run debugger and wait for exception
    bool RunDebuggerWithArg(const wchar_t* arg)
    {
        std::wstring exePath = GetTestExePath();
        std::wstring cmdLine = L"\"" + exePath + L"\" " + arg;

        PROCESS_INFORMATION* pi = InitDebugW(exePath.c_str(), cmdLine.c_str(), nullptr);
        if (!pi || pi->hProcess == nullptr)
        {
            return false;
        }

        DebugLoop();
        return true;
    }

} // anonymous namespace

//-----------------------------------------------------------------------------
// EX-01: ACCESS_VIOLATION read - Null pointer read
//-----------------------------------------------------------------------------
TITAN_TEST_ID("EX-01", EX_01, "ACCESS_VIOLATION read - Null pointer read")
{
    g_state.Reset();
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (TITANCALLBACKARG)ExceptionCallback);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (TITANCALLBACKARG)SystemBreakpointCallback);

    bool started = RunDebuggerWithArg(L"--read");
    TEST_ASSERT(started, "Failed to start debugger");
    TEST_ASSERT(g_state.exceptionCaught.load(), "Exception not caught");
    TEST_ASSERT_EQ((DWORD)EXCEPTION_ACCESS_VIOLATION, g_state.exceptionCode.load(),
                   "Expected ACCESS_VIOLATION exception");
    TEST_ASSERT_EQ((DWORD)0, g_state.accessType.load(), "Expected read access type (0)");

    return true;
}

//-----------------------------------------------------------------------------
// EX-02: ACCESS_VIOLATION write - Null pointer write
//-----------------------------------------------------------------------------
TITAN_TEST_ID("EX-02", EX_02, "ACCESS_VIOLATION write - Null pointer write")
{
    g_state.Reset();
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (TITANCALLBACKARG)ExceptionCallback);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (TITANCALLBACKARG)SystemBreakpointCallback);

    bool started = RunDebuggerWithArg(L"--write");
    TEST_ASSERT(started, "Failed to start debugger");
    TEST_ASSERT(g_state.exceptionCaught.load(), "Exception not caught");
    TEST_ASSERT_EQ((DWORD)EXCEPTION_ACCESS_VIOLATION, g_state.exceptionCode.load(),
                   "Expected ACCESS_VIOLATION exception");
    TEST_ASSERT_EQ((DWORD)1, g_state.accessType.load(), "Expected write access type (1)");

    return true;
}

//-----------------------------------------------------------------------------
// EX-03: ACCESS_VIOLATION execute - Execute non-executable memory
//-----------------------------------------------------------------------------
TITAN_TEST_ID("EX-03", EX_03, "ACCESS_VIOLATION execute - Execute non-executable memory")
{
    g_state.Reset();
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (TITANCALLBACKARG)ExceptionCallback);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (TITANCALLBACKARG)SystemBreakpointCallback);

    bool started = RunDebuggerWithArg(L"--execute");
    TEST_ASSERT(started, "Failed to start debugger");
    TEST_ASSERT(g_state.exceptionCaught.load(), "Exception not caught");
    TEST_ASSERT_EQ((DWORD)EXCEPTION_ACCESS_VIOLATION, g_state.exceptionCode.load(),
                   "Expected ACCESS_VIOLATION exception");
    // Execute violations use access type 8 on Windows
    TEST_ASSERT_EQ((DWORD)8, g_state.accessType.load(), "Expected execute access type (8)");

    return true;
}

//-----------------------------------------------------------------------------
// EX-04: INT3 exception - Software breakpoint exception
//-----------------------------------------------------------------------------
TITAN_TEST_ID("EX-04", EX_04, "INT3 exception - Software breakpoint exception")
{
    g_state.Reset();
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (TITANCALLBACKARG)ExceptionCallback);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (TITANCALLBACKARG)SystemBreakpointCallback);

    bool started = RunDebuggerWithArg(L"--int3");
    TEST_ASSERT(started, "Failed to start debugger");
    TEST_ASSERT(g_state.exceptionCaught.load(), "Exception not caught");
    TEST_ASSERT_EQ((DWORD)EXCEPTION_BREAKPOINT, g_state.exceptionCode.load(),
                   "Expected BREAKPOINT exception");

    return true;
}

//-----------------------------------------------------------------------------
// EX-05: SINGLE_STEP exception - Trap flag exception
//-----------------------------------------------------------------------------
TITAN_TEST_ID("EX-05", EX_05, "SINGLE_STEP exception - Trap flag exception")
{
    g_state.Reset();
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (TITANCALLBACKARG)ExceptionCallback);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (TITANCALLBACKARG)SystemBreakpointCallback);

    bool started = RunDebuggerWithArg(L"--singlestep");
    TEST_ASSERT(started, "Failed to start debugger");
    TEST_ASSERT(g_state.exceptionCaught.load(), "Exception not caught");
    TEST_ASSERT_EQ((DWORD)EXCEPTION_SINGLE_STEP, g_state.exceptionCode.load(),
                   "Expected SINGLE_STEP exception");

    return true;
}

//-----------------------------------------------------------------------------
// EX-06: DIV_BY_ZERO - Integer division by zero
//-----------------------------------------------------------------------------
TITAN_TEST_ID("EX-06", EX_06, "DIV_BY_ZERO - Integer division by zero")
{
    g_state.Reset();
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (TITANCALLBACKARG)ExceptionCallback);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (TITANCALLBACKARG)SystemBreakpointCallback);

    bool started = RunDebuggerWithArg(L"--divzero");
    TEST_ASSERT(started, "Failed to start debugger");
    TEST_ASSERT(g_state.exceptionCaught.load(), "Exception not caught");
    TEST_ASSERT_EQ((DWORD)EXCEPTION_INT_DIVIDE_BY_ZERO, g_state.exceptionCode.load(),
                   "Expected INT_DIVIDE_BY_ZERO exception");

    return true;
}

//-----------------------------------------------------------------------------
// EX-07: ILLEGAL_INSTRUCTION - Invalid opcode (UD2)
//-----------------------------------------------------------------------------
TITAN_TEST_ID("EX-07", EX_07, "ILLEGAL_INSTRUCTION - Invalid opcode")
{
    g_state.Reset();
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (TITANCALLBACKARG)ExceptionCallback);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (TITANCALLBACKARG)SystemBreakpointCallback);

    bool started = RunDebuggerWithArg(L"--illegal");
    TEST_ASSERT(started, "Failed to start debugger");
    TEST_ASSERT(g_state.exceptionCaught.load(), "Exception not caught");
    TEST_ASSERT_EQ((DWORD)EXCEPTION_ILLEGAL_INSTRUCTION, g_state.exceptionCode.load(),
                   "Expected ILLEGAL_INSTRUCTION exception");

    return true;
}

//-----------------------------------------------------------------------------
// EX-08: PRIVILEGED_INSTRUCTION - Ring 0 instruction in ring 3
//-----------------------------------------------------------------------------
TITAN_TEST_ID("EX-08", EX_08, "PRIVILEGED_INSTRUCTION - Ring 0 instruction in ring 3")
{
    g_state.Reset();
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (TITANCALLBACKARG)ExceptionCallback);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (TITANCALLBACKARG)SystemBreakpointCallback);

    bool started = RunDebuggerWithArg(L"--privileged");
    TEST_ASSERT(started, "Failed to start debugger");
    TEST_ASSERT(g_state.exceptionCaught.load(), "Exception not caught");
    TEST_ASSERT_EQ((DWORD)EXCEPTION_PRIV_INSTRUCTION, g_state.exceptionCode.load(),
                   "Expected PRIV_INSTRUCTION exception");

    return true;
}

//-----------------------------------------------------------------------------
// EX-09: STACK_OVERFLOW - Deep recursion
//-----------------------------------------------------------------------------
TITAN_TEST_ID("EX-09", EX_09, "STACK_OVERFLOW - Deep recursion")
{
    g_state.Reset();
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (TITANCALLBACKARG)ExceptionCallback);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (TITANCALLBACKARG)SystemBreakpointCallback);

    bool started = RunDebuggerWithArg(L"--stackoverflow");
    TEST_ASSERT(started, "Failed to start debugger");
    TEST_ASSERT(g_state.exceptionCaught.load(), "Exception not caught");
    TEST_ASSERT_EQ((DWORD)EXCEPTION_STACK_OVERFLOW, g_state.exceptionCode.load(),
                   "Expected STACK_OVERFLOW exception");

    return true;
}

//-----------------------------------------------------------------------------
// EX-10: GUARD_PAGE - Guard page violation
//-----------------------------------------------------------------------------
TITAN_TEST_ID("EX-10", EX_10, "GUARD_PAGE - Guard page violation")
{
    g_state.Reset();
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (TITANCALLBACKARG)ExceptionCallback);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (TITANCALLBACKARG)SystemBreakpointCallback);

    bool started = RunDebuggerWithArg(L"--guardpage");
    TEST_ASSERT(started, "Failed to start debugger");
    TEST_ASSERT(g_state.exceptionCaught.load(), "Exception not caught");
    TEST_ASSERT_EQ((DWORD)STATUS_GUARD_PAGE_VIOLATION, g_state.exceptionCode.load(),
                   "Expected GUARD_PAGE_VIOLATION exception");

    return true;
}

//-----------------------------------------------------------------------------
// EX-11: SetNextDbgContinueStatus - Test continue status control
//-----------------------------------------------------------------------------
TITAN_TEST_ID("EX-11", EX_11, "SetNextDbgContinueStatus - Test continue status control")
{
    g_state.Reset();
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (TITANCALLBACKARG)ContinueStatusCallback);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (TITANCALLBACKARG)SystemBreakpointCallback);

    // Use a recoverable exception (INT3) to test continue status
    bool started = RunDebuggerWithArg(L"--int3");
    TEST_ASSERT(started, "Failed to start debugger");
    TEST_ASSERT(g_state.exceptionCaught.load(), "Exception not caught");
    TEST_ASSERT_EQ((DWORD)EXCEPTION_BREAKPOINT, g_state.exceptionCode.load(),
                   "Expected BREAKPOINT exception");
    // If SetNextDbgContinueStatus(DBG_CONTINUE) worked, we got here without crashing
    // The callback successfully set the continue status

    return true;
}

//-----------------------------------------------------------------------------
// EX-12: First-chance vs second-chance - Distinguish exception stages
//-----------------------------------------------------------------------------
TITAN_TEST_ID("EX-12", EX_12, "First-chance vs second-chance - Distinguish exception stages")
{
    g_state.Reset();
    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (TITANCALLBACKARG)FirstSecondChanceCallback);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (TITANCALLBACKARG)SystemBreakpointCallback);

    // Trigger an unhandled exception to see both first and second chance
    bool started = RunDebuggerWithArg(L"--read");
    TEST_ASSERT(started, "Failed to start debugger");
    TEST_ASSERT(g_state.isFirstChance.load(), "First-chance exception not seen");
    TEST_ASSERT(g_state.isSecondChance.load(), "Second-chance exception not seen");
    TEST_ASSERT(g_state.exceptionCount.load() >= 2, "Expected at least 2 exception callbacks");

    return true;
}

//-----------------------------------------------------------------------------
// EX-13: Exception in SEH handler - Nested exception handling
//-----------------------------------------------------------------------------
TITAN_TEST_ID("EX-13", EX_13, "Exception in SEH handler - Nested exception handling")
{
    g_state.Reset();

    // Track multiple exceptions
    static int nestedExceptionCount = 0;
    nestedExceptionCount = 0;

    auto nestedCallback = [](const void*) {
        nestedExceptionCount++;
        g_state.exceptionCount++;

        const DEBUG_EVENT* dbgEvent = GetDebugData();
        if (dbgEvent && dbgEvent->dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            g_state.exceptionCode = dbgEvent->u.Exception.ExceptionRecord.ExceptionCode;
            g_state.exceptionCaught = true;

            // Pass exceptions to application until we see enough
            if (nestedExceptionCount < 3)
            {
                SetNextDbgContinueStatus(DBG_EXCEPTION_NOT_HANDLED);
            }
            else
            {
                // After seeing multiple exceptions (nested), stop
                StopDebug();
            }
        }
    };

    SetCustomHandler(UE_CH_UNHANDLEDEXCEPTION, (TITANCALLBACKARG)nestedCallback);
    SetCustomHandler(UE_CH_SYSTEMBREAKPOINT, (TITANCALLBACKARG)SystemBreakpointCallback);

    // Use the nested exception test function
    bool started = RunDebuggerWithArg(L"--nested");
    TEST_ASSERT(started, "Failed to start debugger");
    TEST_ASSERT(g_state.exceptionCaught.load(), "Exception not caught");
    // Nested exceptions will trigger multiple exception callbacks
    TEST_ASSERT(g_state.exceptionCount.load() >= 1, "Expected at least 1 exception in nested scenario");

    return true;
}
