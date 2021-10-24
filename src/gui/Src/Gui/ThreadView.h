#pragma once

#include "StdTable.h"
#include <QMenu>

class ThreadView : public StdTable
{
    Q_OBJECT
public:
    explicit ThreadView(StdTable* parent = 0);
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);
    void setupContextMenu();

public slots:
    void updateThreadList();
    void doubleClickedSlot();
    void ExecCommand();
    void GoToThreadEntry();
    void contextMenuSlot(const QPoint & pos);
    void SetNameSlot();

private:
    QAction* makeCommandAction(QAction* action, const QString & command);
    duint mCurrentThreadId;
    MenuBuilder* mMenuBuilder;
};
