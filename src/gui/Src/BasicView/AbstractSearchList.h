#pragma once

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
        FilterRegexCaseInsensitive,
    };

    virtual ~AbstractSearchList() = default;
    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual AbstractStdTable* list() const = 0;
    virtual AbstractStdTable* searchList() const = 0;
    virtual void filter(const QString & filter, FilterType type, duint startColumn) = 0;

    bool rowMatchesFilter(const QString & filter, FilterType type, duint row, duint startColumn) const
    {
        auto count = list()->getColumnCount();
        if(startColumn + 1 > count)
            return false;
        auto cs = Qt::CaseInsensitive;
        switch(type)
        {
        case FilterStartsWithTextCaseSensitive:
            cs = Qt::CaseSensitive;
        case FilterStartsWithTextCaseInsensitive:
            for(duint i = startColumn; i < count; i++)
                if(list()->getCellContent(row, i).startsWith(filter, cs))
                    return true;
            break;
        case FilterContainsTextCaseSensitive:
            cs = Qt::CaseSensitive;
        case FilterContainsTextCaseInsensitive:
            for(duint i = startColumn; i < count; i++)
                if(list()->getCellContent(row, i).contains(filter, cs))
                    return true;
            break;
        case FilterRegexCaseSensitive:
            cs = Qt::CaseSensitive;
        case FilterRegexCaseInsensitive:
            for(duint i = startColumn; i < count; i++)
                if(list()->getCellContent(row, i).contains(QRegExp(filter, cs)))
                    return true;
            break;
        default:
            __debugbreak();
        }
        return false;
    }
};
