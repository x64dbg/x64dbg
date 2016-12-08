#ifndef BREAKPOINTSVIEWTABLE_H
#define BREAKPOINTSVIEWTABLE_H

#include "StdTable.h"

class BreakpointsViewTable : public StdTable
{
    Q_OBJECT
public:
    explicit BreakpointsViewTable(QWidget* parent = 0);
    duint GetCIP() { return DbgValFromString("cip");}

protected:
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);

private:
    duint bpAddr;
};

#endif // BREAKPOINTSVIEWTABLE_H
