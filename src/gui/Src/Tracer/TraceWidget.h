#ifndef TRACEWIDGET_H
#define TRACEWIDGET_H

#include <QWidget>

class QVBoxLayout;
class CPUWidget;
class RegistersView;
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

protected:
    TraceBrowser* mTraceWidget;
    RegistersView* mGeneralRegs;
    CPUInfoBox* mInfo;
    StdTable* mOverview;

private:
    Ui::TraceWidget* ui;
};

#endif // TRACEWIDGET_H
