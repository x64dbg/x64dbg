#include "controlflowanalysis.h"
#include "console.h"

ControlFlowAnalysis::ControlFlowAnalysis(uint base, uint size) : Analysis(base, size)
{
}

void ControlFlowAnalysis::Analyse()
{
    dputs("Starting analysis...");
    DWORD ticks = GetTickCount();

    BasicBlockStarts();
    dprintf("Basic block starts in %ums!\n", GetTickCount() - ticks);
    ticks = GetTickCount();

    BasicBlocks();
    dprintf("Basick blocks in %ums!\n", GetTickCount() - ticks);
    ticks = GetTickCount();

    Functions();
    dprintf("Functions in %ums!\n", GetTickCount() - ticks);

    dprintf("Analysis finished!\n");
}

void ControlFlowAnalysis::SetMarkers()
{
    dprintf("digraph ControlFlow {\n");
    int i = 0;
    std::map<uint, int> nodeMap;
    for(const auto & it : _blocks)
    {
        const auto & block = it.second;
        nodeMap.insert({ block.start, i });
        dprintf("    node%u [label=\"s=%p, e=%p, f=%p\"];\n", i, block.start, block.end, block.function);
        i++;
    }
    for(auto it : _blocks)
    {
        const auto & block = it.second;
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
            if(bSkipFilling)   //handle filling skip mode
            {
                if(!_cp.IsFilling())   //do nothing until the filling stopped
                {
                    bSkipFilling = false;
                    _blockStarts.insert(addr);
                }
            }
            else if(_cp.InGroup(CS_GRP_RET) || _cp.InGroup(CS_GRP_INT))  //RET/INT break control flow
            {
                bSkipFilling = true; //skip INT3/NOP/whatever filling bytes (those are not part of the control flow)
            }
            else if(_cp.InGroup(CS_GRP_JUMP) || _cp.IsLoop())   //branches
            {
                uint dest1 = GetBranchOperand();
                uint dest2 = 0;
                if(_cp.GetId() != X86_INS_JMP)    //unconditional jump
                    dest2 = addr + _cp.Size();

                if(!dest1 && !dest2)  //TODO: better code for this (make sure absolutely no filling is inserted)
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
                    insertBlock(BasicBlock(start, addr, 0, 0)); //leaf block
                    break;
                }
                else if(_cp.InGroup(CS_GRP_JUMP) || _cp.IsLoop())
                {
                    uint dest1 = GetBranchOperand();
                    uint dest2 = _cp.GetId() != X86_INS_JMP ? addr + _cp.Size() : 0;
                    insertBlock(BasicBlock(start, addr, dest1, dest2));
                    insertParent(dest1, start);
                    insertParent(dest2, start);
                    break;
                }
                addr += _cp.Size();
            }
            else
                addr++;
            if(addr == nextStart)   //special case handling overlapping blocks
            {
                insertBlock(BasicBlock(start, prevaddr, 0, nextStart));
                insertParent(nextStart, start);
                break;
            }
        }
    }
}

void ControlFlowAnalysis::Functions()
{
    typedef std::pair<BasicBlock*, UintSet*> DelayedBlock;
    std::vector<DelayedBlock> delayedBlocks;
    for(auto & it : _blocks)
    {
        BasicBlock* block = &it.second;
        UintSet* parents = findParents(block->start);
        if(!block->function)
        {
            if(!parents)  //no parents = function start
            {
                uint functionStart = block->start;
                block->function = functionStart;
                UintSet functionBlocks;
                functionBlocks.insert(functionStart);
                _functions[functionStart] = functionBlocks;
            }
            else //in function
            {
                uint function = findFunctionStart(block, parents);
                if(!function)  //this happens with loops sometimes
                    delayedBlocks.push_back(DelayedBlock(block, parents));
                else
                    block->function = function;
            }
        }
        else
            DebugBreak(); //this should not happen
    }
    dprintf("%u/%u delayed blocks...\n", delayedBlocks.size(), _blocks.size());
    for(auto & delayedBlock : delayedBlocks)
    {
        BasicBlock* block = delayedBlock.first;
        UintSet* parents = delayedBlock.second;
        uint function = findFunctionStart(block, parents);
        if(!function)
        {
            dprintf("unresolved block %s\n", blockToString(block).c_str());
            if(parents)
            {
                dprintf("parents:\n");
                for(auto parent : *parents)
                    dprintf("  %s\n", blockToString(findBlock(parent)).c_str());
            }
            else
                dprintf("parents: null");
            dprintf("left: %s\n", blockToString(findBlock(block->left)).c_str());
            dprintf("right: %s\n", blockToString(findBlock(block->right)).c_str());
            return;
        }
        block->function = function;
    }
}

void ControlFlowAnalysis::insertBlock(BasicBlock block)
{
    if(_blocks.find(block.start) != _blocks.end())
        DebugBreak();
    _blocks[block.start] = block;
}

ControlFlowAnalysis::BasicBlock* ControlFlowAnalysis::findBlock(uint start)
{
    if(!start)
        return nullptr;
    auto found = _blocks.find(start);
    return found != _blocks.end() ? &found->second : nullptr;
}

void ControlFlowAnalysis::insertParent(uint child, uint parent)
{
    if(!child || !parent)
        return;
    auto found = _parentMap.find(child);
    if(found == _parentMap.end())
    {
        UintSet parents;
        parents.insert(parent);
        _parentMap[child] = parents;
    }
    else
        found->second.insert(parent);
}

ControlFlowAnalysis::UintSet* ControlFlowAnalysis::findParents(uint child)
{
    if(!child)
        return nullptr;
    auto found = _parentMap.find(child);
    return found != _parentMap.end() ? &found->second : nullptr;
}

uint ControlFlowAnalysis::findFunctionStart(BasicBlock* block, ControlFlowAnalysis::UintSet* parents)
{
    if(!block)
        return 0;
    if(block->function)
        return block->function;
    BasicBlock* left = findBlock(block->left);
    if(left && left->function)
        return left->function;
    BasicBlock* right = findBlock(block->right);
    if(right && right->function)
        return right->function;
    for(auto start : *parents)
    {
        BasicBlock* parent = findBlock(start);
        if(parent->function)
            return parent->function;
    }
    return 0;
}

String ControlFlowAnalysis::blockToString(BasicBlock* block)
{
    if(!block)
        return String("null");
    return block->toString();
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