#include "controlflowanalysis.h"
#include "console.h"
#include "module.h"
#include "TitanEngine/TitanEngine.h"
#include "memory.h"
#include "function.h"

ControlFlowAnalysis::ControlFlowAnalysis(duint base, duint size, bool exceptionDirectory) : Analysis(base, size)
{
    _functionInfoData = nullptr;
#ifdef _WIN64
    // This will only be valid if the address range is within a loaded module
    _moduleBase = ModBaseFromAddr(base);

    if(exceptionDirectory && _moduleBase != 0)
    {
        char modulePath[MAX_PATH];
        memset(modulePath, 0, sizeof(modulePath));

        ModPathFromAddr(_moduleBase, modulePath, ARRAYSIZE(modulePath));

        HANDLE fileHandle;
        DWORD fileSize;
        HANDLE fileMapHandle;
        ULONG_PTR fileMapVa;
        if(StaticFileLoadW(
                    StringUtils::Utf8ToUtf16(modulePath).c_str(),
                    UE_ACCESS_READ,
                    false,
                    &fileHandle,
                    &fileSize,
                    &fileMapHandle,
                    &fileMapVa))
        {
            // Find a pointer to IMAGE_DIRECTORY_ENTRY_EXCEPTION for later use
            ULONG_PTR virtualOffset = GetPE32DataFromMappedFile(fileMapVa, IMAGE_DIRECTORY_ENTRY_EXCEPTION, UE_SECTIONVIRTUALOFFSET);
            _functionInfoSize = (duint)GetPE32DataFromMappedFile(fileMapVa, IMAGE_DIRECTORY_ENTRY_EXCEPTION, UE_SECTIONVIRTUALSIZE);

            // Unload the file
            StaticFileUnloadW(nullptr, false, fileHandle, fileSize, fileMapHandle, fileMapVa);

            // Get a copy of the function table
            if(virtualOffset)
            {
                // Read the table into a buffer
                _functionInfoData = emalloc(_functionInfoSize);

                if(_functionInfoData)
                    MemRead(virtualOffset + _moduleBase, _functionInfoData, _functionInfoSize);
            }
        }
    }
#endif //_WIN64
}

ControlFlowAnalysis::~ControlFlowAnalysis()
{
    if(_functionInfoData)
        efree(_functionInfoData);
}

void ControlFlowAnalysis::Analyse()
{
    dputs("Starting analysis...");
    DWORD ticks = GetTickCount();

    BasicBlockStarts();
    dprintf("Basic block starts in %ums!\n", GetTickCount() - ticks);
    ticks = GetTickCount();

    BasicBlocks();
    dprintf("Basic blocks in %ums!\n", GetTickCount() - ticks);
    ticks = GetTickCount();

    Functions();
    dprintf("Functions in %ums!\n", GetTickCount() - ticks);
    ticks = GetTickCount();

    FunctionRanges();
    dprintf("Function ranges in %ums!\n", GetTickCount() - ticks);
    ticks = GetTickCount();

    dprintf("Analysis finished!\n");

    // TODO : Fix this
    PARENTMAP *_parentsTemp = new PARENTMAP(_parentMap);
    BASICBLOCKMAP *_blocksTemp = new BASICBLOCKMAP(_blocks);

    CONTROLFLOWINFOS *ctrlFlow = new CONTROLFLOWINFOS;
    ctrlFlow->blocks = (duint*) _blocksTemp;
    ctrlFlow->parents = (duint*) _parentsTemp;

    GuiSetControlFlowInfos(ctrlFlow);
}

void ControlFlowAnalysis::SetMarkers()
{
    FunctionDelRange(_base, _base + _size);
    auto size = _functionRanges.size();
    for(size_t i = size - 1; i != -1; i--)
    {
        const auto & range = _functionRanges[i];
        FunctionAdd(range.first, range.second, false);
    }
    /*dprintf("digraph ControlFlow {\n");
    int i = 0;
    std::map<duint, int> nodeMap;
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
    dprintf("}\n");*/
}

void ControlFlowAnalysis::BasicBlockStarts()
{
    //_blockStarts.insert(_base);
    bool bSkipFilling = false;
    for(duint i = 0; i < _size;)
    {
        duint addr = _base + i;
        if(_cp.Disassemble(addr, TranslateAddress(addr), MAX_DISASM_BUFFER))
        {
            if(bSkipFilling) //handle filling skip mode
            {
                if(!_cp.IsFilling()) //do nothing until the filling stopped
                {
                    bSkipFilling = false;
                    _blockStarts.insert(addr);
                }
            }
            else if(_cp.InGroup(CS_GRP_RET) || _cp.GetId() == X86_INS_INT3) //RET/INT3 break control flow
            {
                bSkipFilling = true; //skip INT3/NOP/whatever filling bytes (those are not part of the control flow)
            }
            else if(_cp.InGroup(CS_GRP_JUMP) || _cp.IsLoop())   //branches
            {
                duint dest1 = GetReferenceOperand();
                duint dest2 = 0;
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
                duint dest1 = GetReferenceOperand();
                if(dest1)
                {
                    _blockStarts.insert(dest1);
                    _functionStarts.insert(dest1);
                }
            }
            else
            {
                duint dest1 = GetReferenceOperand();
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
        duint start = *i;
        if(!IsValidAddress(start))
            continue;
        duint nextStart = _base + _size;
        auto next = std::next(i);
        if(next != _blockStarts.end())
            nextStart = *next;
        for(duint addr = start, prevaddr = 0; addr < _base + _size;)
        {
            prevaddr = addr;
            if(_cp.Disassemble(addr, TranslateAddress(addr), MAX_DISASM_BUFFER))
            {
                if(_cp.InGroup(CS_GRP_RET) || _cp.GetId() == X86_INS_INT3)
                {
                    insertBlock(BasicBlock(start, addr, 0, 0)); //leaf block
                    break;
                }
                else if(_cp.InGroup(CS_GRP_JUMP) || _cp.IsLoop())
                {
                    duint dest1 = GetReferenceOperand();
                    duint dest2 = _cp.GetId() != X86_INS_JMP ? addr + _cp.Size() : 0;
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
    _blockStarts.clear();

#ifdef _WIN64
    int count = 0;
    EnumerateFunctionRuntimeEntries64([&](PRUNTIME_FUNCTION Function)
    {
        const duint funcAddr = _moduleBase + Function->BeginAddress;
        const duint funcEnd = _moduleBase + Function->EndAddress;

        // If within limits...
        if(funcAddr >= _base && funcAddr < _base + _size)
            _functionStarts.insert(funcAddr);
        count++;
        return true;
    });
    dprintf("%u functions from the exception directory...\n", count);
#endif // _WIN64

    dprintf("%u basic blocks, %u function starts detected...\n", _blocks.size(), _functionStarts.size());
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
            if(!parents || _functionStarts.count(block->start))  //no parents = function start
            {
                duint functionStart = block->start;
                block->function = functionStart;
                UintSet functionBlocks;
                functionBlocks.insert(functionStart);
                _functions[functionStart] = functionBlocks;
            }
            else //in function
            {
                duint function = findFunctionStart(block, parents);
                if(!function)  //this happens with loops / unreferenced blocks sometimes
                    delayedBlocks.push_back(DelayedBlock(block, parents));
                else
                    block->function = function;
            }
        }
        else
            DebugBreak(); //this should not happen
    }
    int delayedCount = (int)delayedBlocks.size();
    dprintf("%u/%u delayed blocks...\n", delayedCount, _blocks.size());
    int resolved = 0;
    for(auto & delayedBlock : delayedBlocks)
    {
        BasicBlock* block = delayedBlock.first;
        UintSet* parents = delayedBlock.second;
        duint function = findFunctionStart(block, parents);
        if(!function)
        {
            continue;
            /*dprintf("unresolved block %s\n", blockToString(block).c_str());
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
            return;*/
        }
        block->function = function;
        resolved++;
    }
    dprintf("%u/%u delayed blocks resolved (%u/%u still left, probably unreferenced functions)\n", resolved, delayedCount, delayedCount - resolved, _blocks.size());
    int unreferencedCount = 0;
    for(const auto & block : _blocks)
    {
        auto found = _functions.find(block.second.function);
        if(found == _functions.end())  //unreferenced block
        {
            unreferencedCount++;
            continue;
        }
        found->second.insert(block.second.start);
    }
    dprintf("%u/%u unreferenced blocks\n", unreferencedCount, _blocks.size());
    dprintf("%u functions found!\n", _functions.size());
}

void ControlFlowAnalysis::FunctionRanges()
{
    //iterate over the functions and then find the deepest block = function end
    for(const auto & function : _functions)
    {
        duint start = function.first;
        duint end = start;
        for(auto blockstart : function.second)
        {
            BasicBlock* block = this->findBlock(blockstart);
            if(!block)
                DebugBreak(); //this shouldn't happen
            if(block->end > end)
                end = block->end;
        }
        _functionRanges.push_back({ start, end });
    }
}

void ControlFlowAnalysis::insertBlock(BasicBlock block)
{
    if(_blocks.find(block.start) != _blocks.end())
        DebugBreak();
    _blocks[block.start] = block;
}

ControlFlowAnalysis::BasicBlock* ControlFlowAnalysis::findBlock(duint start)
{
    if(!start)
        return nullptr;
    auto found = _blocks.find(start);
    return found != _blocks.end() ? &found->second : nullptr;
}

void ControlFlowAnalysis::insertParent(duint child, duint parent)
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

ControlFlowAnalysis::UintSet* ControlFlowAnalysis::findParents(duint child)
{
    if(!child)
        return nullptr;
    auto found = _parentMap.find(child);
    return found != _parentMap.end() ? &found->second : nullptr;
}

duint ControlFlowAnalysis::findFunctionStart(BasicBlock* block, ControlFlowAnalysis::UintSet* parents)
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

duint ControlFlowAnalysis::GetReferenceOperand()
{
    for(int i = 0; i < _cp.x86().op_count; i++)
    {
        const cs_x86_op & operand = _cp.x86().operands[i];
        if(operand.type == X86_OP_IMM)
        {
            duint dest = (duint)operand.imm;
            if(dest >= _base && dest < _base + _size)
                return dest;
        }
    }
    return 0;
}

#ifdef _WIN64
void ControlFlowAnalysis::EnumerateFunctionRuntimeEntries64(std::function<bool(PRUNTIME_FUNCTION)> Callback)
{
    if(!_functionInfoData)
        return;

    // Get the table pointer and size
    auto functionTable = (PRUNTIME_FUNCTION)_functionInfoData;
    duint totalCount = (_functionInfoSize / sizeof(RUNTIME_FUNCTION));

    // Enumerate each entry
    for(ULONG i = 0; i < totalCount; i++)
    {
        if(!Callback(&functionTable[i]))
            break;
    }
}
#endif // _WIN64