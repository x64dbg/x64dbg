#include "core/DbgAdapter.h"
#include <cassert>
#include <csignal>

static REGDUMP toRegDump(const ElfBugRegisters & regs)
{
    REGDUMP dump{};
    dump.regcontext.cax = regs.rax;
    dump.regcontext.cbx = regs.rbx;
    dump.regcontext.ccx = regs.rcx;
    dump.regcontext.cdx = regs.rdx;
    dump.regcontext.cbp = regs.rbp;
    dump.regcontext.csp = regs.rsp;
    dump.regcontext.csi = regs.rsi;
    dump.regcontext.cdi = regs.rdi;
    dump.regcontext.r8  = regs.r8;
    dump.regcontext.r9  = regs.r9;
    dump.regcontext.r10 = regs.r10;
    dump.regcontext.r11 = regs.r11;
    dump.regcontext.r12 = regs.r12;
    dump.regcontext.r13 = regs.r13;
    dump.regcontext.r14 = regs.r14;
    dump.regcontext.r15 = regs.r15;
    dump.regcontext.cip = regs.rip;
    dump.regcontext.eflags = regs.eflags;
    dump.regcontext.cs = regs.cs;
    dump.regcontext.ds = regs.ds;
    dump.regcontext.es = regs.es;
    dump.regcontext.fs = regs.fs;
    dump.regcontext.gs = regs.gs;
    dump.regcontext.ss = regs.ss;
    dump.flags.c = (regs.eflags & 1) != 0;
    dump.flags.p = (regs.eflags & (1 << 2)) != 0;
    dump.flags.a = (regs.eflags & (1 << 4)) != 0;
    dump.flags.z = (regs.eflags & (1 << 6)) != 0;
    dump.flags.s = (regs.eflags & (1 << 7)) != 0;
    dump.flags.t = (regs.eflags & (1 << 8)) != 0;
    dump.flags.i = (regs.eflags & (1 << 9)) != 0;
    dump.flags.d = (regs.eflags & (1 << 10)) != 0;
    dump.flags.o = (regs.eflags & (1 << 11)) != 0;
    return dump;
}

std::atomic<DbgAdapter*> DbgAdapter::sInstance{nullptr};

DbgAdapter::DbgAdapter(QObject* parent)
    : QObject(parent)
{
    assert(!sInstance.load() && "Only one DbgAdapter instance is allowed");
    sInstance.store(this);
    DbgSetBreakpointQuery(&DbgAdapter::queryBreakpoint);
}

DbgAdapter::~DbgAdapter()
{
    DbgSetBreakpointQuery(nullptr);
    sInstance.store(nullptr);
    if(mDebugger)
        ElfBugDestroy(mDebugger);
}

bool DbgAdapter::loadEngine()
{
    if(mDebugger)
        return true;

    ElfBugCallbacks cb = {};
    cb.onCreateProcess = &DbgAdapter::onCreateProcess;
    cb.onExitProcess = &DbgAdapter::onExitProcess;
    cb.onCreateThread = &DbgAdapter::onCreateThread;
    cb.onExitThread = &DbgAdapter::onExitThread;
    cb.onSystemBreakpoint = &DbgAdapter::onSystemBreakpoint;
    cb.onBreakpoint = &DbgAdapter::onBreakpoint;
    cb.onStep = &DbgAdapter::onStep;
    cb.onPaused = &DbgAdapter::onPaused;
    cb.onException = &DbgAdapter::onException;
    cb.onError = &DbgAdapter::onError;
    cb.onDebugString = &DbgAdapter::onDebugString;
    cb.userdata = this;

    mDebugger = ElfBugCreate(&cb);
    if(!mDebugger)
    {
        emit logMessage("[x64dbg] ElfBugCreate failed");
        return false;
    }

    emit logMessage("[x64dbg] Engine loaded");
    return true;
}

// -- MemoryProvider interface --

bool DbgAdapter::read(const duint addr, void* dest, const duint size)
{
    return ElfBugMemRead(mDebugger, addr, dest, size);
}

bool DbgAdapter::write(const duint addr, const void* src, const duint size)
{
    return ElfBugMemWrite(mDebugger, addr, src, size);
}

bool DbgAdapter::getRange(const duint addr, duint & base, duint & size)
{
    uint64_t b, s;
    if(!ElfBugMemFindBaseAddr(mDebugger, addr, &b, &s))
        return false;
    base = b;
    size = s;
    return true;
}

bool DbgAdapter::isCodePtr(const duint addr)
{
    return ElfBugMemIsCodePtr(mDebugger, addr);
}

bool DbgAdapter::isValidPtr(const duint addr)
{
    return ElfBugMemIsValidPtr(mDebugger, addr);
}

bool DbgAdapter::writeRegister(const char* name, const duint value)
{
    if(!ElfBugSetRegister(mDebugger, name, value))
        return false;

    ElfBugRegisters regs = {};
    if(ElfBugGetRegisters(mDebugger, &regs))
        emit registersUpdated(toRegDump(regs));
    return true;
}

bool DbgAdapter::modBaseFromAddr(const duint addr, duint & base)
{
    uint64_t b = 0;
    if(!ElfBugModBaseFromAddr(mDebugger, addr, &b))
        return false;
    base = b;
    return true;
}

bool DbgAdapter::modNameFromAddr(const duint addr, char* buf, const duint bufSize, const bool extension)
{
    return ElfBugModNameFromAddr(mDebugger, addr, buf, bufSize, extension);
}

// -- Debugger control --

bool DbgAdapter::launch(const char* path) const
{
    return ElfBugInit(mDebugger, path);
}

void DbgAdapter::Start() const
{
    ElfBugStart(mDebugger);
}

void DbgAdapter::Continue() const
{
    ElfBugContinue(mDebugger);
}

void DbgAdapter::StepInto() const
{
    ElfBugStepInto(mDebugger);
}

void DbgAdapter::StepOver() const
{
    ElfBugStepOver(mDebugger);
}

void DbgAdapter::Pause() const
{
    ElfBugPause(mDebugger);
}

bool DbgAdapter::Stop() const
{
    return ElfBugStop(mDebugger);
}

bool DbgAdapter::isActive() const
{
    return ElfBugGetPid(mDebugger) > 0;
}

bool DbgAdapter::toggleBreakpoint(const duint addr) const
{
    if(!isActive())
        return false;

    if(ElfBugIsBreakpointEffective(mDebugger, addr))
        return ElfBugDeleteBreakpoint(mDebugger, addr);
    return ElfBugSetBreakpoint(mDebugger, addr);
}

bool DbgAdapter::hasBreakpoint(const duint addr) const
{
    return ElfBugIsBreakpointEffective(mDebugger, addr);
}

BPXTYPE DbgAdapter::queryBreakpoint(duint addr)
{
    auto* instance = sInstance.load();
    if(!instance)
        return bp_none;
    return instance->hasBreakpoint(addr) ? bp_normal : bp_none;
}

QString DbgAdapter::threadSuffix() const
{
    return QString(" [thread %1]").arg(ElfBugGetCurrentTid(mDebugger));
}

REGDUMP DbgAdapter::readRegisters() const
{
    ElfBugRegisters regs = {};
    ElfBugGetRegisters(mDebugger, &regs);
    return toRegDump(regs);
}

void DbgAdapter::emitStoppedState(const QString & reason)
{
    emitStoppedState(reason, readRegisters());
}

void DbgAdapter::emitStoppedState(const QString & reason, const REGDUMP & dump)
{
    emit registersUpdated(dump);
    emit stopped(dump.regcontext.cip, reason + threadSuffix());
}

void DbgAdapter::onCreateProcess(const pid_t pid, const uint64_t entryPoint, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    self->mEntryPoint = entryPoint;
    emit self->logMessage(QString("[x64dbg] Process created: PID %1").arg(pid));
}

void DbgAdapter::onExitProcess(const int exitCode, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    emit self->logMessage(QString("[x64dbg] Process exited: %1").arg(exitCode));
    emit self->processExited(exitCode);
}

void DbgAdapter::onCreateThread(const pid_t tid, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    emit self->logMessage(QString("[x64dbg] Thread %1 created").arg(tid));
}

void DbgAdapter::onExitThread(const pid_t tid, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    emit self->logMessage(QString("[x64dbg] Thread %1 exited").arg(tid));
}

void DbgAdapter::onSystemBreakpoint(void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    const REGDUMP dump = self->readRegisters();
    self->mEntryPoint = dump.regcontext.cip;

    emit self->logMessage(QString("[x64dbg] Entry point: 0x%1").arg(self->mEntryPoint, 0, 16));
    emit self->registersUpdated(dump);
    emit self->processCreated(self->mEntryPoint);
    emit self->stopped(self->mEntryPoint, tr("System breakpoint") + self->threadSuffix());
}

void DbgAdapter::onBreakpoint(const uint64_t address, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    emit self->logMessage(QString("[x64dbg] Breakpoint hit: 0x%1").arg(address, 0, 16) + self->threadSuffix());
    self->emitStoppedState(QString("Breakpoint at 0x%1").arg(address, 0, 16));
}

void DbgAdapter::onStep(void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    self->emitStoppedState(tr("Step"));
}

void DbgAdapter::onPaused(void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    self->emitStoppedState(tr("Paused"));
}

// Own table: sigabbrev_np needs glibc 2.32 and the AppImage builds on 2.31.
static QString signalName(const int signal)
{
    switch(signal)
    {
#define SIGNAL_NAME(name) case name: return QStringLiteral(#name);
        SIGNAL_NAME(SIGHUP)
        SIGNAL_NAME(SIGINT)
        SIGNAL_NAME(SIGQUIT)
        SIGNAL_NAME(SIGILL)
        SIGNAL_NAME(SIGTRAP)
        SIGNAL_NAME(SIGABRT)
        SIGNAL_NAME(SIGBUS)
        SIGNAL_NAME(SIGFPE)
        SIGNAL_NAME(SIGKILL)
        SIGNAL_NAME(SIGUSR1)
        SIGNAL_NAME(SIGSEGV)
        SIGNAL_NAME(SIGUSR2)
        SIGNAL_NAME(SIGPIPE)
        SIGNAL_NAME(SIGALRM)
        SIGNAL_NAME(SIGTERM)
        SIGNAL_NAME(SIGCHLD)
        SIGNAL_NAME(SIGCONT)
        SIGNAL_NAME(SIGSTOP)
        SIGNAL_NAME(SIGTSTP)
        SIGNAL_NAME(SIGTTIN)
        SIGNAL_NAME(SIGTTOU)
        SIGNAL_NAME(SIGURG)
        SIGNAL_NAME(SIGXCPU)
        SIGNAL_NAME(SIGXFSZ)
        SIGNAL_NAME(SIGVTALRM)
        SIGNAL_NAME(SIGPROF)
        SIGNAL_NAME(SIGWINCH)
        SIGNAL_NAME(SIGIO)
        SIGNAL_NAME(SIGSYS)
#undef SIGNAL_NAME
    default:
        if(signal >= SIGRTMIN && signal <= SIGRTMAX)
            return QString("SIGRTMIN+%1").arg(signal - SIGRTMIN);
        return QString("signal %1").arg(signal);
    }
}

void DbgAdapter::onException(const int signal, const uint64_t address, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    const REGDUMP dump = self->readRegisters();
    const QString name = signalName(signal);

    QString msg = QString("[x64dbg] %1 (%2) at 0x%3").arg(name).arg(signal).arg(dump.regcontext.cip, 0, 16);
    if(address)
        msg += QString(", address 0x%1").arg(address, 0, 16);
    emit self->logMessage(msg + self->threadSuffix());
    self->emitStoppedState(name, dump);
}

void DbgAdapter::onError(const char* error, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    emit self->logMessage(QString("[x64dbg] Error: %1").arg(error));
}

void DbgAdapter::onDebugString(const char* text, void* userdata)
{
    auto* self = static_cast<DbgAdapter*>(userdata);
    emit self->logMessage(QString("[dbg] %1").arg(text));
}
