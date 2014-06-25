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
    QString paintContent(QPainter *painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h);
    
signals:
    
public slots:
    void stateChangedSlot(DBGSTATE state);

private:
    QString getProtectionString(DWORD Protect);
    
};

#endif // MEMORYMAPVIEW_H
