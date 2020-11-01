#ifndef ABSTRACTSEARCHLIST_H
#define ABSTRACTSEARCHLIST_H

#include "AbstractStdTable.h"

class AbstractSearchList
{
public:
    enum FilterType
    {
        FilterStartsWithTextCaseSensitive, //unused
        FilterStartsWithTextCaseInsensitive, //unused
        FilterContainsTextCaseSensitive, //unused
        FilterContainsTextCaseInsensitive,
        FilterRegexCaseSensitive,
        FilterRegexCaseInsensitive
    };

    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual AbstractStdTable* list() const = 0;
    virtual AbstractStdTable* searchList() const = 0;
    virtual void filter(const QString & filter, FilterType type, int startColumn) = 0;

    bool rowMatchesFilter(const QString & filter, FilterType type, int row, int startColumn) const
    {
        int count = list()->getColumnCount();
        if(startColumn + 1 > count)
            return false;
        auto cs = Qt::CaseInsensitive;
        switch(type)
        {
        case FilterStartsWithTextCaseSensitive:
            cs = Qt::CaseSensitive;
        case FilterStartsWithTextCaseInsensitive:
            for(int i = startColumn; i < count; i++)
                if(list()->getCellContentUnsafe(row, i).startsWith(filter, cs))
                    return true;
            break;
        case FilterContainsTextCaseSensitive:
            cs = Qt::CaseSensitive;
        case FilterContainsTextCaseInsensitive:
            for(int i = startColumn; i < count; i++)
                if(list()->getCellContentUnsafe(row, i).contains(filter, cs))
                    return true;
            break;
        case FilterRegexCaseSensitive:
            cs = Qt::CaseSensitive;
        case FilterRegexCaseInsensitive:
            for(int i = startColumn; i < count; i++)
                if(list()->getCellContentUnsafe(row, i).contains(QRegExp(filter, cs)))
                    return true;
            break;
        default:
            __debugbreak();
        }
        return false;
    }
};

#endif // ABSTRACTSEARCHLIST_H
