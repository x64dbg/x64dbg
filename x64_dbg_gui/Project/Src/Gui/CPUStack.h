#ifndef CPUSTACK_H
#define CPUSTACK_H

#include <QtGui>
#include <QtDebug>
#include "NewTypes.h"
#include "HexDump.h"
#include "Bridge.h"

class CPUStack : public HexDump
{
    Q_OBJECT
public:
    explicit CPUStack(QWidget *parent = 0);
    QString printNonData(int col, int_t wRva, ColumnDescriptor_t descriptor, MemoryPage* memPage);
};

#endif // CPUSTACK_H
