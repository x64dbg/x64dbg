#ifndef _GRAPH_H
#define _GRAPH_H

typedef struct
{
    duint parentGraph; //function of which this node is a part
    duint start; //start of the block
    duint end; //end of the block (inclusive)
    duint brtrue; //destination if condition is true
    duint brfalse; //destination if condition is false
    duint icount; //number of instructions in node
    bool terminal; //node is a RET
    bool split; //node is a split (brtrue points to the next node)
    void* userdata; //user data
    ListInfo exits; //exits (including brtrue and brfalse, duint)
    ListInfo data; //block data
} BridgeCFNodeList;

typedef struct
{
    duint entryPoint; //graph entry point
    void* userdata; //user data
    ListInfo nodes; //graph nodes (BridgeCFNodeList)
} BridgeCFGraphList;

#ifdef __cplusplus
#if _MSC_VER > 1300

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <utility>

struct BridgeCFNode
{
    duint parentGraph; //function of which this node is a part
    duint start; //start of the block
    duint end; //end of the block (inclusive)
    duint brtrue; //destination if condition is true
    duint brfalse; //destination if condition is false
    duint icount; //number of instructions in node
    bool terminal; //node is a RET
    bool split; //node is a split (brtrue points to the next node)
    void* userdata; //user data
    std::vector<duint> exits; //exits (including brtrue and brfalse)
    std::vector<unsigned char> data; //block data

    explicit BridgeCFNode(BridgeCFNodeList* nodeList, bool freedata = true)
    {
        if(!nodeList)
            __debugbreak();
        parentGraph = nodeList->parentGraph;
        start = nodeList->start;
        end = nodeList->end;
        brtrue = nodeList->brtrue;
        brfalse = nodeList->brfalse;
        icount = nodeList->icount;
        terminal = nodeList->terminal;
        split = nodeList->split;
        userdata = nodeList->userdata;
        if(!BridgeList<duint>::ToVector(&nodeList->exits, exits, freedata))
            __debugbreak();
        if(!BridgeList<unsigned char>::ToVector(&nodeList->data, data, freedata))
            __debugbreak();
    }

    explicit BridgeCFNode(duint parentGraph, duint start, duint end)
        : parentGraph(parentGraph),
          start(start),
          end(end),
          brtrue(0),
          brfalse(0),
          icount(0),
          terminal(false),
          split(false),
          userdata(nullptr)
    {
    }

    explicit BridgeCFNode()
        : BridgeCFNode(0, 0, 0)
    {
    }

    BridgeCFNodeList ToNodeList() const
    {
        BridgeCFNodeList out;
        out.parentGraph = parentGraph;
        out.start = start;
        out.end = end;
        out.brtrue = brtrue;
        out.brfalse = brfalse;
        out.icount = icount;
        out.terminal = terminal;
        out.split = split;
        out.userdata = userdata;
        BridgeList<duint>::CopyData(&out.exits, exits);
        BridgeList<unsigned char>::CopyData(&out.data, data);
        return std::move(out);
    }
};

struct BridgeCFGraph
{
    duint entryPoint; //graph entry point
    void* userdata; //user data
    std::unordered_map<duint, BridgeCFNode> nodes; //CFNode.start -> CFNode
    std::unordered_map<duint, std::unordered_set<duint>> parents; //CFNode.start -> parents

    explicit BridgeCFGraph(BridgeCFGraphList* graphList, bool freedata = true)
    {
        if(!graphList || graphList->nodes.size != graphList->nodes.count * sizeof(BridgeCFNodeList))
            __debugbreak();
        entryPoint = graphList->entryPoint;
        userdata = graphList->userdata;
        auto data = (BridgeCFNodeList*)graphList->nodes.data;
        for(int i = 0; i < graphList->nodes.count; i++)
            AddNode(BridgeCFNode(&data[i], freedata));
        if(freedata && data)
            BridgeFree(data);
    }

    explicit BridgeCFGraph(duint entryPoint)
        : entryPoint(entryPoint),
          userdata(nullptr)
    {
    }

    void AddNode(const BridgeCFNode & node)
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
            parents[child] = std::unordered_set<duint>(std::initializer_list<duint> { parent });
        else
            found->second.insert(parent);
    }

    BridgeCFGraphList ToGraphList() const
    {
        BridgeCFGraphList out;
        out.entryPoint = entryPoint;
        out.userdata = userdata;
        std::vector<BridgeCFNodeList> nodeList;
        nodeList.reserve(nodes.size());
        for(const auto & nodeIt : nodes)
            nodeList.push_back(nodeIt.second.ToNodeList());
        BridgeList<BridgeCFNodeList>::CopyData(&out.nodes, nodeList);
        return std::move(out);
    }
};

#endif //_MSC_VER
#endif //__cplusplus

#endif //_GRAPH_H