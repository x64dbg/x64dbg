#include "recursiveanalysis.h"
#include <queue>
#include "console.h"
#include "filehelper.h"
#include "function.h"
#include "label.h"
#include "xrefs.h"
#include "plugin_loader.h"
#include "loop.h"
#include <set>
#include <map>

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
    analyzeLoops(mEntryPoint);
}

void RecursiveAnalysis::SetMarkers()
{
    if(mDump)
        for(const auto & function : mFunctions)
            FileHelper::WriteAllText(StringUtils::sprintf("cfgraph_%p.dot", function.second.entryPoint), GraphToDot(function.second));

    //set function ranges
    for(const auto & functionItr : mFunctions)
    {
        // Split functions with multiple chunks (either due to tail calls or PGO)
        // Example: kernelbase:KernelBaseBaseDllInitialize
        // This algorithm orders basic blocks and then iterates, growing the chunk downwards
        // Function ranges are collected in another ordered map for loop insertion
        const auto & function = functionItr.second;
        std::map<Range, const BridgeCFNode*, RangeCompare> blockRanges, functionRanges;
        for(const auto & nodeItr : function.nodes)
        {
            const auto & node = nodeItr.second;
            if(!blockRanges.emplace(Range(node.start, node.end), &node).second)
                dprintf_untranslated("Overlapping basic block %p-%p, please report a bug!\n", node.start, node.end);
        }

        auto addFunction = [&function, &functionRanges](duint start, duint end, duint icount)
        {
            FunctionDelRange(start, end, false /* Do not override user-defined functions */);
            LoopDeleteRange(start, end); // clear loop range in function
            //XrefDelRange(start, end); // clear xrefs in function
            FunctionAdd(start, end, false, icount, function.entryPoint);
            functionRanges.emplace(Range(start, end), nullptr);
        };

        duint rangeStart = 0, rangeEnd = 0, rangeInstructionCount = 0;
        for(auto rangeItr = blockRanges.begin(); rangeItr != blockRanges.end(); ++rangeItr)
        {
            auto disasmLen = [this](duint addr) -> size_t
            {
                if(!mZydis.Disassemble(addr, translateAddr(addr)))
                    return 1;
                return mZydis.Size();
            };
            const auto & node = *rangeItr->second;
            if(!rangeStart)
            {
                rangeStart = node.start;
                rangeEnd = node.end;
                rangeInstructionCount = node.icount;
            }
            else
            {
#define ALIGN_UP(Address, Align) (((ULONG_PTR)(Address) + (Align) - 1) & ~((Align) - 1))
                auto nextInstr = rangeEnd + disasmLen(rangeEnd);
                // The next instruction(s) might be padding to align IP, also allow this case to count as consecutive
                if(nextInstr == node.start || ((node.start & 0xF) == 0 && ALIGN_UP(nextInstr, 16) == node.start))
                {
                    // Merge the consecutive range
                    rangeEnd = node.end;
                    rangeInstructionCount += node.icount;
                }
                else
                {
                    if(mDump)
                        dprintf_untranslated("Flush partial range %p-%p\n", rangeStart, rangeEnd);
                    addFunction(rangeStart, rangeEnd, rangeInstructionCount);
                    rangeStart = node.start;
                    rangeEnd = node.end;
                    rangeInstructionCount = node.icount;
                }
            }
        }
        if(mDump)
            dprintf_untranslated("Flush range %p-%p\n", rangeStart, rangeEnd);
        addFunction(rangeStart, rangeEnd, rangeInstructionCount);

        // Collect loop ranges
        const auto & loopInfo = mLoopInfo[function.entryPoint];
        std::vector<Range> loopRanges;
        for(const auto & backedge : loopInfo.backedges)
        {
            //dprintf("Backedge %p-%p\n", backedge.first, backedge.second);
            auto startBlock = backedge.second; // destination is the start of the potential loop range
            auto endBlock = backedge.first; // source is the start of the last block in the potential loop range
            auto startFunctionItr = functionRanges.find(Range(startBlock, startBlock));
            auto endFunctionItr = functionRanges.find(Range(endBlock, endBlock));
            if(startFunctionItr != functionRanges.end() && startFunctionItr == endFunctionItr)
            {
                // Loop ranges can only be in the same function chunk range, otherwise they won't insert/display properly
                const auto & endBlockNode = function.nodes.at(endBlock);
                loopRanges.emplace_back(startBlock, endBlockNode.end);
            }
        }
        // Order loop ranges by start address so the outermost loop is inserted first
        std::sort(loopRanges.begin(), loopRanges.end());
        for(const auto & loopRange : loopRanges)
        {
            if(mDump)
                dprintf_untranslated("Loop %p-%p\n", loopRange.first, loopRange.second);
            duint loopInstructionCount = 0;
            for(auto blockItr = blockRanges.find(Range(loopRange.first, loopRange.first)); blockItr != blockRanges.end(); ++blockItr)
            {
                if(mDump)
                    dprintf_untranslated("icount block %p-%p\n", blockItr->second->start, blockItr->second->end);
                loopInstructionCount += blockItr->second->icount;
                if(blockItr->second->end >= loopRange.second)
                    break;
            }
            // TODO: RtlpEnterCriticalSectionContended has some weirdly nested loops that overlap each other
            // this might cause LoopAdd to fail
            LoopAdd(loopRange.first, loopRange.second, false, loopInstructionCount);
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
    visited.reserve(queue.size());
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
                node.end = (duint)mZydis.Address();
                node.terminal = true;
                graph.AddNode(node);
                break;
            }

            node.icount++;
            if(!mZydis.Disassemble(node.end, translateAddr(node.end)))
            {
                node.end++;
                continue;
            }

            //do xref analysis on the instruction
            XREF xref;
            xref.addr = 0;
            xref.from = (duint)mZydis.Address();
            for(auto i = 0; i < mZydis.OpCount(); i++)
            {
                auto dest = (duint)mZydis.ResolveOpValue(i, [](ZydisRegister) -> uint64_t
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

            if(!mZydis.IsNop() && (mZydis.IsJump() || mZydis.IsLoop())) //non-nop jump
            {
                //set the branch destinations
                node.brtrue = (duint)mZydis.BranchDestination();
                if(mZydis.GetId() != ZYDIS_MNEMONIC_JMP) //unconditional jumps dont have a brfalse
                    node.brfalse = node.end + mZydis.Size();

                //consider register/memory branches as terminal nodes
                if(mZydis.OpCount() && mZydis[0].type != ZYDIS_OPERAND_TYPE_IMMEDIATE)
                {
                    //jmp ptr [index * sizeof(duint) + switchTable]
                    if(mZydis[0].type == ZYDIS_OPERAND_TYPE_MEMORY && mZydis[0].mem.base == ZYDIS_REGISTER_NONE && mZydis[0].mem.index != ZYDIS_REGISTER_NONE
                            && mZydis[0].mem.scale == sizeof(duint) && MemIsValidReadPtr(duint(mZydis[0].mem.disp.value)))
                    {
                        Memory<duint*> switchTable(512 * sizeof(duint));
                        duint actualSize, index;
                        MemRead(duint(mZydis[0].mem.disp.value), switchTable(), 512 * sizeof(duint), &actualSize);
                        actualSize /= sizeof(duint);
                        for(index = 0; index < actualSize; index++)
                            if(MemIsCodePage(switchTable()[index], false) == false)
                                break;
                        actualSize = index;
                        if(actualSize >= 2 && actualSize < 512)
                        {
                            node.brtrue = 0;
                            node.brfalse = 0;
                            node.exits.reserve(actualSize);
                            mXrefs.reserve(actualSize);
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
            if(mZydis.IsCall())
            {
                //TODO: add this to a queue to be analyzed later
            }
            if(mZydis.IsRet())
            {
                node.terminal = true;
                graph.AddNode(node);
                break;
            }
            node.end += mZydis.Size();
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
            auto size = mZydis.Disassemble(addr, translateAddr(addr)) ? mZydis.Size() : 1;
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
            auto size = mZydis.Disassemble(addr, translateAddr(addr)) ? mZydis.Size() : 1;
            if(mZydis.IsCall() && mZydis.OpCount()) //call reg / call [reg+X]
            {
                auto & op = mZydis[0];
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
    mFunctions.emplace(entryPoint, graph);
}

void RecursiveAnalysis::analyzeLoops(duint entryPoint)
{
    auto graph = GetFunctionGraph(entryPoint);
    if(!graph)
        return;

    auto & loopInfo = mLoopInfo[entryPoint] = LoopInfo();
    loopInfo.functionEntry = entryPoint;

    // Detect loops to the same basic block
    for(const auto & node : graph->nodes)
        for(duint exit : node.second.exits)
            if(exit == node.first)
                loopInfo.trivialLoops.insert(node.first);

    // Thanks to DefCon42 for help with the algorithm!
    std::vector<duint> stack;
    stack.push_back(entryPoint);
    std::set<duint> visited;
    std::map<duint, std::vector<duint>> state;

    while(!stack.empty())
    {
        auto start = stack.back();
        stack.pop_back();
        if(visited.count(start))  //already visited
            continue;
        visited.insert(start);
        state[start].push_back(start);
        for(duint exit : graph->nodes.at(start).exits)
        {
            if(!visited.count(exit))
            {
                state[exit] = state[start];
                stack.push_back(exit);
            }
            else if(std::count(state[start].begin(), state[start].end(), exit))
            {
                loopInfo.backedges.emplace(start, exit);
            }
        }
    }
}

void RecursiveAnalysis::dominatorAnalysis(duint entryPoint)
{
    auto graph = GetFunctionGraph(entryPoint);
    if(!graph)
        return;

    // WIP algo

    // http://jgaa.info/accepted/2006/GeorgiadisTarjanWerneck2006.10.1.pdf
    // https://www.cs.princeton.edu/courses/archive/fall03/cs528/handouts/a%20fast%20algorithm%20for%20finding.pdf
    std::map<duint, duint> parent, anchestor, vertex;
    std::map<duint, duint> label, semi;
    std::map<duint, std::set<duint>> pred, bucket;
    std::map<duint, duint> dom;

    std::map<duint, duint> indexToAddress, addressToIndex;
    std::map<duint, std::set<duint>> succ;
    {
        size_t curIndex = 1;
        for(const auto & node : graph->nodes)
        {
            indexToAddress[curIndex] = node.first;
            addressToIndex[node.first] = curIndex;
            curIndex++;
        }
        for(const auto & node : graph->nodes)
        {
            auto & s = succ[addressToIndex[node.first]];
            for(duint exit : node.second.exits)
            {
                s.insert(addressToIndex[exit]);
            }
        }
    }

    duint r = addressToIndex[entryPoint];
    duint n = 0;
    std::function<void(duint)> dfs = [&](duint v)
    {
        semi.at(v) = (n = n + 1);
        vertex.at(n) = label.at(v) = v;
        anchestor.at(v) = 0;
        for(duint w : succ.at(v))
        {
            if(semi.at(w) == 0)
            {
                parent.at(w) = v;
                dfs(w);
            }
            pred.at(w).insert(v);
        }
    };

    std::function<void(duint)> compress = [&](duint v)
    {
        if(anchestor.at(anchestor.at(v)) != 0)
        {
            compress(anchestor.at(v));
            if(semi.at(label.at(anchestor.at(v))) < semi.at(label.at(v)))
                label.at(v) = label.at(anchestor.at(v));
            anchestor.at(v) = anchestor.at(anchestor.at(v));
        }
    };

    auto eval = [&](duint v)
    {
        if(anchestor.at(v) == 0)
        {
            return v;
        }
        else
        {
            compress(v);
            return label.at(v);
        }
    };

    auto link = [&](duint v, duint w)
    {
        anchestor.at(w) = v;
    };

    auto print = [](const char* name, const std::map<duint, duint> & m)
    {
        dprintf("%s:\n", name);
        for(const auto & e : m)
            dprintf("  %s[%p] = %p\n", name, e.first, e.second);
    };

    print("indexToAddress", indexToAddress);
    print("addressToIndex", addressToIndex);

    // step1
    for(duint i = 0; i < succ.size(); i++)
    {
        auto v = i + 1;
        pred[v] = bucket[v];
        semi[v] = vertex[v] = anchestor[v] = label[v] = parent[v] = dom[v] = 0;
        dprintf("%d\n", v);
    }
    n = 0;
    dfs(r);

    print("semi", semi);
    print("vertex", vertex);
    print("label", label);
    print("anchestor", anchestor);

    for(duint i = n; i != 1; i--)
    {
        auto w = vertex.at(i);
        // step2
        for(duint v : pred.at(w))
        {
            auto u = eval(v);
            if(semi.at(u) < semi.at(w))
            {
                semi[w] = semi.at(u);
            }
            bucket.at(vertex.at(semi.at(w))).insert(w);
            link(parent.at(w), w);
        }
        // step3
        duint parentw = parent.at(w);
        auto & bp = bucket.at(parent.at(w));
        for(auto itr = bp.begin(); itr != bp.end(); itr = bp.erase(itr))
        {
            auto v = *itr;
            auto u = eval(v);
            if(semi.at(u) < semi.at(v))
            {
                dom.at(v) = u;
            }
            else
            {
                dom.at(v) = parent.at(w);
            }
        }
    }
    // step4
    for(duint i = 2; i != n; i++)
    {
        auto w = vertex.at(i);
        if(dom.at(w) != vertex.at(semi.at(w)))
        {
            dom[w] = dom[dom[w]];
        }
    }
    dom.at(r) = 0;
    print("semi", semi);
    //succ(v)
    for(const auto & d : dom)
    {
        dprintf("dom[%d] = %d\n", d.first, d.second);
    }

    for(const auto & x : indexToAddress)
    {
        char label[256];
        sprintf_s(label, "block%p", (void*)x.first);
        LabelSet(x.second, label, false, true);
    }
}
