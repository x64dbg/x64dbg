#ifndef THREADVIEW_H
#define THREADVIEW_H

#include "StdTable.h"
#include "Bridge.h"

class ThreadView : public StdTable
{
    Q_OBJECT
public:
    explicit ThreadView(StdTable* parent = 0);
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);

public slots:
    void updateThreadList();
    void doubleClickedSlot();

signals:
    void showCpu();

private:
    int mCurrentThread;
};

#endif // THREADVIEW_H
