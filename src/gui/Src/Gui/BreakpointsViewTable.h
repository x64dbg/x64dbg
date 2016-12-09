#ifndef BREAKPOINTSVIEWTABLE_H
#define BREAKPOINTSVIEWTABLE_H

#include "StdTable.h"

class BreakpointsViewTable : public StdTable
{
    Q_OBJECT
public:
    explicit BreakpointsViewTable(QWidget* parent = 0);
    void GetConfigColors();
    duint GetCIP();

protected:
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);

private:
    QColor BgColor;
    QColor TxtColor;
};

#endif // BREAKPOINTSVIEWTABLE_H
