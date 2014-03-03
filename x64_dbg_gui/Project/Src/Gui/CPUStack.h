#ifndef CPUSTACK_H
#define CPUSTACK_H

#include <QtGui>
#include <QtDebug>
#include <QAction>
#include "NewTypes.h"
#include "HexDump.h"
#include "Bridge.h"

class CPUStack : public HexDump
{
    Q_OBJECT
public:
    explicit CPUStack(QWidget *parent = 0);
    QString paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);

public slots:
    void stackDumpAt(uint_t addr, uint_t csp);

private:
    uint_t mCsp;
};

#endif // CPUSTACK_H
