#pragma once

#include "StdTable.h"

// StdIconTable extends StdTable by adding icons to one of the columns
class StdIconTable : public StdTable
{
    Q_OBJECT
public:
    explicit StdIconTable(QWidget* parent = nullptr) : StdTable(parent), mIconColumn(0) {}

    // Data Management
    void setRowIcon(int r, const QIcon & icon); // set the icon for a row
    QIcon getRowIcon(int r) const;
    void setIconColumn(int c); // set in which column the icons appear
    int getIconColumn() const;
    void setRowCount(duint count) override;
    void sortRows(duint column, bool ascending) override;

    QString paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h) override;

protected:
    std::vector<QIcon> mIcon; //listof(row) where row = (listof(col) where col = CellData)
    int mIconColumn;
};
