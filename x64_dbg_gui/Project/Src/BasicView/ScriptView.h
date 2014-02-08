#ifndef SCRIPTVIEW_H
#define SCRIPTVIEW_H

#include <QtGui>
#include "StdTable.h"
#include "Bridge.h"

class ScriptView : public StdTable
{
    Q_OBJECT
public:
    explicit ScriptView(StdTable *parent = 0);
    // Reimplemented Functions
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
};

#endif // SCRIPTVIEW_H
