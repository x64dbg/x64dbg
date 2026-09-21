#include <ElfBug/core/Debugger.h>
#include <ElfBug/process/StepOver.h>
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
            mProcess->DeleteBreakpoint(mStepOver.target);
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
            // Plain step: lift the breakpoint under RIP, restored when the step traps.
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
                snprintf(message, sizeof(message), "step-over: failed to set breakpoint at 0x%llx",
                         static_cast<unsigned long long>(target));
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
                // A single step runs one iteration and leaves RIP on the instruction, so
                // the byte stays lifted until the whole loop is done.
                if(mProcess->DisarmBreakpointByte(rip))
                    mSourceRearms[tid] = rip;
            }
            // Step off the call now so its breakpoint is armed again while the callee
            // runs, for this thread's deeper frames and for every other thread.
            else
            {
                switch(stepPastBreakpointByte(tid, rip))
                {
                case StepOff::Stepped:
                    break;
                case StepOff::Parked:
                    cancelStepOver(tid);
                    return StepOverArm::Parked;
                case StepOff::Consumed:
                    cancelStepOver(tid);
                    return StepOverArm::Consumed;
                }
            }
        }

        return StepOverArm::Armed;
    }
}
