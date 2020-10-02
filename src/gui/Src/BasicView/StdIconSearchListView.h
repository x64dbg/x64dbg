#ifndef STDICONSEARCHLISTVIEW_H
#define STDICONSEARCHLISTVIEW_H

#include "StdSearchListView.h"
#include "StdIconTable.h"

class StdIconSearchListView : public StdSearchListView
{
    Q_OBJECT
public:
    StdIconSearchListView(QWidget* parent, bool enableRegex, bool enableLock);
    StdIconSearchListView(QWidget* parent, bool enableRegex, bool enableLock, StdTableSearchList* tableSearchList);

public slots:
    void setIconColumn(int c);
    void setRowIcon(int r, const QIcon & icon);

private:
    StdTableSearchList* mSearchListData;

protected:
    friend class SymbolView;
    friend class Bridge;
};
#endif // STDICONSEARCHLISTVIEW_H
