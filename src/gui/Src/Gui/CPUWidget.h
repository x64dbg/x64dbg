#ifndef CPUWIDGET_H
#define CPUWIDGET_H

#include <QWidget>

class QVBoxLayout;
class CPUSideBar;
class CPUDisassembly;
class CPUMultiDump;
class CPUStack;
class RegistersView;
class CPUInfoBox;
class CPUArgumentWidget;

namespace Ui
{
    class CPUWidget;
}

class CPUWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int viewId MEMBER m_viewId)
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
    CPUArgumentWidget* mArgumentWidget;

private:
    int m_viewId;
    Ui::CPUWidget* ui;
};

#endif // CPUWIDGET_H
