#include "CodeFolding.h"

CodeFoldingHelper::CodeFoldingHelper()
{

}

CodeFoldingHelper::~CodeFoldingHelper()
{

}

/**
 * @brief     Whether va is the beginning ot of a code folding segment.
 * @param va  The virtual address
 * @return    True if va satifies condition.
 */
bool CodeFoldingHelper::isFoldStart(duint va) const
{
    const FoldTree* temp = getFoldTree(va);
    return temp != nullptr && va == temp->range.first;
}

/**
 * @brief    Whether va is inside a code folding segment.
 * @param va The virtual address.
 * @return   True if va satisfies condition.
 */
bool CodeFoldingHelper::isFoldBody(duint va) const
{
    return root.find(Range(va, va)) != root.cend();
}

/**
 * @brief    Whether va is the end of a code folding segment.
 * @param va The virtual address.
 * @return   True if va satisfies condition.
 */
bool CodeFoldingHelper::isFoldEnd(duint va) const
{
    const FoldTree* temp = getFoldTree(va);
    return temp != nullptr && va == temp->range.second;
}

/**
 * @brief    Whether va is folded and should be invisible.
 * @param va The virtual address.
 * @return   True if va satisfies condition.
 */
bool CodeFoldingHelper::isFolded(duint va) const
{
    const FoldTree* temp = getFoldTree(va);
    return temp != nullptr && temp->folded;
}

/**
 * @brief    Whether there are some code folding segments within the specified range.
 * @param vaStart The beginning virtual address of the data range.
 * @param vaEnd   The ending virtual address of the data range.
 * @return   True if such code folding segment present.
 */
bool CodeFoldingHelper::isDataRangeFolded(duint vaStart, duint vaEnd) const
{
    CompareFunc less;
    Range lower(vaStart, vaStart);
    Range upper(vaEnd, vaEnd);
    auto iteratorStart = root.upper_bound(lower);
    auto iteratorEnd = root.lower_bound(upper);
    return (iteratorStart != root.cend() && less(iteratorStart->first, upper)) && (iteratorEnd != root.cend() && less(lower, iteratorEnd->first));
}

/**
 * @brief    Get the beginning virtual address of the code folding segment.
 * @param va The virtual address which lies within the code folding segment.
 * @return   The beginning virtual address, or 0 if such code folding segment is not found.
 */
duint CodeFoldingHelper::getFoldBegin(duint va) const
{
    const FoldTree* temp = getFirstFoldedTree(va);
    return temp == nullptr ? 0 : temp->range.first;
}

/**
 * @brief    Get the ending virtual address of the code folding segment.
 * @param va The virtual address which lies within the code folding segment.
 * @return   The ending virtual address, or 0 if such code folding segment is not found.
 */
duint CodeFoldingHelper::getFoldEnd(duint va) const
{
    const FoldTree* temp = getFirstFoldedTree(va);
    return temp == nullptr ? 0 : temp->range.second;
}

/**
 * @brief    Get the total bytes within range that should be invisible due to code folding.
 * @param vaStart  The beginning virtual address of the range.
 * @param vaEnd    The ending virtual address of the range.
 * @return   The total number bytes that are folded.
 */
duint CodeFoldingHelper::getFoldedSize(duint vaStart, duint vaEnd) const
{
    CompareFunc less;
    Range keyLower(vaStart, vaStart);
    Range keyUpper(vaEnd, vaEnd);
    duint size = 0;
    auto iteratorLower = root.upper_bound(keyLower);
    auto iteratorUpper = root.lower_bound(keyUpper);
    if(iteratorLower == root.cend() || iteratorUpper == root.cend() || less(iteratorUpper->first, iteratorLower->first))
        return 0;
    for(auto i = iteratorLower; i != iteratorUpper; i ++)
        size += getFoldedSize(&i->second);
    return size;
}

/**
 * @brief    Internal. Get the total bytes that should be invisible due to code folding withing a code folding segment.
 * @param node The code folding segment.
 * @return   Number of bytes folded.
 */
duint CodeFoldingHelper::getFoldedSize(const FoldTree* node) const
{
    if(node->folded)
        return node->range.second - node->range.first;
    else
    {
        duint size = 0;
        for(auto i = node->children.cbegin(); i != node->children.cend(); i++)
            size += getFoldedSize(&i->second);
        return size;
    }
}

/**
 * @brief    Set the folding state of a code folding segment.
 * @param va The virtual address within the specified code folding segment.
 * @param folded  True if the segment should be folded, false otherwise.
 */
void CodeFoldingHelper::setFolded(duint va, bool folded)
{
    FoldTree* temp = getFoldTree(va);
    if(temp)
        temp->folded = folded;
}

/**
 * @brief    Add a code folding segment.
 * @param va The beginning virtual address of the new code folding segment.
 * @param length The total number of bytes within the new code folding segment.
 * @param folded The initial state of the new code folding segment.
 * @return   True if the operation succeeded.
 * @remark  It cannot add a new code folding segment that overlaps an existing code folding segments.
 */
bool CodeFoldingHelper::addFoldSegment(duint va, duint length, bool folded)
{
    FoldTree* temp = getFoldTree(va);
    FoldTree* temp2 = getFoldTree(va + length);
    if(temp != temp2)
        return false;
    if(isDataRangeFolded(va, va + length))
    {
        // There's some folded code inside the range
        return false;
    }
    auto map = (temp == nullptr) ? &root : &temp->children;
    auto result = map->insert(std::make_pair(std::make_pair(va, va + length), FoldTree(folded, va, va + length)));
    return result.second;
}

/**
 * @brief    Deletes an existing code folding segment.
 * @param va The virtual address within the specified code folding segment.
 * @return   True if such code folding segment is found and successfully deleted.
 */
bool CodeFoldingHelper::delFoldSegment(duint va)
{
    Range key(va, va);
    auto temp1 = root.find(key);
    if(temp1 == root.cend())
        return false;
    auto parent = &root;
    do
    {
        auto temp2 = temp1->second.children.find(key);
        if(temp2 == temp1->second.children.cend())
            break;
        parent = &temp1->second.children;
        temp1 = temp2;
    }
    while(true);
    parent->erase(temp1);
    return true;
}

const CodeFoldingHelper::FoldTree* CodeFoldingHelper::getFoldTree(duint va) const
{
    Range key(va, va);
    auto temp1 = root.find(key);
    if(temp1 == root.cend())
        return nullptr;
    do
    {
        auto temp2 = temp1->second.children.find(key);
        if(temp2 == temp1->second.children.cend())
            break;
        temp1 = temp2;
    }
    while(true);
    return &temp1->second;
}

const CodeFoldingHelper::FoldTree* CodeFoldingHelper::getFirstFoldedTree(duint va) const
{
    Range key(va, va);
    auto temp1 = root.find(key);
    if(temp1 == root.cend())
        return nullptr;
    while(!temp1->second.folded)
    {
        auto temp2 = temp1->second.children.find(key);
        if(temp2 == temp1->second.children.cend())
            break;
        temp1 = temp2;
    }
    return &temp1->second;
}

CodeFoldingHelper::FoldTree* CodeFoldingHelper::getFoldTree(duint va)
{
    Range key(va, va);
    auto temp1 = root.find(key);
    if(temp1 == root.cend())
        return nullptr;
    do
    {
        auto temp2 = temp1->second.children.find(key);
        if(temp2 == temp1->second.children.cend())
            break;
        temp1 = temp2;
    }
    while(true);
    return &temp1->second;
}

CodeFoldingHelper::FoldTree* CodeFoldingHelper::getFirstFoldedTree(duint va)
{
    Range key(va, va);
    auto temp1 = root.find(key);
    if(temp1 == root.cend())
        return nullptr;
    while(!temp1->second.folded)
    {
        auto temp2 = temp1->second.children.find(key);
        if(temp2 == temp1->second.children.cend())
            break;
        temp1 = temp2;
    }
    return &temp1->second;
}

/**
 * @brief    Expands appropriate code folding segments so that the specified virtual address is unfolded.
 * @param va The specified virtual address.
 */
void CodeFoldingHelper::expandFoldSegment(duint va)
{
    Range key(va, va);
    auto temp1 = root.find(key);
    if(temp1 == root.cend())
        return;
    temp1->second.folded = false;
    do
    {
        auto temp2 = temp1->second.children.find(key);
        if(temp2 == temp1->second.children.cend())
            break;
        temp2->second.folded = false;
        temp1 = temp2;
    }
    while(true);
}

bool CodeFoldingHelper::CompareFunc::operator()(const CodeFoldingHelper::Range & lhs, const CodeFoldingHelper::Range & rhs) const
{
    return lhs.second < rhs.first;
}
