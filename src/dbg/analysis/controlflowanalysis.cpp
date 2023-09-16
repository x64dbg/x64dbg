#include "controlflowanalysis.h"
#include "console.h"
#include "module.h"
#include "TitanEngine/TitanEngine.h"
#include "memory.h"
#include "function.h"

ControlFlowAnalysis::ControlFlowAnalysis(duint base, duint size, bool exceptionDirectory)
    : Analysis(base, size),
      mFunctionInfoSize(0),
      mFunctionInfoData(nullptr)
{
#ifdef _WIN64
    // This will only be valid if the address range is within a loaded module
    mModuleBase = ModBaseFromAddr(base);

    if(exceptionDirectory && mModuleBase != 0)
    {
        char modulePath[MAX_PATH];
        memset(modulePath, 0, sizeof(modulePath));

        ModPathFromAddr(mModuleBase, modulePath, ARRAYSIZE(modulePath));

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
            auto virtualOffset = GetPE32DataFromMappedFile(fileMapVa, IMAGE_DIRECTORY_ENTRY_EXCEPTION, UE_SECTIONVIRTUALOFFSET);
            mFunctionInfoSize = duint(GetPE32DataFromMappedFile(fileMapVa, IMAGE_DIRECTORY_ENTRY_EXCEPTION, UE_SECTIONVIRTUALSIZE));

            // Unload the file
            StaticFileUnloadW(nullptr, false, fileHandle, fileSize, fileMapHandle, fileMapVa);

            // Get a copy of the function table
            if(virtualOffset)
            {
                // Read the table into a buffer
                mFunctionInfoData = emalloc(mFunctionInfoSize);

                if(mFunctionInfoData)
                    MemRead(virtualOffset + mModuleBase, mFunctionInfoData, mFunctionInfoSize);
            }
        }
    }
#endif //_WIN64
}

ControlFlowAnalysis::~ControlFlowAnalysis()
{
    if(mFunctionInfoData)
        efree(mFunctionInfoData);
}

void ControlFlowAnalysis::Analyse()
{
    dputs("Starting analysis...");
    auto ticks = GetTickCount();

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

    dprintf("Analysis finished!\n");
}

void ControlFlowAnalysis::SetMarkers()
{
    FunctionDelRange(mBase, mBase + mSize - 1, false);
    auto size = mFunctionRanges.size();
    for(auto i = size - 1; i != -1; i--)
    {
        const auto & range = mFunctionRanges[i];
        FunctionAdd(range.first, range.second, false);
    }
    /*dprintf("digraph ControlFlow {\n");
    int i = 0;
    std::map<duint, int> nodeMap;
    for(const auto & it : mBlocks)
    {
        const auto & block = it.second;
        nodeMap.insert({ block.start, i });
        dprintf("    node%u [label=\"s=%p, e=%p, f=%p\"];\n", i, block.start, block.end, block.function);
        i++;
    }
    for(auto it : mBlocks)
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
    mBlockStarts.insert(mBase);
    auto bSkipFilling = false;
    for(duint i = 0; i < mSize;)
    {
        auto addr = mBase + i;
        if(mCp.Disassemble(addr, translateAddr(addr), MAX_DISASM_BUFFER))
        {
            if(bSkipFilling) //handle filling skip mode
            {
                if(!mCp.IsFilling()) //do nothing until the filling stopped
                {
                    bSkipFilling = false;
                    mBlockStarts.insert(addr);
                }
            }
            else if(mCp.IsRet()) //RET breaks control flow
            {
                bSkipFilling = true; //skip INT3/NOP/whatever filling bytes (those are not part of the control flow)
            }
            else if(mCp.IsJump() || mCp.IsLoop()) //branches
            {
                auto dest1 = getReferenceOperand();
                duint dest2 = 0;
                if(mCp.GetId() != ZYDIS_MNEMONIC_JMP) //conditional jump
                    dest2 = addr + mCp.Size();

                if(!dest1 && !dest2) //TODO: better code for this (make sure absolutely no filling is inserted)
                    bSkipFilling = true;
                if(dest1)
                    mBlockStarts.insert(dest1);
                if(dest2)
                    mBlockStarts.insert(dest2);
            }
            else if(mCp.IsCall())
            {
                auto dest1 = getReferenceOperand();
                if(dest1)
                {
                    mBlockStarts.insert(dest1);
                    mFunctionStarts.insert(dest1);
                }
            }
            else
            {
                auto dest1 = getReferenceOperand();
                if(dest1)
                    mBlockStarts.insert(dest1);
            }
            i += mCp.Size();
        }
        else
            i++;
    }
}

void ControlFlowAnalysis::BasicBlocks()
{
    for(auto i = mBlockStarts.begin(); i != mBlockStarts.end(); ++i)
    {
        auto start = *i;
        if(!inRange(start))
            continue;
        auto nextStart = mBase + mSize;
        auto next = std::next(i);
        if(next != mBlockStarts.end())
            nextStart = *next;
        for(duint addr = start, prevaddr; addr < mBase + mSize;)
        {
            prevaddr = addr;
            if(mCp.Disassemble(addr, translateAddr(addr), MAX_DISASM_BUFFER))
            {
                if(mCp.IsRet())
                {
                    insertBlock(BasicBlock(start, addr, 0, 0)); //leaf block
                    break;
                }
                else if(mCp.IsJump() || mCp.IsLoop())
                {
                    auto dest1 = getReferenceOperand();
                    auto dest2 = mCp.GetId() != ZYDIS_MNEMONIC_JMP ? addr + mCp.Size() : 0;
                    insertBlock(BasicBlock(start, addr, dest1, dest2));
                    insertParent(dest1, start);
                    insertParent(dest2, start);
                    break;
                }
                addr += mCp.Size();
            }
            else
                addr++;
            if(addr == nextStart) //special case handling overlapping blocks
            {
                insertBlock(BasicBlock(start, prevaddr, 0, nextStart));
                insertParent(nextStart, start);
                break;
            }
        }
    }
    mBlockStarts.clear();

#ifdef _WIN64
    auto count = 0;
    enumerateFunctionRuntimeEntries64([&](PRUNTIME_FUNCTION Function)
    {
        auto funcAddr = mModuleBase + Function->BeginAddress;
        auto funcEnd = mModuleBase + Function->EndAddress;

        // If within limits...
        if(inRange(funcAddr) && inRange(funcEnd))
            mFunctionStarts.insert(funcAddr);
        count++;
        return true;
    });
    dprintf("%d functions from the exception directory...\n", count);
#endif // _WIN64

    dprintf("%d basic blocks, %d function starts detected...\n", int(mBlocks.size()), int(mFunctionStarts.size()));
}

void ControlFlowAnalysis::Functions()
{
    typedef std::pair<BasicBlock*, const UintSet*> DelayedBlock;
    std::vector<DelayedBlock> delayedBlocks;
    for(auto & it : mBlocks)
    {
        auto block = &it.second;
        auto parents = findParents(block->start);
        if(!block->function)
        {
            if(!parents || mFunctionStarts.count(block->start)) //no parents = function start
            {
                auto functionStart = block->start;
                block->function = functionStart;
                UintSet functionBlocks;
                functionBlocks.insert(functionStart);
                mFunctions[functionStart] = functionBlocks;
            }
            else //in function
            {
                auto function = findFunctionStart(block, parents);
                if(!function) //this happens with loops / unreferenced blocks sometimes
                    delayedBlocks.emplace_back(block, parents);
                else
                    block->function = function;
            }
        }
        else
            DebugBreak(); //this should not happen
    }
    auto delayedCount = int(delayedBlocks.size());
    dprintf("%d/%d delayed blocks...\n", delayedCount, int(mBlocks.size()));
    auto resolved = 0;
    for(auto & delayedBlock : delayedBlocks)
    {
        auto block = delayedBlock.first;
        auto parents = delayedBlock.second;
        auto function = findFunctionStart(block, parents);
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
    dprintf("%d/%d delayed blocks resolved (%d/%d still left, probably unreferenced functions)\n", resolved, delayedCount, delayedCount - resolved, int(mBlocks.size()));
    auto unreferencedCount = 0;
    for(const auto & block : mBlocks)
    {
        auto found = mFunctions.find(block.second.function);
        if(found == mFunctions.end()) //unreferenced block
        {
            unreferencedCount++;
            continue;
        }
        found->second.insert(block.second.start);
    }
    dprintf("%d/%u unreferenced blocks\n", unreferencedCount, DWORD(mBlocks.size()));
    dprintf("%u functions found!\n", DWORD(mFunctions.size()));
}

void ControlFlowAnalysis::FunctionRanges()
{
    //iterate over the functions and then find the deepest block = function end
    for(const auto & function : mFunctions)
    {
        auto start = function.first;
        auto end = start;
        for(auto blockstart : function.second)
        {
            auto block = findBlock(blockstart);
            if(!block)
                __debugbreak(); //this shouldn't happen
            if(block->end > end)
                end = block->end;
        }
        mFunctionRanges.push_back({ start, end });
    }
}

void ControlFlowAnalysis::insertBlock(const BasicBlock & block)
{
    if(mBlocks.find(block.start) != mBlocks.end())
        DebugBreak();
    mBlocks[block.start] = block;
}

const ControlFlowAnalysis::BasicBlock* ControlFlowAnalysis::findBlock(duint start) const
{
    if(!start)
        return nullptr;
    auto found = mBlocks.find(start);
    return found != mBlocks.end() ? &found->second : nullptr;
}

void ControlFlowAnalysis::insertParent(duint child, duint parent)
{
    if(!child || !parent)
        return;
    auto found = mParentMap.find(child);
    if(found == mParentMap.end())
    {
        UintSet parents;
        parents.insert(parent);
        mParentMap[child] = parents;
    }
    else
        found->second.insert(parent);
}

const ControlFlowAnalysis::UintSet* ControlFlowAnalysis::findParents(duint child) const
{
    if(!child)
        return nullptr;
    auto found = mParentMap.find(child);
    return found != mParentMap.end() ? &found->second : nullptr;
}

duint ControlFlowAnalysis::findFunctionStart(const BasicBlock* block, const UintSet* parents) const
{
    if(!block)
        return 0;
    if(block->function)
        return block->function;
    auto left = findBlock(block->left);
    if(left && left->function)
        return left->function;
    auto right = findBlock(block->right);
    if(right && right->function)
        return right->function;
    for(auto start : *parents)
    {
        auto parent = findBlock(start);
        if(parent->function)
            return parent->function;
    }
    return 0;
}

String ControlFlowAnalysis::blockToString(const BasicBlock* block)
{
    if(!block)
        return String("null");
    return block->toString();
}

duint ControlFlowAnalysis::getReferenceOperand() const
{
    for(auto i = 0; i < mCp.OpCount(); i++)
    {
        const auto & op = mCp[i];
        if(op.type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
        {
            auto dest = duint(op.imm.value.u);
            if(inRange(dest))
                return dest;
        }
        else if(op.type == ZYDIS_OPERAND_TYPE_MEMORY)
        {
            auto dest = duint(op.mem.disp.value);
            if(op.mem.base == ZYDIS_REGISTER_RIP) //rip-relative
                dest += mCp.Address() + mCp.Size();
            if(inRange(dest))
                return dest;
        }
    }
    return 0;
}

#ifdef _WIN64
void ControlFlowAnalysis::enumerateFunctionRuntimeEntries64(const std::function<bool(PRUNTIME_FUNCTION)> & Callback) const
{
    if(!mFunctionInfoData)
        return;

    // Get the table pointer and size
    auto functionTable = PRUNTIME_FUNCTION(mFunctionInfoData);
    auto totalCount = mFunctionInfoSize / sizeof(RUNTIME_FUNCTION);

    // Enumerate each entry
    for(duint i = 0; i < totalCount; i++)
    {
        if(!Callback(&functionTable[i]))
            break;
    }
}
#endif // _WIN64