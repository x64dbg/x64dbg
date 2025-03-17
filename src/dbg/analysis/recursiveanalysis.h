#pragma once

#include <unordered_set>
#include <unordered_map>

#include "analysis.h"

class RecursiveAnalysis : public Analysis
{
public:
    explicit RecursiveAnalysis(duint base, duint size, duint entryPoint, bool usePlugins, bool dump = false);
    void Analyse() override;
    void SetMarkers() override;

    using UintSet = std::unordered_set<duint>;

    template<class T>
    using UintMap = std::unordered_map<duint, T>;

    using CFNode = BridgeCFNode;
    using CFGraph = BridgeCFGraph;

    static String NodeToString(const CFNode & n)
    {
#ifdef _WIN64
        return StringUtils::sprintf("start: %p, %lld\nend: %p\nfunction: %p", n.start, n.icount, n.end, n.parentGraph); //TODO: %llu or %u
#else //x86
        return StringUtils::sprintf("start: %p, %d\nend: %p\nfunction: %p", n.start, n.icount, n.end, n.parentGraph); //TODO: %llu or %u
#endif //_WIN64
    }

    static const char* GetNodeColor(const CFGraph & graph, const CFNode & node)
    {
        if(node.terminal)
            return "firebrick";
        if(node.start == graph.entryPoint)
            return "forestgreen";
        return "lightblue";
    }

    static String GraphToDot(const CFGraph & graph)
    {
        String result = "digraph CFGraph {\n";
        for(const auto & node : graph.nodes)
            result += StringUtils::sprintf("    n%p[label=\"%s\" style=filled fillcolor=%s shape=box]\n",
                                           node.second.start,
                                           NodeToString(node.second).c_str(),
                                           GetNodeColor(graph, node.second));
        result += "\n";
        for(const auto & node : graph.nodes)
        {
            if(node.second.brtrue)
                result += StringUtils::sprintf("    n%p-> n%p [color=%s]\n",
                                               node.second.start,
                                               node.second.brtrue,
                                               node.second.split ? "black" : "green");
            if(node.second.brfalse)
                result += StringUtils::sprintf("    n%p-> n%p [color=red]\n",
                                               node.second.start,
                                               node.second.brfalse);
        }
        result += "\n";

        for(const auto & parent : graph.parents)
        {
            for(const auto & node : parent.second)
                result += StringUtils::sprintf("    n%p-> n%p [style=dotted color=grey]\n",
                                               node,
                                               parent.first);
        }
        result += "}";
        return std::move(result);
    }

    const CFGraph* GetFunctionGraph(duint entry) const
    {
        auto itr = mFunctions.find(entry);
        return itr == mFunctions.end() ? nullptr : &itr->second;
    }

protected:
    duint mEntryPoint;
    std::unordered_map<duint, CFGraph> mFunctions;

private:
    bool mUsePlugins;
    bool mDump;

    struct XREF
    {
        duint addr;
        duint from;
    };

    std::vector<XREF> mXrefs;

    struct LoopInfo
    {
        duint functionEntry = 0;
        std::unordered_set<duint> trivialLoops; // loops to the same basic block
        std::unordered_map<duint, duint> backedges; // backedges in the CFG
    };

    std::unordered_map<duint, LoopInfo> mLoopInfo;

    void analyzeFunction(duint entryPoint);
    void analyzeLoops(duint entryPoint);
    void dominatorAnalysis(duint entryPoint);
};