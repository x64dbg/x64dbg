#ifndef _TREE_H
#define _TREE_H

#include "Node.h"

#include <vector>
#include <memory>
#include <map>
#include <QMessageBox>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include "Imports.h"

using namespace std;

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
        ogdf::node ogdfNode = _G->newNode();
        Node<T> *node = new Node<T>(_G, _GA, ogdfNode, data);
        _ogdfDataMap[ogdfNode] = node;
        _nodePool.push_back(unique_ptr<Node<T>>(node));
        return node;
    }
    Node<T>* findNode(ogdf::node ogdfNode)
    {
        auto found = _ogdfDataMap.find(ogdfNode);
        return found != _ogdfDataMap.end() ? found->second : nullptr;
    }
    Node<T>* findNodeByAddress(duint address)
    {
        auto it = std::find_if(_ogdfDataMap.begin(), _ogdfDataMap.end(), [address](const pair<ogdf::node, Node<T>* > &itr)
        {
            return itr.second->data()->address() == address;
        });

        // If found
        if(it != _ogdfDataMap.end())
        {
            return it->second;
        }
        else
        {
            return nullptr;
        }
    }

    void clear()
    {
        for(duint i=0; i < _nodePool.size(); i++)
            _nodePool[i].reset();

        _nodePool.clear();

        map<ogdf::node, Node<T>*>::iterator it = _ogdfDataMap.begin();
        for(it; it != _ogdfDataMap.end(); it++)
            delete it->second;

        _ogdfDataMap.clear();
    }

private:
    ogdf::Graph* _G;
    ogdf::GraphAttributes* _GA;
    vector<unique_ptr<Node<T>>> _nodePool;
    map<ogdf::node, Node<T>*> _ogdfDataMap;
};

#endif //_TREE_H
