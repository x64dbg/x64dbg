#include <ElfBug/core/Debugger.h>
#include <ElfBug/process/StepOver.h>
#include <cinttypes>
#include <cstring>

namespace ElfBug
{
    void Debugger::maskPushedTrapFlag()
    {
        if(!mThread || !mProcess)
            return;

        // TF is bit 8 of EFLAGS: bit 0 of the second pushed byte for pushf and pushfq alike.
        const ptr flagsHigh = mThread->registers.Gsp() + 1;
        uint8 byte = 0;
        if(!mProcess->MemRead(flagsHigh, &byte, 1))
            return;
        byte &= static_cast<uint8>(~0x01);
        mProcess->MemWrite(flagsHigh, &byte, 1);
    }

    // Only the lifting thread may re-arm; anyone else would re-trap it in place.
    void Debugger::restoreSourceByte(const pid_t tid)
    {
        const auto it = mSourceRearms.find(tid);
        if(it == mSourceRearms.end())
            return;

        if(mProcess)
            mProcess->RearmBreakpointByte(it->second);
        mSourceRearms.erase(it);
    }

    void Debugger::cancelStepOver(const pid_t tid)
    {
        restoreSourceByte(tid);

        if(!mStepOver.active)
            return;
        if(mStepOver.planted && mProcess)
            deleteBreakpoint(mStepOver.target);
        mStepOver = {};
    }

    void Debugger::cancelStepOverIfOwner(const pid_t tid)
    {
        if(mStepOver.active && mStepOver.tid != tid)
        {
            restoreSourceByte(tid);
            return;
        }
        cancelStepOver(tid);
    }

    Debugger::StepOverArm Debugger::armStepOver(const pid_t tid)
    {
        cancelStepOver(tid);

        if(!mThread || !mProcess)
            return StepOverArm::SingleStep;

        const ptr rip = mThread->registers.Gip();

        ptr target = 0;
        const StepOverKind kind = mProcess->ClassifyStepOverAt(rip, target);
        if(kind == StepOverKind::None || kind == StepOverKind::Pushf)
        {
            if(mProcess->DisarmBreakpointByte(rip))
                mSourceRearms[tid] = rip;
            return StepOverArm::SingleStep;
        }

        bool planted = false;
        if(!mProcess->HasBreakpoint(target))
        {
            if(!mProcess->SetBreakpoint(target, false, SoftwareType::ShortInt3))
            {
                char message[64];
                snprintf(message, sizeof(message), "step-over: failed to set breakpoint at 0x%" PRIx64, target);
                cbInternalError(message);
                if(mProcess->DisarmBreakpointByte(rip))
                    mSourceRearms[tid] = rip;
                return StepOverArm::SingleStep;
            }
            planted = true;
        }

        mStepOver.active = true;
        mStepOver.target = target;
        mStepOver.tid = tid;
        mStepOver.rspFloor = mThread->registers.Gsp();
        mStepOver.planted = planted;

        if(mProcess->HasBreakpoint(rip))
        {
            if(kind == StepOverKind::Rep)
            {
                if(mProcess->DisarmBreakpointByte(rip))
                    mSourceRearms[tid] = rip;
            }
            else
            {
                switch(stepOffAlone(tid, rip))
                {
                case StepOff::Stepped:
                    break;
                case StepOff::Parked:
                    cancelStepOver(tid);
                    return StepOverArm::Parked;
                case StepOff::Consumed:
                    cancelStepOver(tid);
                    return StepOverArm::Consumed;
                case StepOff::Deferred:
                    cancelStepOver(tid);
                    if(const auto silent = mSilentStepOffs.find(tid); silent != mSilentStepOffs.end())
                        silent->second.stepOver = true;
                    return StepOverArm::Consumed;
                }
            }
        }

        return StepOverArm::Armed;
    }

    void Debugger::cancelStepResume()
    {
        if(!mStepResume.active)
            return;
        if(mStepResume.planted && mProcess)
            deleteBreakpoint(mStepResume.address);
        mStepResume = {};
        mStepPending.store(false, std::memory_order_release);
    }

    bool Debugger::armStepResume(const pid_t tid, const int signal, const siginfo_t* info)
    {
        cancelStepResume();

        if(!mThread || !mProcess || !mThread->registers.Read())
            return false;

        const ptr rip = mThread->registers.Gip();
        const bool planted = !mProcess->HasBreakpoint(rip);
        if(planted && !mProcess->SetBreakpoint(rip, false, SoftwareType::ShortInt3))
        {
            char message[64];
            snprintf(message, sizeof(message), "step: failed to set breakpoint at 0x%" PRIx64, rip);
            cbInternalError(message);
            return false;
        }

        bool resumed = false;
        {
            std::unique_lock lock(mProcessMutex);
            if(!mThread->IsSuspended())
            {
                mThread->ClearPendingBreakpoint();
                restoreSignalInfo(tid, signal, info);
                if(ptrace(PTRACE_CONT, tid, nullptr, PtraceData(signal)) != -1)
                {
                    mThread->SetRunning(true);
                    resumed = true;
                }
            }
        }
        if(!resumed)
        {
            if(planted)
                deleteBreakpoint(rip);
            return false;
        }

        takeSilentStepOff(tid, rip, mThread->registers.Gsp());
        mStepResume.active = true;
        mStepResume.tid = tid;
        mStepResume.address = rip;
        mStepResume.rsp = mThread->registers.Gsp();
        mStepResume.planted = planted;
        return true;
    }

    // sigreturn restores RSP exactly; a handler on an alternate stack can run above it, so no floor test.
    bool Debugger::takeStepResumeTrap(const pid_t tid, const ptr address)
    {
        if(!mStepResume.active || address != mStepResume.address || !mThread || !mProcess)
            return false;

        const bool arrived = tid == mStepResume.tid && mThread->registers.Gsp() == mStepResume.rsp;
        if(!arrived && !mStepResume.planted)
            return false;

        mThread->registers.Gip() = address;
        mThread->registers.Write();

        if(!arrived)
        {
            if(!stepOffAndRun(tid, address))
                abandonAllStop(tid);
            return true;
        }

        const bool planted = mStepResume.planted;
        cancelStepResume();
        mThread->SetAtBreakpoint(!planted);
        mStepPending.store(true, std::memory_order_release);
        pauseAndResume(tid);
        return true;
    }

    std::optional<Debugger::SilentStepOff> Debugger::takeSilentStepOff(const pid_t tid, const ptr address, const ptr rsp)
    {
        const auto it = mSilentStepOffs.find(tid);
        if(it == mSilentStepOffs.end() || it->second.address != address || it->second.rsp != rsp)
            return std::nullopt;
        const SilentStepOff taken = it->second;
        mSilentStepOffs.erase(it);
        return taken;
    }
}
