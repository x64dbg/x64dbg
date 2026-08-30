#pragma once

#include <windows.h>
#include <stdint.h>

#pragma pack(push, 1)

// Global.Constant.Structure.Declaration:
// Engine.External:
enum TitanStructureType
{
    UE_STRUCT_TITAN_ENGINE_CONTEXT = 16,
};

enum TitanAccessType
{
    UE_ACCESS_READ = 0,
    UE_ACCESS_WRITE = 1,
    UE_ACCESS_ALL = 2,
};

enum TitanEngineVariable
{
    UE_ENGINE_NO_CONSOLE_WINDOW = 4,
    UE_ENGINE_SET_DEBUG_PRIVILEGE = 9,
    UE_ENGINE_SAFE_ATTACH = 10,
    UE_ENGINE_MEMBP_ALT = 11,
    UE_ENGINE_DISABLE_ASLR = 12,
    UE_ENGINE_SAFE_STEP = 13,
};

enum TitanBreakpointRemoveOption
{
    UE_OPTION_REMOVEALL = 1,
};

enum TitanCustomHandler
{
    UE_CH_CREATETHREAD = 14,
    UE_CH_EXITTHREAD = 15,
    UE_CH_CREATEPROCESS = 16,
    UE_CH_EXITPROCESS = 17,
    UE_CH_LOADDLL = 18,
    UE_CH_UNLOADDLL = 19,
    UE_CH_OUTPUTDEBUGSTRING = 20,
    UE_CH_SYSTEMBREAKPOINT = 23,
    UE_CH_UNHANDLEDEXCEPTION = 24,
    UE_CH_DEBUGEVENT = 26,
};

enum TitanBreakpointType
{
    UE_BREAKPOINT_INT3 = 1,
    UE_BREAKPOINT_LONG_INT3 = 2,
    UE_BREAKPOINT_UD2 = 3,
};

enum TitanSoftwareBreakpointType
{
    UE_BREAKPOINT = 0,
    UE_SINGLESHOOT = 1,
    UE_BREAKPOINT_TYPE_INT3 = 0x10000000,
    UE_BREAKPOINT_TYPE_LONG_INT3 = 0x20000000,
    UE_BREAKPOINT_TYPE_UD2 = 0x30000000,
};

enum TitanMemoryBreakpointType
{
    UE_MEMORY = 3,
    UE_MEMORY_READ = 4,
    UE_MEMORY_WRITE = 5,
    UE_MEMORY_EXECUTE = 6,
};

enum TitanHardwareBreakpointType
{
    UE_HARDWARE_EXECUTE = 4,
    UE_HARDWARE_WRITE = 5,
    UE_HARDWARE_READWRITE = 6,
};

enum TitanHardwareBreakpointSize
{
    UE_HARDWARE_SIZE_1 = 7,
    UE_HARDWARE_SIZE_2 = 8,
    UE_HARDWARE_SIZE_4 = 9,
    UE_HARDWARE_SIZE_8 = 10,
};

enum TitanRegister
{
    UE_EAX = 1,
    UE_EBX = 2,
    UE_ECX = 3,
    UE_EDX = 4,
    UE_EDI = 5,
    UE_ESI = 6,
    UE_EBP = 7,
    UE_ESP = 8,
    UE_EIP = 9,
    UE_EFLAGS = 10,
    UE_DR0 = 11,
    UE_DR1 = 12,
    UE_DR2 = 13,
    UE_DR3 = 14,
    UE_DR6 = 15,
    UE_DR7 = 16,
    UE_RAX = 17,
    UE_RBX = 18,
    UE_RCX = 19,
    UE_RDX = 20,
    UE_RDI = 21,
    UE_RSI = 22,
    UE_RBP = 23,
    UE_RSP = 24,
    UE_RIP = 25,
    UE_RFLAGS = 26,
    UE_R8 = 27,
    UE_R9 = 28,
    UE_R10 = 29,
    UE_R11 = 30,
    UE_R12 = 31,
    UE_R13 = 32,
    UE_R14 = 33,
    UE_R15 = 34,
    UE_CIP = 35,
    UE_CSP = 36,
#ifdef _WIN64
#define UE_CFLAGS UE_RFLAGS
#else
#define UE_CFLAGS UE_EFLAGS
#endif
    UE_SEG_GS = 37,
    UE_SEG_FS = 38,
    UE_SEG_ES = 39,
    UE_SEG_DS = 40,
    UE_SEG_CS = 41,
    UE_SEG_SS = 42,
    UE_x87_r0 = 43,
    UE_x87_r1 = 44,
    UE_x87_r2 = 45,
    UE_x87_r3 = 46,
    UE_x87_r4 = 47,
    UE_x87_r5 = 48,
    UE_x87_r6 = 49,
    UE_x87_r7 = 50,
    UE_X87_STATUSWORD = 51,
    UE_X87_CONTROLWORD = 52,
    UE_X87_TAGWORD = 53,
    UE_MXCSR = 54,
    UE_MMX0 = 55,
    UE_MMX1 = 56,
    UE_MMX2 = 57,
    UE_MMX3 = 58,
    UE_MMX4 = 59,
    UE_MMX5 = 60,
    UE_MMX6 = 61,
    UE_MMX7 = 62,
    UE_XMM0 = 63,
    UE_XMM1 = 64,
    UE_XMM2 = 65,
    UE_XMM3 = 66,
    UE_XMM4 = 67,
    UE_XMM5 = 68,
    UE_XMM6 = 69,
    UE_XMM7 = 70,
    UE_XMM8 = 71,
    UE_XMM9 = 72,
    UE_XMM10 = 73,
    UE_XMM11 = 74,
    UE_XMM12 = 75,
    UE_XMM13 = 76,
    UE_XMM14 = 77,
    UE_XMM15 = 78,
    UE_x87_ST0 = 79,
    UE_x87_ST1 = 80,
    UE_x87_ST2 = 81,
    UE_x87_ST3 = 82,
    UE_x87_ST4 = 83,
    UE_x87_ST5 = 84,
    UE_x87_ST6 = 85,
    UE_x87_ST7 = 86,
    UE_YMM0 = 87,
    UE_YMM1 = 88,
    UE_YMM2 = 89,
    UE_YMM3 = 90,
    UE_YMM4 = 91,
    UE_YMM5 = 92,
    UE_YMM6 = 93,
    UE_YMM7 = 94,
    UE_YMM8 = 95,
    UE_YMM9 = 96,
    UE_YMM10 = 97,
    UE_YMM11 = 98,
    UE_YMM12 = 99,
    UE_YMM13 = 100,
    UE_YMM14 = 101,
    UE_YMM15 = 102,
};

typedef void(*TITANCALLBACKARG)(const void*);
typedef void(*TITANCALLBACK)();

typedef TITANCALLBACK TITANCBCH;
typedef TITANCALLBACK TITANCBSTEP;
typedef TITANCALLBACK TITANCBSOFTBP;
typedef TITANCALLBACKARG TITANCBHWBP;
typedef TITANCALLBACKARG TITANCBMEMBP;

typedef struct DECLSPEC_ALIGN(16) _XmmRegister_t
{
    ULONGLONG Low;
    LONGLONG High;
} XmmRegister_t;

typedef struct
{
    XmmRegister_t Low; //XMM/SSE part
    XmmRegister_t High; //AVX part
} YmmRegister_t;

typedef struct
{
    YmmRegister_t Low; //AVX part
    YmmRegister_t High; //AVX-512 part
} ZmmRegister_t;

typedef struct
{
    BYTE    data[10];
    int     st_value;
    int     tag;
} x87FPURegister_t;

typedef struct
{
    WORD   ControlWord;
    WORD   StatusWord;
    WORD   TagWord;
    DWORD   ErrorOffset;
    DWORD   ErrorSelector;
    DWORD   DataOffset;
    DWORD   DataSelector;
    DWORD   Cr0NpxState;
} x87FPU_t;

typedef struct
{
    ULONG_PTR cax;
    ULONG_PTR ccx;
    ULONG_PTR cdx;
    ULONG_PTR cbx;
    ULONG_PTR csp;
    ULONG_PTR cbp;
    ULONG_PTR csi;
    ULONG_PTR cdi;
#ifdef _WIN64
    ULONG_PTR r8;
    ULONG_PTR r9;
    ULONG_PTR r10;
    ULONG_PTR r11;
    ULONG_PTR r12;
    ULONG_PTR r13;
    ULONG_PTR r14;
    ULONG_PTR r15;
#endif //_WIN64
    ULONG_PTR cip;
    ULONG_PTR eflags;
    unsigned short gs;
    unsigned short fs;
    unsigned short es;
    unsigned short ds;
    unsigned short cs;
    unsigned short ss;
    ULONG_PTR dr0;
    ULONG_PTR dr1;
    ULONG_PTR dr2;
    ULONG_PTR dr3;
    ULONG_PTR dr6;
    ULONG_PTR dr7;
    BYTE RegisterArea[80];
    x87FPU_t x87fpu;
    DWORD MxCsr;
#ifdef _WIN64
    XmmRegister_t XmmRegisters[16];
    YmmRegister_t YmmRegisters[16];
#else // x86
    XmmRegister_t XmmRegisters[8];
    YmmRegister_t YmmRegisters[8];
#endif
} TITAN_ENGINE_CONTEXT_t;

typedef struct
{
#ifdef _WIN64
    ZmmRegister_t ZmmRegisters[32];
#else // x86
    ZmmRegister_t ZmmRegisters[8];
#endif
    ULONGLONG Opmask[8];
} TITAN_ENGINE_CONTEXT_AVX512_t;

#ifdef __cplusplus
extern "C"
{
#endif

// Global.Function.Declaration:
__declspec(dllexport) bool MemoryReadSafe(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead);
__declspec(dllexport) bool MemoryWriteSafe(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten);
__declspec(dllexport) SIZE_T MemoryQuerySafe(HANDLE hProcess, LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength);
__declspec(dllexport) LPVOID MemoryAllocSafe(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
__declspec(dllexport) bool MemoryFreeSafe(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);
__declspec(dllexport) bool MemoryProtectSafe(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
// TitanEngine.Hider.functions:
__declspec(dllexport) ULONG_PTR GetPEBLocation(HANDLE hProcess);
__declspec(dllexport) ULONG_PTR GetTEBLocation(HANDLE hThread);
// TitanEngine.Debugger.functions:
__declspec(dllexport) PROCESS_INFORMATION* InitDebugW(const wchar_t* szFileName, const wchar_t* szCommandLine, const wchar_t* szCurrentFolder);
__declspec(dllexport) bool StopDebug();
__declspec(dllexport) void SetBPXOptions(TitanBreakpointType DefaultBreakPointType);
__declspec(dllexport) bool IsBPXEnabled(ULONG_PTR bpxAddress);
__declspec(dllexport) bool SetBPX(ULONG_PTR bpxAddress, DWORD bpxType, TITANCBSOFTBP bpxCallBack);
__declspec(dllexport) bool DeleteBPX(ULONG_PTR bpxAddress);
__declspec(dllexport) bool SetMemoryBPXEx(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory, TitanMemoryBreakpointType BreakPointType, bool RestoreOnHit, TITANCBMEMBP bpxCallBack);
__declspec(dllexport) bool RemoveMemoryBPX(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory);
__declspec(dllexport) bool GetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext);
__declspec(dllexport) bool SetFullContextDataEx(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext);
__declspec(dllexport) ULONG_PTR GetContextDataEx(HANDLE hActiveThread, TitanRegister IndexOfRegister);
__declspec(dllexport) bool SetContextDataEx(HANDLE hActiveThread, TitanRegister IndexOfRegister, ULONG_PTR NewRegisterValue);
__declspec(dllexport) bool GetAVXContext(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext);
__declspec(dllexport) bool SetAVXContext(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_t* titcontext);
__declspec(dllexport) bool GetAVX512Context(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_AVX512_t* titcontext);
__declspec(dllexport) bool SetAVX512Context(HANDLE hActiveThread, TITAN_ENGINE_CONTEXT_AVX512_t* titcontext);
__declspec(dllexport) const DEBUG_EVENT* GetDebugData(); // TODO: remove?
__declspec(dllexport) void SetCustomHandler(TitanCustomHandler ExceptionId, TITANCALLBACKARG CallBack);
__declspec(dllexport) void StepInto(TITANCBSTEP traceCallBack);
__declspec(dllexport) void StepOver(TITANCBSTEP traceCallBack);
__declspec(dllexport) bool GetUnusedHardwareBreakPointRegister(LPDWORD RegisterIndex);
__declspec(dllexport) bool SetHardwareBreakPoint(ULONG_PTR bpxAddress, DWORD IndexOfRegister, TitanHardwareBreakpointType bpxType, TitanHardwareBreakpointSize bpxSize, TITANCBHWBP bpxCallBack);
__declspec(dllexport) bool DeleteHardwareBreakPoint(DWORD IndexOfRegister);
__declspec(dllexport) bool RemoveAllBreakPoints(TitanBreakpointRemoveOption RemoveOption);
__declspec(dllexport) void DebugLoop();
__declspec(dllexport) void SetNextDbgContinueStatus(DWORD SetDbgCode);
__declspec(dllexport) bool AttachDebugger(DWORD ProcessId, bool KillOnExit, LPVOID DebugInfo, TITANCALLBACK CallBack);
__declspec(dllexport) bool DetachDebuggerEx(DWORD ProcessId);
__declspec(dllexport) bool IsFileBeingDebugged();
// TitanEngine.Process.functions:
__declspec(dllexport) HANDLE TitanOpenProcess(DWORD dwDesiredAccess, bool bInheritHandle, DWORD dwProcessId);
__declspec(dllexport) HANDLE TitanOpenThread(DWORD dwDesiredAccess, bool bInheritHandle, DWORD dwThreadId);
__declspec(dllexport) bool TitanCloseHandle(HANDLE hEngineHandle);
__declspec(dllexport) bool ProcessIsWow64(HANDLE hProcess, PBOOL isWow64);
__declspec(dllexport) bool TitanTerminateProcess(HANDLE hProcess, DWORD exitCode);
__declspec(dllexport) bool TitanDebugBreakProcess(HANDLE hProcess);
__declspec(dllexport) HANDLE TitanCreateRemoteThread(HANDLE hProcess, LPTHREAD_START_ROUTINE start, LPVOID argument, DWORD creationFlags, LPDWORD threadId);
__declspec(dllexport) DWORD TitanSuspendThread(HANDLE hThread);
__declspec(dllexport) DWORD TitanResumeThread(HANDLE hThread);
__declspec(dllexport) bool TitanTerminateThread(HANDLE hThread, DWORD exitCode);
__declspec(dllexport) DWORD TitanGetThreadId(HANDLE hThread);
__declspec(dllexport) int TitanGetThreadPriority(HANDLE hThread);
__declspec(dllexport) bool TitanSetThreadPriority(HANDLE hThread, int priority);
__declspec(dllexport) bool TitanGetThreadTimes(HANDLE hThread, LPFILETIME creation, LPFILETIME exit, LPFILETIME kernel, LPFILETIME user);
__declspec(dllexport) bool TitanQueryThreadCycleTime(HANDLE hThread, PULONG64 cycleTime);
// TitanEngine.Engine.functions:
__declspec(dllexport) void SetEngineVariable(TitanEngineVariable VariableId, bool VariableSet);
__declspec(dllexport) bool EngineCheckStructAlignment(TitanStructureType StructureType, ULONG_PTR StructureSize);

#ifdef __cplusplus
}
#endif

#pragma pack(pop)
