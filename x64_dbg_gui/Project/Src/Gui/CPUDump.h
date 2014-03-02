#ifndef CPUDUMP_H
#define CPUDUMP_H

#include <QtGui>
#include <QtDebug>
#include "NewTypes.h"
#include "HexDump.h"
#include "Bridge.h"

class CPUDump : public HexDump
{
    Q_OBJECT
public:
    explicit CPUDump(QWidget *parent = 0);
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
};

#endif // CPUDUMP_H
