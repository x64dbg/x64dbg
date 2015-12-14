#ifndef _NODE_H
#define _NODE_H

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>

template <typename T>
class Node
{
public:
    explicit Node(ogdf::Graph* G, ogdf::GraphAttributes* GA, ogdf::node ogdfNode, T data)
    {
        this->_G = G;
        this->_GA = GA;
        this->_ogdfNode = ogdfNode;
        this->_data = data;
    }

    Node<T>* setLeft(Node* left)
    {
        return left ? this->_left = makeNodeWithEdge(left) : nullptr;
    }

    Node<T>* setRight(Node* right)
    {
        return right ? this->_right = makeNodeWithEdge(right) : nullptr;
    }

    Node<T>* setData(T data)
    {
        this->_data = data;
        return this;
    }

    Node<T>* left()
    {
        return this->_left;
    }

    Node<T>* right()
    {
        return this->_right;
    }

    T data()
    {
        return this->_data;
    }

    ogdf::node ogdfNode()
    {
        return this->_ogdfNode;
    }

private:
    Node<T>* _left;
    Node<T>* _right;
    T _data;
    ogdf::Graph* _G;
    ogdf::GraphAttributes* _GA;
    ogdf::node _ogdfNode;

    Node* makeNodeWithEdge(Node<T>* node)
    {
        auto edge = this->_G->newEdge(this->_ogdfNode, node->_ogdfNode);
        this->_GA->arrowType(edge) = ogdf::EdgeArrow::eaLast;
        return node;
    }
};

#endif //_NODE_H
