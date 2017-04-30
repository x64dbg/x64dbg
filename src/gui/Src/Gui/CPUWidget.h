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

public:
    explicit CPUWidget(QWidget* parent = 0);
    ~CPUWidget();

    // Misc
    void setDefaultDisposition();
    void setDisasmFocus();

    void saveWindowSettings();
    void loadWindowSettings();

    // Widget getters
    CPUSideBar* getSidebarWidget();
    CPUDisassembly* getDisasmWidget();
    CPUMultiDump* getDumpWidget();
    CPUStack* getStackWidget();
    CPUInfoBox* getInfoBoxWidget();

protected:
    CPUSideBar* mSideBar;
    CPUDisassembly* mDisas;
    CPUMultiDump* mDump;
    CPUStack* mStack;
    RegistersView* mGeneralRegs;
    CPUInfoBox* mInfo;
    CPUArgumentWidget* mArgumentWidget;

private:
    Ui::CPUWidget* ui;

private slots:
    void splitterMoved(int pos, int index);
};

#endif // CPUWIDGET_H
