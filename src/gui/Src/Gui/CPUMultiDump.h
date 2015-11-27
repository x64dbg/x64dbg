#ifndef CPUMULTIDUMP_H
#define CPUMULTIDUMP_H

#include <QWidget>
#include "CPUDump.h"
#include "TabWidget.h"

class CPUDump;

class CPUMultiDump : public MHTabWidget
{
    Q_OBJECT
public:
    explicit CPUMultiDump(CPUDisassembly* disas, int nbCpuDumpTabs = 1, QWidget* parent = 0);
    CPUDump* getCurrentCPUDump();

signals:
    void displayReferencesWidget();

public slots:
    void updateCurrentTabSlot(int tabIndex);
    void printDumpAtSlot(dsint parVa);
    void dumpAtNSlot(duint parVa, int index);
    void selectionGetSlot(SELECTIONDATA* selectionData);
    void selectionSetSlot(const SELECTIONDATA* selectionData);
    void dbgStateChangedSlot(DBGSTATE dbgState);
    void openChangeTabTitleDialogSlot(int tabIndex);
    void displayReferencesWidgetSlot();

private:
    CPUDump* mCurrentCPUDump;
    bool mInitAllDumpTabs;
    uint mMaxCPUDumpTabs;

};

#endif // CPUMULTIDUMP_H
