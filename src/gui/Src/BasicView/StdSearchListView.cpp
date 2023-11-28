#include "StdSearchListView.h"
#include "MethodInvoker.h"

StdSearchListView::StdSearchListView(QWidget* parent, bool enableRegex, bool enableLock)
    : StdSearchListView(parent, enableRegex, enableLock, new StdTableSearchList())
{
}

StdSearchListView::StdSearchListView(QWidget* parent, bool enableRegex, bool enableLock, StdTableSearchList* tableSearchList)
    : SearchListView(parent, mSearchListData = tableSearchList, enableRegex, enableLock)
{
    setAddressColumn(0);
}

StdSearchListView::~StdSearchListView()
{
    delete mSearchListData;
}

void StdSearchListView::setInternalTitle(const QString & title)
{
    stdList()->setWindowTitle(title);
}

int StdSearchListView::getCharWidth()
{
    return stdList()->getCharWidth();
}

void StdSearchListView::addColumnAt(int width, QString title, bool isClickable, QString copyTitle, StdTable::SortBy::t sortFn)
{
    stdList()->addColumnAt(width, title, isClickable, copyTitle, sortFn);
    stdSearchList()->addColumnAt(width, title, isClickable, copyTitle, sortFn);
}

void StdSearchListView::setDrawDebugOnly(bool value)
{
    stdList()->setDrawDebugOnly(value);
    stdSearchList()->setDrawDebugOnly(value);
}

void StdSearchListView::enableMultiSelection(bool enabled)
{
    stdList()->enableMultiSelection(enabled);
    stdSearchList()->enableMultiSelection(enabled);
}

void StdSearchListView::setAddressColumn(int col, bool cipBase)
{
    stdList()->setAddressColumn(col, cipBase);
    stdSearchList()->setAddressColumn(col, cipBase);
}

void StdSearchListView::loadColumnFromConfig(const QString & viewName)
{
    stdList()->loadColumnFromConfig(viewName);
    stdSearchList()->loadColumnFromConfig(viewName);
}

void StdSearchListView::setRowCount(duint count)
{
    //clearFilter();
    stdList()->setRowCount(count);
}

void StdSearchListView::setCellContent(duint row, duint column, QString s)
{
    //clearFilter();
    stdList()->setCellContent(row, column, s);
}

void StdSearchListView::setCellUserdata(duint row, duint column, duint userdata)
{
    //clearFilter();
    stdList()->setCellUserdata(row, column, userdata);
}

void StdSearchListView::reloadData()
{
    //clearFilter();
    stdList()->reloadData();
    MethodInvoker::invokeMethod([this]()
    {
        stdList()->setFocus();
    });
}

void StdSearchListView::setSearchStartCol(duint col)
{
    if(col < stdList()->getColumnCount())
        mSearchStartCol = col;
}

StdTable* StdSearchListView::stdList()
{
    return mSearchListData->mList;
}

StdTable* StdSearchListView::stdSearchList()
{
    return mSearchListData->mSearchList;
}
