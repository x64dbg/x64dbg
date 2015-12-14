#ifndef _TREE_H
#define _TREE_H

#include "Node.h"

#include <vector>
#include <memory>
#include <map>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

template <typename T>
class Tree
{
public:
    explicit Tree(ogdf::Graph* G, ogdf::GraphAttributes* GA)
    {
        this->_G = G;
        this->_GA = GA;
    }

    Node<T>* newNode(T data)
    {
        auto ogdfNode = _G->newNode();
        auto node = new Node<T>(_G, _GA, ogdfNode, data);
        _ogdfDataMap[ogdfNode] = node;
        _nodePool.push_back(std::unique_ptr<Node<T>>(node));
        return node;
    }

    Node<T>* findNode(ogdf::node ogdfNode)
    {
        auto found = _ogdfDataMap.find(ogdfNode);
        return found != _ogdfDataMap.end() ? found->second : nullptr;
    }

private:
    ogdf::Graph* _G;
    ogdf::GraphAttributes* _GA;
    std::vector<std::unique_ptr<Node<T>>> _nodePool;
    std::map<ogdf::node, Node<T>*> _ogdfDataMap;
};

#endif //_TREE_H
