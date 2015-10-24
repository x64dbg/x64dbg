#pragma once

enum class Event
{
    CreateProcess,  // Process created
    ExitProcess,    // Process exited

    CreateThread,   // Thread created
    ExitThread,     // Thread exited
    NameThread,     // Set thread name request

    ExeLoad,        // EXE loaded
    DllLoad,        // DLL loaded
    DllUnload,      // DLL unloaded

    SystemBreak,    // Initial system breakpoint
    Breakpoint,     // Breakpoint callback before any other handler

    Exception,      // Unhandled exception
    DebugString,    // OutputDebugString exception

    DebugEvent,     // Miscellaneous debug event
};

enum class Breakpoint
{
    Normal,
    Memory,
    Hardware,
};

// Debugger engine register context
class IEngineContext
{
public:
    virtual int64 Register(int Index);
    virtual void SetRegister(int Index, int64 Value);
};

// Main debugger engine import interface
class IEngine
{
public:
    // Initialization
    virtual bool Initialize();
    virtual bool Open(const wchar_t *Path, wchar_t *Arguments, wchar_t *WorkingDirectory);      // Auto-determined EXE or DLL path
    virtual bool OpenExe(const wchar_t *Path, wchar_t *Arguments, wchar_t *WorkingDirectory);   // Must be a EXE file path
    virtual bool OpenDll(const wchar_t *Path);                                                  // Must be a DLL file path

    virtual void RunLoop();                                                                     // Internal message processing loop

    // Event handlers
    // TODO: Auto-map certain callback signatures with event id's (using templates)
    template<typename T>
    inline void SetEventHandler(Event Type, T Handler)
    {
        this->SetEventHandler(Type, *(PVOID *)&Handler);
    }

    virtual void SetEventHandler(Event Type, PVOID Handler);

    // Control flow and stepping
    virtual void Run();
    virtual void Break();
    virtual void Step();
    virtual void StepIn();
    virtual void StepOver();

    virtual bool IsRunning();

    // Exceptions
    virtual void SkipException(bool SkipNext);
    virtual void SetContinueStatus(DWORD Status);

    // Breakpoints
    bool SetBreakpoint(Breakpoint Type, int64 Address, int Size, PVOID Callback = nullptr, bool RestoreMemoryHit = false);
    void RemoveBreakpoint(Breakpoint Type, int64 Address);

    // Memory
    virtual bool ReadMemory(int64 BaseAddress, PVOID Buffer, size_t Size, size_t *NumberOfBytesRead = nullptr);
    virtual bool WriteMemory(int64 BaseAddress, PVOID Buffer, size_t Size, size_t *NumberOfBytesWritten = nullptr);

    // Threading
    virtual HANDLE GetThread();     // Active thread handle
    virtual DWORD GetThreadId();    // Active thread ID

    // Interfaces
    virtual IEngineContext *Context();

    // Miscellaneous
    const char          *ErrorString;
    PROCESS_INFORMATION *ProcessInfo;
};

extern IEngine *Engine;