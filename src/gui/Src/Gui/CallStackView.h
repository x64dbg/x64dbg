#ifndef CALLSTACKVIEW_H
#define CALLSTACKVIEW_H

#include "StdTable.h"

class CallStackView : public StdTable
{
    Q_OBJECT
    Q_PROPERTY(int viewId MEMBER m_viewId)
public:
    explicit CallStackView(StdTable* parent = 0);
    void setupContextMenu();
	
signals:
    void showCpu();

protected slots:
    void updateCallStack();
    void contextMenuSlot(const QPoint pos);
    void followAddress();
    void followTo();
    void followFrom();

private:
    int m_viewId;
    MenuBuilder* mMenuBuilder;
};

#endif // CALLSTACKVIEW_H
