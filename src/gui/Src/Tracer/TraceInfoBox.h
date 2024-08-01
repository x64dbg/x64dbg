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
    void setupFollowMenu(QMenu* menu, duint va);
    void addFollowMenuItem(QMenu* menu, QString name, duint value);
    ~TraceInfoBox();

    void update(TRACEINDEX selection, TraceFileReader* traceFile, const REGDUMP & registers);
    void clear();

public slots:
    void contextMenuSlot(QPoint pos);
    void followActionSlot();

private:
    void setupContextMenu();
    void setupShortcuts();

    QAction* mCopyLineAction;
    TraceWidget* mParent;
    duint mCurAddr;
};
