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
        mToggleHwBpAction->setText("Remove Hardware");
    }
    else
    {
        mToggleHwBpAction->setText("Set Hardware on Execution");
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
    mToggleInt3BpAction = new QAction("Toggle INT3", this);
    mToggleInt3BpAction->setShortcutContext(Qt::WidgetShortcut);
    mToggleInt3BpAction->setShortcut(QKeySequence(Qt::Key_F2));
    this->addAction(mToggleInt3BpAction);
    connect(mToggleInt3BpAction, SIGNAL(triggered()), this, SLOT(toggleInt3BPAction()));
    wBPMenu->addAction(mToggleInt3BpAction);

    // HW BP
    mToggleHwBpAction = new QAction("Set Hardware on Execution", this);
    connect(mToggleHwBpAction, SIGNAL(triggered()), this, SLOT(toggleHwBpActionSlot()));
    wBPMenu->addAction(mToggleHwBpAction);

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


void CPUDisassembly::toggleInt3BPAction()
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

    Bridge::getBridge()->execCmd(wCmd.toUtf8().constData());
}


void CPUDisassembly::toggleHwBpActionSlot()
{
    int_t wVA = rvaToVa(getInitialSelection());
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bphardware) == bphardware)
    {
        wCmd = "bphwc " + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }
    else
    {
        wCmd = "bphws " + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    }

    Bridge::getBridge()->execCmd(wCmd.toUtf8().constData());
}


void CPUDisassembly::setNewOriginHereActionSlot()
{
    int_t wVA = rvaToVa(getInitialSelection());
    QString wCmd = "cip=" + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    Bridge::getBridge()->execCmd(wCmd.toUtf8().constData());
}
