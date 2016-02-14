#ifndef CPUWIDGET_H
#define CPUWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include "CPUSideBar.h"
#include "CPUDisassembly.h"
#include "CPUMultiDump.h"
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

    // Misc
    void setDefaultDisposition();
    void setDisasmFocus();

    // Layout getters
    QVBoxLayout* getTopLeftUpperWidget();
    QVBoxLayout* getTopLeftLowerWidget();
    QVBoxLayout* getTopRightWidget();
    QVBoxLayout* getBotLeftWidget();
    QVBoxLayout* getBotRightWidget();

    // Widget getters
    CPUSideBar* getSidebarWidget();
    CPUDisassembly* getDisasmWidget();
    CPUMultiDump* getDumpWidget();
    CPUStack* getStackWidget();

protected:
    CPUSideBar* mSideBar;
    CPUDisassembly* mDisas;
    CPUMultiDump* mDump;
    CPUStack* mStack;
    RegistersView* mGeneralRegs;
    CPUInfoBox* mInfo;

private:
    Ui::CPUWidget* ui;
};

#endif // CPUWIDGET_H
