#include "CPUDisassembly.h"

CPUDisassembly::CPUDisassembly(QWidget *parent) : Disassembly(parent)
{
    // Create the action list for the right click context menu
    setupRightClickContextMenu();

}


/************************************************************************************
                            Mouse Management
************************************************************************************/
/**
 * @brief       This method has been reimplemented. It manages the richt click context menu.
 *
 * @param[in]   event       Context menu event
 *
 * @return      Nothing.
 */
void CPUDisassembly::contextMenuEvent(QContextMenuEvent* event)
{
    int_t wVA = rvaToVa(getInitialSelection());
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);

    if((wBpType & bphardware) == bphardware)
    {
        mToogleHwBpAction->setText("Remove Hardware");
    }
    else
    {
        mToogleHwBpAction->setText("Set Hardware on Execution");
    }

    QAction* wAction = mRigthClickContextMenu->exec(event->globalPos());
}


/************************************************************************************
                         Context Menu Management
************************************************************************************/
void CPUDisassembly::setupRightClickContextMenu()
{
    mRigthClickContextMenu = new QMenu(this);

//---------------------- Breakpoints -----------------------------
    QMenu* wBPMenu = new QMenu("Breakpoints", this);

    // INT3 BP
    mToogleInt3BpAction = new QAction("Toogle INT3", this);
    mToogleInt3BpAction->setShortcutContext(Qt::WidgetShortcut);
    mToogleInt3BpAction->setShortcut(QKeySequence(Qt::Key_F2));
    this->addAction(mToogleInt3BpAction);
    connect(mToogleInt3BpAction, SIGNAL(triggered()), this, SLOT(toogleInt3BPAction()));
    wBPMenu->addAction(mToogleInt3BpAction);

    // HW BP
    mToogleHwBpAction = new QAction("Set Hardware on Execution", this);
    connect(mToogleHwBpAction, SIGNAL(triggered()), this, SLOT(toogleHwBpActionSlot()));
    wBPMenu->addAction(mToogleHwBpAction);

    mRigthClickContextMenu->addMenu(wBPMenu);

    // Separator
    mRigthClickContextMenu->addSeparator();

//---------------------- New origin here -----------------------------
    mSetNewOriginHere = new QAction("Set New Origin Here", this);
    mSetNewOriginHere->setShortcutContext(Qt::WidgetShortcut);
    mSetNewOriginHere->setShortcut(QKeySequence("ctrl+*"));
    this->addAction(mSetNewOriginHere);
    connect(mSetNewOriginHere, SIGNAL(triggered()), this, SLOT(setNewOriginHereActionSlot()));

    mRigthClickContextMenu->addAction(mSetNewOriginHere);
}


void CPUDisassembly::toogleInt3BPAction()
{
    int_t wVA = rvaToVa(getInitialSelection());
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bpnormal) == bpnormal)
    {
        wCmd = "bc " + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }
    else
    {
        wCmd = "bp " + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }

    Bridge::getBridge()->execCmd(wCmd.toAscii().data());
}


void CPUDisassembly::toogleHwBpActionSlot()
{
    int_t wVA = rvaToVa(getInitialSelection());
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bphardware) == bphardware)
    {
        // Todo
        QMessageBox::information(this, "Remove Hardware Breakpoint", "Not yet implemented!");
        return;
    }
    else
    {
        wCmd = "bph " + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }

    Bridge::getBridge()->execCmd(wCmd.toAscii().data());
}


void CPUDisassembly::setNewOriginHereActionSlot()
{
    int_t wVA = rvaToVa(getInitialSelection());
    QString wCmd = "";

#ifdef _WIN64
    wCmd = "rip=";
#else
    wCmd = "eip=";
#endif

    wCmd += QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    Bridge::getBridge()->execCmd(wCmd.toAscii().data());
}
