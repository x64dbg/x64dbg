#pragma once

#include "analysis.h"

class AdvancedAnalysis : public Analysis
{
public:
    explicit AdvancedAnalysis(duint base, duint size, bool dump = false);
    ~AdvancedAnalysis();
    void Analyse() override;
    void SetMarkers() override;

    using UintSet = std::unordered_set<duint>;

    template<class T>
    using UintMap = std::unordered_map<duint, T>;

    struct CFNode
    {
        duint parentGraph; //function of which this node is a part
        duint start; //start of the block
        duint end; //end of the block (inclusive)
        duint brtrue; //destination if condition is true
        duint brfalse; //destination if condition is false
        duint icount; //number of instructions in node
        bool terminal; //node is a RET

        explicit CFNode(duint parentGraph, duint start, duint end)
            : parentGraph(parentGraph),
              start(start),
              end(end),
              brtrue(0),
              brfalse(0),
              icount(0),
              terminal(false)
        {
        }

        explicit CFNode()
            : CFNode(0, 0, 0)
        {
        }

        String ToString() const
        {
            return StringUtils::sprintf("start: %p\nend: %p\nfunction: %p", start, end, parentGraph);
        }
    };

    struct CFGraph
    {
        duint entryPoint; //graph entry point
        UintMap<CFNode> nodes; //CFNode.start -> CFNode
        UintMap<UintSet> parents; //CFNode.start -> parents

        explicit CFGraph(duint entryPoint)
            : entryPoint(entryPoint)
        {
        }

        void AddNode(const CFNode & node)
        {
            nodes[node.start] = node;
            AddParent(node.start, node.brtrue);
            AddParent(node.start, node.brfalse);
        }

        void AddParent(duint child, duint parent)
        {
            if(!child || !parent)
                return;
            auto found = parents.find(child);
            if(found == parents.end())
            {
                UintSet p;
                p.insert(parent);
                parents[child] = p;
            }
            else
                found->second.insert(parent);
        }

        const char* GetNodeColor(const CFNode & node) const
        {
            if(node.terminal)
                return "firebrick";
            if(node.start == entryPoint)
                return "forestgreen";
            return "lightblue";
        }

        String ToDot() const
        {
            String result = "digraph CFGraph {\n";
            for(const auto & node : nodes)
                result += StringUtils::sprintf("    n%p[label=\"%s\" style=filled fillcolor=%s shape=box]\n",
                                               node.second.start,
                                               node.second.ToString().c_str(),
                                               GetNodeColor(node.second));
            result += "\n";
            for(const auto & node : nodes)
            {
                if(node.second.brtrue)
                    result += StringUtils::sprintf("    n%p-> n%p [color=green]\n",
                                                   node.second.start,
                                                   node.second.brtrue);
                if(node.second.brfalse)
                    result += StringUtils::sprintf("    n%p-> n%p [color=red]\n",
                                                   node.second.start,
                                                   node.second.brfalse);
            }
            result += "\n";

            for(const auto & parent : parents)
            {
                for(const auto & node : parent.second)
                    result += StringUtils::sprintf("    n%p-> n%p [style=dotted color=grey]\n",
                                                   node,
                                                   parent.first);
            }
            result += "}";
            return result;
        }
    };

protected:
    struct XREF
    {
        duint addr;
        duint from;
        XREFTYPE type;
        bool valid;
    };

    std::unordered_set<duint> mEntryPoints;
    std::unordered_set<duint> mCandidateEPs;
    std::unordered_set<duint> mFuzzyEPs;
    std::vector<CFGraph> mFunctions;
    std::unordered_map<duint, std::vector<XREF>> mXrefs;
    uint8_t* mEncMap = nullptr;
private:

    bool mDump;
    void linearXrefPass();
    void findInvalidXrefs();
    void writeDataXrefs();

    void findFuzzyEntryPoints();
    void findEntryPoints();
    void analyzeCandidateFunctions(bool writedata);
    void analyzeFunction(duint entryPoint, bool writedata);
};