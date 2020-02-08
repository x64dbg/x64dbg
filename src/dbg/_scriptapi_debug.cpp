#include "_scriptapi_debug.h"
#include "threading.h"
#include "debugger.h"
#include <map>

class DebugContext
{
    using CallbackUserdata = std::pair<Script::Debug::CallbackPtr, void*>;

    DWORD threadId = 0;
    std::map<std::pair<BPXTYPE, duint>, CallbackUserdata> callbacks;
    std::vector<CallbackUserdata> waitstack;
    std::vector<CallbackUserdata> finalizers;

private:
    DebugContext(const DebugContext &) = default;
    DebugContext & operator=(const DebugContext &) = default;

public:
    DebugContext(DWORD threadId) : threadId(threadId) { }
    DebugContext(DebugContext &&) = delete;

    ~DebugContext()
    {
        for(const auto & finalizer : finalizers)
            finalizer.first(finalizer.second);
    }

    static void ctxassert(bool condition, const char* message)
    {
        if(!condition)
            MessageBoxA(GuiGetWindowHandle(), message, "Script::Debug Assertion", MB_ICONERROR);
    }

    void create()
    {
        ctxassert(threadId == 0, "You cannot call CreateContext with an existing context!");
        *this = DebugContext(GetCurrentThreadId());
    }

    void destroy()
    {
        check();
        ctxassert(threadId != 0, "You cannot call DestroyContext without CreateContext first!");
        *this = DebugContext(0);
    }

    void check() const
    {
        SHARED_ACQUIRE(LockScriptDebugContext);
        auto currentThreadId = GetCurrentThreadId();
        ctxassert(currentThreadId != dbggetdebugloopthreadid(), "You cannot call Script::Debug functions in the debug loop!");
        ctxassert(threadId == 0 || threadId == currentThreadId, "You cannot mix Script::Debug contexts!");
    }

    void postwait()
    {
        EXCLUSIVE_ACQUIRE(LockScriptDebugContext);
        if(waitstack.empty())
            return;
        auto waitCallback = waitstack.back();
        waitstack.pop_back();
        EXCLUSIVE_RELEASE();
        waitCallback.first(waitCallback.second);
    }

    void setBreakpointCallback(BPXTYPE type, duint address, Script::Debug::CallbackPtr callback, void* userdata)
    {
        EXCLUSIVE_ACQUIRE(LockScriptDebugContext);
        check();
        callbacks[ {type, address}] = { callback, userdata };
    }

    void deleteBreakpointCallback(BPXTYPE type, duint address)
    {
        callbacks.erase({ type, address });
    }

    void handleBreakpoint(const BRIDGEBP & bp)
    {
        EXCLUSIVE_ACQUIRE(LockScriptDebugContext);
        auto itr = callbacks.find({ bp.type, bp.addr });
        if(itr == callbacks.end())
            return;
        waitstack.push_back(itr->second);
    }

    void addFinalizer(Script::Debug::CallbackPtr callback, void* userdata)
    {
        EXCLUSIVE_ACQUIRE(LockScriptDebugContext);
        check();
        ctxassert(threadId != 0, "Finalizers cannot be added without a context");
        CallbackUserdata finalizerCallback;
        finalizers.emplace_back(callback, userdata);
    }
};

static DebugContext ctx(0);

static void cbFlatten(void* userdata)
{
    ((Script::Debug::Callback)userdata)();
}

SCRIPT_EXPORT void Script::Debug::Wait()
{
    ctx.check();
    _plugin_waituntilpaused();
    ctx.postwait();
}

SCRIPT_EXPORT void Script::Debug::Run()
{
    if(DbgCmdExecDirect("run"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::Pause()
{
    if(DbgCmdExecDirect("pause"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::Stop()
{
    if(DbgCmdExecDirect("StopDebug"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::StepIn()
{
    if(DbgCmdExecDirect("StepInto"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::StepOver()
{
    if(DbgCmdExecDirect("StepOver"))
        Wait();
}

SCRIPT_EXPORT void Script::Debug::StepOut()
{
    if(DbgCmdExecDirect("StepOut"))
        Wait();
}

SCRIPT_EXPORT bool Script::Debug::SetBreakpoint(duint address)
{
    char command[128] = "";
    sprintf_s(command, "bp %p", address);
    return DbgCmdExecDirect(command);
}

SCRIPT_EXPORT bool Script::Debug::SetBreakpoint(duint address, Callback callback)
{
    return SetBreakpoint(address, cbFlatten, callback);
}

SCRIPT_EXPORT bool Script::Debug::SetBreakpoint(duint address, CallbackPtr callback, void* userdata)
{
    if(!SetBreakpoint(address))
        return false;
    ctx.setBreakpointCallback(bp_normal, address, callback, userdata);
    return true;
}

SCRIPT_EXPORT bool Script::Debug::DeleteBreakpoint(duint address)
{
    char command[128] = "";
    sprintf_s(command, "bc %p", address);
    if(!DbgCmdExecDirect(command))
        return false;
    ctx.deleteBreakpointCallback(bp_normal, address);
    return true;
}

SCRIPT_EXPORT bool Script::Debug::DisableBreakpoint(duint address)
{
    char command[128] = "";
    sprintf_s(command, "bd %p", address);
    return DbgCmdExecDirect(command);
}

SCRIPT_EXPORT bool Script::Debug::SetHardwareBreakpoint(duint address, HardwareType type)
{
    char command[128] = "";
    const char* types[] = { "rw", "w", "x" };
    sprintf_s(command, "bphws %p, %s", address, types[type]);
    return DbgCmdExecDirect(command);
}

namespace Script
{
    namespace Debug
    {
        SCRIPT_EXPORT bool SetHardwareBreakpoint(duint address, HardwareType type) // binary-compatibility
        {
            return SetHardwareBreakpoint(address, HardwareExecute,)
        }
    }
}


SCRIPT_EXPORT bool Script::Debug::DeleteHardwareBreakpoint(duint address)
{
    char command[128] = "";
    sprintf_s(command, "bphwc %p", address);
    return DbgCmdExecDirect(command);
}

SCRIPT_EXPORT void Script::Debug::SetBreakpointCallback(BPXTYPE type, duint address, CallbackPtr callback, void* userdata)
{
    ctx.setBreakpointCallback(type, address, callback, userdata);
}

SCRIPT_EXPORT void Script::Debug::Context::Create()
{
    EXCLUSIVE_ACQUIRE(LockScriptDebugContext);
    ctx.create();
}

SCRIPT_EXPORT void Script::Debug::Context::Destroy()
{
    EXCLUSIVE_ACQUIRE(LockScriptDebugContext);
    ctx.destroy();
}

SCRIPT_EXPORT void Script::Debug::Context::AddFinalizer(CallbackPtr callback, void* userdata)
{
    ctx.addFinalizer(callback, userdata);
}

void Script::Debug::Internal::BreakpointHandler(const BRIDGEBP & bp)
{
    ctx.handleBreakpoint(bp);
}
