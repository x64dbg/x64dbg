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
#define ELFBUG_PROC_NAME_SIZE 16
#define ELFBUG_PATH_SIZE 512
#define ELFBUG_CMDLINE_SIZE 512

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
    uint32_t suspend_count; // nesting count; 0 means running
    char name[ELFBUG_THREAD_NAME_SIZE]; // /proc/<pid>/task/<tid>/comm, empty if unreadable
    char wait_reason[ELFBUG_WAIT_REASON_SIZE];
    // TODO: entry needs the start routine recorded at clone; last error needs errno
    // located through libc symbols. Both are Windows thread-list columns.
} ElfBugThreadInfo;

typedef struct
{
    pid_t pid;
    ElfBugArch arch;                        // lets a caller refuse i386 before attaching
    bool traced;                            // TracerPid != 0: already being debugged
    char name[ELFBUG_PROC_NAME_SIZE];       // /proc/<pid>/comm
    char path[ELFBUG_PATH_SIZE];            // readlink /proc/<pid>/exe
    char command_line[ELFBUG_CMDLINE_SIZE]; // /proc/<pid>/cmdline, NULs replaced by spaces
} ElfBugProcessInfo;

typedef void (*ElfBugCbCreateProcess)(pid_t pid, uint64_t entryPoint, void* userdata);
typedef void (*ElfBugCbExitProcess)(int exitCode, void* userdata);
typedef void (*ElfBugCbCreateThread)(pid_t tid, void* userdata);
typedef void (*ElfBugCbExitThread)(pid_t tid, void* userdata);
typedef void (*ElfBugCbSystemBreakpoint)(void* userdata);
typedef void (*ElfBugCbAttachBreakpoint)(void* userdata);
typedef void (*ElfBugCbBreakpoint)(uint64_t address, void* userdata);
typedef void (*ElfBugCbStep)(void* userdata);
typedef void (*ElfBugCbPaused)(void* userdata);
typedef void (*ElfBugCbException)(int signal, uint64_t address, void* userdata);
typedef void (*ElfBugCbError)(const char* error, void* userdata);
typedef void (*ElfBugCbDebugString)(const char* text, void* userdata);
typedef void (*ElfBugCbDetach)(void* userdata);
typedef void (*ElfBugCbExec)(void* userdata);

typedef struct
{
    ElfBugCbCreateProcess onCreateProcess;
    ElfBugCbExitProcess onExitProcess;
    ElfBugCbCreateThread onCreateThread;
    ElfBugCbExitThread onExitThread;
    ElfBugCbSystemBreakpoint onSystemBreakpoint;
    ElfBugCbAttachBreakpoint onAttachBreakpoint;
    ElfBugCbBreakpoint onBreakpoint;
    ElfBugCbStep onStep;
    ElfBugCbPaused onPaused;
    ElfBugCbException onException;
    ElfBugCbDetach onDetach;
    ElfBugCbExec onExec;
    ElfBugCbError onError;
    ElfBugCbDebugString onDebugString;
    void* userdata;
} ElfBugCallbacks;

ELFBUG_EXPORT uint32_t ElfBugEnumProcesses(ElfBugProcessInfo* list, uint32_t capacity);

ELFBUG_EXPORT ElfBugDebugger* ElfBugCreate(const ElfBugCallbacks* callbacks);
ELFBUG_EXPORT void ElfBugDestroy(ElfBugDebugger* dbg);

ELFBUG_EXPORT bool ElfBugInit(ElfBugDebugger* dbg, const char* path);
ELFBUG_EXPORT bool ElfBugAttach(ElfBugDebugger* dbg, pid_t pid);
ELFBUG_EXPORT void ElfBugStart(ElfBugDebugger* dbg);       // Blocks - runs debug loop
ELFBUG_EXPORT void ElfBugContinue(ElfBugDebugger* dbg);    // Thread-safe
ELFBUG_EXPORT void ElfBugStepInto(ElfBugDebugger* dbg);    // Thread-safe
ELFBUG_EXPORT void ElfBugStepOver(ElfBugDebugger* dbg);    // Thread-safe
ELFBUG_EXPORT void ElfBugPause(ElfBugDebugger* dbg);       // Thread-safe
ELFBUG_EXPORT bool ElfBugStop(ElfBugDebugger* dbg);        // Thread-safe

ELFBUG_EXPORT bool ElfBugDetach(ElfBugDebugger* dbg);      // Thread-safe

ELFBUG_EXPORT bool ElfBugIsPaused(const ElfBugDebugger* dbg);

ELFBUG_EXPORT bool ElfBugGetRegisters(const ElfBugDebugger* dbg, ElfBugRegisters* regs);
ELFBUG_EXPORT pid_t ElfBugGetPid(const ElfBugDebugger* dbg);
ELFBUG_EXPORT pid_t ElfBugGetCurrentTid(const ElfBugDebugger* dbg);

ELFBUG_EXPORT uint32_t ElfBugGetThreadList(const ElfBugDebugger* dbg, ElfBugThreadInfo* list, uint32_t capacity);
ELFBUG_EXPORT bool ElfBugSwitchThread(ElfBugDebugger* dbg, pid_t tid);

ELFBUG_EXPORT bool ElfBugSetThreadSuspended(ElfBugDebugger* dbg, pid_t tid, bool suspended);

ELFBUG_EXPORT ElfBugArch ElfBugGetArch(const ElfBugDebugger* dbg);

ELFBUG_EXPORT bool ElfBugMemRead(const ElfBugDebugger* dbg, uint64_t addr, void* dest, uint64_t size);
ELFBUG_EXPORT bool ElfBugMemWrite(const ElfBugDebugger* dbg, uint64_t addr, const void* src, uint64_t size);
ELFBUG_EXPORT bool ElfBugMemFindBaseAddr(const ElfBugDebugger* dbg, uint64_t addr, uint64_t* base, uint64_t* size);
ELFBUG_EXPORT bool ElfBugMemIsCodePtr(const ElfBugDebugger* dbg, uint64_t addr);
ELFBUG_EXPORT bool ElfBugMemIsValidPtr(const ElfBugDebugger* dbg, uint64_t addr);

ELFBUG_EXPORT bool ElfBugModBaseFromAddr(const ElfBugDebugger* dbg, uint64_t addr, uint64_t* base);

ELFBUG_EXPORT bool ElfBugModNameFromAddr(const ElfBugDebugger* dbg, uint64_t addr,
        char* buf, uint64_t bufSize, bool extension);

ELFBUG_EXPORT bool ElfBugSetRegister(ElfBugDebugger* dbg, const char* name, uint64_t value);

ELFBUG_EXPORT bool ElfBugSetBreakpoint(ElfBugDebugger* dbg, uint64_t addr);
ELFBUG_EXPORT bool ElfBugDeleteBreakpoint(ElfBugDebugger* dbg, uint64_t addr);

// True if `addr` has a breakpoint.
ELFBUG_EXPORT bool ElfBugIsBreakpointEffective(const ElfBugDebugger* dbg, uint64_t addr);

#ifdef __cplusplus
}

#include <vector>

inline std::vector<ElfBugProcessInfo> ElfBugProcessList()
{
    std::vector<ElfBugProcessInfo> list;
    for(uint32_t capacity = 512; capacity <= (1u << 20); capacity *= 2)
    {
        list.resize(capacity);
        const uint32_t total = ElfBugEnumProcesses(list.data(), capacity);
        if(total <= capacity)
        {
            list.resize(total);
            break;
        }
    }
    return list;
}

inline std::vector<ElfBugThreadInfo> ElfBugThreadList(const ElfBugDebugger* dbg)
{
    std::vector<ElfBugThreadInfo> list;
    for(uint32_t capacity = ElfBugGetThreadList(dbg, nullptr, 0); capacity != 0;)
    {
        list.resize(capacity);
        const uint32_t total = ElfBugGetThreadList(dbg, list.data(), capacity);
        if(total <= capacity)
        {
            list.resize(total);
            break;
        }
        capacity = total;
    }
    return list;
}
#endif
