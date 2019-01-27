#include "recursiveanalysis.h"
#include <queue>
#include "console.h"
#include "filehelper.h"
#include "function.h"
#include "xrefs.h"
#include "plugin_loader.h"
#include <memory.h>

RecursiveAnalysis::RecursiveAnalysis(duint base, duint size, duint entryPoint, bool usePlugins, bool dump)
    : Analysis(base, size),
      mEntryPoint(entryPoint),
      mUsePlugins(usePlugins),
      mDump(dump)
{
}

void RecursiveAnalysis::Analyse()
{
    //TODO: implement queue to analyze multiple functions
    analyzeFunction(mEntryPoint);
}

void RecursiveAnalysis::SetMarkers()
{
    if(mDump)
        for(const auto & function : mFunctions)
            FileHelper::WriteAllText(StringUtils::sprintf("cfgraph_%p.dot", function.entryPoint), GraphToDot(function));

    //set function ranges
    for(const auto & function : mFunctions)
    {
        duint start = ~0;
        duint end = 0;
        duint icount = 0;
        for(const auto & node : function.nodes)
        {
            if(!inRange(node.second.start))
                continue;
            icount += node.second.icount;
            start = min(node.second.start, start);
            end = max(node.second.end, end);
        }
        XrefDelRange(start, end);
        if(!FunctionAdd(start, end, false, icount))
        {
            FunctionDelete(start);
            FunctionDelete(end);
            FunctionAdd(start, end, false, icount);
        }
    }

    //set xrefs
    for(const auto & xref : mXrefs)
        XrefAdd(xref.addr, xref.from);

    GuiUpdateAllViews();
}

void RecursiveAnalysis::analyzeFunction(duint entryPoint)
{
    //first pass: BFS through the disassembly starting at entryPoint
    CFGraph graph(entryPoint);
    UintSet visited;
    std::queue<duint> queue;
    queue.push(graph.entryPoint);
    while(!queue.empty())
    {
        auto start = queue.front();
        queue.pop();
        if(visited.count(start)) //already visited
            continue;
        visited.insert(start);

        CFNode node(graph.entryPoint, start, start);

        if(!inRange(start)) //out of range
        {
            graph.AddNode(node);
            continue;
        }

        while(true)
        {
            if(!inRange(node.end))
            {
                node.end = mCp.Address();
                node.terminal = true;
                graph.AddNode(node);
                break;
            }

            node.icount++;
            if(!mCp.Disassemble(node.end, translateAddr(node.end)))
            {
                node.end++;
                continue;
            }

            //do xref analysis on the instruction
            XREF xref;
            xref.addr = 0;
            xref.from = mCp.Address();
            for(auto i = 0; i < mCp.OpCount(); i++)
            {
                duint dest = mCp.ResolveOpValue(i, [](ZydisRegister)->size_t
                {
                    return 0;
                });
                if(inRange(dest))
                {
                    xref.addr = dest;
                    break;
                }
            }
            if(xref.addr)
                mXrefs.push_back(xref);

            if(!mCp.IsNop() && (mCp.IsJump() || mCp.IsLoop())) //non-nop jump
            {
                //set the branch destinations
                node.brtrue = mCp.BranchDestination();
                if(mCp.GetId() != ZYDIS_MNEMONIC_JMP) //unconditional jumps dont have a brfalse
                    node.brfalse = node.end + mCp.Size();

                //consider register/memory branches as terminal nodes
                if(mCp.OpCount() && mCp[0].type != ZYDIS_OPERAND_TYPE_IMMEDIATE)
                {
                    //jmp ptr [index * sizeof(duint) + switchTable]
                    if(mCp[0].type == ZYDIS_OPERAND_TYPE_MEMORY && mCp[0].mem.base == ZYDIS_REGISTER_NONE && mCp[0].mem.index != ZYDIS_REGISTER_NONE
                            && mCp[0].mem.scale == sizeof(duint) && MemIsValidReadPtr(duint(mCp[0].mem.disp.value)))
                    {
                        Memory<duint*> switchTable(512 * sizeof(duint));
                        duint actualSize, index;
                        MemRead(duint(mCp[0].mem.disp.value), switchTable(), 512 * sizeof(duint), &actualSize);
                        actualSize /= sizeof(duint);
                        for(index = 0; index < actualSize; index++)
                            if(MemIsCodePage(switchTable()[index], false) == false)
                                break;
                        actualSize = index;
                        if(actualSize >= 2 && actualSize < 512)
                        {
                            node.brtrue = 0;
                            node.brfalse = 0;
                            for(index = 0; index < actualSize; index++)
                            {
                                node.exits.push_back(switchTable()[index]);
                                queue.emplace(switchTable()[index]);
                                xref.addr = switchTable()[index];
                                mXrefs.push_back(xref);
                            }
                        }
                        else
                            node.terminal = true;
                    }
                    else
                        node.terminal = true;
                }

                //add node to the function graph
                graph.AddNode(node);

                //enqueue branch destinations
                if(node.brtrue)
                    queue.push(node.brtrue);
                if(node.brfalse)
                    queue.push(node.brfalse);

                break;
            }
            if(mCp.IsCall())
            {
                //TODO: add this to a queue to be analyzed later
            }
            if(mCp.IsRet())
            {
                node.terminal = true;
                graph.AddNode(node);
                break;
            }
            node.end += mCp.Size();
        }
    }
    //second pass: split overlapping blocks introduced by backedges
    for(auto & nodeIt : graph.nodes)
    {
        auto & node = nodeIt.second;
        duint addr = node.start;
        duint icount = 0;
        while(addr < node.end)
        {
            icount++;
            auto size = mCp.Disassemble(addr, translateAddr(addr)) ? mCp.Size() : 1;
            if(graph.nodes.count(addr + size))
            {
                node.end = addr;
                node.split = true;
                node.brtrue = addr + size;
                node.brfalse = 0;
                node.terminal = false;
                node.icount = icount;
                break;
            }
            addr += size;
        }
    }
    //third pass: correct the parents + add brtrue and brfalse to the exits + get data
    graph.parents.clear();
    for(auto & nodeIt : graph.nodes)
    {
        auto & node = nodeIt.second;
        graph.AddParent(node.start, node.brtrue);
        graph.AddParent(node.start, node.brfalse);
        if(node.brtrue)
            node.exits.push_back(node.brtrue);
        if(node.brfalse)
            node.exits.push_back(node.brfalse);
        if(node.brtrue && !node.brfalse)
            node.brtrue = 0;
        if(!node.icount)
            continue;
        node.instrs.reserve(node.icount);
        auto addr = node.start;
        while(addr <= node.end) //disassemble all instructions
        {
            auto size = mCp.Disassemble(addr, translateAddr(addr)) ? mCp.Size() : 1;
            if(mCp.IsCall() && mCp.OpCount()) //call reg / call [reg+X]
            {
                auto & op = mCp[0];
                switch(op.type)
                {
                case ZYDIS_OPERAND_TYPE_REGISTER:
                    node.indirectcall = true;
                    break;
                case ZYDIS_OPERAND_TYPE_MEMORY:
                    node.indirectcall |= op.mem.base != ZYDIS_REGISTER_RIP &&
                                         (op.mem.base != ZYDIS_REGISTER_NONE || op.mem.index != ZYDIS_REGISTER_NONE);
                    break;
                default:
                    break;
                }
            }
            BridgeCFInstruction instr;
            instr.addr = addr;
            for(int i = 0; i < size; i++)
                instr.data[i] = inRange(addr + i) ? *translateAddr(addr + i) : 0;
            node.instrs.push_back(instr);
            addr += size;
        }
    }
    //fourth pass: allow plugins to manipulate the graph
    if(mUsePlugins && !plugincbempty(CB_ANALYZE))
    {
        PLUG_CB_ANALYZE info;
        info.graph = graph.ToGraphList();
        plugincbcall(CB_ANALYZE, &info);
        graph = BridgeCFGraph(&info.graph, true);
    }
    mFunctions.push_back(graph);
}
