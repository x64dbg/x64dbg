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
    uint_t wVA = rvaToVa(getInitialSelection());
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);

    if((wBpType & bp_hardware) == bp_hardware)
    {
        mToggleHwBpAction->setText("Remove Hardware");
    }
    else
    {
        mToggleHwBpAction->setText("Set Hardware on Execution");
    }

    QAction* wAction = mRightClickContextMenu->exec(event->globalPos());
}


/************************************************************************************
                         Context Menu Management
************************************************************************************/
void CPUDisassembly::setupRightClickContextMenu()
{
    mRightClickContextMenu = new QMenu(this);

    //label/comment
    mSetLabel = new QAction("Label", this);
    mSetLabel->setShortcutContext(Qt::WidgetShortcut);
    mSetLabel->setShortcut(QKeySequence(":"));
    this->addAction(mSetLabel);
    connect(mSetLabel, SIGNAL(triggered()), this, SLOT(setLabel()));

    mSetComment = new QAction("Comment", this);
    mSetComment->setShortcutContext(Qt::WidgetShortcut);
    mSetComment->setShortcut(QKeySequence(";"));
    this->addAction(mSetComment);
    connect(mSetComment, SIGNAL(triggered()), this, SLOT(setComment()));

    mSetBookmark = new QAction("Bookmark", this);
    mSetBookmark->setShortcutContext(Qt::WidgetShortcut);
    mSetBookmark->setShortcut(QKeySequence("ctrl+d"));
    this->addAction(mSetBookmark);
    connect(mSetBookmark, SIGNAL(triggered()), this, SLOT(setBookmark()));


    //---------------------- Go to -----------------------------------
    QMenu* wGotoMenu = new QMenu("Go to", this);
    mGotoOrigin = new QAction("Origin", this);
    mGotoOrigin->setShortcutContext(Qt::WidgetShortcut);
    mGotoOrigin->setShortcut(QKeySequence("*"));
    this->addAction(mGotoOrigin);
    connect(mGotoOrigin, SIGNAL(triggered()), this, SLOT(gotoOrigin()));
    wGotoMenu->addAction(mGotoOrigin);


    //---------------------- Breakpoints -----------------------------
    QMenu* wBPMenu = new QMenu("Breakpoint", this);

    // Standard breakpoint (option set using SetBPXOption)
    mToggleInt3BpAction = new QAction("Toggle", this);
    mToggleInt3BpAction->setShortcutContext(Qt::WidgetShortcut);
    mToggleInt3BpAction->setShortcut(QKeySequence(Qt::Key_F2));
    this->addAction(mToggleInt3BpAction);
    connect(mToggleInt3BpAction, SIGNAL(triggered()), this, SLOT(toggleInt3BPAction()));
    wBPMenu->addAction(mToggleInt3BpAction);

    // HW BP
    mToggleHwBpAction = new QAction("Set Hardware on Execution", this);
    connect(mToggleHwBpAction, SIGNAL(triggered()), this, SLOT(toggleHwBpActionSlot()));
    wBPMenu->addAction(mToggleHwBpAction);

    //---------------------- New origin here -----------------------------
    mSetNewOriginHere = new QAction("Set New Origin Here", this);
    mSetNewOriginHere->setShortcutContext(Qt::WidgetShortcut);
    mSetNewOriginHere->setShortcut(QKeySequence("ctrl+*"));
    this->addAction(mSetNewOriginHere);
    connect(mSetNewOriginHere, SIGNAL(triggered()), this, SLOT(setNewOriginHereActionSlot()));

    //Add to menu
    mRightClickContextMenu->addAction(mSetLabel);
    mRightClickContextMenu->addAction(mSetComment);
    mRightClickContextMenu->addAction(mSetBookmark);
    mRightClickContextMenu->addMenu(wBPMenu); //Breakpoint->
    mRightClickContextMenu->addSeparator(); //Seperator
    mRightClickContextMenu->addMenu(wGotoMenu); //Go to->
    mRightClickContextMenu->addAction(mSetNewOriginHere); //New origin here
}

void CPUDisassembly::gotoOrigin()
{
    Bridge::getBridge()->execCmd("d cip");
}


void CPUDisassembly::toggleInt3BPAction()
{
    uint_t wVA = rvaToVa(getInitialSelection());
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_normal) == bp_normal)
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
    uint_t wVA = rvaToVa(getInitialSelection());
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_hardware) == bp_hardware)
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
    uint_t wVA = rvaToVa(getInitialSelection());
    QString wCmd = "cip=" + QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    Bridge::getBridge()->execCmd(wCmd.toUtf8().constData());
}

void CPUDisassembly::setLabel()
{
    uint_t wVA = rvaToVa(getInitialSelection());
    LineEditDialog mLineEdit(this);
    QString addr_text=QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    char label_text[MAX_COMMENT_SIZE]="";
    if(DbgGetLabelAt((duint)wVA, SEG_DEFAULT, label_text))
        mLineEdit.setText(QString(label_text));
    mLineEdit.setWindowTitle("Add label at " + addr_text);
    if(mLineEdit.exec()!=QDialog::Accepted)
        return;
    if(!DbgSetLabelAt(wVA, mLineEdit.editText.toUtf8().constData()))
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "DbgSetLabelAt failed!");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.exec();
    }
}

void CPUDisassembly::setComment()
{
    uint_t wVA = rvaToVa(getInitialSelection());
    LineEditDialog mLineEdit(this);
    QString addr_text=QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    char comment_text[MAX_COMMENT_SIZE]="";
    if(DbgGetCommentAt((duint)wVA, comment_text))
        mLineEdit.setText(QString(comment_text));
    mLineEdit.setWindowTitle("Add comment at " + addr_text);
    if(mLineEdit.exec()!=QDialog::Accepted)
        return;
    if(!DbgSetCommentAt(wVA, mLineEdit.editText.toUtf8().constData()))
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "DbgSetCommentAt failed!");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.exec();
    }
}

void CPUDisassembly::setBookmark()
{
    uint_t wVA = rvaToVa(getInitialSelection());
    bool result;
    if(DbgGetBookmarkAt(wVA))
        result=DbgSetBookmarkAt(wVA, false);
    else
        result=DbgSetBookmarkAt(wVA, true);
    if(!result)
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "DbgSetBookmarkAt failed!");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.exec();
    }
}
