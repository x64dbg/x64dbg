#include "advancedanalysis.h"
#include <queue>
#include "console.h"
#include "filehelper.h"
#include "function.h"
#include "xrefs.h"
#include "encodemap.h"

AdvancedAnalysis::AdvancedAnalysis(duint base, duint size, bool dump)
    : Analysis(base, size),
      mDump(dump)
{
    mEncMap = new uint8_t[size];
    memset(mEncMap, 0, size);
}

AdvancedAnalysis::~AdvancedAnalysis()
{
    delete[] mEncMap;
}

void AdvancedAnalysis::Analyse()
{
    linearXrefPass();
    findEntryPoints();
    analyzeCandidateFunctions(true);
    findFuzzyEntryPoints();
    analyzeCandidateFunctions(true);
    findInvalidXrefs();
    writeDataXrefs();
}

void AdvancedAnalysis::SetMarkers()
{
    if(mDump)
        for(const auto & function : mFunctions)
            FileHelper::WriteAllText(StringUtils::sprintf("cfgraph_%p.dot", function.entryPoint), function.ToDot());

    duint encMapSize;
    uint8_t* buffer = (uint8_t*)EncodeMapGetBuffer(mBase, &encMapSize, true);
    memcpy(buffer, mEncMap, encMapSize);
    EncodeMapReleaseBuffer(buffer);

    XrefDelRange(mBase, mBase + mSize - 1);
    for(const auto & vec : mXrefs)
    {
        for(const auto & xref : vec.second)
        {
            if(xref.valid)
                XrefAdd(xref.addr, xref.from);
        }
    }

    FunctionClear();
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

void AdvancedAnalysis::analyzeFunction(duint entryPoint, bool writedata)
{
    //BFS through the disassembly starting at entryPoint
    CFGraph graph(entryPoint);
    UintSet visited;
    std::queue<duint> queue;
    visited.reserve(queue.size());
    mEntryPoints.insert(entryPoint);
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
            if(!mZydis.Disassemble(node.end, translateAddr(node.end)))
            {
                if(writedata)
                    mEncMap[node.end - mBase] = (uint8_t)enc_byte;
                // If the next byte would be out of the memory range finish this node
                if(!inRange(node.end + 1))
                {
                    graph.AddNode(node);
                    break;
                }
                node.end++;
                continue;
            }
            // If the memory range doesn't fit the entire instruction
            // mark it as bytes and finish this node
            if(!inRange(node.end + mZydis.Size() - 1))
            {
                duint remainingSize = mBase + mSize - node.end;
                memset(&mEncMap[node.end - mBase], (uint8_t)enc_byte, remainingSize);
                graph.AddNode(node);
                break;
            }
            if(writedata)
            {
                mEncMap[node.end - mBase] = (uint8_t)enc_code;
                for(size_t i = 1; i < mZydis.Size(); i++)
                    mEncMap[node.end - mBase + i] = (uint8_t)enc_middle;
            }
            if(mZydis.IsJump() || mZydis.IsLoop()) //jump
            {
                //set the branch destinations
                node.brtrue = (duint)mZydis.BranchDestination();
                if(mZydis.GetId() != ZYDIS_MNEMONIC_JMP) //unconditional jumps dont have a brfalse
                    node.brfalse = node.end + mZydis.Size();

                //add node to the function graph
                graph.AddNode(node);

                //enqueue branch destinations
                if(node.brtrue)
                    queue.push(node.brtrue);
                if(node.brfalse)
                    queue.push(node.brfalse);

                break;
            }
            if(mZydis.IsCall()) //call
            {
                //TODO: handle no return
                auto target = (duint)mZydis.BranchDestination();
                if(inRange(target) && mEntryPoints.find(target) == mEntryPoints.end())
                    mCandidateEPs.insert(target);
            }
            if(mZydis.IsRet()) //return
            {
                node.terminal = true;
                graph.AddNode(node);
                break;
            }
            // If this instruction finishes the memory range, end the loop for this entry point
            if(!inRange(node.end + mZydis.Size()))
            {
                graph.AddNode(node);
                break;
            }
            node.end += mZydis.Size();
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
        if(!mZydis.Disassemble(addr, translateAddr(addr)))
        {
            addr++;
            continue;
        }
        addr += mZydis.Size();

        XREF xref;
        xref.valid = true;
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
        {
            if(mZydis.IsCall())
                xref.type = XREF_CALL;
            else if(mZydis.IsJump())
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

    dprintf("%d xrefs found in %ums!\n", int(mXrefs.size()), GetTickCount() - ticks);
}

void AdvancedAnalysis::findInvalidXrefs()
{
    for(auto & vec : mXrefs)
    {
        duint jmps = 0, calls = 0;
        duint addr = vec.first;
        auto desttype = mEncMap[vec.first - mBase];
        for(auto & xref : vec.second)
        {
            auto type = mEncMap[xref.from - mBase];
            if(desttype == enc_code && type != enc_unknown && type != enc_code)
                xref.valid = false;
            else if(desttype == enc_middle)
                xref.valid = false;
        }
    }
}

bool isFloatInstruction(ZydisMnemonic opcode)
{
    switch(opcode)
    {
    case ZYDIS_MNEMONIC_FLD:
    case ZYDIS_MNEMONIC_FST:
    case ZYDIS_MNEMONIC_FSTP:
    case ZYDIS_MNEMONIC_FADD:
    case ZYDIS_MNEMONIC_FSUB:
    case ZYDIS_MNEMONIC_FSUBR:
    case ZYDIS_MNEMONIC_FMUL:
    case ZYDIS_MNEMONIC_FDIV:
    case ZYDIS_MNEMONIC_FDIVR:
    case ZYDIS_MNEMONIC_FCOM:
    case ZYDIS_MNEMONIC_FCOMP:

        return true;
    default:
        return false;
    }
}

void AdvancedAnalysis::writeDataXrefs()
{
    for(auto & vec : mXrefs)
    {
        for(auto & xref : vec.second)
        {
            if(xref.type == XREF_DATA && xref.valid)
            {
                if(!mZydis.Disassemble(xref.from, translateAddr(xref.from)))
                {
                    xref.valid = false;
                    continue;
                }
                auto opcode = mZydis.GetId();
                bool isfloat = isFloatInstruction(opcode);
                for(auto i = 0; i < mZydis.OpCount(); i++)
                {
                    auto & op = mZydis[i];
                    ENCODETYPE type = enc_unknown;

                    //Todo: Analyze op type and set correct type
                    if(op.type == ZYDIS_OPERAND_TYPE_MEMORY)
                    {
                        duint size = op.size / 8;
                        duint offset = xref.addr - mBase;
                        switch(op.size)
                        {
                        case 8:
                            type = enc_byte;
                            break;
                        case 16:
                            type = enc_word;
                            break;
                        case 32:
                            type = isfloat ? enc_real4 : enc_dword;
                            break;
                        case 48:
                            type = enc_fword;
                            break;
                        case 64:
                            type = isfloat ? enc_real8 : enc_qword;
                            break;
                        case 80:
                            type = isfloat ? enc_real10 : enc_tbyte;
                            break;
                        case 128:
                            type = enc_oword;
                            break;
                        case 256:
                            type = enc_ymmword;
                            break;
                        //case 512: type = enc_zmmword; break;
                        default:
                            __debugbreak();
                        }
                        if(size == 1)
                        {
                            mEncMap[offset] = (uint8_t)type;
                        }
                        else
                        {
                            // Check if the entire referenced data fits into the memory range
                            if((offset + size) <= mSize)
                            {
                                mEncMap[offset] = (uint8_t)type;
                                memset(mEncMap + offset + 1, (uint8_t)enc_middle, size - 1);
                            }
                            else
                            {
                                // If it doesn't fit, mark the remaining places as bytes
                                duint remainingSize = mSize - offset;
                                memset(mEncMap + offset, (uint8_t)enc_byte, size);
                            }
                        }
                    }
                }
            }
        }
    }
}

void AdvancedAnalysis::findFuzzyEntryPoints()
{
    mCandidateEPs.reserve(mFuzzyEPs.size());
    for(const auto & entryPoint : mFuzzyEPs)
    {
        mCandidateEPs.insert(entryPoint);
    }
}

void AdvancedAnalysis::findEntryPoints()
{
    duint modbase = ModBaseFromAddr(mBase);
    if(modbase)
    {
        apienumexports(modbase, [&](duint Base, const char* Module, const char* Name, duint Address)
        {
            // If within limits...
            if(inRange(Address))
            {
                mCandidateEPs.insert(Address);
            }
        });
    }


    for(const auto & vec : mXrefs)
    {
        duint jmps = 0, calls = 0;
        duint addr = vec.first;
        for(const auto & xref : vec.second)
        {
            if(xref.type == XREF_CALL)
                calls++;
            else if(xref.type == XREF_JMP)
                jmps++;
        }
        if(calls >= 1 && jmps + calls > 1)
            mCandidateEPs.insert(addr);
        else if(calls >= 1)
            mFuzzyEPs.insert(addr);
    }
}

void AdvancedAnalysis::analyzeCandidateFunctions(bool writedata)
{
    std::unordered_set<duint> pendingEPs;
    while(true)
    {
        pendingEPs.clear();
        if(mCandidateEPs.size() == 0)
            return;
        pendingEPs.reserve(mCandidateEPs.size());
        for(const auto & entryPoint : mCandidateEPs)
        {
            pendingEPs.insert(entryPoint);
        }
        mCandidateEPs.clear();

        for(const auto & entryPoint : pendingEPs)
        {
            if(mEntryPoints.find(entryPoint) == mEntryPoints.end())
            {
                analyzeFunction(entryPoint, true);
            }
        }
    }

}
