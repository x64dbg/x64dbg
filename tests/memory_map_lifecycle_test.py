"""Compile production background-map gating and detach rollback with a forced teardown race."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

from trace_filter_test import function

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="clang++")
    parser.add_argument("--unsafe-negative-control", action="store_true")
    args = parser.parse_args()
    memory = (ROOT / "src/dbg/memory.cpp").read_text()
    start = memory.index("static std::mutex memUpdateMapMutex;")
    state = memory[start:memory.index("static DWORD WINAPI memUpdateMap()", start)]
    if args.unsafe_negative_control:
        state = state.replace("std::lock_guard<std::mutex> lock(memUpdateMapMutex);", "", 1)
    code = r'''
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
using DWORD = unsigned long;
#define WINAPI
std::atomic<bool> debugging{true}, alive{true}, readAfterTeardown{false};
std::atomic<int> reads{0};
std::mutex gateMutex;
std::condition_variable gate;
bool holdScan = false, entered = false, releaseScan = false;
bool DbgIsDebugging() { return debugging; }
void MemUpdateMap()
{
    ++reads;
    std::unique_lock<std::mutex> lock(gateMutex);
    entered = true;
    gate.notify_all();
    if(holdScan)
        gate.wait(lock, [] { return releaseScan; });
    if(!alive)
        readAfterTeardown = true;
}
void GuiUpdateMemoryView() {}
'''
    code += state + function("src/dbg/memory.cpp", "static DWORD WINAPI memUpdateMap()")
    code += r'''
struct ProcessInfo { unsigned dwProcessId = 1; } info;
ProcessInfo* fdProcessInfo = &info;
struct PLUG_CB_DETACH { ProcessInfo* fdProcessInfo; };
const int CB_DETACH = 0, WAITID_RUN = 0;
#define QT_TRANSLATE_NOOP(a, b) b
void plugincbcall(int, void*) {}
void dbgclearpausebreakpoint() {}
void dbgdetachDisableAllBreakpoints() {}
void BpEnumAll(void(*)()) { assert(!memUpdateMapEnabled); }
bool detachSucceeds = false;
bool DetachDebuggerEx(unsigned)
{
    assert(!memUpdateMapEnabled);
    if(detachSucceeds)
        alive = false;
    return detachSucceeds;
}
void dputs(const char*) {}
void _dbg_animatestop() {}
void unlock(int) {}
void HistoryClear() {}
'''
    code += function("src/dbg/commands/cmd-debug-control.cpp", "bool cbDebugDetach(")
    code += r'''
int main()
{
    // DbgIsDebugging can still be true during/after engine process destruction.
    memUpdateMap();
    assert(reads == 0);
    assert(!MemSetAutoUpdateEnabled(true));
    memUpdateMap();
    assert(reads == 1);
    debugging = false;
    memUpdateMap();
    assert(reads == 1);
    debugging = true;

    // Hold an actual production worker invocation inside a simulated engine read.
    holdScan = true;
    entered = false;
    std::thread worker([] { memUpdateMap(); });
    {
        std::unique_lock<std::mutex> lock(gateMutex);
        assert(gate.wait_for(lock, std::chrono::seconds(2), [] { return entered; }));
    }
    std::promise<void> stopping;
    auto stopped = std::async(std::launch::async, [&] {
        stopping.set_value();
        bool previous = MemSetAutoUpdateEnabled(false);
        alive = false;
        return previous;
    });
    stopping.get_future().wait();
    assert(stopped.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout);
    {
        std::lock_guard<std::mutex> lock(gateMutex);
        releaseScan = true;
    }
    gate.notify_all();
    worker.join();
    assert(stopped.get());
    assert(!readAfterTeardown);
    int count = reads;
    // Late queued wakeups must not dereference the dead engine process.
    memUpdateMap();
    assert(reads == count);

    // Restart and failed-detach rollback preserve the previous enable state.
    alive = true;
    holdScan = false;
    assert(!MemSetAutoUpdateEnabled(true));
    cbDebugDetach(0, nullptr);
    assert(memUpdateMapEnabled && alive);
    memUpdateMap();
    assert(reads == count + 1);
    MemSetAutoUpdateEnabled(false);
    cbDebugDetach(0, nullptr);
    assert(!memUpdateMapEnabled && alive);
    MemSetAutoUpdateEnabled(true);
    detachSucceeds = true;
    cbDebugDetach(0, nullptr);
    assert(!memUpdateMapEnabled && !alive);
    memUpdateMap();
    assert(reads == count + 1 && !readAfterTeardown);
}
'''
    # Protect the lifecycle wiring as well as executing the synchronization code.
    exit_process = function("src/dbg/debugger.cpp", "static void cbExitProcess(")
    assert exit_process.split("{", 1)[1].lstrip().startswith("MemSetAutoUpdateEnabled(false);")
    create_process = function("src/dbg/debugger.cpp", "static void cbCreateProcess(")
    assert create_process.index("MemUpdateMap();") < create_process.index("MemSetAutoUpdateEnabled(true);")
    stop = function("src/dbg/commands/cmd-debug-control.cpp", "bool cbDebugStop(")
    assert stop.index("MemSetAutoUpdateEnabled(false);") < stop.index("StopDebug();")
    shutdown = function("src/dbg/debugger.cpp", "void dbgstop()")
    assert shutdown.index("MemSetAutoUpdateEnabled(false);") < shutdown.index("bStopMemMapThread = true;")
    with tempfile.TemporaryDirectory(prefix="x64dbg-map-lifecycle-") as directory:
        temp = Path(directory)
        source = temp / "test.cpp"
        source.write_text(code)
        executable = temp / "test.exe"
        flags = [] if os.name == "nt" else ["-pthread"]
        subprocess.run([args.compiler, "-std=c++14", *flags, str(source), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True, timeout=15)
    print("Background memory-map lifecycle regression tests passed")


if __name__ == "__main__":
    main()
