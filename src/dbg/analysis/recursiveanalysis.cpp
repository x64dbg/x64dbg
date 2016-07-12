#include "recursiveanalysis.h"
#include <queue>
#include "console.h"
#include "filehelper.h"
#include "function.h"
#include "xrefs.h"

RecursiveAnalysis::RecursiveAnalysis(duint base, duint size, duint entryPoint, duint maxDepth, bool dump)
    : Analysis(base, size),
      mEntryPoint(entryPoint),
      mMaxDepth(maxDepth),
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
            FileHelper::WriteAllText(StringUtils::sprintf("cfgraph_" fhex ".dot", function.entryPoint), function.ToDot());

    //set function ranges
    for(const auto & function : mFunctions)
    {
        duint start = ~0;
        duint end = 0;
        duint icount = 0;
        for(const auto & node : function.nodes)
        {
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
        if(visited.count(start) || !inRange(start)) //already visited or out of range
            continue;
        visited.insert(start);

        CFNode node(graph.entryPoint, start, start);
        while(true)
        {
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
                duint dest = mCp.ResolveOpValue(i, [](x86_reg)->size_t
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

            if(mCp.InGroup(CS_GRP_JUMP) || mCp.IsLoop()) //jump
            {
                //set the branch destinations
                node.brtrue = mCp.BranchDestination();
                if(mCp.GetId() != X86_INS_JMP)  //unconditional jumps dont have a brfalse
                    node.brfalse = node.end + mCp.Size();

                //add node to the function graph
                graph.AddNode(node);

                //enqueue branch destinations
                if(node.brtrue)
                    queue.push(node.brtrue);
                if(node.brfalse)
                    queue.push(node.brfalse);

                break;
            }
            if(mCp.InGroup(CS_GRP_CALL))  //call
            {
                //TODO: add this to a queue to be analyzed later
            }
            if(mCp.InGroup(CS_GRP_RET))  //return
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
    //third pass: correct the parents
    graph.parents.clear();
    for(const auto & nodeIt : graph.nodes)
    {
        const auto & node = nodeIt.second;
        graph.AddParent(node.start, node.brtrue);
        graph.AddParent(node.start, node.brfalse);
    }
    mFunctions.push_back(graph);
}
