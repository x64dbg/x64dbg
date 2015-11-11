#ifndef CPUWIDGET_H
#define CPUWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include "CPUSideBar.h"
#include "CPUDisassembly.h"
#include "CPUDump.h"
#include "CPUStack.h"
#include "RegistersView.h"
#include "CPUInfoBox.h"

namespace Ui
{
class CPUWidget;
}

class CPUWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CPUWidget(QWidget* parent = 0);
    ~CPUWidget();
    void setDefaultDisposition(void);

    // Layout getters
    QVBoxLayout* getTopLeftUpperWidget();
    QVBoxLayout* getTopLeftLowerWidget();
    QVBoxLayout* getTopRightWidget();
    QVBoxLayout* getBotLeftWidget();
    QVBoxLayout* getBotRightWidget();

    // Widget getters
    CPUSideBar* getSidebarWidget();

public:
    CPUSideBar* mSideBar;
    CPUDisassembly* mDisas;
    CPUDump* mDump;
    CPUStack* mStack;
    RegistersView* mGeneralRegs;
    CPUInfoBox* mInfo;

private:
    Ui::CPUWidget* ui;
};

#endif // CPUWIDGET_H
