#include "CPUMultiDump.h"
#include "Bridge.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QTabBar>

CPUMultiDump::CPUMultiDump(CPUDisassembly* disas, int nbCpuDumpTabs, QWidget* parent)
    : MHTabWidget(parent, true, false)
{
    mMaxCPUDumpTabs = nbCpuDumpTabs;
    mInitAllDumpTabs = false;

    for(uint i = 0; i < mMaxCPUDumpTabs; i++)
    {
        CPUDump* cpuDump = new CPUDump(disas, this);
        connect(cpuDump, SIGNAL(displayReferencesWidget()), this, SLOT(displayReferencesWidgetSlot()));
        addTab(cpuDump, "Dump " + QString::number(i + 1));
    }

    mCurrentCPUDump = (CPUDump*)currentWidget();

    connect(tabBar(), SIGNAL(OnDoubleClickTabIndex(int)), this, SLOT(openChangeTabTitleDialogSlot(int)));
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(updateCurrentTabSlot(int)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChangedSlot(DBGSTATE)));

    connect(Bridge::getBridge(), SIGNAL(dumpAt(dsint)), this, SLOT(printDumpAtSlot(dsint)));
    connect(Bridge::getBridge(), SIGNAL(selectionDumpGet(SELECTIONDATA*)), this, SLOT(selectionGetSlot(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(selectionDumpSet(const SELECTIONDATA*)), this, SLOT(selectionSetSlot(const SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(dumpAtN(duint, int)), this, SLOT(dumpAtNSlot(duint, int)));
    connect(mCurrentCPUDump, SIGNAL(selectionUpdated()), mCurrentCPUDump, SLOT(selectionUpdatedSlot()));
}

CPUDump* CPUMultiDump::getCurrentCPUDump()
{
    return mCurrentCPUDump;
}

void CPUMultiDump::updateCurrentTabSlot(int tabIndex)
{
    mCurrentCPUDump = (CPUDump*)widget(tabIndex);
}

void CPUMultiDump::printDumpAtSlot(dsint parVa)
{
    if(mInitAllDumpTabs)
    {
        CPUDump* cpuDump = NULL;
        for(int i = 0; i < count(); i++)
        {
            cpuDump = (CPUDump*)widget(i);
            cpuDump->printDumpAt(parVa);
        }
        mInitAllDumpTabs = false;
    }
    else
    {
        mCurrentCPUDump->printDumpAt(parVa);
    }
}

void CPUMultiDump::dumpAtNSlot(duint parVa, int index)
{
    setCurrentIndex(index);

    mCurrentCPUDump = (CPUDump*) widget(index);
    mCurrentCPUDump->printDumpAt(parVa);
}

void CPUMultiDump::selectionGetSlot(SELECTIONDATA* selectionData)
{
    mCurrentCPUDump->selectionGet(selectionData);
}

void CPUMultiDump::selectionSetSlot(const SELECTIONDATA* selectionData)
{
    mCurrentCPUDump->selectionSet(selectionData);
}

void CPUMultiDump::dbgStateChangedSlot(DBGSTATE dbgState)
{
    if(dbgState == initialized)
    {
        mInitAllDumpTabs = true;
    }
}

void CPUMultiDump::openChangeTabTitleDialogSlot(int tabIndex)
{
    bool bUserPressedOk;
    QString sCurrentTabName = tabBar()->tabText(tabIndex);

    QString sNewTabName = QInputDialog::getText(this, "Change Tab " + QString::number(tabIndex + 1) + " Name", "Tab Name", QLineEdit::Normal, sCurrentTabName, &bUserPressedOk, Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
    if(bUserPressedOk)
    {
        if(sNewTabName.length() != 0)
            tabBar()->setTabText(tabIndex, sNewTabName);
    }
}

void CPUMultiDump::displayReferencesWidgetSlot()
{
    emit displayReferencesWidget();
}
