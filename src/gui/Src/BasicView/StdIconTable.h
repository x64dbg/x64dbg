#pragma once

#include "StdTable.h"

// StdIconTable extends StdTable by adding icons to one of the columns
class StdIconTable : public StdTable
{
    Q_OBJECT
public:
    explicit StdIconTable(QWidget* parent = 0) : StdTable(parent), mIconColumn(0) {};

    // Data Management
    void setRowIcon(int r, const QIcon & icon); // set the icon for a row
    QIcon getRowIcon(int r) const;
    void setIconColumn(int c); // set in which column the icons appear
    int getIconColumn() const;
    void setRowCount(dsint count) override;
    void sortRows(int column, bool ascending) override;

    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h) override;

protected:
    std::vector<QIcon> mIcon; //listof(row) where row = (listof(col) where col = CellData)
    int mIconColumn;
};
