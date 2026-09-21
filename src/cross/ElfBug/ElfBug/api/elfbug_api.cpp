#include <ElfBug/api/elfbug_api.h>
#include <ElfBug/core/Debugger.h>
#include <ElfBug/process/ProcessList.h>
#include <ElfBug/thread/Registers.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cinttypes>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace
{
    constexpr std::size_t kNoRegister = static_cast<std::size_t>(-1);
    constexpr auto kRegisterWriteTimeout = std::chrono::seconds(1);

    std::size_t registerOffset(const char* name)
    {
        static const std::unordered_map<std::string, std::size_t> offsets =
        {
            {"csp", offsetof(user_regs_struct, rsp)}, {"rsp", offsetof(user_regs_struct, rsp)},
            {"cip", offsetof(user_regs_struct, rip)}, {"rip", offsetof(user_regs_struct, rip)},
            {"rax", offsetof(user_regs_struct, rax)}, {"rbx", offsetof(user_regs_struct, rbx)},
            {"rcx", offsetof(user_regs_struct, rcx)}, {"rdx", offsetof(user_regs_struct, rdx)},
            {"rsi", offsetof(user_regs_struct, rsi)}, {"rdi", offsetof(user_regs_struct, rdi)},
            {"rbp", offsetof(user_regs_struct, rbp)},
            {"r8", offsetof(user_regs_struct, r8)},   {"r9", offsetof(user_regs_struct, r9)},
            {"r10", offsetof(user_regs_struct, r10)}, {"r11", offsetof(user_regs_struct, r11)},
            {"r12", offsetof(user_regs_struct, r12)}, {"r13", offsetof(user_regs_struct, r13)},
            {"r14", offsetof(user_regs_struct, r14)}, {"r15", offsetof(user_regs_struct, r15)},
        };
        const auto it = offsets.find(name);
        return it == offsets.end() ? kNoRegister : it->second;
    }

    ElfBugArch toApiArch(const ElfBug::Arch arch)
    {
        switch(arch)
        {
        case ElfBug::Arch::X86_64:
            return ElfBugArch_X86_64;
        case ElfBug::Arch::I386:
            return ElfBugArch_I386;
        default:
            return ElfBugArch_Unknown;
        }
    }

    void copyString(char* dest, const size_t size, const std::string & source)
    {
        if(size == 0)
            return;
        const size_t n = std::min(source.size(), size - 1);
        std::memcpy(dest, source.data(), n);
        dest[n] = '\0';
    }
}

struct ElfBugDebugger : ElfBug::Debugger
{
    enum class BreakpointAction
    {
        Set,
        Delete
    };

    struct MemRegion
    {
        uint64_t start = 0;
        uint64_t end = 0;
        bool executable = false;
        std::string pathname;
    };

    struct BreakpointRequest
    {
        uint64_t addr = 0;
        BreakpointAction action = BreakpointAction::Set;
    };

    struct RegisterRequest
    {
        std::string name;
        uint64_t value = 0;
        bool done = false;
        bool ok = false;
        bool abandoned = false;
    };

    void SetCallbacks(const ElfBugCallbacks & callbacks)
    {
        mCb = callbacks;
    }

    [[nodiscard]] bool IsActive() const
    {
        return mActive.load(std::memory_order_acquire);
    }

    [[nodiscard]] pid_t Pid() const
    {
        return mActivePid.load(std::memory_order_acquire);
    }

    void NoteThreadSuspended(const pid_t tid, const bool suspended)
    {
        std::lock_guard lock(mThreadMutex);
        const uint32_t count = suspendCountOf(tid);
        const std::string reason = suspended ? "Suspended" : resolveWaitReason(tid);
        for(auto & info : mThreadList)
        {
            if(info.tid == tid)
            {
                info.suspend_count = count;
                copyString(info.wait_reason, sizeof(info.wait_reason), reason);
            }
        }
    }

    bool FindBaseAddr(const uint64_t addr, uint64_t* base, uint64_t* size) const
    {
        if(!IsActive())
            return false;

        std::lock_guard lock(mMapMutex);
        const auto* region = findRegion(addr);
        if(!region)
            return false;

        *base = region->start;
        *size = region->end - region->start;
        return true;
    }

    [[nodiscard]] bool IsCodePtr(const uint64_t addr) const
    {
        if(!IsActive())
            return false;

        std::lock_guard lock(mMapMutex);
        const auto* region = findRegion(addr);
        return region && region->executable;
    }

    [[nodiscard]] bool IsValidPtr(const uint64_t addr) const
    {
        if(!IsActive())
            return false;

        std::lock_guard lock(mMapMutex);
        return findRegion(addr) != nullptr;
    }

    bool ModBase(const uint64_t addr, uint64_t* base) const
    {
        if(!IsActive())
            return false;
        return modLookup(addr, base, nullptr);
    }

    bool ModName(const uint64_t addr, std::string & name, const bool extension) const
    {
        if(!IsActive())
            return false;

        std::string path;
        if(!modLookup(addr, nullptr, &path))
            return false;

        const size_t slash = path.find_last_of('/');
        name = (slash == std::string::npos) ? path : path.substr(slash + 1);
        if(extension)
            return true;

        size_t soPos = std::string::npos;
        for(size_t p = name.find(".so"); p != std::string::npos; p = name.find(".so", p + 1))
        {
            const size_t after = p + 3;
            if(after == name.size() || name[after] == '.')
            {
                soPos = p;
                break;
            }
        }

        if(soPos == std::string::npos)
        {
            const size_t dot = name.find_last_of('.');
            if(dot != std::string::npos && dot > 0)
                name.resize(dot);
        }
        else if(soPos + 3 == name.size())
            name.resize(soPos);
        else
            name.erase(soPos, 3);

        return true;
    }

    bool QueueBreakpoint(const uint64_t addr, const BreakpointAction action)
    {
        if(!IsActive())
            return false;

        std::lock_guard lock(mBreakpointQueueMutex);
        mPendingBreakpoints.push_back({addr, action});
        return true;
    }

    [[nodiscard]] bool IsBreakpointEffective(const uint64_t addr) const
    {
        {
            std::lock_guard lock(mBreakpointQueueMutex);
            for(auto it = mPendingBreakpoints.rbegin(); it != mPendingBreakpoints.rend(); ++it)
            {
                if(it->addr == addr)
                    return it->action == BreakpointAction::Set;
            }
        }

        std::lock_guard lock(mBreakpointMutex);
        return mBreakpointAddresses.contains(addr);
    }

    void SampleWaitReasonsForPause()
    {
        std::lock_guard threads(mThreadMutex);
        std::shared_lock lock(mProcessMutex);
        mPauseWaitReasons.clear();
        if(!mProcess)
            return;
        for(const auto & [tid, thread] : mProcess->threads)
        {
            if(thread->IsRunning())
                mPauseWaitReasons[tid] = readWaitReason(mProcess->pid, tid);
        }
    }

    void ClearPauseWaitReasons()
    {
        std::lock_guard lock(mThreadMutex);
        mPauseWaitReasons.clear();
    }

    uint32_t GetThreadList(ElfBugThreadInfo* list, const uint32_t capacity) const
    {
        std::lock_guard lock(mThreadMutex);
        const auto count = static_cast<uint32_t>(mThreadList.size());
        if(list)
        {
            const uint32_t n = std::min(count, capacity);
            for(uint32_t i = 0; i < n; ++i)
                list[i] = mThreadList[i];
        }
        return count;
    }

    ElfBugArch GetArch() const
    {
        std::shared_lock lock(mProcessMutex);
        if(!mProcess)
            return ElfBugArch_Unknown;
        return toApiArch(mProcess->arch);
    }

    pid_t CurrentTid() const
    {
        std::shared_lock lock(mProcessMutex);
        if(!mActive.load(std::memory_order_acquire) || !IsPaused() || !mThread)
            return 0;
        return mThread->tid;
    }

    bool ReadRegisters(ElfBugRegisters* out) const
    {
        std::shared_lock lock(mProcessMutex);
        if(!mActive.load(std::memory_order_acquire) || !mThread)
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

    bool MemRead(const uint64_t addr, void* dest, const uint64_t size) const
    {
        if(!dest)
            return false;
        std::shared_lock lock(mProcessMutex);
        if(!mActive.load(std::memory_order_acquire) || !mProcess)
        {
            memset(dest, 0, size);
            return false;
        }
        return mProcess->MemRead(static_cast<ElfBug::ptr>(addr), dest, static_cast<ElfBug::ptr>(size));
    }

    bool MemWrite(const uint64_t addr, const void* src, const uint64_t size) const
    {
        std::shared_lock lock(mProcessMutex);
        if(!mActive.load(std::memory_order_acquire) || !mProcess)
            return false;
        return mProcess->MemWrite(static_cast<ElfBug::ptr>(addr), src, static_cast<ElfBug::ptr>(size));
    }

    bool SetRegister(const char* name, const uint64_t value) const
    {
        if(!mActive.load(std::memory_order_acquire) || !IsPaused())
            return false;
        if(registerOffset(name) == kNoRegister)
            return false;

        auto request = std::make_shared<RegisterRequest>();
        request->name = name;
        request->value = value;

        std::unique_lock lock(mRegisterQueueMutex);
        mPendingRegisters.push_back(request);
        if(!mRegisterQueueCv.wait_for(lock, kRegisterWriteTimeout, [&] { return request->done; }))
        {
            request->abandoned = true;
            const auto it = std::find(mPendingRegisters.begin(), mPendingRegisters.end(), request);
            if(it != mPendingRegisters.end())
                mPendingRegisters.erase(it);
            return false;
        }
        return request->ok;
    }


protected:
    void cbCreateProcess(const pid_t pid, const ElfBug::ptr ep) override
    {
        mActivePid.store(pid, std::memory_order_release);
        mEntryPoint = ep;
        {
            std::lock_guard lock(mThreadMutex);
            mThreadNumbers.clear();
            mThreadNumbers[pid] = 0;
            mNextThreadNumber = 1;
        }
        refreshThreadList(false);
        refreshMemoryMap();
        mActive.store(true, std::memory_order_release);
        if(mCb.onCreateProcess)
            mCb.onCreateProcess(pid, ep, mCb.userdata);
    }

    void cbExitProcess(const int exitCode) override
    {
        clearSessionSnapshot();
        if(mCb.onExitProcess)
            mCb.onExitProcess(exitCode, mCb.userdata);
    }

    void cbExec() override
    {
        {
            std::lock_guard lock(mBreakpointQueueMutex);
            mPendingBreakpoints.clear();
        }
        mEntryPoint = 0;
        refreshMemoryMap();
        refreshThreadList(false);
        if(mCb.onExec)
            mCb.onExec(mCb.userdata);
    }

    void cbCreateThread(const pid_t tid) override
    {
        {
            std::lock_guard lock(mThreadMutex);
            mThreadNumbers[tid] = mNextThreadNumber++;
        }
        refreshThreadList(false);
        if(mCb.onCreateThread)
            mCb.onCreateThread(tid, mCb.userdata);
    }

    void cbExitThread(const pid_t tid) override
    {
        {
            std::lock_guard lock(mThreadMutex);
            mThreadNumbers.erase(tid);
            mPauseWaitReasons.erase(tid);
        }
        refreshThreadList(false);
        if(mCb.onExitThread)
            mCb.onExitThread(tid, mCb.userdata);
    }

    void cbSystemBreakpoint() override
    {
        if(mThread)
        {
            mThread->registers.Read();
            mEntryPoint = mThread->registers.Gip();
            refreshMemoryMap();
            processPendingBreakpoints();
            refreshThreadList(true);
        }
        if(mCb.onSystemBreakpoint)
            mCb.onSystemBreakpoint(mCb.userdata);
    }

    void cbAttachBreakpoint() override
    {
        if(mThread)
        {
            mThread->registers.Read();
            mEntryPoint = mThread->registers.Gip();
            refreshMemoryMap();
            processPendingBreakpoints();
            refreshThreadList(true);
        }
        if(mCb.onAttachBreakpoint)
            mCb.onAttachBreakpoint(mCb.userdata);
    }

    void cbDetach() override
    {
        clearSessionSnapshot();
        if(mCb.onDetach)
            mCb.onDetach(mCb.userdata);
    }

    void cbBreakpoint(const ElfBug::BreakpointInfo & info) override
    {
        processPendingBreakpoints();
        refreshMemoryMap();
        refreshThreadList(true);
        if(mCb.onBreakpoint)
            mCb.onBreakpoint(info.address, mCb.userdata);
    }

    void cbStep() override
    {
        processPendingBreakpoints();
        refreshMemoryMap();
        refreshThreadList(true);
        if(mCb.onStep)
            mCb.onStep(mCb.userdata);
    }

    void cbPaused() override
    {
        processPendingBreakpoints();
        refreshMemoryMap();
        refreshThreadList(true);
        if(mCb.onPaused)
            mCb.onPaused(mCb.userdata);
    }

    void cbException(const int signal, const ElfBug::ptr address) override
    {
        processPendingBreakpoints();
        refreshMemoryMap();
        refreshThreadList(true);
        if(mCb.onException)
            mCb.onException(signal, address, mCb.userdata);
    }

    void cbPauseTick() override
    {
        processPendingBreakpoints();
        processPendingRegisters();
    }

    void cbInternalError(const std::string & error) override
    {
        if(mCb.onError)
            mCb.onError(error.c_str(), mCb.userdata);
    }

    void cbDebugString(const std::string & text) override
    {
        if(mCb.onDebugString)
            mCb.onDebugString(text.c_str(), mCb.userdata);
    }

private:
    uint32_t suspendCountOf(const pid_t tid) const
    {
        std::shared_lock lock(mProcessMutex);
        if(!mProcess)
            return 0;
        const auto it = mProcess->threads.find(tid);
        return it != mProcess->threads.end() ? it->second->SuspendCount() : 0u;
    }

    std::string resolveWaitReason(const pid_t tid)
    {
        std::string reason;
        {
            std::shared_lock lock(mProcessMutex);
            if(mProcess)
            {
                const auto it = mProcess->threads.find(tid);
                if(it != mProcess->threads.end())
                    reason = it->second->WaitReason();
            }
        }
        if(reason.empty())
        {
            const auto sampled = mPauseWaitReasons.find(tid);
            if(sampled != mPauseWaitReasons.end())
                reason = sampled->second;
        }
        return reason;
    }

    static void readThreadName(const pid_t pid, const pid_t tid, char* name, const size_t size)
    {
        name[0] = '\0';
        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/task/%d/comm", pid, tid);
        FILE* f = fopen(path, "r");
        if(!f)
            return;
        if(fgets(name, static_cast<int>(size), f))
            name[strcspn(name, "\n")] = '\0';
        fclose(f);
    }

    uint64_t readBootTimeMs()
    {
        if(mBootTimeMs != 0)
            return mBootTimeMs;
        FILE* f = fopen("/proc/stat", "r");
        if(!f)
            return 0;
        char line[256];
        while(fgets(line, sizeof(line), f))
        {
            uint64_t btime = 0;
            if(sscanf(line, "btime %" SCNu64, &btime) == 1)
            {
                mBootTimeMs = btime * 1000u;
                break;
            }
        }
        fclose(f);
        return mBootTimeMs;
    }

    void readThreadStat(const pid_t pid, const pid_t tid, ElfBugThreadInfo & info)
    {
        info.policy = -1;
        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/task/%d/stat", pid, tid);
        FILE* f = fopen(path, "r");
        if(!f)
            return;
        char line[1024];
        const bool ok = fgets(line, sizeof(line), f) != nullptr;
        fclose(f);
        if(!ok)
            return;

        const char* fields = strrchr(line, ')');
        if(!fields)
            return;
        uint64_t utime = 0, stime = 0, starttime = 0;
        int32_t nice = 0;
        uint32_t rtPriority = 0, policy = 0;
        const int matched = sscanf(fields + 1,
                                   " %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %" SCNu64 " %" SCNu64
                                   " %*d %*d %*d %" SCNd32 " %*d %*d %" SCNu64
                                   " %*u %*d %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*d %*d %" SCNu32 " %" SCNu32,
                                   &utime, &stime, &nice, &starttime, &rtPriority, &policy);
        if(matched != 6)
            return;

        const long ticks = sysconf(_SC_CLK_TCK);
        if(ticks <= 0)
            return;
        const auto ticksPerSecond = static_cast<uint64_t>(ticks);
        info.user_time_ms = utime * 1000u / ticksPerSecond;
        info.kernel_time_ms = stime * 1000u / ticksPerSecond;
        const uint64_t boot = readBootTimeMs();
        info.start_time_ms = boot ? boot + starttime * 1000u / ticksPerSecond : 0;
        info.nice = nice;
        info.rt_priority = static_cast<int32_t>(rtPriority);
        info.policy = static_cast<int32_t>(policy);
    }

    void refreshThreadList(const bool withRegisters)
    {
        std::lock_guard threads(mThreadMutex);
        std::vector<ElfBugThreadInfo> list;
        {
            std::unique_lock lock(mProcessMutex);
            if(mProcess)
            {
                for(auto & [tid, thread] : mProcess->threads)
                {
                    const auto number = mThreadNumbers.find(tid);
                    if(number == mThreadNumbers.end())
                        continue;
                    if(withRegisters && !thread->IsRunning())
                        thread->registers.Read();
                    ElfBugThreadInfo info = {};
                    info.tid = tid;
                    info.number = number->second;
                    info.rip = thread->registers.Native().rip;
                    info.fs_base = thread->registers.Native().fs_base;
                    readThreadStat(mProcess->pid, tid, info);
                    info.suspend_count = thread->SuspendCount();
                    std::string reason = thread->WaitReason();
                    if(reason.empty())
                    {
                        const auto sampled = mPauseWaitReasons.find(tid);
                        if(sampled != mPauseWaitReasons.end())
                            reason = sampled->second;
                    }
                    copyString(info.wait_reason, sizeof(info.wait_reason), reason);
                    readThreadName(mProcess->pid, tid, info.name, sizeof(info.name));
                    list.push_back(info);
                }
            }
        }
        std::sort(list.begin(), list.end(), [](const ElfBugThreadInfo & a, const ElfBugThreadInfo & b)
        {
            return a.number < b.number;
        });
        mThreadList = std::move(list);
    }

    void refreshMemoryMap()
    {
        std::vector<MemRegion> newMaps;
        if(!mProcess)
        {
            std::lock_guard lock(mMapMutex);
            mMemoryMap.clear();
            return;
        }

        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/maps", mProcess->pid);
        FILE* f = fopen(path, "r");
        if(!f)
        {
            std::lock_guard lock(mMapMutex);
            mMemoryMap.clear();
            return;
        }

        char line[512];
        while(fgets(line, sizeof(line), f))
        {
            uint64_t start = 0, end = 0;
            char perms[8] = {};
            int pathOffset = 0;
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

        std::lock_guard lock(mMapMutex);
        mMemoryMap = std::move(newMaps);

        mModuleBases.clear();
        for(const auto& r : mMemoryMap)
        {
            if(r.pathname.empty())
                continue;
            auto it = mModuleBases.find(r.pathname);
            if(it == mModuleBases.end() || r.start < it->second)
                mModuleBases[r.pathname] = r.start;
        }
    }

    const MemRegion* findRegion(const uint64_t addr) const
    {
        auto it = std::upper_bound(mMemoryMap.begin(), mMemoryMap.end(), addr,
        [](const uint64_t a, const MemRegion & r) { return a < r.start; });
        if(it != mMemoryMap.begin())
        {
            --it;
            if(addr >= it->start && addr < it->end)
                return &*it;
        }
        return nullptr;
    }

    bool modLookup(const uint64_t addr, uint64_t* baseOut, std::string* pathOut) const
    {
        std::lock_guard lock(mMapMutex);
        const auto* region = findRegion(addr);
        if(!region || region->pathname.empty())
            return false;

        const auto it = mModuleBases.find(region->pathname);
        if(it == mModuleBases.end())
            return false;

        if(baseOut)
            *baseOut = it->second;
        if(pathOut)
            *pathOut = region->pathname;
        return true;
    }

    void processPendingBreakpoints()
    {
        std::lock_guard queueLock(mBreakpointQueueMutex);
        for(const auto & req : mPendingBreakpoints)
        {
            if(!mProcess)
                continue;

            const auto addr = static_cast<ElfBug::ptr>(req.addr);
            if(req.action == BreakpointAction::Set)
            {
                if(mProcess->SetBreakpoint(addr))
                {
                    std::lock_guard lock(mBreakpointMutex);
                    mBreakpointAddresses.insert(req.addr);
                }
            }
            else
            {
                if(mProcess->DeleteBreakpoint(addr))
                {
                    std::lock_guard lock(mBreakpointMutex);
                    mBreakpointAddresses.erase(req.addr);
                }
            }
        }
        mPendingBreakpoints.clear();
    }

    bool writeRegisterOnTracer(const std::string & name, const uint64_t value) const
    {
        const std::size_t offset = registerOffset(name.c_str());
        if(offset == kNoRegister)
            return false;

        std::unique_lock lock(mProcessMutex);
        if(!mThread)
            return false;

        errno = 0;
        if(ptrace(PTRACE_POKEUSER, mThread->tid, reinterpret_cast<void*>(offset),
                  reinterpret_cast<void*>(static_cast<uintptr_t>(value))) == -1 && errno != 0)
            return false;

        return mThread->registers.Read();
    }

    void processPendingRegisters() const
    {
        std::vector<std::shared_ptr<RegisterRequest>> requests;
        {
            std::lock_guard lock(mRegisterQueueMutex);
            if(mPendingRegisters.empty())
                return;
            requests.swap(mPendingRegisters);
        }

        for(const auto & request : requests)
        {
            {
                std::lock_guard lock(mRegisterQueueMutex);
                if(request->abandoned)
                    continue;
            }
            const bool ok = writeRegisterOnTracer(request->name, request->value);
            std::lock_guard lock(mRegisterQueueMutex);
            request->ok = ok;
            request->done = true;
        }
        mRegisterQueueCv.notify_all();
    }

    void clearSessionSnapshot()
    {
        mActivePid.store(0, std::memory_order_release);
        mActive.store(false, std::memory_order_release);
        {
            std::lock_guard lock(mMapMutex);
            mMemoryMap.clear();
            mModuleBases.clear();
        }
        {
            std::lock_guard lock(mBreakpointMutex);
            mBreakpointAddresses.clear();
        }
        {
            std::lock_guard lock(mBreakpointQueueMutex);
            mPendingBreakpoints.clear();
        }
        {
            std::lock_guard lock(mRegisterQueueMutex);
            for(const auto & request : mPendingRegisters)
                request->done = true;
            mPendingRegisters.clear();
        }
        mRegisterQueueCv.notify_all();
        {
            std::lock_guard lock(mThreadMutex);
            mThreadNumbers.clear();
            mThreadList.clear();
            mPauseWaitReasons.clear();
        }
    }

    ElfBugCallbacks mCb = {};
    std::atomic<bool> mActive{false};
    std::atomic<pid_t> mActivePid{0};
    uint64_t mEntryPoint = 0;
    uint64_t mBootTimeMs = 0;

    mutable std::mutex mMapMutex;
    std::vector<MemRegion> mMemoryMap;
    std::unordered_map<std::string, uint64_t> mModuleBases;

    mutable std::mutex mBreakpointMutex;
    std::set<uint64_t> mBreakpointAddresses;

    mutable std::mutex mBreakpointQueueMutex;
    std::vector<BreakpointRequest> mPendingBreakpoints;

    mutable std::mutex mRegisterQueueMutex;
    mutable std::condition_variable mRegisterQueueCv;
    mutable std::vector<std::shared_ptr<RegisterRequest>> mPendingRegisters;

    mutable std::mutex mThreadMutex;
    std::unordered_map<pid_t, uint32_t> mThreadNumbers;
    uint32_t mNextThreadNumber = 0;
    std::vector<ElfBugThreadInfo> mThreadList;
    std::unordered_map<pid_t, std::string> mPauseWaitReasons;
};

extern "C" {

    uint32_t ElfBugEnumProcesses(ElfBugProcessInfo* list, const uint32_t capacity)
    {
        const auto entries = ElfBug::EnumProcesses();
        const auto count = static_cast<uint32_t>(entries.size());
        if(list)
        {
            const uint32_t n = std::min(count, capacity);
            for(uint32_t i = 0; i < n; ++i)
            {
                const auto & entry = entries[i];
                ElfBugProcessInfo & info = list[i];
                info = {};
                info.pid = entry.pid;
                info.arch = toApiArch(entry.arch);
                info.traced = entry.traced;
                copyString(info.name, sizeof(info.name), entry.name);
                copyString(info.path, sizeof(info.path), entry.path);
                copyString(info.command_line, sizeof(info.command_line), entry.commandLine);
            }
        }
        return count;
    }

    ElfBugDebugger* ElfBugCreate(const ElfBugCallbacks* callbacks)
    {
        auto* dbg = new ElfBugDebugger();
        if(callbacks)
            dbg->SetCallbacks(*callbacks);
        return dbg;
    }

    void ElfBugDestroy(ElfBugDebugger* dbg)
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

    bool ElfBugAttach(ElfBugDebugger* dbg, const pid_t pid)
    {
        if(!dbg)
            return false;
        return dbg->Attach(pid);
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
        dbg->ClearPauseWaitReasons();
        dbg->Continue();
    }

    void ElfBugStepInto(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return;
        dbg->ClearPauseWaitReasons();
        dbg->StepInto();
    }

    void ElfBugStepOver(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return;
        dbg->ClearPauseWaitReasons();
        dbg->StepOver();
    }

    void ElfBugPause(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return;
        dbg->SampleWaitReasonsForPause();
        dbg->Pause();
    }

    bool ElfBugStop(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return false;
        return dbg->Stop();
    }

    bool ElfBugDetach(ElfBugDebugger* dbg)
    {
        if(!dbg)
            return false;
        return dbg->Detach();
    }

    bool ElfBugGetRegisters(const ElfBugDebugger* dbg, ElfBugRegisters* regs)
    {
        if(!dbg || !regs)
            return false;
        return dbg->ReadRegisters(regs);
    }

    pid_t ElfBugGetPid(const ElfBugDebugger* dbg)
    {
        if(!dbg)
            return 0;
        return dbg->Pid();
    }

    bool ElfBugIsPaused(const ElfBugDebugger* dbg)
    {
        if(!dbg)
            return false;
        return dbg->IsPaused();
    }

    pid_t ElfBugGetCurrentTid(const ElfBugDebugger* dbg)
    {
        if(!dbg)
            return 0;
        return dbg->CurrentTid();
    }

    uint32_t ElfBugGetThreadList(const ElfBugDebugger* dbg, ElfBugThreadInfo* list, const uint32_t capacity)
    {
        if(!dbg)
            return 0;
        return dbg->GetThreadList(list, capacity);
    }

    bool ElfBugSwitchThread(ElfBugDebugger* dbg, const pid_t tid)
    {
        if(!dbg)
            return false;
        if(!dbg->IsActive())
            return false;
        return dbg->SwitchThread(tid);
    }

    bool ElfBugSetThreadSuspended(ElfBugDebugger* dbg, const pid_t tid, const bool suspended)
    {
        if(!dbg)
            return false;
        if(!dbg->IsActive())
            return false;
        if(!dbg->SetThreadSuspended(tid, suspended))
            return false;

        dbg->NoteThreadSuspended(tid, suspended);
        return true;
    }

    ElfBugArch ElfBugGetArch(const ElfBugDebugger* dbg)
    {
        if(!dbg)
            return ElfBugArch_Unknown;
        return dbg->GetArch();
    }

    bool ElfBugMemRead(const ElfBugDebugger* dbg, const uint64_t addr, void* dest, const uint64_t size)
    {
        if(!dbg)
            return false;
        return dbg->MemRead(addr, dest, size);
    }

    bool ElfBugMemWrite(const ElfBugDebugger* dbg, const uint64_t addr, const void* src, const uint64_t size)
    {
        if(!dbg || !src)
            return false;
        return dbg->MemWrite(addr, src, size);
    }

    bool ElfBugSetRegister(ElfBugDebugger* dbg, const char* name, const uint64_t value)
    {
        if(!dbg || !name)
            return false;
        return dbg->SetRegister(name, value);
    }

    bool ElfBugMemFindBaseAddr(const ElfBugDebugger* dbg, const uint64_t addr, uint64_t* base, uint64_t* size)
    {
        if(!dbg || !base || !size)
            return false;
        return dbg->FindBaseAddr(addr, base, size);
    }

    bool ElfBugMemIsCodePtr(const ElfBugDebugger* dbg, const uint64_t addr)
    {
        if(!dbg)
            return false;
        return dbg->IsCodePtr(addr);
    }

    bool ElfBugMemIsValidPtr(const ElfBugDebugger* dbg, const uint64_t addr)
    {
        if(!dbg)
            return false;
        return dbg->IsValidPtr(addr);
    }

    bool ElfBugModBaseFromAddr(const ElfBugDebugger* dbg, const uint64_t addr, uint64_t* base)
    {
        if(!dbg || !base)
            return false;
        return dbg->ModBase(addr, base);
    }

    bool ElfBugModNameFromAddr(const ElfBugDebugger* dbg, const uint64_t addr,
                               char* buf, const uint64_t bufSize, const bool extension)
    {
        if(!dbg || !buf || bufSize == 0)
            return false;

        std::string base;
        if(!dbg->ModName(addr, base, extension))
            return false;
        if(base.size() + 1 > bufSize)
            return false;

        memcpy(buf, base.c_str(), base.size() + 1);
        return true;
    }

    bool ElfBugSetBreakpoint(ElfBugDebugger* dbg, uint64_t addr)
    {
        if(!dbg)
            return false;
        return dbg->QueueBreakpoint(addr, ElfBugDebugger::BreakpointAction::Set);
    }

    bool ElfBugDeleteBreakpoint(ElfBugDebugger* dbg, const uint64_t addr)
    {
        if(!dbg)
            return false;
        return dbg->QueueBreakpoint(addr, ElfBugDebugger::BreakpointAction::Delete);
    }

    bool ElfBugIsBreakpointEffective(const ElfBugDebugger* dbg, const uint64_t addr)
    {
        if(!dbg)
            return false;
        return dbg->IsBreakpointEffective(addr);
    }

} // extern "C"
