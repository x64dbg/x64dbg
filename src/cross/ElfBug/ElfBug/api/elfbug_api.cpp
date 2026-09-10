#include "elfbug_api.h"
#include "../core/Debugger.h"
#include "../thread/Registers.h"

#include <algorithm>
#include <atomic>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <fcntl.h>

struct ElfBugDebugger : ElfBug::Debugger
{
    ElfBugCallbacks cb = {};

    std::atomic<bool> active{false};
    std::atomic<pid_t> activePid{0};
    uint64_t entryPoint = 0;

    struct MemRegion
    {
        uint64_t start, end;
        bool executable;
        std::string pathname;
    };
    mutable std::mutex mapMutex;
    std::vector<MemRegion> memoryMap;
    std::unordered_map<std::string, uint64_t> moduleBases;

    mutable std::mutex bpDataMutex;
    std::set<uint64_t> breakpointAddrs;

    mutable std::mutex bpQueueMutex;
    struct BpRequest
    {
        uint64_t addr;
        bool setOrDelete; // true = set, false = delete
    };
    std::vector<BpRequest> pendingBpRequests;

    void refreshMemoryMap()
    {
        std::vector<MemRegion> newMaps;
        if(!mProcess)
        {
            std::lock_guard lock(mapMutex);
            memoryMap.clear();
            return;
        }

        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/maps", mProcess->pid);
        FILE* f = fopen(path, "r");
        if(!f)
        {
            std::lock_guard lock(mapMutex);
            memoryMap.clear();
            return;
        }

        char line[512];
        while(fgets(line, sizeof(line), f))
        {
            uint64_t start = 0, end = 0;
            char perms[8] = {};
            int pathOffset = 0;
            // %n captures the byte offset after the fixed prefix so we can take the pathname verbatim (it may contain spaces).
            if(sscanf(line, "%" SCNx64 "-%" SCNx64 " %4s %*x %*x:%*x %*u %n",
                      &start, &end, perms, &pathOffset) < 3)
                continue;

            std::string pathname;
            if(pathOffset > 0 && pathOffset < static_cast<int>(sizeof(line)))
            {
                const char* p = line + pathOffset;
                while(*p == ' ' || *p == '\t') ++p;
                size_t len = strlen(p);
                while(len > 0 && (p[len - 1] == '\n' || p[len - 1] == '\r' || p[len - 1] == ' '))
                    --len;
                if(len > 0 && p[0] != '[')
                    pathname.assign(p, len);

                static constexpr std::string_view deletedSuffix{" (deleted)"};
                if(pathname.size() >= deletedSuffix.size() &&
                        std::string_view(pathname).substr(pathname.size() - deletedSuffix.size()) == deletedSuffix)
                {
                    pathname.resize(pathname.size() - deletedSuffix.size());
                }
            }

            newMaps.push_back({start, end, perms[2] == 'x', std::move(pathname)});
        }
        fclose(f);

        std::lock_guard lock(mapMutex);
        memoryMap = std::move(newMaps);

        moduleBases.clear();
        for(const auto& r : memoryMap)
        {
            if(r.pathname.empty())
                continue;
            auto it = moduleBases.find(r.pathname);
            if(it == moduleBases.end() || r.start < it->second)
                moduleBases[r.pathname] = r.start;
        }
    }

    const MemRegion* findRegion(const uint64_t addr) const
    {
        auto it = std::upper_bound(memoryMap.begin(), memoryMap.end(), addr,
        [](const uint64_t a, const MemRegion & r) { return a < r.start; });
        if(it != memoryMap.begin())
        {
            --it;
            if(addr >= it->start && addr < it->end)
                return &*it;
        }
        return nullptr;
    }

    bool modLookup(const uint64_t addr, uint64_t* baseOut, std::string* pathOut) const
    {
        std::lock_guard lock(mapMutex);
        const auto* region = findRegion(addr);
        if(!region || region->pathname.empty())
            return false;

        const auto it = moduleBases.find(region->pathname);
        if(it == moduleBases.end())
            return false;

        if(baseOut)
            *baseOut = it->second;
        if(pathOut)
            *pathOut = region->pathname;
        return true;
    }

    void processPendingBreakpoints()
    {
        std::lock_guard queueLock(bpQueueMutex);
        for(const auto & req : pendingBpRequests)
        {
            if(!mProcess)
                continue;

            const auto addr = static_cast<ElfBug::ptr>(req.addr);
            if(req.setOrDelete)
            {
                if(mProcess->SetBreakpoint(addr))
                {
                    std::lock_guard lock(bpDataMutex);
                    breakpointAddrs.insert(req.addr);
                }
            }
            else
            {
                if(mProcess->DeleteBreakpoint(addr))
                {
                    std::lock_guard lock(bpDataMutex);
                    breakpointAddrs.erase(req.addr);
                }
            }
        }
        pendingBpRequests.clear();
    }

    bool memRead(const uint64_t addr, void* dest, const uint64_t size) const
    {
        if(!dest)
            return false;
        std::shared_lock lock(mProcessMutex);
        if(!active.load(std::memory_order_acquire) || !mProcess)
        {
            memset(dest, 0, size);
            return false;
        }
        return mProcess->MemRead(static_cast<ElfBug::ptr>(addr), dest, static_cast<ElfBug::ptr>(size));
    }

    bool memWrite(const uint64_t addr, const void* src, const uint64_t size) const
    {
        std::shared_lock lock(mProcessMutex);
        if(!active.load(std::memory_order_acquire) || !mProcess)
            return false;
        return mProcess->MemWrite(static_cast<ElfBug::ptr>(addr), src, static_cast<ElfBug::ptr>(size));
    }

    bool setRegister(const char* name, const uint64_t value) const
    {
        std::shared_lock lock(mProcessMutex);
        if(!active.load(std::memory_order_acquire) || !mThread)
            return false;

        auto & r = mThread->registers;
        const auto v = static_cast<ElfBug::ptr>(value);

        if(!strcmp(name, "csp") || !strcmp(name, "rsp"))       r.Rsp() = v;
        else if(!strcmp(name, "cip") || !strcmp(name, "rip"))  r.Rip() = v;
        else if(!strcmp(name, "rax"))                          r.Rax() = v;
        else if(!strcmp(name, "rbx"))                          r.Rbx() = v;
        else if(!strcmp(name, "rcx"))                          r.Rcx() = v;
        else if(!strcmp(name, "rdx"))                          r.Rdx() = v;
        else if(!strcmp(name, "rsi"))                          r.Rsi() = v;
        else if(!strcmp(name, "rdi"))                          r.Rdi() = v;
        else if(!strcmp(name, "rbp"))                          r.Rbp() = v;
        else if(!strcmp(name, "r8"))                           r.R8()  = v;
        else if(!strcmp(name, "r9"))                           r.R9()  = v;
        else if(!strcmp(name, "r10"))                          r.R10() = v;
        else if(!strcmp(name, "r11"))                          r.R11() = v;
        else if(!strcmp(name, "r12"))                          r.R12() = v;
        else if(!strcmp(name, "r13"))                          r.R13() = v;
        else if(!strcmp(name, "r14"))                          r.R14() = v;
        else if(!strcmp(name, "r15"))                          r.R15() = v;
        else
            return false;

        return r.Write();
    }

    ElfBugArch getArch() const
    {
        std::shared_lock lock(mProcessMutex);
        if(!mProcess)
            return ElfBugArch_Unknown;
        switch(mProcess->arch)
        {
        case ElfBug::Arch::X86_64:
            return ElfBugArch_X86_64;
        case ElfBug::Arch::I386:
            return ElfBugArch_I386;
        default:
            return ElfBugArch_Unknown;
        }
    }

    pid_t currentTid() const
    {
        std::shared_lock lock(mProcessMutex);
        if(!active.load(std::memory_order_acquire) || !IsPaused() || !mThread)
            return 0;
        return mThread->tid;
    }

    bool readRegisters(ElfBugRegisters* out) const
    {
        std::shared_lock lock(mProcessMutex);
        if(!active.load(std::memory_order_acquire) || !mThread)
            return false;

        const auto native = mThread->registers.Native();

        out->rax = native.rax;
        out->rbx = native.rbx;
        out->rcx = native.rcx;
        out->rdx = native.rdx;
        out->rbp = native.rbp;
        out->rsp = native.rsp;
        out->rsi = native.rsi;
        out->rdi = native.rdi;
        out->r8  = native.r8;
        out->r9  = native.r9;
        out->r10 = native.r10;
        out->r11 = native.r11;
        out->r12 = native.r12;
        out->r13 = native.r13;
        out->r14 = native.r14;
        out->r15 = native.r15;
        out->rip = native.rip;
        out->eflags = native.eflags;
        out->cs = static_cast<uint16_t>(native.cs);
        out->ds = static_cast<uint16_t>(native.ds);
        out->es = static_cast<uint16_t>(native.es);
        out->fs = static_cast<uint16_t>(native.fs);
        out->gs = static_cast<uint16_t>(native.gs);
        out->ss = static_cast<uint16_t>(native.ss);
        out->fs_base = native.fs_base;
        out->gs_base = native.gs_base;
        return true;
    }

protected:
    void cbCreateProcessEvent(const pid_t pid, const ElfBug::ptr ep) override
    {
        activePid.store(pid, std::memory_order_release);
        entryPoint = ep;
        refreshMemoryMap();
        active.store(true, std::memory_order_release);
        if(cb.onCreateProcess)
            cb.onCreateProcess(pid, ep, cb.userdata);
    }

    void cbExitProcessEvent(const int exitCode) override
    {
        activePid.store(0, std::memory_order_release);
        active.store(false, std::memory_order_release);
        {
            std::lock_guard lock(mapMutex);
            memoryMap.clear();
            moduleBases.clear();
        }
        {
            std::lock_guard lock(bpDataMutex);
            breakpointAddrs.clear();
        }
        {
            std::lock_guard lock(bpQueueMutex);
            pendingBpRequests.clear();
        }
        if(cb.onExitProcess)
            cb.onExitProcess(exitCode, cb.userdata);
    }

    void cbCreateThreadEvent(const pid_t tid) override
    {
        if(cb.onCreateThread)
            cb.onCreateThread(tid, cb.userdata);
    }

    void cbExitThreadEvent(const pid_t tid) override
    {
        if(cb.onExitThread)
            cb.onExitThread(tid, cb.userdata);
    }

    void cbSystemBreakpoint() override
    {
        if(mThread)
        {
            mThread->registers.Read();
            entryPoint = mThread->registers.Gip();
            refreshMemoryMap();
            processPendingBreakpoints();
        }
        if(cb.onSystemBreakpoint)
            cb.onSystemBreakpoint(cb.userdata);
    }

    void cbBreakpoint(const ElfBug::BreakpointInfo & info) override
    {
        processPendingBreakpoints();
        refreshMemoryMap();
        if(cb.onBreakpoint)
            cb.onBreakpoint(info.address, cb.userdata);
    }

    void cbStep() override
    {
        processPendingBreakpoints();
        refreshMemoryMap();
        if(cb.onStep)
            cb.onStep(cb.userdata);
    }

    void cbPaused() override
    {
        processPendingBreakpoints();
        refreshMemoryMap();
        if(cb.onPaused)
            cb.onPaused(cb.userdata);
    }

    void cbExceptionEvent(const int signal, const ElfBug::ptr address) override
    {
        processPendingBreakpoints();
        refreshMemoryMap();
        if(cb.onException)
            cb.onException(signal, address, cb.userdata);
    }

    void cbPauseTick() override
    {
        processPendingBreakpoints();
    }

    void cbInternalError(const std::string & error) override
    {
        if(cb.onError)
            cb.onError(error.c_str(), cb.userdata);
    }

    void cbDebugStringEvent(const std::string & text) override
    {
        if(cb.onDebugString)
            cb.onDebugString(text.c_str(), cb.userdata);
    }
};

extern "C" {

    ElfBugDebugger* ElfBugCreate(const ElfBugCallbacks* callbacks)
    {
        auto* dbg = new ElfBugDebugger();
        if(callbacks)
            dbg->cb = *callbacks;
        return dbg;
    }

    void ElfBugDestroy(const ElfBugDebugger* dbg)
    {
        if(!dbg)
            return;
        delete dbg;
    }

    bool ElfBugInit(ElfBugDebugger* dbg, const char* path)
    {
        if(!dbg)
            return false;
        return dbg->Init(path);
    }

    void ElfBugStart(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return;
        dbg->Start();
    }

    void ElfBugContinue(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return;
        dbg->Continue();
    }

    void ElfBugStepInto(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return;
        dbg->StepInto();
    }

    void ElfBugStepOver(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return;
        dbg->StepOver();
    }

    void ElfBugPause(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return;
        dbg->Pause();
    }

    bool ElfBugStop(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return false;
        return dbg->Stop();
    }

    bool ElfBugGetRegisters(const ElfBugDebugger* dbg, ElfBugRegisters* regs)
    {
        if(!dbg || !regs)
            return false;
        return dbg->readRegisters(regs);
    }

    pid_t ElfBugGetPid(const ElfBugDebugger* dbg)
    {
        if(!dbg)
            return 0;
        return dbg->activePid.load(std::memory_order_acquire);
    }

    pid_t ElfBugGetCurrentTid(const ElfBugDebugger* dbg)
    {
        if(!dbg)
            return 0;
        return dbg->currentTid();
    }

    ElfBugArch ElfBugGetArch(const ElfBugDebugger* dbg)
    {
        if(!dbg)
            return ElfBugArch_Unknown;
        return dbg->getArch();
    }

    bool ElfBugMemRead(const ElfBugDebugger* dbg, const uint64_t addr, void* dest, const uint64_t size)
    {
        if(!dbg)
            return false;
        return dbg->memRead(addr, dest, size);
    }

    bool ElfBugMemWrite(const ElfBugDebugger* dbg, const uint64_t addr, const void* src, const uint64_t size)
    {
        if(!dbg || !src)
            return false;
        return dbg->memWrite(addr, src, size);
    }

    bool ElfBugSetRegister(const ElfBugDebugger* dbg, const char* name, const uint64_t value)
    {
        if(!dbg || !name)
            return false;
        return dbg->setRegister(name, value);
    }

    bool ElfBugMemFindBaseAddr(const ElfBugDebugger* dbg, const uint64_t addr, uint64_t* base, uint64_t* size)
    {
        if(!dbg || !base || !size)
            return false;
        if(!dbg->active.load(std::memory_order_acquire))
            return false;

        std::lock_guard lock(dbg->mapMutex);
        const auto* region = dbg->findRegion(addr);
        if(!region)
            return false;

        *base = region->start;
        *size = region->end - region->start;
        return true;
    }

    bool ElfBugMemIsCodePtr(const ElfBugDebugger* dbg, const uint64_t addr)
    {
        if(!dbg)
            return false;
        if(!dbg->active.load(std::memory_order_acquire))
            return false;

        std::lock_guard lock(dbg->mapMutex);
        const auto* region = dbg->findRegion(addr);
        return region && region->executable;
    }

    bool ElfBugMemIsValidPtr(const ElfBugDebugger* dbg, const uint64_t addr)
    {
        if(!dbg)
            return false;
        if(!dbg->active.load(std::memory_order_acquire))
            return false;

        std::lock_guard lock(dbg->mapMutex);
        return dbg->findRegion(addr) != nullptr;
    }

    bool ElfBugModBaseFromAddr(const ElfBugDebugger* dbg, const uint64_t addr, uint64_t* base)
    {
        if(!dbg || !base)
            return false;
        if(!dbg->active.load(std::memory_order_acquire))
            return false;

        uint64_t b = 0;
        if(!dbg->modLookup(addr, &b, nullptr))
            return false;
        *base = b;
        return true;
    }

    bool ElfBugModNameFromAddr(const ElfBugDebugger* dbg, const uint64_t addr,
                               char* buf, const uint64_t bufSize, const bool extension)
    {
        if(!dbg || !buf || bufSize == 0)
            return false;
        if(!dbg->active.load(std::memory_order_acquire))
            return false;

        std::string path;
        if(!dbg->modLookup(addr, nullptr, &path))
            return false;

        const size_t slash = path.find_last_of('/');
        std::string base = (slash == std::string::npos) ? path : path.substr(slash + 1);

        if(!extension)
        {
            size_t soPos = std::string::npos;
            for(size_t p = base.find(".so"); p != std::string::npos; p = base.find(".so", p + 1))
            {
                const size_t after = p + 3;
                if(after == base.size() || base[after] == '.')
                {
                    soPos = p;
                    break;
                }
            }

            if(soPos != std::string::npos)
            {
                const size_t after = soPos + 3;
                if(after == base.size())
                    base.resize(soPos);       // "libfoo.so" -> "libfoo"
                else
                    base.erase(soPos, 3);     // "libfoo.so.6" -> "libfoo.6"
            }
            else
            {
                const size_t dot = base.find_last_of('.');
                if(dot != std::string::npos && dot > 0)
                    base.resize(dot);
            }
        }

        if(base.size() + 1 > bufSize)
            return false;

        memcpy(buf, base.c_str(), base.size() + 1);
        return true;
    }

    bool ElfBugSetBreakpoint(ElfBugDebugger* dbg, uint64_t addr)
    {
        if(!dbg)
            return false;
        if(!dbg->active.load(std::memory_order_acquire))
            return false;

        std::lock_guard lock(dbg->bpQueueMutex);
        dbg->pendingBpRequests.push_back({addr, true});
        return true;
    }

    bool ElfBugDeleteBreakpoint(ElfBugDebugger* dbg, const uint64_t addr)
    {
        if(!dbg)
            return false;
        if(!dbg->active.load(std::memory_order_acquire))
            return false;

        std::lock_guard lock(dbg->bpQueueMutex);
        dbg->pendingBpRequests.push_back({addr, false});
        return true;
    }

    bool ElfBugIsBreakpointEffective(const ElfBugDebugger* dbg, const uint64_t addr)
    {
        if(!dbg)
            return false;

        {
            std::lock_guard lock(dbg->bpQueueMutex);
            for(auto it = dbg->pendingBpRequests.rbegin(); it != dbg->pendingBpRequests.rend(); ++it)
            {
                if(it->addr == addr)
                    return it->setOrDelete;
            }
        }

        std::lock_guard lock(dbg->bpDataMutex);
        return dbg->breakpointAddrs.contains(addr);
    }

} // extern "C"
