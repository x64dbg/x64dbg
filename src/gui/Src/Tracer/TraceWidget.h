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
class TraceFileReader;
class StdTable;

namespace Ui
{
    class TraceWidget;
}

class TraceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TraceWidget(Architecture* architecture, const QString & fileName, QWidget* parent);
    ~TraceWidget();

signals:
    void closeFile();

protected slots:
    void traceSelectionChanged(unsigned long long selection);
    void parseFinishedSlot();
    void closeFileSlot();

protected:
    TraceFileReader* mTraceFile;
    TraceBrowser* mTraceBrowser;
    TraceInfoBox* mInfo;
    TraceDump* mDump;
    TraceRegisters* mGeneralRegs;
    TraceFileDumpMemoryPage* mMemoryPage;
    StdTable* mOverview;

private:
    Ui::TraceWidget* ui;
};
