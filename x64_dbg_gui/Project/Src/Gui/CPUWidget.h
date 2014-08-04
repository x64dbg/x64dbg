#ifndef CPUWIDGET_H
#define CPUWIDGET_H

#include <QtGui>
#include <QVBoxLayout>
#include <QTableWidget>
#include "CPUDisassembly.h"
#include "CPUDump.h"
#include "CPUStack.h"
#include "RegistersView.h"
#include "CPUInfoBox.h"
#include "CPUSideBar.h"

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
    QVBoxLayout* getTopLeftUpperWidget(void);
    QVBoxLayout* getTopLeftLowerWidget(void);
    QVBoxLayout* getTopRightWidget(void);
    QVBoxLayout* getBotLeftWidget(void);
    QVBoxLayout* getBotRightWidget(void);

public:
    CPUSideBar* mSideBar;
    CPUDisassembly* mDisas;
    CPUDump* mDump;
    CPUStack* mStack;
    RegistersView* mGeneralRegs;
    CPUInfoBox* mInfo;
    QTabWidget* mRegsTab;

private:
    Ui::CPUWidget* ui;
};

#endif // CPUWIDGET_H
