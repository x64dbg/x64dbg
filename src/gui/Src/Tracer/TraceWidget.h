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
class TraceStack;
class TraceFileReader;
class TraceXrefBrowseDialog;

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
    void displayLogWidget();

protected slots:
    void displayLogWidgetSlot();
    void traceSelectionChanged(unsigned long long selection);
    void parseFinishedSlot();
    void closeFileSlot();
    void xrefSlot(duint addr);

protected:
    TraceFileReader* mTraceFile;
    TraceBrowser* mTraceBrowser;
    TraceInfoBox* mInfo;
    TraceDump* mDump;
    TraceRegisters* mGeneralRegs;
    TraceFileDumpMemoryPage* mMemoryPage;
    TraceStack* mStack;
    TraceXrefBrowseDialog* mXrefDlg;

private:
    Ui::TraceWidget* ui;
};
