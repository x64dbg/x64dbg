#include "functionanalysis.h"
#include "console.h"
#include "memory.h"
#include "function.h"

#include "AnalysisPass.h"
#include "BasicBlock.h"
#include "FunctionPass.h"
#include "LinearPass.h"

FunctionAnalysis::FunctionAnalysis(uint base, uint size) : Analysis(base, size)
{
}

void FunctionAnalysis::Analyse()
{
    dputs("Starting analysis...");
    DWORD ticks = GetTickCount();

    BBlockArray blocks;
    LinearPass* pass1 = new LinearPass(_base, _base + _size, blocks);
    pass1->Analyse();

    FunctionPass* pass2 = new FunctionPass(_base, _base + _size, blocks);
    pass2->Analyse();
    /*
    PopulateReferences();
    dprintf("%u called functions populated\n", _functions.size());
    AnalyseFunctions();
    */
    dprintf("Analysis finished in %ums!\n", GetTickCount() - ticks);
}

void FunctionAnalysis::SetMarkers()
{
    FunctionDelRange(_base, _base + _size);
    for(auto & function : _functions)
    {
        if(!function.end)
            continue;
        FunctionAdd(function.start, function.end, false);
    }
}

void FunctionAnalysis::SortCleanup()
{
    //sort & remove duplicates
    std::sort(_functions.begin(), _functions.end());
    auto last = std::unique(_functions.begin(), _functions.end());
    _functions.erase(last, _functions.end());
}

void FunctionAnalysis::PopulateReferences()
{
    //linear immediate reference scan (call <addr>, push <addr>, mov [somewhere], <addr>)
    for(uint i = 0; i < _size;)
    {
        uint addr = _base + i;
        if(_cp.Disassemble(addr, TranslateAddress(addr), MAX_DISASM_BUFFER))
        {
            uint ref = GetReferenceOperand();
            if(ref)
                _functions.push_back({ ref, 0 });
            i += _cp.Size();
        }
        else
            i++;
    }
    SortCleanup();
}

void FunctionAnalysis::AnalyseFunctions()
{
    for(size_t i = 0; i < _functions.size(); i++)
    {
        FunctionInfo & function = _functions[i];
        if(function.end)  //skip already-analysed functions
            continue;
        uint maxaddr = _base + _size;
        if(i < _functions.size() - 1)
            maxaddr = _functions[i + 1].start;

        uint end = FindFunctionEnd(function.start, maxaddr);
        if(end)
        {
            _cp.Disassemble(end, TranslateAddress(end), MAX_DISASM_BUFFER);
            function.end = end + _cp.Size() - 1;
        }
    }
}

uint FunctionAnalysis::FindFunctionEnd(uint start, uint maxaddr)
{
    //disassemble first instruction for some heuristics
    if(_cp.Disassemble(start, TranslateAddress(start), MAX_DISASM_BUFFER))
    {
        //JMP [123456] ; import
        if(_cp.InGroup(CS_GRP_JUMP) && _cp.x86().operands[0].type == X86_OP_MEM)
            return 0;
    }

    //linear search with some trickery
    uint end = 0;
    uint jumpback = 0;
    for(uint addr = start, fardest = 0; addr < maxaddr;)
    {
        if(_cp.Disassemble(addr, TranslateAddress(addr), MAX_DISASM_BUFFER))
        {
            if(addr + _cp.Size() > maxaddr)  //we went past the maximum allowed address
                break;

            const cs_x86_op & operand = _cp.x86().operands[0];
            if((_cp.InGroup(CS_GRP_JUMP) || _cp.IsLoop()) && operand.type == X86_OP_IMM)   //jump
            {
                uint dest = (uint)operand.imm;

                if(dest >= maxaddr)   //jump across function boundaries
                {
                    //currently unused
                }
                else if(dest > addr && dest > fardest)   //save the farthest JXX destination forward
                {
                    fardest = dest;
                }
                else if(end && dest < end && (_cp.GetId() == X86_INS_JMP || _cp.GetId() == X86_INS_LOOP)) //save the last JMP backwards
                {
                    jumpback = addr;
                }
            }
            else if(_cp.InGroup(CS_GRP_RET))   //possible function end?
            {
                end = addr;
                if(fardest < addr)  //we stop if the farthest JXX destination forward is before this RET
                    break;
            }

            addr += _cp.Size();
        }
        else
            addr++;
    }
    return end < jumpback ? jumpback : end;
}

uint FunctionAnalysis::GetReferenceOperand()
{
    for(int i = 0; i < _cp.x86().op_count; i++)
    {
        const cs_x86_op & operand = _cp.x86().operands[i];
        if(_cp.InGroup(CS_GRP_JUMP) || _cp.IsLoop())  //skip jumps/loops
            continue;
        if(operand.type == X86_OP_IMM)  //we are looking for immediate references
        {
            uint dest = (uint)operand.imm;
            if(dest >= _base && dest < _base + _size)
                return dest;
        }
    }
    return 0;
}