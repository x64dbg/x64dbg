#pragma once

#include "SearchListView.h"
#include "StdTableSearchList.h"

class StdSearchListView : public SearchListView
{
    Q_OBJECT
public:
    explicit StdSearchListView(QWidget* parent = nullptr);
    StdSearchListView(QWidget* parent, bool enableRegex, bool enableLock);
    StdSearchListView(QWidget* parent, bool enableRegex, bool enableLock, StdTableSearchList* tableSearchList);
    ~StdSearchListView() override;

    void setInternalTitle(const QString & title);
    int getCharWidth();
    void addColumnAt(int width, QString title, bool isClickable, QString copyTitle = "", StdTable::SortBy::t sortFn = StdTable::SortBy::AsText);
    void setDrawDebugOnly(bool value);
    void enableMultiSelection(bool enabled);
    void setAddressColumn(int col, bool cipBase = false);
    void loadColumnFromConfig(const QString & viewName);
    virtual void setRowCount(duint count);
    void setCellContent(duint row, duint column, QString s);
    void setCellUserdata(duint row, duint column, duint userdata);
    void setSearchStartCol(duint column);

public slots:
    void reloadData();

private:
    StdTableSearchList* mSearchListData;

protected:
    friend class SymbolView;
    friend class Bridge;
    friend class HandlesView;

    StdTable* stdList();
    StdTable* stdSearchList();
};
