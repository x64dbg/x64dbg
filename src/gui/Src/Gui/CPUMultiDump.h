#ifndef CPUMULTIDUMP_H
#define CPUMULTIDUMP_H

#include <QWidget>
#include "TabWidget.h"
#include "Bridge.h"

class CPUDump;
class WatchView;
class StructWidget;
class CPUDisassembly;
class LocalVarsView;

class CPUMultiDump : public MHTabWidget
{
    Q_OBJECT
public:
    explicit CPUMultiDump(CPUDisassembly* disas, int nbCpuDumpTabs = 1, QWidget* parent = 0);
    CPUDump* getCurrentCPUDump();
    void getTabNames(QList<QString> & names);
    int getMaxCPUTabs();
    QMenu* mDumpPluginMenu;

signals:
    void displayReferencesWidget();

public slots:
    void updateCurrentTabSlot(int tabIndex);
    void printDumpAtSlot(dsint parVa);
    void printDumpAtNSlot(duint parVa, int index);
    void selectionGetSlot(SELECTIONDATA* selectionData);
    void selectionSetSlot(const SELECTIONDATA* selectionData);
    void dbgStateChangedSlot(DBGSTATE dbgState);
    void openChangeTabTitleDialogSlot(int tabIndex);
    void displayReferencesWidgetSlot();
    void focusCurrentDumpSlot();
    void getDumpAttention();

private:
    CPUDump* mCurrentCPUDump;
    bool mInitAllDumpTabs;
    uint mMaxCPUDumpTabs;

    WatchView* mWatch;
    LocalVarsView* mLocalVars;
    StructWidget* mStructWidget;

    int GetDumpWindowIndex(int dump);
    int GetWatchWindowIndex();
    void SwitchToDumpWindow();
    void SwitchToWatchWindow();
};

#endif // CPUMULTIDUMP_H
