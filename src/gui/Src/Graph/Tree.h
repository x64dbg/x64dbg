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


template <typename T>
class Tree
{
public:
    explicit Tree(ogdf::Graph* G, ogdf::GraphAttributes* GA) : mG(G), mGA(GA)
    {}

    Node<T>* newNode(T data)
    {
        ogdf::node ogdfNode = mG->newNode();
        auto node = new Node<T>(mG, mGA, ogdfNode, data);
        mOgdfDataMap[ogdfNode] = node;
        mNodePool.push_back(std::unique_ptr<Node<T>>(node));
        return node;
    }
    Node<T>* findNode(ogdf::node ogdfNode)
    {
        auto found = mOgdfDataMap.find(ogdfNode);
        return found != mOgdfDataMap.end() ? found->second : nullptr;
    }
    Node<T>* findNodeByAddress(duint address)
    {
        auto it = std::find_if(mOgdfDataMap.begin(), mOgdfDataMap.end(), [address](const std::pair<ogdf::node, Node<T>*> & itr)
        {
            return itr.second && itr.second->data() && itr.second->data()->address() == address;
        });

        // If found
        if(it != mOgdfDataMap.end())
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
        for(duint i = 0; i < mNodePool.size(); i++)
            mNodePool[i].reset();

        mNodePool.clear();
        mOgdfDataMap.clear();
    }

private:
    ogdf::Graph* mG;
    ogdf::GraphAttributes* mGA;
    std::vector<std::unique_ptr<Node<T>>> mNodePool;
    std::map<ogdf::node, Node<T>*> mOgdfDataMap;
};

#endif //_TREE_H
