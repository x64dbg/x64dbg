#pragma once

#include "analysis.h"

class RecursiveAnalysis : public Analysis
{
public:
    explicit RecursiveAnalysis(duint base, duint size, duint entryPoint, duint maxDepth, bool dump = false);
    void Analyse() override;
    void SetMarkers() override;

    using UintSet = std::unordered_set<duint>;

    template<class T>
    using UintMap = std::unordered_map<duint, T>;

    using CFNode = BridgeCFNode;
    using CFGraph = BridgeCFGraph;

    static String NodeToString(const CFNode & n)
    {
        return StringUtils::sprintf("start: " fhex ", %" fext "d\nend: " fhex "\nfunction: " fhex, n.start, n.icount, n.end, n.parentGraph);
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
            result += StringUtils::sprintf("    n" fhex "[label=\"%s\" style=filled fillcolor=%s shape=box]\n",
                                           node.second.start,
                                           NodeToString(node.second).c_str(),
                                           GetNodeColor(graph, node.second));
        result += "\n";
        for(const auto & node : graph.nodes)
        {
            if(node.second.brtrue)
                result += StringUtils::sprintf("    n" fhex "-> n" fhex " [color=%s]\n",
                                               node.second.start,
                                               node.second.brtrue,
                                               node.second.split ? "black" : "green");
            if(node.second.brfalse)
                result += StringUtils::sprintf("    n" fhex "-> n" fhex " [color=red]\n",
                                               node.second.start,
                                               node.second.brfalse);
        }
        result += "\n";

        for(const auto & parent : graph.parents)
        {
            for(const auto & node : parent.second)
                result += StringUtils::sprintf("    n" fhex "-> n" fhex " [style=dotted color=grey]\n",
                                               node,
                                               parent.first);
        }
        result += "}";
        return result;
    }

    const CFGraph* GetFunctionGraph(duint entry) const
    {
        for(const auto & function : mFunctions)
            if(function.entryPoint == entry)
                return &function;
        return nullptr;
    }

protected:
    duint mEntryPoint;
    std::vector<CFGraph> mFunctions;

private:
    duint mMaxDepth;
    bool mDump;

    struct XREF
    {
        duint addr;
        duint from;
    };

    std::vector<XREF> mXrefs;

    void analyzeFunction(duint entryPoint);
};