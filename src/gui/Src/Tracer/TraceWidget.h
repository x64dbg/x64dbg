#pragma once

#include <QWidget>
#include "Bridge.h"

class QVBoxLayout;
class CPUWidget;
class TraceRegisters;
class TraceBrowser;
class TraceFileReader;
class TraceFileDumpMemoryPage;
class TraceInfoBox;
class TraceDump;
class StdTable;

namespace Ui
{
    class TraceWidget;
}

class TraceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TraceWidget(QWidget* parent);
    ~TraceWidget();

    TraceBrowser* getTraceBrowser();

public slots:
    void openSlot(const QString & fileName);

protected slots:
    void traceSelectionChanged(unsigned long long selection);
    void updateSlot();

protected:
    TraceBrowser* mTraceWidget;
    TraceInfoBox* mInfo;
    TraceDump* mDump;
    TraceRegisters* mGeneralRegs;
    TraceFileDumpMemoryPage* mMemoryPage;
    StdTable* mOverview;

private:
    Ui::TraceWidget* ui;
};
