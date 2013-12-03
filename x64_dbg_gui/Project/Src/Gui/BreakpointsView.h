#ifndef BREAKPOINTSVIEW_H
#define BREAKPOINTSVIEW_H

#include <QtGui>
#include "StdTable.h"
#include <QDebug>

class BreakpointsView : public QWidget
{
    Q_OBJECT
public:
    explicit BreakpointsView(QWidget *parent = 0);
    void paintEvent(QPaintEvent* event);

signals:
    
public slots:

private:
    QVBoxLayout* mVertLayout;
    StdTable* mHardBPTable;
    StdTable* mSoftBPTable;
    
};

#endif // BREAKPOINTSVIEW_H
