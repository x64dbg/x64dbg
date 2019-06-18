#pragma once

#include "AbstractSearchList.h"
#include "StdTable.h"

class StdTableSearchList : public AbstractSearchList
{
public:
    friend class StdSearchListView;

    StdTableSearchList() : StdTableSearchList(new StdTable(), new StdTable()) { }
    StdTableSearchList(StdTable* list, StdTable* searchList) : mList(list), mSearchList(searchList) { }
    ~StdTableSearchList() { delete mList; delete mSearchList; }

    void lock() override { }
    void unlock() override { }
    AbstractStdTable* list() const override { return mList; }
    AbstractStdTable* searchList() const override { return mSearchList; }

    void filter(const QString & filter, FilterType type, int startColumn) override;

private:
    StdTable* mList;
    StdTable* mSearchList;
};
