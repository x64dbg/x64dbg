#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ELFBUG_BUILDING
#define ELFBUG_EXPORT __attribute__((visibility("default")))
#else
#define ELFBUG_EXPORT
#endif

typedef struct ElfBugDebugger ElfBugDebugger;

typedef enum
{
    ElfBugArch_Unknown = 0,
    ElfBugArch_X86_64 = 1,
    ElfBugArch_I386 = 2,
} ElfBugArch;

typedef struct
{
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rbp, rsp, rsi, rdi;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip;
    uint64_t eflags;
    uint16_t cs, ds, es, fs, gs, ss;
    uint64_t fs_base, gs_base;
} ElfBugRegisters;

#define ELFBUG_THREAD_NAME_SIZE 16
#define ELFBUG_WAIT_REASON_SIZE 32

typedef struct
{
    pid_t tid;
    uint32_t number; // 0 for the main thread, then creation order
    uint64_t rip;
    uint64_t fs_base; // thread pointer
    uint64_t user_time_ms;
    uint64_t kernel_time_ms;
    uint64_t start_time_ms; // unix epoch milliseconds, 0 if unknown
    int32_t nice;
    int32_t policy; // SCHED_* value, -1 if unknown
    int32_t rt_priority; // 1..99 for FIFO and RR, 0 otherwise
    uint32_t suspend_count; // 0 or 1
    char name[ELFBUG_THREAD_NAME_SIZE]; // /proc/<pid>/task/<tid>/comm, empty if unreadable
    // Kernel function the thread was blocked in when the debugger stopped it, empty if
    // it was running or stopped on its own.
    char wait_reason[ELFBUG_WAIT_REASON_SIZE];
    // TODO: entry needs the start routine recorded at clone; last error needs errno
    // located through libc symbols. Both are Windows thread-list columns.
} ElfBugThreadInfo;

typedef void (*ElfBugCbCreateProcess)(pid_t pid, uint64_t entryPoint, void* userdata);
typedef void (*ElfBugCbExitProcess)(int exitCode, void* userdata);
typedef void (*ElfBugCbCreateThread)(pid_t tid, void* userdata);
typedef void (*ElfBugCbExitThread)(pid_t tid, void* userdata);
typedef void (*ElfBugCbSystemBreakpoint)(void* userdata);
typedef void (*ElfBugCbBreakpoint)(uint64_t address, void* userdata);
typedef void (*ElfBugCbStep)(void* userdata);
typedef void (*ElfBugCbPaused)(void* userdata);
// Signal delivery stop. `address` is si_addr for faults, 0 otherwise.
typedef void (*ElfBugCbException)(int signal, uint64_t address, void* userdata);
typedef void (*ElfBugCbError)(const char* error, void* userdata);
typedef void (*ElfBugCbDebugString)(const char* text, void* userdata);

typedef struct
{
    ElfBugCbCreateProcess onCreateProcess;
    ElfBugCbExitProcess onExitProcess;
    ElfBugCbCreateThread onCreateThread;
    ElfBugCbExitThread onExitThread;
    ElfBugCbSystemBreakpoint onSystemBreakpoint;
    ElfBugCbBreakpoint onBreakpoint;
    ElfBugCbStep onStep;
    ElfBugCbPaused onPaused;
    ElfBugCbException onException;
    ElfBugCbError onError;
    ElfBugCbDebugString onDebugString;
    void* userdata;
} ElfBugCallbacks;

ELFBUG_EXPORT ElfBugDebugger* ElfBugCreate(const ElfBugCallbacks* callbacks);
ELFBUG_EXPORT void ElfBugDestroy(const ElfBugDebugger* dbg);

ELFBUG_EXPORT bool ElfBugInit(ElfBugDebugger* dbg, const char* path);
ELFBUG_EXPORT void ElfBugStart(ElfBugDebugger* dbg);      // Blocks - runs debug loop
ELFBUG_EXPORT void ElfBugContinue(ElfBugDebugger* dbg);    // Thread-safe
ELFBUG_EXPORT void ElfBugStepInto(ElfBugDebugger* dbg);    // Thread-safe
ELFBUG_EXPORT void ElfBugStepOver(ElfBugDebugger* dbg);    // Thread-safe
ELFBUG_EXPORT void ElfBugPause(ElfBugDebugger* dbg);       // Thread-safe
ELFBUG_EXPORT bool ElfBugStop(ElfBugDebugger* dbg);        // Thread-safe

ELFBUG_EXPORT bool ElfBugGetRegisters(const ElfBugDebugger* dbg, ElfBugRegisters* regs);
ELFBUG_EXPORT pid_t ElfBugGetPid(const ElfBugDebugger* dbg);
// Current thread while paused: the one that reported the stop, or the one last switched
// to. 0 while running or before the first stop.
ELFBUG_EXPORT pid_t ElfBugGetCurrentTid(const ElfBugDebugger* dbg);

// Threads as of the last stop or thread event, ordered by number. Returns the total
// count and copies up to `capacity` entries when `list` is non-null. Empty after exit.
ELFBUG_EXPORT uint32_t ElfBugGetThreadList(const ElfBugDebugger* dbg, ElfBugThreadInfo* list, uint32_t capacity);

// Make `tid` the current thread: registers, steps and the next resume act on it.
// Fails unless the debuggee is paused and `tid` is one of its stopped threads.
ELFBUG_EXPORT bool ElfBugSwitchThread(ElfBugDebugger* dbg, pid_t tid);

// Freeze or thaw one thread. A suspended thread stays stopped across Continue and steps
// until resumed. Paused only.
ELFBUG_EXPORT bool ElfBugSetThreadSuspended(ElfBugDebugger* dbg, pid_t tid, bool suspended);

ELFBUG_EXPORT ElfBugArch ElfBugGetArch(const ElfBugDebugger* dbg);

ELFBUG_EXPORT bool ElfBugMemRead(const ElfBugDebugger* dbg, uint64_t addr, void* dest, uint64_t size);
ELFBUG_EXPORT bool ElfBugMemWrite(const ElfBugDebugger* dbg, uint64_t addr, const void* src, uint64_t size);
ELFBUG_EXPORT bool ElfBugMemFindBaseAddr(const ElfBugDebugger* dbg, uint64_t addr, uint64_t* base, uint64_t* size);
ELFBUG_EXPORT bool ElfBugMemIsCodePtr(const ElfBugDebugger* dbg, uint64_t addr);
ELFBUG_EXPORT bool ElfBugMemIsValidPtr(const ElfBugDebugger* dbg, uint64_t addr);

// Lowest mapped start address of the module containing `addr`, or 0 if the
// region is anonymous (heap/stack/vdso/etc).
ELFBUG_EXPORT bool ElfBugModBaseFromAddr(const ElfBugDebugger* dbg, uint64_t addr, uint64_t* base);

// Module basename for the region containing `addr`. When `extension` is false,
// trims `.so` but keeps any version suffix (`libc.so.6` -> `libc.6`).
ELFBUG_EXPORT bool ElfBugModNameFromAddr(const ElfBugDebugger* dbg, uint64_t addr,
        char* buf, uint64_t bufSize, bool extension);

// Write a register. Tracee must be in ptrace-stop. Names:
// "csp"/"rsp", "cip"/"rip", "rax"..."rdi", "rbp", "r8"..."r15".
ELFBUG_EXPORT bool ElfBugSetRegister(const ElfBugDebugger* dbg, const char* name, uint64_t value);

ELFBUG_EXPORT bool ElfBugSetBreakpoint(ElfBugDebugger* dbg, uint64_t addr);
ELFBUG_EXPORT bool ElfBugDeleteBreakpoint(ElfBugDebugger* dbg, uint64_t addr);

// True if `addr` has a breakpoint. Pending queue overrides applied set.
ELFBUG_EXPORT bool ElfBugIsBreakpointEffective(const ElfBugDebugger* dbg, uint64_t addr);

#ifdef __cplusplus
}
#endif
