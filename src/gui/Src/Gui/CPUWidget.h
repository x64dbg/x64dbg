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
    CPUDump* getDumpWidget();
    CPUStack* getStackWidget();

protected:
    CPUSideBar* mSideBar;
    CPUDisassembly* mDisas;
    CPUDump* mDump;
    CPUStack* mStack;
    RegistersView* mGeneralRegs;
    CPUInfoBox* mInfo;
private slots:
    void getFocusedCPUWindow();
private:
    Ui::CPUWidget* ui;
};

#endif // CPUWIDGET_H
