#ifndef MEMORYMAPVIEW_H
#define MEMORYMAPVIEW_H

#include <QtGui>
#include "StdTable.h"
#include "Bridge.h"

class MemoryMapView : public StdTable
{
    Q_OBJECT
public:
    explicit MemoryMapView(StdTable *parent = 0);
    
signals:
    
public slots:
    void stateChangedSlot(DBGSTATE state);

private:
    QString getProtectionString(DWORD Protect);
    
};

#endif // MEMORYMAPVIEW_H
