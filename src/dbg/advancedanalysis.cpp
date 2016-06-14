#include "advancedanalysis.h"
#include <queue>
#include "console.h"
#include "filehelper.h"
#include "function.h"

AdvancedAnalysis::AdvancedAnalysis(duint base, duint size, duint maxDepth, bool dump)
    : Analysis(base, size),
      mMaxDepth(maxDepth),
      mDump(dump)
{
}

void AdvancedAnalysis::Analyse()
{
    //TODO: implement queue to analyze multiple functions
    linearXrefPass();

    for(const auto & entryPoint : mEntryPoints)
        analyzeFunction(entryPoint);
}

void AdvancedAnalysis::SetMarkers()
{
    if(mDump)
        for(const auto & function : mFunctions)
            FileHelper::WriteAllText(StringUtils::sprintf("cfgraph_" fhex ".dot", function.entryPoint), function.ToDot());

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
        if(!FunctionAdd(start, end, false, icount))
        {
            FunctionDelete(start);
            FunctionDelete(end);
            FunctionAdd(start, end, false, icount);
        }
    }
    GuiUpdateAllViews();
}

void AdvancedAnalysis::analyzeFunction(duint entryPoint)
{
    //BFS through the disassembly starting at entryPoint
    CFGraph graph(entryPoint);
    UintSet visited;
    std::queue<duint> queue;
    queue.push(graph.entryPoint);
    while(!queue.empty())
    {
        auto start = queue.front();
        queue.pop();
        if(visited.count(start) || !inRange(start))   //already visited or out of range
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
            if(mCp.InGroup(CS_GRP_JUMP) || mCp.IsLoop())   //jump
            {
                //set the branch destinations
                node.brtrue = mCp.BranchDestination();
                if(mCp.GetId() != X86_INS_JMP)   //unconditional jumps dont have a brfalse
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
            if(mCp.InGroup(CS_GRP_CALL))   //call
            {
                //TODO: add this to a queue to be analyzed later
            }
            if(mCp.InGroup(CS_GRP_RET))   //return
            {
                node.terminal = true;
                graph.AddNode(node);
                break;
            }
            node.end += mCp.Size();
        }
    }
    mFunctions.push_back(graph);
}


void AdvancedAnalysis::linearXrefPass()
{
    dputs("Starting xref analysis...");
    auto ticks = GetTickCount();

    for(auto addr = mBase; addr < mBase + mSize;)
    {
        if(!mCp.Disassemble(addr, translateAddr(addr)))
        {
            addr++;
            continue;
        }
        addr += mCp.Size();

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
        {
            if(mCp.InGroup(CS_GRP_CALL))
                xref.type = XREF_CALL;
            else if(mCp.InGroup(CS_GRP_JUMP))
                xref.type = XREF_JMP;
            else
                xref.type = XREF_DATA;

            auto found = mXrefs.find(xref.addr);
            if(found == mXrefs.end())
            {
                std::vector<XREF> vec;
                vec.push_back(xref);
                mXrefs[xref.addr] = vec;
            }
            else
            {
                found->second.push_back(xref);
            }
        }
    }

    dprintf("%u xrefs found in %ums!\n", mXrefs.size(), GetTickCount() - ticks);
}

void AdvancedAnalysis::findEntryPoints()
{

}
