#pragma once

#include <QWidget>
#include "Bridge.h"

class QVBoxLayout;
class QPushButton;
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
    void loadDump();
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
    QPushButton* mLoadDump;
    Architecture* mArchitecture;

private:
    Ui::TraceWidget* ui;
    void setupDumpInitialAddresses(unsigned long long selection);
};
