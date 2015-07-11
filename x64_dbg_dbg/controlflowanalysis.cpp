#include "controlflowanalysis.h"
#include "memory.h"
#include "console.h"

ControlFlowAnalysis::ControlFlowAnalysis(uint base, uint size) : Analysis(base, size)
{
}

void ControlFlowAnalysis::Analyse()
{
    dputs("Starting analysis...");
    DWORD ticks = GetTickCount();

    BasicBlockStarts();
    BasicBlocks();

    dprintf("Analysis finished in %ums!\n", GetTickCount() - ticks);
}

void ControlFlowAnalysis::SetMarkers()
{
    dprintf("digraph ControlFlow {\n");
    int i = 0;
    std::map<uint, int> nodeMap;
    for(auto block : _blocks)
    {
        nodeMap.insert({ block.start, i });
        dprintf("    node%u [label=\"start=%p, end=%p\"];\n", i, block.start, block.end);
        i++;
    }
    for(auto block : _blocks)
    {
        int startNode = nodeMap[block.start];
        if(block.left)
        {
            if(nodeMap.count(block.left))
                dprintf("    node%u -> node%u;\n", startNode, nodeMap[block.left]);
        }
        else
        {
            dprintf("    node%u [shape=point];\n", i);
            dprintf("    node%u -> node%u;\n", startNode, i);
            i++;
        }
        if(block.right)
        {
            if(nodeMap.count(block.right))
                dprintf("    node%u -> node%u;\n", startNode, nodeMap[block.right]);
        }
        else
        {
            dprintf("    node%u [shape=point];\n", i);
            dprintf("    node%u -> node%u;\n", startNode, i);
            i++;
        }
    }
    dprintf("}\n");
}

void ControlFlowAnalysis::BasicBlockStarts()
{
    _blockStarts.insert(_base);
    bool bSkipFilling = false;
    for(uint i = 0; i < _size;)
    {
        uint addr = _base + i;
        if(_cp.Disassemble(addr, TranslateAddress(addr), MAX_DISASM_BUFFER))
        {
            if(bSkipFilling)  //handle filling skip mode
            {
                if(!_cp.IsFilling())  //do nothing until the filling stopped
                {
                    bSkipFilling = false;
                    _blockStarts.insert(addr);
                }
            }
            else if(_cp.InGroup(CS_GRP_RET) || _cp.InGroup(CS_GRP_INT)) //RET/INT break control flow
            {
                bSkipFilling = true; //skip INT3/NOP/whatever filling bytes (those are not part of the control flow)
            }
            else if(_cp.InGroup(CS_GRP_JUMP) || _cp.IsLoop())  //branches
            {
                uint dest1 = GetBranchOperand();
                uint dest2 = 0;
                if(_cp.GetId() != X86_INS_JMP)   //unconditional jump
                    dest2 = addr + _cp.Size();

                if(!dest1 && !dest2) //TODO: better code for this (make sure absolutely no filling is inserted)
                    bSkipFilling = true;
                if(dest1)
                    _blockStarts.insert(dest1);
                if(dest2)
                    _blockStarts.insert(dest2);
            }
            else if(_cp.InGroup(CS_GRP_CALL))
            {
                uint dest1 = GetBranchOperand();
                if(dest1)
                    _blockStarts.insert(dest1);
            }
            else
            {
                uint dest1 = GetBranchOperand();
                if(dest1)
                    _blockStarts.insert(dest1);
            }
            i += _cp.Size();
        }
        else
            i++;
    }
}

void ControlFlowAnalysis::BasicBlocks()
{
    for(auto i = _blockStarts.begin(); i != _blockStarts.end(); ++i)
    {
        uint start = *i;
        if(!IsValidAddress(start))
            continue;
        uint nextStart = _base + _size;
        auto next = std::next(i);
        if(next != _blockStarts.end())
            nextStart = *next;
        for(uint addr = start, prevaddr = 0; addr < _base + _size;)
        {
            prevaddr = addr;
            if(_cp.Disassemble(addr, TranslateAddress(addr), MAX_DISASM_BUFFER))
            {
                if(_cp.InGroup(CS_GRP_RET))
                {
                    _blocks.push_back(BasicBlock(start, addr, 0, 0)); //leaf block
                    break;
                }
                else if(_cp.InGroup(CS_GRP_JUMP) || _cp.IsLoop())
                {
                    uint dest1 = GetBranchOperand();
                    uint dest2 = _cp.GetId() != X86_INS_JMP ? addr + _cp.Size() : 0;
                    _blocks.push_back(BasicBlock(start, addr, dest1, dest2));
                    break;
                }
                addr += _cp.Size();
            }
            else
                addr++;
            if(addr == nextStart)  //special case handling overlapping blocks
            {
                _blocks.push_back(BasicBlock(start, prevaddr, 0, nextStart));
                break;
            }
        }
    }
}

uint ControlFlowAnalysis::GetBranchOperand()
{
    for(int i = 0; i < _cp.x86().op_count; i++)
    {
        const cs_x86_op & operand = _cp.x86().operands[i];
        if(operand.type == X86_OP_IMM)
        {
            uint dest = (uint)operand.imm;
            if(dest >= _base && dest < _base + _size)
                return dest;
        }
    }
    return 0;
}