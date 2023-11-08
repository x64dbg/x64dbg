#pragma once

#include "StdTable.h"
#include <QMenu>

class ThreadView : public StdTable
{
    Q_OBJECT
public:
    explicit ThreadView(StdTable* parent = nullptr);
    QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) override;
    void setupContextMenu();
signals:
    void displayThreadsView();

public slots:
    void selectionThreadsSet(const SELECTIONDATA* selection);
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
