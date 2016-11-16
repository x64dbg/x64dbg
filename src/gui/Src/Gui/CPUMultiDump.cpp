#include "CPUMultiDump.h"
#include "Bridge.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QTabBar>

CPUMultiDump::CPUMultiDump(CPUDisassembly* disas, int nbCpuDumpTabs, QWidget* parent)
    : MHTabWidget(parent, true)
{
    setWindowTitle("CPUMultiDump");
    mMaxCPUDumpTabs = nbCpuDumpTabs;
    mInitAllDumpTabs = false;

    for(uint i = 0; i < mMaxCPUDumpTabs; i++)
    {
        CPUDump* cpuDump = new CPUDump(disas, this);
        cpuDump->loadColumnFromConfig(QString("CPUDump%1").arg(i + 1)); //TODO: needs a workaround because the columns change
        connect(cpuDump, SIGNAL(displayReferencesWidget()), this, SLOT(displayReferencesWidgetSlot()));
        auto nativeTitle = QString("Dump ") + QString::number(i + 1);
        this->addTabEx(cpuDump, DIcon("dump.png"), tr("Dump ") + QString::number(i + 1), nativeTitle);
        cpuDump->setWindowTitle(nativeTitle);
    }

    mCurrentCPUDump = dynamic_cast<CPUDump*>(currentWidget());

    mWatch = new WatchView(this);

    //mMaxCPUDumpTabs++;
    auto nativeTitle = QString("Watch 1");
    this->addTabEx(mWatch, DIcon("animal-dog.png"), tr("Watch ") + QString::number(1), nativeTitle);
    mWatch->setWindowTitle(nativeTitle);
    mWatch->loadColumnFromConfig("Watch1");

    connect(this, SIGNAL(currentChanged(int)), this, SLOT(updateCurrentTabSlot(int)));
    connect(tabBar(), SIGNAL(OnDoubleClickTabIndex(int)), this, SLOT(openChangeTabTitleDialogSlot(int)));

    connect(Bridge::getBridge(), SIGNAL(dumpAt(dsint)), this, SLOT(printDumpAtSlot(dsint)));
    connect(Bridge::getBridge(), SIGNAL(dumpAtN(duint, int)), this, SLOT(printDumpAtNSlot(duint, int)));
    connect(Bridge::getBridge(), SIGNAL(selectionDumpGet(SELECTIONDATA*)), this, SLOT(selectionGetSlot(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(selectionDumpSet(const SELECTIONDATA*)), this, SLOT(selectionSetSlot(const SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChangedSlot(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(focusDump()), this, SLOT(focusCurrentDumpSlot()));

    connect(mCurrentCPUDump, SIGNAL(selectionUpdated()), mCurrentCPUDump, SLOT(selectionUpdatedSlot()));
}

CPUDump* CPUMultiDump::getCurrentCPUDump()
{
    return mCurrentCPUDump;
}

void CPUMultiDump::getTabNames(QList<QString> & names)
{
    bool addedDetachedWindows = false;
    names.clear();
    for(int i = 0; i < count(); i++)
    {
        if(!getNativeName(i).startsWith("Dump "))
            continue;
        // If empty name, then widget is detached
        if(this->tabBar()->tabText(i).length() == 0)
        {
            // If we added all the detached windows once, no need to do it again
            if(addedDetachedWindows)
                continue;

            QString windowName;
            // Loop through all detached widgets
            for(int n = 0; n < this->windows().size(); n++)
            {
                // Get the name and add it to the list
                windowName = ((MHDetachedWindow*)this->windows().at(n)->parent())->windowTitle();
                names.push_back(windowName);
            }
            addedDetachedWindows = true;
        }
        else
        {
            names.push_back(this->tabBar()->tabText(i));
        }
    }
}

int CPUMultiDump::getMaxCPUTabs()
{
    return mMaxCPUDumpTabs;
}

int CPUMultiDump::GetDumpWindowIndex(int dump)
{
    QString dumpNativeName = QString("Dump ") + QString::number(dump);
    for(int i = 0; i < count(); i++)
    {
        if(getNativeName(i) == dumpNativeName)
            return i;
    }
    return 2147483647;
}

int CPUMultiDump::GetWatchWindowIndex()
{
    QString watchNativeName = QString("Watch 1");
    for(int i = 0; i < count(); i++)
    {
        if(getNativeName(i) == watchNativeName)
            return i;
    }
    return 2147483647;
}

void CPUMultiDump::SwitchToDumpWindow()
{
    if(!mCurrentCPUDump)
        setCurrentIndex(GetDumpWindowIndex(1));
}

void CPUMultiDump::SwitchToWatchWindow()
{
    if(mCurrentCPUDump)
        setCurrentIndex(GetWatchWindowIndex());
}

void CPUMultiDump::updateCurrentTabSlot(int tabIndex)
{
    CPUDump* t = qobject_cast<CPUDump*>(widget(tabIndex));
    mCurrentCPUDump = t;
}

void CPUMultiDump::printDumpAtSlot(dsint parVa)
{
    if(mInitAllDumpTabs)
    {
        CPUDump* cpuDump = NULL;
        for(int i = 0; i < count(); i++)
        {
            if(!getNativeName(i).startsWith("Dump "))
                continue;
            cpuDump = qobject_cast<CPUDump*>(widget(i));
            if(cpuDump)
            {
                cpuDump->historyClear();
                cpuDump->addVaToHistory(parVa);
                cpuDump->printDumpAt(parVa);
            }
        }

        mInitAllDumpTabs = false;
    }
    else
    {
        SwitchToDumpWindow();
        mCurrentCPUDump->printDumpAt(parVa);
        mCurrentCPUDump->addVaToHistory(parVa);
    }
}

void CPUMultiDump::printDumpAtNSlot(duint parVa, int index)
{
    int tabindex = GetDumpWindowIndex(index);
    if(tabindex == 2147483647)
        return;
    CPUDump* current = qobject_cast<CPUDump*>(widget(tabindex));
    if(!current)
        return;
    setCurrentIndex(tabindex);
    current->printDumpAt(parVa);
    current->addVaToHistory(parVa);
}

void CPUMultiDump::selectionGetSlot(SELECTIONDATA* selectionData)
{
    SwitchToDumpWindow();
    mCurrentCPUDump->selectionGet(selectionData);
}

void CPUMultiDump::selectionSetSlot(const SELECTIONDATA* selectionData)
{
    SwitchToDumpWindow();
    mCurrentCPUDump->selectionSet(selectionData);
}

void CPUMultiDump::dbgStateChangedSlot(DBGSTATE dbgState)
{
    if(dbgState == initialized)
        mInitAllDumpTabs = true;
}

void CPUMultiDump::openChangeTabTitleDialogSlot(int tabIndex)
{
    bool bUserPressedOk;
    QString sCurrentTabName = tabBar()->tabText(tabIndex);

    QString sNewTabName = QInputDialog::getText(this, tr("Change Tab %1 Name").arg(tabIndex + 1), tr("Tab Name"), QLineEdit::Normal, sCurrentTabName, &bUserPressedOk, Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
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

void CPUMultiDump::focusCurrentDumpSlot()
{
    SwitchToDumpWindow();
    mCurrentCPUDump->setFocus();
}
