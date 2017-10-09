#include "linearanalysis.h"
#include "console.h"
#include "memory.h"
#include "function.h"
#include <algorithm>

LinearAnalysis::LinearAnalysis(duint base, duint size) : Analysis(base, size)
{
}

void LinearAnalysis::Analyse()
{
    dputs("Starting analysis...");
    auto ticks = GetTickCount();

    populateReferences();
    dprintf("%u called functions populated\n", DWORD(mFunctions.size()));
    analyseFunctions();

    dprintf("Analysis finished in %ums!\n", GetTickCount() - ticks);
}

void LinearAnalysis::SetMarkers()
{
    FunctionDelRange(mBase, mBase + mSize - 1, false);
    for(auto & function : mFunctions)
    {
        if(!function.end)
            continue;
        FunctionAdd(function.start, function.end, false);
    }
}

void LinearAnalysis::sortCleanup()
{
    //sort & remove duplicates
    std::sort(mFunctions.begin(), mFunctions.end());
    auto last = std::unique(mFunctions.begin(), mFunctions.end());
    mFunctions.erase(last, mFunctions.end());
}

void LinearAnalysis::populateReferences()
{
    //linear immediate reference scan (call <addr>, push <addr>, mov [somewhere], <addr>)
    for(duint i = 0; i < mSize;)
    {
        auto addr = mBase + i;
        if(mCp.Disassemble(addr, translateAddr(addr), MAX_DISASM_BUFFER))
        {
            auto ref = getReferenceOperand();
            if(ref)
                mFunctions.push_back({ ref, 0 });
            i += mCp.Size();
        }
        else
            i++;
    }
    sortCleanup();
}

void LinearAnalysis::analyseFunctions()
{
    for(size_t i = 0; i < mFunctions.size(); i++)
    {
        auto & function = mFunctions[i];
        if(function.end) //skip already-analysed functions
            continue;
        auto maxaddr = mBase + mSize;
        if(i < mFunctions.size() - 1)
            maxaddr = mFunctions[i + 1].start;

        auto end = findFunctionEnd(function.start, maxaddr);
        if(end)
        {
            if(mCp.Disassemble(end, translateAddr(end), MAX_DISASM_BUFFER))
                function.end = end + mCp.Size() - 1;
            else
                function.end = end;
        }
    }
}

duint LinearAnalysis::findFunctionEnd(duint start, duint maxaddr)
{
    //disassemble first instruction for some heuristics
    if(mCp.Disassemble(start, translateAddr(start), MAX_DISASM_BUFFER))
    {
        //JMP [123456] ; import
        if(mCp.IsJump() && mCp.OpCount() && mCp[0].type == ZYDIS_OPERAND_TYPE_MEMORY)
            return 0;
    }

    //linear search with some trickery
    duint end = 0;
    duint jumpback = 0;
    for(duint addr = start, fardest = 0; addr < maxaddr;)
    {
        if(mCp.Disassemble(addr, translateAddr(addr), MAX_DISASM_BUFFER))
        {
            if(addr + mCp.Size() > maxaddr) //we went past the maximum allowed address
                break;

            if((mCp.IsJump() || mCp.IsLoop()) && mCp.OpCount() && mCp[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE) //jump
            {
                auto dest = duint(mCp[0].imm.value.u);

                if(dest >= maxaddr) //jump across function boundaries
                {
                    //currently unused
                }
                else if(dest > addr && dest > fardest) //save the farthest JXX destination forward
                {
                    fardest = dest;
                }
                else if(end && dest < end && (mCp.GetId() == ZYDIS_MNEMONIC_JMP || mCp.GetId() == ZYDIS_MNEMONIC_LOOP)) //save the last JMP backwards
                {
                    jumpback = addr;
                }
            }
            else if(mCp.IsRet()) //possible function end?
            {
                end = addr;
                if(fardest < addr) //we stop if the farthest JXX destination forward is before this RET
                    break;
            }

            addr += mCp.Size();
        }
        else
            addr++;
    }
    return end < jumpback ? jumpback : end;
}

duint LinearAnalysis::getReferenceOperand() const
{
    for(auto i = 0; i < mCp.OpCount(); i++)
    {
        const auto & op = mCp[i];
        if(mCp.IsJump() || mCp.IsLoop()) //skip jumps/loops
            continue;
        if(op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE) //we are looking for immediate references
        {
            auto dest = duint(op.imm.value.u);
            if(inRange(dest))
                return dest;
        }
    }
    return 0;
}