#ifndef STDICONTABLE_H
#define STDICONTABLE_H

#include "StdTable.h"

class StdIconTable : public StdTable
{
    Q_OBJECT
public:
    explicit StdIconTable(QWidget* parent = 0) : StdTable(parent), mIconColumn(0) {};

    // Data Management
    void setRowIcon(int r, const QIcon & icon);
    QIcon getRowIcon(int r) const;
    void setIconColumn(int c);
    int getIconColumn() const;
    void setRowCount(dsint count) override;
    void sortRows(int column, bool ascending) override;

    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h) override;

protected:
    std::vector<QIcon> mIcon; //listof(row) where row = (listof(col) where col = CellData)
    int mIconColumn;
};

#endif // STDICONTABLE_H
