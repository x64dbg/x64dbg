#ifndef TRACEWIDGET_H
#define TRACEWIDGET_H

#include <QWidget>
#include "Bridge.h"

class QVBoxLayout;
class CPUWidget;
class TraceRegisters;
class TraceBrowser;
class TraceFileReader;
class TraceInfoBox;
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

protected slots:
    void traceSelectionChanged(unsigned long long selection);
    void updateSlot();

protected:
    TraceBrowser* mTraceWidget;
    TraceInfoBox* mInfo;
    TraceRegisters* mGeneralRegs;
    StdTable* mOverview;

private:
    Ui::TraceWidget* ui;
};

#endif // TRACEWIDGET_H
