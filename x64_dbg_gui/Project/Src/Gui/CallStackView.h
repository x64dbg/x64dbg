#ifndef CALLSTACKVIEW_H
#define CALLSTACKVIEW_H

#include "StdTable.h"

class CallStackView : public StdTable
{
    Q_OBJECT
public:
    explicit CallStackView(StdTable* parent = 0);
    void setupContextMenu();

signals:
    void showCpu();

protected slots:
    void updateCallStack();
    void contextMenuSlot(const QPoint pos);
    void doubleClickedSlot();
    void followAddress();
    void followTo();
    void followFrom();

private:
    QAction* mFollowAddress;
    QAction* mFollowTo;
    QAction* mFollowFrom;
};

#endif // CALLSTACKVIEW_H
