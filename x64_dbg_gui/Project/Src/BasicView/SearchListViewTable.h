#ifndef SEARCHLISTVIEWTABLE_H
#define SEARCHLISTVIEWTABLE_H

#include "StdTable.h"

class SearchListViewTable : public StdTable
{
    Q_OBJECT
public:
    explicit SearchListViewTable(StdTable* parent = 0);
    QString highlightText;

protected:
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
};

#endif // SEARCHLISTVIEWTABLE_H
