#include "StdIconSearchListView.h"

StdIconSearchListView::StdIconSearchListView(QWidget* parent, bool enableRegex, bool enableLock)
    : StdIconSearchListView(parent, enableRegex, enableLock, new StdTableSearchList(new StdIconTable(), new StdIconTable()))
{
}

StdIconSearchListView::StdIconSearchListView(QWidget* parent, bool enableRegex, bool enableLock, StdTableSearchList* tableSearchList)
    : StdSearchListView(parent, enableRegex, enableLock, tableSearchList)
{
}

void StdIconSearchListView::setIconColumn(int c)
{
    qobject_cast<StdIconTable*>(stdList())->setIconColumn(c);
    qobject_cast<StdIconTable*>(stdSearchList())->setIconColumn(c);
}

void StdIconSearchListView::setRowIcon(int r, const QIcon & icon)
{
    clearFilter();
    qobject_cast<StdIconTable*>(stdList())->setRowIcon(r, icon);
}
