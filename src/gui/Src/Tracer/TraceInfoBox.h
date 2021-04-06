#ifndef TRACEINFOBOX_H
#define TRACEINFOBOX_H
#include "StdTable.h"

class TraceWidget;
class TraceFileReader;

class TraceInfoBox : public StdTable
{
    Q_OBJECT
public:
    TraceInfoBox(TraceWidget* parent);
    ~TraceInfoBox();

    void update(unsigned long long selection, TraceFileReader* traceFile, const REGDUMP & registers);

public slots:
    void contextMenuSlot(QPoint pos);

private:
    void setupContextMenu();
    void setupShortcuts();
    void clean();

    QAction* mCopyLineAction;
};

#endif //TRACEINFOBOX_H
