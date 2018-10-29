#ifndef STDSEARCHLISTVIEW_H
#define STDSEARCHLISTVIEW_H

#include "SearchListView.h"
#include "StdTable.h"

class StdTableSearchList;

class StdSearchListView : public SearchListView
{
    Q_OBJECT
public:
    StdSearchListView(QWidget* parent, bool enableRegex, bool enableLock);
    ~StdSearchListView() override;

    void setInternalTitle(const QString & title);
    int getCharWidth();
    void addColumnAt(int width, QString title, bool isClickable);
    void setDrawDebugOnly(bool value);
    void enableMultiSelection(bool enabled);
    void setAddressColumn(int col, bool cipBase = false);
    void loadColumnFromConfig(const QString & viewName);
    bool setDisassemblyPopupEnabled(bool enabled);

public slots:
    virtual void setRowCount(dsint count);
    void setCellContent(int r, int c, QString s);
    void reloadData();
    void setSearchStartCol(int col);

private:
    StdTableSearchList* mSearchListData;

protected:
    friend class SymbolView;
    friend class Bridge;
    StdTable* stdList();
    StdTable* stdSearchList();
};
#endif // STDSEARCHLISTVIEW_H
