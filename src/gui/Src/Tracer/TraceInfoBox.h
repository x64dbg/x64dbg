#pragma once

#include "StdTable.h"
#include "TraceFileReader.h"

class TraceWidget;

class TraceInfoBox : public StdTable
{
    Q_OBJECT
public:
    TraceInfoBox(TraceWidget* parent);
    int getHeight();
    ~TraceInfoBox();

    void update(TRACEINDEX selection, TraceFileReader* traceFile, const REGDUMP & registers);
    void clear();

public slots:
    void contextMenuSlot(QPoint pos);

private:
    void setupContextMenu();
    void setupShortcuts();

    QAction* mCopyLineAction;
    TraceWidget* mParent;
    duint mCurAddr;
};
