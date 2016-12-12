#ifndef BREAKPOINTSVIEWTABLE_H
#define BREAKPOINTSVIEWTABLE_H

#include "StdTable.h"

class BreakpointsViewTable : public StdTable
{
    Q_OBJECT
public:
    explicit BreakpointsViewTable(QWidget* parent = 0);
    void GetConfigColors();
    void updateColors() override;

public slots:
    void disassembleAtSlot(dsint cip, dsint addr);

protected:
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);

private:
    QColor mCipBackgroundColor;
    QColor mCipColor;
    duint mCip;
};

#endif // BREAKPOINTSVIEWTABLE_H
