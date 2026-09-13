"""Regression tests for filtered-trace startup and Pause using production functions.

Run: python tests/trace_filter_test.py [--compiler clang++]
Functions are compiled unchanged against recorder/debug-engine test doubles.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def function(path, signature):
    text = (ROOT / path).read_text()
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


SHIM = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
using duint = uintptr_t;
using DWORD = unsigned long;
using ULONGLONG = unsigned long long;
using HANDLE = int;
using TITANCBSTEP = void(*)();
using STEPFUNCTION = void(*)(TITANCBSTEP);
using MODULEPARTY = int;
const int mod_user = 0, mod_system = 1, UE_CIP = 1, UE_BREAKPOINT = 0, WM_NULL = 0;
const int history_clear = 0;
#ifndef _countof
#define _countof(a) (sizeof(a) / sizeof((a)[0]))
#endif
#define QT_TRANSLATE_NOOP(context, text) text
#define dputs(...) ((void)0)
#define dprintf(...) ((void)0)

duint cip = 0x1000;
int selectedParty = -1;
HANDLE hActiveThread = 7;
bool tracing = false, running = false, abortTrace = false, bAbortStepping = false;
bool partyRun = false, validCondition = true;
TITANCBSTEP pendingStep = nullptr, gStepIntoPartyCallback = nullptr, gStepOverPartyCallback = nullptr;
bool over = false;
ULONGLONG clockMs = 100;
duint events = 1;
int breakins = 0, pauseBreakpoints = 0;
std::vector<duint> logged;
int ModGetParty(duint address) { return address >= 0x2000 ? mod_system : mod_user; }
duint GetContextDataEx(HANDLE, int) { return cip; }
struct TraceRecordManager
{
    bool rtEnabled = true, rtPrevInstAvailable = false, rtNeedThreadId = false;
    unsigned rtRecordedInstructions = 0;
    bool rtOldContextChanged[8] = {};
    struct { struct { struct { duint cip = 0; } regcontext; } registers; } rtOldContext;
    std::vector<duint> written;
    void FilterPendingTraceRecord(int party);
    void queue(duint address)
    {
        rtOldContext.registers.regcontext.cip = address;
        rtPrevInstAvailable = true;
        ++rtRecordedInstructions;
    }
    void FlushTraceExecuteRecord()
    {
        if(rtEnabled && rtPrevInstAvailable)
            written.push_back(rtOldContext.registers.regcontext.cip);
        rtPrevInstAvailable = false;
    }
} TraceRecord;
void StepIntoWow64(TITANCBSTEP callback) { pendingStep = callback; over = false; }
void StepOverWrapper(TITANCBSTEP callback) { pendingStep = callback; over = true; }
bool tryTraceRunToParty(int, TITANCBSTEP, STEPFUNCTION) { return false; }
bool IsArgumentsLessThan(int argc, int count) { return argc < count; }
bool dbgtraceactive() { return tracing; }
bool dbgisrunning() { return running; }
bool BridgeSettingGetUint(const char*, const char*, duint*) { return false; }
bool valfromstring(const char*, duint*, bool) { return true; }
bool dbgsettracecondition(const char*, duint) { return tracing = validCondition; }
int dbggettracepartyfilter() { return selectedParty; }
bool cbDebugRunInternal(int, char*[], int) { running = true; return true; }
bool _dbg_isanimating() { return false; }
void _dbg_animatestop() {}
bool DbgIsDebugging() { return true; }
void dbgforcebreaktrace() { abortTrace = true; }
bool dbgstepactive() { return gStepIntoPartyCallback || gStepOverPartyCallback; }
void dbgforcebreakstep() { if(dbgstepactive()) bAbortStepping = true; }
bool RunToPartyIsActive() { return partyRun; }
ULONGLONG GetTickCount64() { return clockMs; }
duint dbggetdbgeventcount() { return events; }
bool dbgspawnbreakinthread() { ++breakins; return true; }
DWORD dbggetattachmainthread() { return 0; }
HANDLE ThreadGetHandle(DWORD id) { return int(id); }
DWORD GetThreadId(HANDLE id) { return id; }
DWORD SuspendThread(HANDLE) { return 0; }
DWORD ResumeThread(HANDLE) { return 0; }
void PostThreadMessageA(DWORD, int, int, int) {}
void cbPauseBreakpoint() {}
bool SetBPX(duint, int, TITANCBSTEP) { ++pauseBreakpoints; return true; }
'''

TEST = r'''
void traceCallback()
{
    assert(abortTrace || ModGetParty(cip) == selectedParty);
    logged.push_back(cip);
    TraceRecord.FlushTraceExecuteRecord();
    TraceRecord.queue(cip);
}
void reset()
{
    TraceRecord = TraceRecordManager{};
    cip = 0x1000;
    selectedParty = mod_system;
    tracing = running = abortTrace = bAbortStepping = partyRun = false;
    validCondition = true;
    gStepIntoPartyCallback = gStepOverPartyCallback = pendingStep = nullptr;
    logged.clear();
    breakins = pauseBreakpoints = 0;
    ++events;
    ++clockMs;
}
void step(duint address)
{
    assert(pendingStep);
    auto callback = pendingStep;
    pendingStep = nullptr;
    cip = address;
    ++events;
    callback();
}
void start(STEPFUNCTION stepFunction)
{
    char command[] = "TraceIntoConditional", condition[] = "0";
    char* argv[] = {command, condition};
    assert(genericConditionalTraceCommand(traceCallback, stepFunction, 2, argv));
}
int main()
{
    // Recording is enabled BEFORE configuring the filter: it seeds user CIP.
    for(auto stepFunction : {StepIntoSystem, StepOverSystem})
    {
        reset();
        TraceRecord.queue(cip);
        start(stepFunction);
        assert(!TraceRecord.rtPrevInstAvailable && TraceRecord.rtRecordedInstructions == 0);
        assert(TraceRecord.rtNeedThreadId);
        for(auto changed : TraceRecord.rtOldContextChanged) assert(changed);
        step(0x1001); // first step is still excluded; flushing must not log the seed
        assert(TraceRecord.written.empty() && logged.empty());
        step(0x2000);
        TraceRecord.FlushTraceExecuteRecord();
        assert(TraceRecord.written == std::vector<duint>{0x2000});
        assert(logged == std::vector<duint>{0x2000});
    }
    // Directly crossing parties on the first step must also exclude the seed.
    reset();
    TraceRecord.queue(cip);
    start(StepIntoSystem);
    step(0x2000);
    assert(TraceRecord.written.empty());

    // Symmetric case: User Only starts in system code.
    reset();
    selectedParty = mod_user;
    cip = 0x2000;
    TraceRecord.queue(cip);
    start(StepIntoUser);
    step(0x2001);
    step(0x1000);
    TraceRecord.FlushTraceExecuteRecord();
    assert(TraceRecord.written == std::vector<duint>{0x1000});

    // Do not change unfiltered/included/disabled/no-pending recording states.
    for(int party : {-1, mod_user})
    {
        reset();
        TraceRecord.queue(cip);
        TraceRecord.FilterPendingTraceRecord(party);
        assert(TraceRecord.rtPrevInstAvailable && TraceRecord.rtRecordedInstructions == 1);
    }
    reset();
    TraceRecord.FilterPendingTraceRecord(mod_system);
    assert(TraceRecord.rtRecordedInstructions == 0);
    TraceRecord.queue(cip);
    TraceRecord.rtEnabled = false;
    TraceRecord.FilterPendingTraceRecord(mod_system);
    assert(TraceRecord.rtPrevInstAvailable && TraceRecord.rtRecordedInstructions == 1);

    // Existing file history is retained; classify the pending address, not CIP
    // (the user might have edited CIP while paused).
    reset();
    TraceRecord.written = {0x2000, 0x2001};
    TraceRecord.rtRecordedInstructions = 2;
    TraceRecord.queue(0x1000);
    cip = 0x2002;
    TraceRecord.FilterPendingTraceRecord(mod_system);
    assert((TraceRecord.written == std::vector<duint>{0x2000, 0x2001}));
    assert(TraceRecord.rtRecordedInstructions == 2 && !TraceRecord.rtPrevInstAvailable);

    // Failed trace setup must not discard the queued instruction.
    reset();
    TraceRecord.queue(cip);
    validCondition = false;
    char cmd[] = "trace", expr[] = "bad";
    char* argv[] = {cmd, expr};
    assert(!genericConditionalTraceCommand(traceCallback, StepIntoSystem, 2, argv));
    assert(TraceRecord.rtPrevInstAvailable && !pendingStep);

    // Pause must escape BOTH party-skip loops before the target party is hit.
    for(auto stepFunction : {StepIntoSystem, StepOverSystem})
    {
        reset();
        start(stepFunction);
        assert(cbDebugPause(0, nullptr));
        assert(abortTrace && bAbortStepping && !pauseBreakpoints);
        step(0x1001);
        assert(logged == std::vector<duint>{0x1001});
        assert(!dbgstepactive());
    }
    // A fast run has no forthcoming single-step event: use real Pause machinery.
    reset();
    start(StepIntoSystem);
    partyRun = true;
    assert(cbDebugPause(0, nullptr));
    assert(abortTrace && bAbortStepping && pauseBreakpoints == 1);

    // A regular step can block in a syscall. A second Pause after 2 seconds
    // with no new debug events must reach the break-in-thread fallback.
    reset();
    start(StepIntoSystem);
    assert(cbDebugPause(0, nullptr));
    assert(breakins == 0);
    clockMs += 2500;
    assert(cbDebugPause(0, nullptr));
    assert(breakins == 1);
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="clang++")
    args = parser.parse_args()
    code = SHIM + function("src/dbg/TraceRecord.cpp", "void TraceRecordManager::FilterPendingTraceRecord(")
    code += function("src/dbg/commands/cmd-tracing.cpp", "static bool genericConditionalTraceCommand(")
    for signature in ("template<MODULEPARTY StopParty>\nstatic void cbStepIntoParty()",
                      "void StepIntoUser(", "void StepIntoSystem(",
                      "template<MODULEPARTY StopParty>\nstatic void cbStepOverParty()",
                      "void StepOverUser(", "void StepOverSystem("):
        code += function("src/dbg/debugger.cpp", signature)
    code += function("src/dbg/commands/cmd-debug-control.cpp", "bool cbDebugPause(")
    code += TEST
    with tempfile.TemporaryDirectory(prefix="x64dbg-trace-filter-") as directory:
        temp = Path(directory)
        (temp / "test.cpp").write_text(code)
        executable = temp / "test.exe"
        subprocess.run([args.compiler, "-std=c++14", str(temp / "test.cpp"), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
    print("Trace startup/filter/Pause regression tests passed")


if __name__ == "__main__":
    main()
