#ifndef HANDLESWINDOWVIEWTABLE_H
#define HANDLESWINDOWVIEWTABLE_H

#include "StdTable.h"

class HandlesWindowViewTable : public StdTable
{
    Q_OBJECT
public:
    explicit HandlesWindowViewTable(QWidget* parent = 0);
    void GetConfigColors();
    void updateColors() override;

protected:
    QString paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h);

private:
    QColor mBpBackgroundColor;
    QColor mBpColor;
};

#endif // HANDLESWINDOWVIEWTABLE_H
