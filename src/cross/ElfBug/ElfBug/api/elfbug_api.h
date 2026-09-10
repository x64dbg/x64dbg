#pragma once

#include <cstdint>
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
// Thread the last stop was reported on. 0 while running or before the first stop.
ELFBUG_EXPORT pid_t ElfBugGetCurrentTid(const ElfBugDebugger* dbg);
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
