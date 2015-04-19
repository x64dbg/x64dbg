#include "functionanalysis.h"
#include "console.h"
#include "memory.h"
#include "function.h"

FunctionAnalysis::FunctionAnalysis(uint base, uint size)
{
    _base = base;
    _size = size;
    _data = new unsigned char[_size + MAX_DISASM_BUFFER];
    MemRead((void*)_base, _data, _size, 0);
}

FunctionAnalysis::~FunctionAnalysis()
{
    delete[] _data;
}

const unsigned char* FunctionAnalysis::TranslateAddress(uint addr)
{
    return (addr >= _base && addr < _base + _size) ? _data + (addr - _base) : nullptr;
}

void FunctionAnalysis::Analyse()
{
    dputs("analysis started...");
    DWORD ticks = GetTickCount();

    PopulateReferences();
    dprintf("%u called functions populated\n", _functions.size());
    AnalyseFunctions();

    dprintf("analysis finished in %ums!\n", GetTickCount() - ticks);
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

        //dprintf("analysing function %p-??? maxaddr: %p\n", function.start, maxaddr);
        uint end = FindFunctionEnd(function.start, maxaddr);
        if(end)
            function.end = end;
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
    for(uint addr = start, newaddr = 0; addr < maxaddr;)
    {
        if(_cp.Disassemble(addr, TranslateAddress(addr), MAX_DISASM_BUFFER))
        {
            if(addr + _cp.Size() > maxaddr)  //we went past the maximum allowed address
                break;

            //dprintf("    %p: %s %s\n", addr, _cp.GetInstr()->mnemonic, _cp.GetInstr()->op_str);
            const cs_x86_op & operand = _cp.x86().operands[0];
            if(_cp.InGroup(CS_GRP_JUMP) && operand.type == X86_OP_IMM)  //jump
            {
                uint dest = (uint)operand.imm;

                if(dest >= maxaddr)  //jump across function boundaries (TODO: something with conditional jumps)
                {
                    //add destination to function buffer?
                    //end = addr;
                    //break;
                }
                else if(dest > addr && dest > newaddr)  //save the farthest jump
                {
                    newaddr = dest;
                }
                else if(end && dest < end)  //jump back to ret
                {
                    jumpback = addr;
                }
            }
            else if(_cp.InGroup(CS_GRP_RET))  //function end
            {
                end = addr;
                if(newaddr >= addr)
                {
                    addr = newaddr;
                    newaddr = 0;
                    continue;
                }
                else
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
        if(_cp.InGroup(CS_GRP_JUMP))  //skip jumps
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