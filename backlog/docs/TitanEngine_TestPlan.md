# TitanEngine Test Plan

## Overview

This document outlines a comprehensive test suite for the TitanEngine interface used by x64dbg. The goal is to provide confidence when refactoring that core debugger functionality works as expected.

## Context Engineering Philosophy

Every token of output goes into the LLM context. Design all test output for minimal, actionable information:

- **Success output**: `"87/87 tests passed"` (single line)
- **Failure output**: Only the failing test, assertion, file:line, and minimal context
- **Never**: Print progress per test, verbose logs, or "test X passed" for each test
- **Exit codes**: 0 = all pass, non-zero = failure count

This matters because:
1. LLM context only accumulates (no removal)
2. Many agents may run tests repeatedly during refactoring
3. Wasted context = wasted cost and degraded reasoning

## Test Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      Test Runner (C++)                          │
│  - Direct TitanEngine API calls                                 │
│  - Result validation                                            │
│  - Test orchestration                                           │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    TitanEngine API                              │
│  InitDebugW, SetBPX, SetHardwareBreakPoint, StepInto, etc.     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Test Executables                             │
│  - Exported symbols for resolvable addresses                    │
│  - Deterministic behavior                                       │
│  - Coverage of all debug event types                            │
└─────────────────────────────────────────────────────────────────┘
```

## TitanEngine API Coverage Matrix

### 1. Debugger Initialization & Control
| Function | Test Priority | Notes |
|----------|--------------|-------|
| `InitDebugW` | Critical | Process creation with various parameters |
| `StopDebug` | Critical | Clean shutdown |
| `AttachDebugger` | Critical | Attach to running process |
| `DetachDebuggerEx` | Critical | Detach without terminating |
| `DebugLoop` | Critical | Main event loop |
| `IsFileBeingDebugged` | Medium | State query |
| `SetEngineVariable` | Medium | Engine configuration |

### 2. Software Breakpoints
| Function | Test Priority | Notes |
|----------|--------------|-------|
| `SetBPX` | Critical | All breakpoint types |
| `DeleteBPX` | Critical | Removal, including during execution |
| `IsBPXEnabled` | Medium | State query |
| `SetBPXOptions` | Medium | Default breakpoint type |
| `RemoveAllBreakPoints` | Medium | Bulk removal |

**Breakpoint Types to Test:**
- `UE_BREAKPOINT_INT3` (0xCC)
- `UE_BREAKPOINT_LONG_INT3` (0xCD 0x03)
- `UE_BREAKPOINT_UD2` (0x0F 0x0B)

### 3. Hardware Breakpoints
| Function | Test Priority | Notes |
|----------|--------------|-------|
| `SetHardwareBreakPoint` | Critical | All 4 DR registers |
| `DeleteHardwareBreakPoint` | Critical | Register cleanup |
| `GetUnusedHardwareBreakPointRegister` | Medium | Allocation tracking |

**Hardware Breakpoint Types:**
- `UE_HARDWARE_EXECUTE` - Execution breakpoint
- `UE_HARDWARE_WRITE` - Write watchpoint
- `UE_HARDWARE_READWRITE` - Read/Write watchpoint

**Hardware Breakpoint Sizes:**
- `UE_HARDWARE_SIZE_1` (1 byte)
- `UE_HARDWARE_SIZE_2` (2 bytes)
- `UE_HARDWARE_SIZE_4` (4 bytes)
- `UE_HARDWARE_SIZE_8` (8 bytes, x64 only)

### 4. Memory Breakpoints
| Function | Test Priority | Notes |
|----------|--------------|-------|
| `SetMemoryBPXEx` | Critical | Page protection based |
| `RemoveMemoryBPX` | Critical | Restore protection |

**Memory Breakpoint Types:**
- `UE_MEMORY_READ` - Read access
- `UE_MEMORY_WRITE` - Write access
- `UE_MEMORY_EXECUTE` - Execute access
- `UE_MEMORY` - All access

### 5. Stepping
| Function | Test Priority | Notes |
|----------|--------------|-------|
| `StepInto` | Critical | Single instruction |
| `StepOver` | Critical | Skip calls |

### 6. Context Manipulation
| Function | Test Priority | Notes |
|----------|--------------|-------|
| `GetContextDataEx` | Critical | Individual registers |
| `SetContextDataEx` | Critical | Modify execution |
| `GetFullContextDataEx` | Critical | Complete context |
| `SetFullContextDataEx` | Critical | Bulk modification |
| `GetAVXContext` | Medium | AVX registers |
| `SetAVXContext` | Medium | AVX modification |
| `GetAVX512Context` | Low | AVX-512 (if supported) |
| `SetAVX512Context` | Low | AVX-512 modification |

### 7. Custom Event Handlers
| Handler | Test Priority | Notes |
|---------|--------------|-------|
| `UE_CH_CREATEPROCESS` | Critical | Process creation |
| `UE_CH_EXITPROCESS` | Critical | Process termination |
| `UE_CH_CREATETHREAD` | Critical | Thread creation |
| `UE_CH_EXITTHREAD` | Critical | Thread termination |
| `UE_CH_LOADDLL` | Critical | DLL load |
| `UE_CH_UNLOADDLL` | Critical | DLL unload |
| `UE_CH_SYSTEMBREAKPOINT` | Critical | Initial break |
| `UE_CH_OUTPUTDEBUGSTRING` | Medium | Debug output |
| `UE_CH_UNHANDLEDEXCEPTION` | Critical | Exception handling |
| `UE_CH_DEBUGEVENT` | Medium | Generic events |

### 8. Memory Operations
| Function | Test Priority | Notes |
|----------|--------------|-------|
| `MemoryReadSafe` | Critical | Cross-process read |
| `MemoryWriteSafe` | Critical | Cross-process write |

### 9. Process/Thread Handles
| Function | Test Priority | Notes |
|----------|--------------|-------|
| `TitanOpenProcess` | Medium | Process access |
| `TitanOpenThread` | Medium | Thread access |
| `GetPEBLocation` | Medium | PEB address |
| `GetTEBLocation` | Medium | TEB address |

---

## Test Executable Specifications

### TestExe_Breakpoints.exe
**Purpose:** Test all breakpoint types at known locations

```cpp
// All functions are exported for address resolution
extern "C" {
    __declspec(dllexport) volatile int g_Counter = 0;

    // Software breakpoint targets
    __declspec(dllexport) void __cdecl BpTarget_Simple() {
        g_Counter++;
    }

    __declspec(dllexport) void __cdecl BpTarget_Loop() {
        for (int i = 0; i < 10; i++) {
            g_Counter++;
        }
    }

    __declspec(dllexport) void __cdecl BpTarget_Nested() {
        BpTarget_Simple();
        BpTarget_Simple();
    }

    // Hardware breakpoint data targets
    __declspec(dllexport) volatile int g_WatchVar1 = 0;
    __declspec(dllexport) volatile int g_WatchVar2 = 0;
    __declspec(dllexport) volatile char g_WatchByte = 0;
    __declspec(dllexport) volatile short g_WatchWord = 0;
    __declspec(dllexport) volatile long long g_WatchQword = 0;

    __declspec(dllexport) void __cdecl HwBp_WriteVar1() {
        g_WatchVar1 = 42;
    }

    __declspec(dllexport) void __cdecl HwBp_ReadVar1() {
        volatile int x = g_WatchVar1;
    }

    __declspec(dllexport) void __cdecl HwBp_WriteAllSizes() {
        g_WatchByte = 1;
        g_WatchWord = 2;
        g_WatchVar1 = 4;
        g_WatchQword = 8;
    }

    // Memory breakpoint targets (aligned to page boundaries)
    __declspec(dllexport) __declspec(align(4096)) char g_MemPage[4096] = {0};

    __declspec(dllexport) void __cdecl MemBp_WritePage() {
        g_MemPage[0] = 'A';
        g_MemPage[100] = 'B';
    }

    __declspec(dllexport) void __cdecl MemBp_ReadPage() {
        volatile char x = g_MemPage[0];
    }

    // Entry point that calls all functions
    __declspec(dllexport) void __cdecl RunAllTests() {
        BpTarget_Simple();
        BpTarget_Loop();
        BpTarget_Nested();
        HwBp_WriteVar1();
        HwBp_ReadVar1();
        HwBp_WriteAllSizes();
        MemBp_WritePage();
        MemBp_ReadPage();
    }
}

int main() {
    RunAllTests();
    return g_Counter;
}
```

### TestExe_Threading.exe
**Purpose:** Multi-threaded debugging scenarios

```cpp
extern "C" {
    __declspec(dllexport) volatile LONG g_ThreadCounter = 0;
    __declspec(dllexport) volatile LONG g_BpHitCount = 0;
    __declspec(dllexport) HANDLE g_StartEvent = NULL;

    // Target for multi-thread breakpoint testing
    __declspec(dllexport) void __cdecl ThreadTarget() {
        InterlockedIncrement(&g_ThreadCounter);
    }

    // Target for simultaneous breakpoint hits
    __declspec(dllexport) void __cdecl RaceTarget() {
        InterlockedIncrement(&g_BpHitCount);
    }

    DWORD WINAPI ThreadProc(LPVOID param) {
        WaitForSingleObject(g_StartEvent, INFINITE);
        for (int i = 0; i < 1000; i++) {
            RaceTarget();
        }
        return 0;
    }

    __declspec(dllexport) void __cdecl StartThreads(int count) {
        g_StartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        HANDLE threads[32];

        for (int i = 0; i < count && i < 32; i++) {
            threads[i] = CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL);
        }

        SetEvent(g_StartEvent);  // Release all threads simultaneously

        WaitForMultipleObjects(count, threads, TRUE, INFINITE);

        for (int i = 0; i < count && i < 32; i++) {
            CloseHandle(threads[i]);
        }
        CloseHandle(g_StartEvent);
    }
}

int main() {
    StartThreads(5);
    return (int)g_BpHitCount;
}
```

### TestExe_Exceptions.exe
**Purpose:** Test all exception types and handling

```cpp
extern "C" {
    __declspec(dllexport) volatile int g_ExceptionCount = 0;

    // Access violation - read
    __declspec(dllexport) void __cdecl TriggerAV_Read() {
        volatile int* p = (volatile int*)0x1;
        volatile int x = *p;
    }

    // Access violation - write
    __declspec(dllexport) void __cdecl TriggerAV_Write() {
        volatile int* p = (volatile int*)0x1;
        *p = 42;
    }

    // Access violation - execute
    __declspec(dllexport) void __cdecl TriggerAV_Execute() {
        void (*f)() = (void(*)())0x1;
        f();
    }

    // INT3 breakpoint
    __declspec(dllexport) void __cdecl TriggerInt3() {
        __debugbreak();
    }

    // Single step (via trap flag)
    __declspec(dllexport) void __cdecl TriggerSingleStep() {
        // Set trap flag - will be handled by debugger
        __asm {
            pushfd
            or dword ptr [esp], 0x100
            popfd
            nop  // Single step will fire here
        }
    }

    // Integer divide by zero
    __declspec(dllexport) void __cdecl TriggerDivZero() {
        volatile int x = 1;
        volatile int y = 0;
        volatile int z = x / y;
    }

    // Integer overflow (INTO)
    __declspec(dllexport) void __cdecl TriggerOverflow() {
        __asm {
            mov eax, 0x7FFFFFFF
            add eax, 1
            into
        }
    }

    // Illegal instruction (UD2)
    __declspec(dllexport) void __cdecl TriggerIllegalInsn() {
        __asm {
            ud2
        }
    }

    // Privileged instruction
    __declspec(dllexport) void __cdecl TriggerPrivInsn() {
        __asm {
            cli  // Privileged - will cause exception
        }
    }

    // Stack overflow
    __declspec(dllexport) void __cdecl TriggerStackOverflow() {
        volatile char buf[65536];
        buf[0] = 1;
        TriggerStackOverflow();  // Recursive
    }

    // Guard page violation
    __declspec(dllexport) void __cdecl TriggerGuardPage() {
        LPVOID mem = VirtualAlloc(NULL, 4096, MEM_COMMIT, PAGE_READWRITE | PAGE_GUARD);
        volatile char* p = (volatile char*)mem;
        *p = 'A';  // First access triggers guard page
        VirtualFree(mem, 0, MEM_RELEASE);
    }

    // Breakpoint on data (for EXCEPTION_SINGLE_STEP from DR)
    __declspec(dllexport) volatile int g_DataBpTarget = 0;

    __declspec(dllexport) void __cdecl WriteDataBpTarget() {
        g_DataBpTarget = 42;
    }

    // OutputDebugString
    __declspec(dllexport) void __cdecl TriggerDebugString() {
        OutputDebugStringW(L"Test debug output message");
    }

    // Structured Exception Handling test
    __declspec(dllexport) int __cdecl TestSEH() {
        __try {
            TriggerAV_Read();
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            g_ExceptionCount++;
            return 1;  // Exception caught
        }
        return 0;  // Should not reach
    }

    // Vectored Exception Handler test
    LONG WINAPI VEH_Handler(PEXCEPTION_POINTERS ep) {
        g_ExceptionCount++;
        return EXCEPTION_CONTINUE_SEARCH;
    }

    __declspec(dllexport) void __cdecl SetupVEH() {
        AddVectoredExceptionHandler(1, VEH_Handler);
    }
}
```

### TestExe_Stepping.exe
**Purpose:** Test stepping scenarios including step-into-breakpoint

```cpp
extern "C" {
    __declspec(dllexport) volatile int g_StepCount = 0;

    // Simple function for step-into
    __declspec(dllexport) void __cdecl StepTarget_Simple() {
        g_StepCount++;
    }

    // Nested calls for step-over vs step-into
    __declspec(dllexport) void __cdecl StepTarget_Inner() {
        g_StepCount += 10;
    }

    __declspec(dllexport) void __cdecl StepTarget_Outer() {
        StepTarget_Inner();  // Step over should skip this
        g_StepCount++;
    }

    // REP instruction (requires special handling)
    __declspec(dllexport) void __cdecl StepTarget_Rep() {
        char src[16] = "Hello World!";
        char dst[16];
        __asm {
            lea esi, src
            lea edi, dst
            mov ecx, 16
            rep movsb  // Step over should skip entire REP
        }
    }

    // Loop with breakpoint inside
    __declspec(dllexport) void __cdecl StepTarget_LoopBody() {
        g_StepCount++;
    }

    __declspec(dllexport) void __cdecl StepTarget_Loop() {
        for (int i = 0; i < 5; i++) {
            StepTarget_LoopBody();  // BP here, step should continue loop
        }
    }

    // Conditional branch for step testing
    __declspec(dllexport) void __cdecl StepTarget_Branch(int cond) {
        if (cond) {
            g_StepCount += 100;
        } else {
            g_StepCount += 1;
        }
    }

    // INT3 followed by another instruction (step after INT3)
    __declspec(dllexport) void __cdecl StepTarget_AfterInt3() {
        __debugbreak();
        g_StepCount++;  // Stepping here after INT3
    }

    // Hardware BP target followed by software BP
    __declspec(dllexport) volatile int g_HwBpVar = 0;

    __declspec(dllexport) void __cdecl StepTarget_HwThenSw() {
        g_HwBpVar = 1;      // HW breakpoint here
        StepTarget_Simple(); // SW breakpoint here - test stepping into
    }

    // Call instruction for step-over
    __declspec(dllexport) void __cdecl StepTarget_Call() {
        StepTarget_Simple();  // Step over should land after this
        StepTarget_Simple();  // And then here
    }
}
```

### TestExe_DllLoad.exe + TestDll.dll
**Purpose:** Test DLL load/unload events

```cpp
// TestExe_DllLoad.exe
extern "C" {
    __declspec(dllexport) HMODULE g_LoadedDll = NULL;

    __declspec(dllexport) void __cdecl LoadTestDll() {
        g_LoadedDll = LoadLibraryW(L"TestDll.dll");
    }

    __declspec(dllexport) void __cdecl UnloadTestDll() {
        if (g_LoadedDll) {
            FreeLibrary(g_LoadedDll);
            g_LoadedDll = NULL;
        }
    }

    __declspec(dllexport) void __cdecl CallDllFunction() {
        if (g_LoadedDll) {
            typedef void (*DllFunc)();
            DllFunc f = (DllFunc)GetProcAddress(g_LoadedDll, "DllExportedFunc");
            if (f) f();
        }
    }
}

// TestDll.dll
extern "C" {
    __declspec(dllexport) volatile int g_DllCounter = 0;

    __declspec(dllexport) void __cdecl DllExportedFunc() {
        g_DllCounter++;
    }

    __declspec(dllexport) void __cdecl DllBpTarget() {
        g_DllCounter += 10;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            g_DllCounter = 1;
            break;
        case DLL_THREAD_ATTACH:
            InterlockedIncrement(&g_DllCounter);
            break;
        case DLL_THREAD_DETACH:
            InterlockedDecrement(&g_DllCounter);
            break;
        case DLL_PROCESS_DETACH:
            g_DllCounter = 0;
            break;
    }
    return TRUE;
}
```

### TestExe_Attach.exe
**Purpose:** Test attach/detach scenarios

```cpp
extern "C" {
    __declspec(dllexport) volatile BOOL g_Running = TRUE;
    __declspec(dllexport) volatile int g_LoopCount = 0;
    __declspec(dllexport) HANDLE g_ReadyEvent = NULL;

    __declspec(dllexport) void __cdecl AttachTarget() {
        g_LoopCount++;
    }

    __declspec(dllexport) void __cdecl SignalReady() {
        if (g_ReadyEvent) SetEvent(g_ReadyEvent);
    }

    __declspec(dllexport) void __cdecl StopLoop() {
        g_Running = FALSE;
    }
}

int main(int argc, char* argv[]) {
    // Create named event for synchronization with test runner
    g_ReadyEvent = CreateEventW(NULL, TRUE, FALSE, L"TestAttachReady");

    SignalReady();

    while (g_Running) {
        AttachTarget();
        Sleep(10);
    }

    CloseHandle(g_ReadyEvent);
    return g_LoopCount;
}
```

### TestExe_Context.exe
**Purpose:** Test register reading/writing for all register types

```cpp
extern "C" {
    // Storage for context verification
    __declspec(dllexport) ULONG_PTR g_SavedRegs[32] = {0};
    __declspec(dllexport) double g_SavedFPU[8] = {0};
    __declspec(dllexport) __m128 g_SavedXMM[16];
    __declspec(dllexport) __m256 g_SavedYMM[16];

    // Checkpoint function - debugger reads/writes context here
    __declspec(dllexport) void __cdecl ContextCheckpoint() {
        __debugbreak();
    }

    // Set known values in all registers then checkpoint
    __declspec(dllexport) void __cdecl SetAllRegs() {
        // General purpose
        __asm {
            mov eax, 0xAAAAAAAA
            mov ebx, 0xBBBBBBBB
            mov ecx, 0xCCCCCCCC
            mov edx, 0xDDDDDDDD
            mov esi, 0xEEEEEEEE
            mov edi, 0xFFFFFFFF
        }
        ContextCheckpoint();
    }

    // FPU test
    __declspec(dllexport) void __cdecl SetFPURegs() {
        double vals[8] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
        __asm {
            fld qword ptr [vals]
            fld qword ptr [vals+8]
            fld qword ptr [vals+16]
        }
        ContextCheckpoint();
        __asm {
            fstp st(0)
            fstp st(0)
            fstp st(0)
        }
    }

    // XMM test
    __declspec(dllexport) void __cdecl SetXMMRegs() {
        __m128 val = _mm_set_ps(1.0f, 2.0f, 3.0f, 4.0f);
        __asm {
            movaps xmm0, val
            movaps xmm1, val
            movaps xmm2, val
        }
        ContextCheckpoint();
    }

    // YMM test (AVX)
    __declspec(dllexport) void __cdecl SetYMMRegs() {
        if (!IsProcessorFeaturePresent(PF_AVX_INSTRUCTIONS_AVAILABLE)) {
            return;
        }
        __m256 val = _mm256_set_ps(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f);
        _mm256_storeu_ps((float*)&g_SavedYMM[0], val);
        __asm {
            vmovaps ymm0, ymmword ptr [g_SavedYMM]
        }
        ContextCheckpoint();
    }

    // Debug registers (read only from user mode)
    __declspec(dllexport) void __cdecl ReadDRRegs() {
        // Debugger will set DR0-DR3 and we verify via GetContextDataEx
        ContextCheckpoint();
    }

    // Flags test
    __declspec(dllexport) void __cdecl TestFlags() {
        __asm {
            // Set known flags
            mov eax, 0
            cmp eax, 0  // ZF=1, CF=0
        }
        ContextCheckpoint();
        __asm {
            mov eax, 0
            sub eax, 1  // CF=1, ZF=0
        }
        ContextCheckpoint();
    }
}
```

---

## Test Scenarios

### Category 1: Software Breakpoints

| ID | Scenario | Expected Result |
|----|----------|-----------------|
| SW-01 | Set INT3 breakpoint, verify hit | Callback invoked, IP at BP address |
| SW-02 | Set LONG_INT3 breakpoint, verify hit | Callback invoked |
| SW-03 | Set UD2 breakpoint, verify hit | Callback invoked |
| SW-04 | Delete breakpoint, verify no hit | Continue without stop |
| SW-05 | Multiple BPs on same address | Latest callback wins |
| SW-06 | BP in loop, count hits | Exact hit count |
| SW-07 | Delete BP during callback | Clean removal |
| SW-08 | IsBPXEnabled accuracy | True when set, false when deleted |
| SW-09 | RemoveAllBreakPoints | All BPs cleared |
| SW-10 | BP on DLL function | Hit after LoadLibrary |

### Category 2: Hardware Breakpoints

| ID | Scenario | Expected Result |
|----|----------|-----------------|
| HW-01 | Execute BP (DR0-DR3) | Stop before instruction |
| HW-02 | Write BP size 1 | Stop on byte write |
| HW-03 | Write BP size 2 | Stop on word write |
| HW-04 | Write BP size 4 | Stop on dword write |
| HW-05 | Write BP size 8 (x64) | Stop on qword write |
| HW-06 | Read/Write BP | Stop on read or write |
| HW-07 | All 4 DR registers used | All fire correctly |
| HW-08 | GetUnusedHardwareBreakPointRegister | Returns available DR |
| HW-09 | Delete HW BP | DR cleared, no further hits |
| HW-10 | HW BP + SW BP same function | Both fire in order |

### Category 3: Memory Breakpoints

| ID | Scenario | Expected Result |
|----|----------|-----------------|
| MB-01 | Memory read BP | Stop on read access |
| MB-02 | Memory write BP | Stop on write access |
| MB-03 | Memory execute BP | Stop on execute |
| MB-04 | Memory BP spanning pages | All pages protected |
| MB-05 | RestoreOnHit=true | Single hit only |
| MB-06 | RestoreOnHit=false | Multiple hits |
| MB-07 | Remove memory BP | Protection restored |
| MB-08 | Overlapping memory BPs | Correct behavior |

### Category 4: Stepping

| ID | Scenario | Expected Result |
|----|----------|-----------------|
| ST-01 | StepInto basic | Execute one instruction |
| ST-02 | StepInto into CALL | Enter called function |
| ST-03 | StepOver CALL | Skip function, stop after |
| ST-04 | StepOver REP instruction | Skip entire REP |
| ST-05 | StepInto from INT3 | Execute next instruction |
| ST-06 | StepInto hits SW BP | Both handled correctly |
| ST-07 | StepInto hits HW BP | Both handled correctly |
| ST-08 | StepOver function with BP inside | Function executes, BP fires, returns |
| ST-09 | Step in multi-threaded | Only current thread steps |
| ST-10 | Consecutive steps | 10 steps execute 10 instructions |

### Category 5: Exception Handling

| ID | Scenario | Expected Result |
|----|----------|-----------------|
| EX-01 | ACCESS_VIOLATION read | Exception callback, correct address |
| EX-02 | ACCESS_VIOLATION write | Exception callback |
| EX-03 | ACCESS_VIOLATION execute | Exception callback |
| EX-04 | INT3 exception | Breakpoint callback |
| EX-05 | SINGLE_STEP exception | Step callback |
| EX-06 | DIV_BY_ZERO | Exception callback |
| EX-07 | ILLEGAL_INSTRUCTION | Exception callback |
| EX-08 | PRIVILEGED_INSTRUCTION | Exception callback |
| EX-09 | STACK_OVERFLOW | Exception callback |
| EX-10 | GUARD_PAGE | Exception callback |
| EX-11 | SetNextDbgContinueStatus | Continue with specified status |
| EX-12 | First-chance vs second-chance | Correct classification |
| EX-13 | Exception in SEH handler | Proper propagation |

### Category 6: Debug Events

| ID | Scenario | Expected Result |
|----|----------|-----------------|
| DE-01 | CREATE_PROCESS | Handler called, valid info |
| DE-02 | EXIT_PROCESS | Handler called, exit code |
| DE-03 | CREATE_THREAD | Handler called per thread |
| DE-04 | EXIT_THREAD | Handler called, exit code |
| DE-05 | LOAD_DLL | Handler called, DLL info |
| DE-06 | UNLOAD_DLL | Handler called |
| DE-07 | OUTPUT_DEBUG_STRING | String received correctly |
| DE-08 | SYSTEM_BREAKPOINT | Initial break handled |
| DE-09 | GetDebugData validity | Correct event struct |

### Category 7: Multi-Threading

| ID | Scenario | Expected Result |
|----|----------|-----------------|
| MT-01 | BP hit by multiple threads | Each hit counted |
| MT-02 | Simultaneous BP hits | No race condition |
| MT-03 | Delete BP during multi-thread hit | Clean handling |
| MT-04 | HW BP per-thread | Fires on all threads |
| MT-05 | Step in one thread | Others suspended |
| MT-06 | Context per thread | Correct context returned |
| MT-07 | Thread create during step | New thread handled |
| MT-08 | Thread exit during BP | Clean cleanup |
| MT-09 | 32 threads with same BP | All hits counted |
| MT-10 | Thread-specific BP (TID filter) | Only target thread stops |

### Category 8: Context Operations

| ID | Scenario | Expected Result |
|----|----------|-----------------|
| CX-01 | Read all GPRs | Correct values |
| CX-02 | Write GPR, continue | Modified value used |
| CX-03 | Read/Write IP | Execution redirected |
| CX-04 | Read/Write FLAGS | Flag changes observed |
| CX-05 | Read/Write DR0-DR7 | Debug registers work |
| CX-06 | FPU registers | Correct floating point |
| CX-07 | XMM registers | SIMD values correct |
| CX-08 | YMM registers (AVX) | AVX values correct |
| CX-09 | Segment registers | Correct selectors |
| CX-10 | Full context round-trip | Get, Set, verify |

### Category 9: Attach/Detach

| ID | Scenario | Expected Result |
|----|----------|-----------------|
| AD-01 | AttachDebugger to running | Successful attach |
| AD-02 | AttachDebugger KillOnExit=true | Process dies on detach |
| AD-03 | AttachDebugger KillOnExit=false | Process continues |
| AD-04 | DetachDebuggerEx | Clean detach |
| AD-05 | Set BP after attach | BP works |
| AD-06 | Attach, detach, re-attach | All work |
| AD-07 | Attach to multi-threaded | All threads accessible |

### Category 10: Combined Scenarios

| ID | Scenario | Expected Result |
|----|----------|-----------------|
| CB-01 | SW BP → step → HW BP | All fire in sequence |
| CB-02 | HW BP → step → Memory BP | All fire |
| CB-03 | Exception → continue → BP | Both handled |
| CB-04 | DLL load → set BP in DLL | BP fires |
| CB-05 | Thread create → set BP → thread hits | BP fires in new thread |
| CB-06 | Multiple exception types in sequence | All handled correctly |
| CB-07 | BP + thread exit | Clean state |
| CB-08 | Memory BP + SW BP same page | Both work |
| CB-09 | Detach with active BPs | Clean removal |
| CB-10 | Step over function that raises exception | Exception handled, step completes |

---

## Test Runner Architecture

### Test CLI Design

Test executables accept test names on command line for flexibility:

```bash
# Run all tests
TitanTestRunner.exe

# Run specific test(s)
TitanTestRunner.exe SW-01 SW-02 HW-01

# Run category
TitanTestRunner.exe SW-*

# List available tests
TitanTestRunner.exe --list
```

**Output format:**
```
# Success (all pass)
47/47 passed
```

```
# Failure
SW-03: FAIL at breakpoint.cpp:142 - expected hit_count=1, got 0
HW-07: FAIL at hardware.cpp:89 - DR3 not set
2/47 failed
```

### Direct API Tests (Unit-level)

```cpp
class TitanEngineTest {
    std::wstring m_testExePath;
    PROCESS_INFORMATION* m_pi;
    std::vector<TestResult> m_results;

    // Event counters
    std::atomic<int> m_bpHitCount{0};
    std::atomic<int> m_exceptionCount{0};
    std::map<DWORD, int> m_threadEvents;

public:
    void Setup(const wchar_t* testExe) {
        m_testExePath = testExe;
    }

    void Teardown() {
        StopDebug();
    }

    // Example test
    bool Test_SW_01_Int3Breakpoint() {
        m_pi = InitDebugW(m_testExePath.c_str(), L"", L".");
        if (!m_pi) return false;

        // Get address of exported function
        ULONG_PTR targetAddr = GetExportedFunctionAddress(L"BpTarget_Simple");

        // Set breakpoint
        bool bpSet = SetBPX(targetAddr, UE_BREAKPOINT | UE_SINGLESHOOT,
            [](void) {
                // Verify we're at the right address
                auto ctx = GetDebugData();
                // ... validation
            });

        if (!bpSet) return false;

        SetCustomHandler(UE_CH_CREATEPROCESS, ProcessCreated);

        DebugLoop();

        return m_bpHitCount == 1;
    }
};
```

### End-to-End Tests (via Headless + Script)

```
// test_sw_breakpoint.x64dbg
InitDebug "TestExe_Breakpoints.exe"
bp BpTarget_Simple
g
// Verify we stopped at breakpoint
log "Hit count: {$breakpointcount}"
bc BpTarget_Simple
g
// Process should exit
log "Test complete"
```

### Test Result Format

```json
{
    "test_run": {
        "timestamp": "2025-01-11T12:00:00Z",
        "platform": "x64",
        "titan_version": "1.0.0"
    },
    "results": [
        {
            "id": "SW-01",
            "name": "Set INT3 breakpoint, verify hit",
            "status": "PASS",
            "duration_ms": 150,
            "details": {
                "expected_hits": 1,
                "actual_hits": 1
            }
        }
    ],
    "summary": {
        "total": 100,
        "passed": 98,
        "failed": 2,
        "skipped": 0
    }
}
```

---

## Implementation Priority

### Phase 1: Core Functionality (Critical)
1. Test executables: TestExe_Breakpoints, TestExe_Exceptions
2. Test runner framework
3. SW-01 through SW-10
4. EX-01 through EX-13
5. DE-01 through DE-09

### Phase 2: Advanced Breakpoints
1. Test executable: TestExe_Context
2. HW-01 through HW-10
3. MB-01 through MB-08
4. CX-01 through CX-10

### Phase 3: Multi-Threading
1. Test executable: TestExe_Threading
2. MT-01 through MT-10
3. Threading race condition tests (from DebugLoopRace)

### Phase 4: Stepping & Combined
1. Test executable: TestExe_Stepping
2. ST-01 through ST-10
3. CB-01 through CB-10

### Phase 5: Attach/Detach & DLL
1. Test executables: TestExe_Attach, TestExe_DllLoad, TestDll.dll
2. AD-01 through AD-07
3. DLL-related scenarios

---

## CI Integration

```yaml
# .github/workflows/titan-tests.yml
name: TitanEngine Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: windows-latest
    strategy:
      matrix:
        arch: [x86, x64]

    steps:
      - uses: actions/checkout@v4

      - name: Build Test Executables
        run: |
          cmake -B build -A ${{ matrix.arch == 'x64' && 'x64' || 'Win32' }}
          cmake --build build --config Release

      - name: Run TitanEngine Tests
        run: |
          ./build/Release/TitanTestRunner.exe --output results.json

      - name: Upload Results
        uses: actions/upload-artifact@v4
        with:
          name: test-results-${{ matrix.arch }}
          path: results.json
```

---

## Design Decisions

1. **Test Isolation**:
   - Direct TitanEngine tests: Reuse process where possible for efficiency
   - x64dbg end-to-end tests: Configurable (isolated or batched)
   - Teardown tests: Always isolated to verify clean shutdown
   - Ability to run individual tests in isolation for debugging failures

2. **ASLR Configuration**:
   - Primary test executables: `/DYNAMICBASE:NO` for deterministic addresses
   - Secondary variants: `/DYNAMICBASE:YES` for testing breakpoint database restore after ASLR relocation

3. **Architecture Support**:
   - Both x86 and x64 tests required
   - Separate test executables for each architecture
   - CI must run both configurations

4. **AVX-512 Testing**:
   - Conditional on CPU support detection at runtime
   - Skip gracefully if not available

5. **Performance Metrics**:
   - Track latency for breakpoint hit response, step duration, context read/write
   - Include in test results JSON

6. **Future Work (Separate Tasks)**:
   - WoW64 support (32-bit debuggee with 64-bit debugger) - not currently supported by engine
   - Anti-debug scenario testing - important for exception handling transparency verification
   - Debug engine detection resistance tests

---

## Test Executable Variants

Each test executable should have two variants:

| Variant | Link Flags | Purpose |
|---------|------------|---------|
| `TestExe_*_Static.exe` | `/DYNAMICBASE:NO /INCREMENTAL:NO` | Deterministic addresses for basic testing |
| `TestExe_*_ASLR.exe` | `/DYNAMICBASE:YES /INCREMENTAL:NO` | Breakpoint database restore testing |
