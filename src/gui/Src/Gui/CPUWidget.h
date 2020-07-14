#ifndef CPUWIDGET_H
#define CPUWIDGET_H

#include <QWidget>
#include "Bridge.h"

class QVBoxLayout;
class CPUSideBar;
class CPUDisassembly;
class CPUMultiDump;
class CPUStack;
class CPURegistersView;
class CPUInfoBox;
class CPUArgumentWidget;
class DisassemblerGraphView;

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
    void setGraphFocus();

    void saveWindowSettings();
    void loadWindowSettings();

    duint getSelectionVa();

    // Widget getters
    CPUSideBar* getSidebarWidget();
    CPUDisassembly* getDisasmWidget();
    CPUMultiDump* getDumpWidget();
    CPUStack* getStackWidget();
    CPUInfoBox* getInfoBoxWidget();

protected:
    CPUSideBar* mSideBar;
    CPUDisassembly* mDisas;
    DisassemblerGraphView* mGraph;
    CPUMultiDump* mDump;
    CPUStack* mStack;
    CPURegistersView* mGeneralRegs;
    CPUInfoBox* mInfo;
    CPUArgumentWidget* mArgumentWidget;

    bool disasMode;

private:
    Ui::CPUWidget* ui;

private slots:
    void splitterMoved(int pos, int index);
};

#endif // CPUWIDGET_H
