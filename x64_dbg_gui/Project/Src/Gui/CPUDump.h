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
    QString printNonData(int col, int_t wRva, ColumnDescriptor_t descriptor, MemoryPage* memPage);
};

#endif // CPUDUMP_H
