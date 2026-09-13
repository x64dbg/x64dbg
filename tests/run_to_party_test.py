"""Hermetic tests of the real runtoparty.cpp with a simulated debug engine.

Run: python tests/run_to_party_test.py [--compiler clang++]
No debuggee is launched. This tests ownership, fallback and callback behavior;
it does not substitute for live TitanEngine/GleeBug integration testing.
"""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SHIM = r'''
#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <map>
#include <vector>
using duint = uintptr_t;
using DWORD = unsigned long;
using LPCVOID = const void*;
using TITANCBSTEP = void(*)();
using STEPFUNCTION = void(*)(TITANCBSTEP);
using MemCallback = void(*)(const void*);
#define QT_TRANSLATE_NOOP(context, text) text
void dprintf(const char*, ...) {}
void dputs(const char*) {}
const DWORD PAGE_EXECUTE = 0x10, PAGE_EXECUTE_READ = 0x20,
    PAGE_EXECUTE_READWRITE = 0x40, PAGE_EXECUTE_WRITECOPY = 0x80,
    PAGE_GUARD = 0x100, MEM_COMMIT = 0x1000;
const DWORD ERROR_INVALID_PARAMETER = 87, UE_CIP = 1, UE_MEMORY_EXECUTE = 6;
const int BPMEMORY = 1;
struct MEMORY_BASIC_INFORMATION { void* BaseAddress; duint RegionSize; DWORD State, Protect; };
struct MODINFO { duint base, size; int party; };
struct BREAKPOINT { int type; bool enabled; };
struct ProcessInfo { int hProcess; } processInfo{1};
auto fdProcessInfo = &processInfo;
struct DebugData { DWORD dwThreadId; } debugData{7};
int hActiveThread = 0;
int lockDepth = 0;
struct Guard { Guard() { ++lockDepth; } ~Guard() { --lockDepth; } };
#define EXCLUSIVE_ACQUIRE(x) Guard guard
#define SHARED_ACQUIRE(x) Guard guard
std::vector<MEMORY_BASIC_INFORMATION> regions;
std::vector<MODINFO> modules;
std::vector<BREAKPOINT> userBreakpoints;
struct Installed { duint size; MemCallback callback; };
std::map<duint, Installed> installed;
DWORD lastError = 0;
duint queryFailure = duint(-1), cip = 0;
int failInstallAt = -1, installCalls = 0, removeCalls = 0;
TITANCBSTEP pendingStep = nullptr;
bool steppedOver = false;
void StepIntoWow64(TITANCBSTEP callback) { pendingStep = callback; steppedOver = false; }
void StepOverWrapper(TITANCBSTEP callback) { pendingStep = callback; steppedOver = true; }
bool RunToParty(int, TITANCBSTEP, STEPFUNCTION = StepIntoWow64);
bool RunToPartyIsActive();
void RunToPartyClear();
void RunToPartyOnModuleChange();
DebugData* GetDebugData() { return &debugData; }
int ThreadGetHandle(DWORD tid) { return int(tid); }
duint GetContextDataEx(int, DWORD) { return cip; }
DWORD GetLastError() { return lastError; }
size_t VirtualQueryEx(int, LPCVOID address, MEMORY_BASIC_INFORMATION* mbi, size_t)
{
    auto addr = duint(address);
    if(addr == queryFailure) { lastError = 5; return 0; }
    for(auto region : regions)
        if(addr >= duint(region.BaseAddress) && addr < duint(region.BaseAddress) + region.RegionSize)
        { *mbi = region; return sizeof(*mbi); }
    lastError = ERROR_INVALID_PARAMETER;
    return 0;
}
void BpGetList(std::vector<BREAKPOINT>* result) { *result = userBreakpoints; }
template<class F> void ModEnum(F f) { for(auto mod : modules) f(mod); }
int ModGetParty(duint addr)
{
    for(auto mod : modules)
        if(addr >= mod.base && addr < mod.base + mod.size) return mod.party;
    return 0;
}
bool SetMemoryBPXEx(duint addr, duint size, DWORD type, bool restore, MemCallback callback)
{
    assert(type == UE_MEMORY_EXECUTE && restore && size);
    if(installCalls++ == failInstallAt) return false;
    for(auto bp : installed)
        if(addr < bp.first + bp.second.size && bp.first < addr + size) return false;
    installed.emplace(addr, Installed{size, callback});
    return true;
}
bool RemoveMemoryBPX(duint addr, duint size)
{
    assert(installed.count(addr) && installed.at(addr).size == size);
    installed.erase(addr);
    ++removeCalls;
    return true;
}
'''
TEST = r'''
#include "runtoparty.cpp"
#include <iostream>
int completed = 0;
void done()
{
    assert(lockDepth == 0 && !RunToPartyIsActive() && installed.empty());
    ++completed;
}
void reset()
{
    RunToPartyClear();
    regions = {{(void*)0, 0x1000, 0, 0},
               {(void*)0x1000, 0x5000, MEM_COMMIT, PAGE_EXECUTE_READ},
               {(void*)0x6000, 0xA000, MEM_COMMIT, 4}};
    modules = {{0x2000, 0x2000, 1}, {0x4000, 0x1000, 0}};
    userBreakpoints.clear();
    queryFailure = duint(-1);
    failInstallAt = -1;
    installCalls = removeCalls = completed = 0;
    pendingStep = nullptr;
    cip = 0x2000;
}
void hit(duint address)
{
    cip = address;
    for(auto bp : installed)
        if(address >= bp.first && address < bp.first + bp.second.size)
        { auto callback = bp.second.callback; callback((void*)address); return; }
    assert(false);
}
void again()
{
    done();
    assert(RunToParty(1, done)); // cleanup and unlock must precede the callback
}
int main()
{
    reset();
    assert(RunToParty(0, done));
    assert(installed.size() == 3); // module boundaries plus private executable memory
    assert(installed.at(0x1000).size == 0x1000);
    assert(installed.at(0x4000).size == 0x1000);
    assert(installed.at(0x5000).size == 0x1000);
    assert(!RunToParty(1, done)); // cannot replace an active operation
    debugData.dwThreadId = 99;
    hit(0x5000);
    assert(completed == 1 && hActiveThread == 99);

    reset();
    assert(RunToParty(1, done));
    assert(installed.size() == 1 && installed.at(0x2000).size == 0x2000);
    hit(0x2000);
    assert(completed == 1);

    reset();
    failInstallAt = 1;
    assert(!RunToParty(0, done));
    assert(!RunToPartyIsActive() && installed.empty() && removeCalls == 1);

    reset();
    userBreakpoints.push_back({BPMEMORY, true});
    assert(!RunToParty(0, done) && installCalls == 0);
    userBreakpoints[0].enabled = false;
    assert(RunToParty(0, done));
    RunToPartyClear();

    reset();
    regions[1].Protect |= PAGE_GUARD;
    assert(!RunToParty(0, done) && installCalls == 0);
    reset();
    queryFailure = 0x6000;
    assert(!RunToParty(0, done) && installCalls == 0);
    reset();
    assert(!RunToParty(42, done) && !RunToPartyIsActive());

    reset();
    assert(RunToParty(0, done, StepOverWrapper));
    RunToPartyOnModuleChange(); // excluded CIP: keep operation, use saved step mode
    assert(installed.empty() && RunToPartyIsActive() && pendingStep && steppedOver);
    auto next = pendingStep;
    cip = 0x1000;
    next();
    assert(completed == 1 && !RunToPartyIsActive());

    reset();
    cip = 0x1000;
    assert(RunToParty(0, done, StepOverWrapper));
    RunToPartyOnModuleChange(); // even a matching loader context is not a trace step
    assert(completed == 0 && installed.empty() && pendingStep && steppedOver);
    pendingStep();
    assert(completed == 1 && !RunToPartyIsActive());

    reset();
    assert(RunToParty(0, done));
    auto staleHit = installed.begin()->second.callback;
    RunToPartyClear(); // manual pause, normal break, or exit
    staleHit(nullptr);
    assert(!RunToPartyIsActive() && completed == 0 && !pendingStep);

    reset();
    assert(RunToParty(0, again));
    hit(0x1000);
    assert(completed == 1 && RunToPartyIsActive());
    hit(0x2000);
    assert(completed == 2);

    reset();
    assert(RunToParty(1, done));
    modules[0].party = 0; // no longer the requested party: don't deliver a false hit
    hit(0x2000);
    assert(completed == 0 && pendingStep && !steppedOver && installed.empty());
    RunToPartyClear();
    std::cout << "RunToParty primitive tests passed\n";
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="clang++")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="x64dbg-runtoparty-") as directory:
        temp = Path(directory)
        # Copy the production implementation unchanged; replace only its external
        # engine/platform headers with test doubles in this isolated directory.
        shutil.copyfile(ROOT / "src/dbg/runtoparty.cpp", temp / "runtoparty.cpp")
        (temp / "shim.h").write_text(SHIM)
        for header in ("runtoparty.h", "breakpoint.h", "console.h", "module.h", "thread.h", "threading.h"):
            (temp / header).write_text('#include "shim.h"\n')
        (temp / "test.cpp").write_text(TEST)
        executable = temp / "test.exe"
        subprocess.run([args.compiler, "-std=c++14", str(temp / "test.cpp"), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
