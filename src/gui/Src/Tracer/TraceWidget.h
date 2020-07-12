#ifndef TRACEWIDGET_H
#define TRACEWIDGET_H

#include <QWidget>

class QVBoxLayout;
class CPUWidget;
class TraceRegisters;
class TraceBrowser;
class CPUInfoBox;
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
    void updateTraceRegistersView(void* registers);
    void updateSlot();

protected:
    TraceBrowser* mTraceWidget;
    TraceRegisters* mGeneralRegs;
    CPUInfoBox* mInfo;
    StdTable* mOverview;

private:
    Ui::TraceWidget* ui;
};

#endif // TRACEWIDGET_H
