#pragma once

#include "Imports.h"
#include <map>

class CodeFoldingHelper
{
public:
    CodeFoldingHelper();
    ~CodeFoldingHelper();

    bool isFoldStart(duint va) const;
    bool isFoldBody(duint va) const;
    bool isFoldEnd(duint va) const;
    bool isFolded(duint va) const;
    bool isDataRangeFolded(duint vaStart, duint vaEnd) const;
    duint getFoldBegin(duint va) const;
    duint getFoldEnd(duint va) const;
    duint getFoldedSize(duint vaStart, duint vaEnd) const;
    void setFolded(duint va, bool folded);
    bool addFoldSegment(duint va, duint length, bool folded = true);
    bool delFoldSegment(duint va);
    void expandFoldSegment(duint va);

protected:
    typedef std::pair<duint, duint> Range;
    class CompareFunc
    {
    public:
        bool operator()(const Range & lhs, const Range & rhs) const;
    };
    class FoldTree
    {
    public:
        FoldTree(bool _folded, duint start, duint end) : folded(_folded), range(std::make_pair(start, end)) {}
        bool folded;
        Range range;
        std::map<Range, FoldTree, CompareFunc> children;
    };

    std::map<Range, FoldTree, CompareFunc> root;

    const FoldTree* getFoldTree(duint va) const;
    FoldTree* getFoldTree(duint va);
    const FoldTree* getFirstFoldedTree(duint va) const;
    FoldTree* getFirstFoldedTree(duint va);
    duint getFoldedSize(const FoldTree* node) const;
};
