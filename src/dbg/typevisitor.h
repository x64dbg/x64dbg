#pragma once

#include "_global.h"
#include "types.h"

#include <functional>

extern int gDefaultMaxPtrDepth;
extern int gDefaultMaxExpandDepth;
extern int gDefaultMaxExpandArray;

struct NodeVisitor : Types::TypeManager::Visitor
{
    using AddNodeFn = std::function<void* (void* parent, const TYPEDESCRIPTOR* type)>;

    explicit NodeVisitor(AddNodeFn addNode, void* rootNode, duint addr)
        : mAddNode(std::move(addNode))
        , mRootNode(rootNode)
        , mAddr(addr)
    {
    }

    int mMaxPtrDepth = gDefaultMaxPtrDepth;
    int mMaxExpandDepth = gDefaultMaxExpandDepth;
    int mMaxExpandArray = gDefaultMaxExpandArray;
    bool mCreateLabels = false;

protected:
    bool visitType(const Types::Member & member, const Types::Typedef & type, const std::string & prettyType) override;
    bool visitStructUnion(const Types::Member & member, const Types::StructUnion & type, const std::string & prettyType) override;
    bool visitEnum(const Types::Member & member, const Types::Enum & num, const std::string & prettyType) override;
    bool visitArray(const Types::Member & member, const std::string & prettyType) override;
    bool visitPtr(const Types::Member & member, const Types::Typedef & type, const std::string & prettyType) override;
    bool visitBack(const Types::Member & member) override;

private:
    struct Parent
    {
        enum Type
        {
            Struct,
            Union,
            Array,
            Pointer
        };

        Type type;
        unsigned int arrayIndex = 0;
        duint addr = 0;
        duint offset = 0;
        void* node = nullptr;
        int size = 0;

        explicit Parent(Type type)
            : type(type)
        {
        }
    };

    Parent & parent()
    {
        return mParents.back();
    }

    void* parentNode() const
    {
        return mParents.empty() ? mRootNode : mParents.back().node;
    }

    AddNodeFn mAddNode;
    void* mRootNode = nullptr;

    std::vector<Parent> mParents;
    duint mOffset = 0;
    duint mBitOffset = 0;
    duint mAddr = 0;
    int mPtrDepth = 0;
    void* mNode = nullptr;
    std::vector<std::string> mPath;
};