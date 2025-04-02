#pragma once

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
    explicit CPUMultiDump(CPUDisassembly* disassembly, int nbCpuDumpTabs = 1, QWidget* parent = nullptr);
    Architecture* getArchitecture() const;
    CPUDump* getCurrentCPUDump();
    void getTabNames(QList<QString> & names);
    int getMaxCPUTabs();
    QMenu* mDumpPluginMenu; // TODO: no
    void saveWindowSettings();
    void loadWindowSettings();

signals:
    void displayReferencesWidget();

public slots:
    void updateCurrentTabSlot(int tabIndex);
    void printDumpAtSlot(duint va);
    void printDumpAtNSlot(duint va, int index);
    void selectionGetSlot(SELECTIONDATA* selectionData);
    void selectionSetSlot(const SELECTIONDATA* selectionData);
    void dbgStateChangedSlot(DBGSTATE dbgState);
    void openChangeTabTitleDialogSlot(int tabIndex);
    void displayReferencesWidgetSlot();
    void focusCurrentDumpSlot();
    void focusStructSlot();
    void showDisassemblyTabSlot(duint selectionStart, duint selectionEnd, duint firstAddress);
    void getDumpAttention();

private:
    CPUDump* mCurrentCPUDump;
    bool mInitAllDumpTabs;
    uint mMaxCPUDumpTabs;

    WatchView* mWatch;
    LocalVarsView* mLocalVars;
    StructWidget* mStructWidget;
    CPUDisassembly* mMainDisassembly = nullptr;
    CPUDisassembly* mExtraDisassembly = nullptr;

    int GetDumpWindowIndex(int dump);
    int GetStructWindowIndex();
    int GetWatchWindowIndex();
    void SwitchToDumpWindow();
    void SwitchToStructWindow();
    void SwitchToWatchWindow();
};
