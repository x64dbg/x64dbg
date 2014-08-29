#include "ThreadView.h"
#include "Configuration.h"
#include "Bridge.h"

void ThreadView::contextMenuSlot(const QPoint & pos)
{
    if(!DbgIsDebugging())
        return;

    QMenu* wMenu = new QMenu(this); //create context menu
    wMenu->addAction(mSwitchThread);
    wMenu->addAction(mSuspendThread);
    wMenu->addAction(mResumeThread);
    wMenu->addAction(mKillThread);
    wMenu->addSeparator();
    wMenu->addMenu(mSetPriority);
    bool ok;
    ULONGLONG entry = getCellContent(getInitialSelection(), 2).toULongLong(&ok, 16);
    if(ok && DbgMemIsValidReadPtr(entry))
    {
        wMenu->addSeparator();
        wMenu->addAction(mGoToThreadEntry);
    }
    wMenu->addSeparator();
    QMenu wCopyMenu("&Copy", this);
    setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu->addSeparator();
        wMenu->addMenu(&wCopyMenu);
    }


    foreach(QAction * action, mSetPriority->actions())
    {
        action->setCheckable(true);
        action->setChecked(false);
    }

    QString priority = getCellContent(getInitialSelection(), 6);
    if(priority == "Normal")
        mSetPriorityNormal->setChecked(true);
    else if(priority == "AboveNormal")
        mSetPriorityAboveNormal->setChecked(true);
    else if(priority == "TimeCritical")
        mSetPriorityTimeCritical ->setChecked(true);
    else if(priority == "Idle")
        mSetPriorityIdle->setChecked(true);
    else if(priority == "BelowNormal")
        mSetPriorityBelowNormal->setChecked(true);
    else if(priority == "Highest")
        mSetPriorityHighest->setChecked(true);
    else if(priority == "Lowest")
        mSetPriorityLowest->setChecked(true);

    wMenu->exec(mapToGlobal(pos)); //execute context menu
}

void ThreadView::GoToThreadEntry()
{
    QString addr_text = getCellContent(getInitialSelection(), 2);
    DbgCmdExecDirect(QString("disasm " + addr_text).toUtf8().constData());
    emit showCpu();
}

void ThreadView::SwitchThread()
{
    QString threadId = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("switchthread " + threadId).toUtf8().constData());
}

void ThreadView::SuspendThread()
{
    QString threadId = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("suspendthread " + threadId).toUtf8().constData());
}

void ThreadView::ResumeThread()
{
    QString threadId = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("resumethread " + threadId).toUtf8().constData());
}

void ThreadView::KillThread()
{
    QString threadId = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("killthread " + threadId).toUtf8().constData());
}

void ThreadView::SetPriorityIdleSlot()
{
    QString threadId = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("setprioritythread " + threadId + ", Idle").toUtf8().constData());
}

void ThreadView::SetPriorityAboveNormalSlot()
{
    QString threadId = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("setprioritythread " + threadId + ", AboveNormal").toUtf8().constData());
}

void ThreadView::SetPriorityBelowNormalSlot()
{
    QString threadId = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("setprioritythread " + threadId + ", BelowNormal").toUtf8().constData());
}

void ThreadView::SetPriorityHighestSlot()
{
    QString threadId = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("setprioritythread " + threadId + ", Highest").toUtf8().constData());
}

void ThreadView::SetPriorityLowestSlot()
{
    QString threadId = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("setprioritythread " + threadId + ", Lowest").toUtf8().constData());
}

void ThreadView::SetPriorityNormalSlot()
{
    QString threadId = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("setprioritythread " + threadId + ", Normal").toUtf8().constData());
}

void ThreadView::SetPriorityTimeCriticalSlot()
{
    QString threadId = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("setprioritythread " + threadId + ", TimeCritical").toUtf8().constData());
}

void ThreadView::setupContextMenu()
{
    //Switch thread menu
    mSwitchThread = new QAction("Switch Thread", this);
    connect(mSwitchThread, SIGNAL(triggered()), this, SLOT(SwitchThread()));

    //Suspend thread menu
    mSuspendThread = new QAction("Suspend Thread", this);
    connect(mSuspendThread, SIGNAL(triggered()), this, SLOT(SuspendThread()));

    //Resume thread menu
    mResumeThread = new QAction("Resume Thread", this);
    connect(mResumeThread, SIGNAL(triggered()), this, SLOT(ResumeThread()));

    //Kill thread menu
    mKillThread = new QAction("Kill Thread", this);
    connect(mKillThread, SIGNAL(triggered()), this, SLOT(KillThread()));

    // Set priority
    mSetPriority = new QMenu("Set Priority", this);

    mSetPriorityIdle = new QAction("Idle", this);
    connect(mSetPriorityIdle, SIGNAL(triggered()), this, SLOT(SetPriorityIdleSlot()));
    mSetPriority->addAction(mSetPriorityIdle);

    mSetPriorityAboveNormal = new QAction("Above Normal", this);
    connect(mSetPriorityAboveNormal, SIGNAL(triggered()), this, SLOT(SetPriorityAboveNormalSlot()));
    mSetPriority->addAction(mSetPriorityAboveNormal);

    mSetPriorityBelowNormal = new QAction("Below Normal", this);
    connect(mSetPriorityBelowNormal, SIGNAL(triggered()), this, SLOT(SetPriorityBelowNormalSlot()));
    mSetPriority->addAction(mSetPriorityBelowNormal);

    mSetPriorityHighest = new QAction("Highest", this);
    connect(mSetPriorityHighest, SIGNAL(triggered()), this, SLOT(SetPriorityHighestSlot()));
    mSetPriority->addAction(mSetPriorityHighest);

    mSetPriorityLowest = new QAction("Lowest", this);
    connect(mSetPriorityLowest, SIGNAL(triggered()), this, SLOT(SetPriorityLowestSlot()));
    mSetPriority->addAction(mSetPriorityLowest);

    mSetPriorityNormal = new QAction("Normal", this);
    connect(mSetPriorityNormal, SIGNAL(triggered()), this, SLOT(SetPriorityNormalSlot()));
    mSetPriority->addAction(mSetPriorityNormal);

    mSetPriorityTimeCritical = new QAction("Time Critical", this);
    connect(mSetPriorityTimeCritical, SIGNAL(triggered()), this, SLOT(SetPriorityTimeCriticalSlot()));
    mSetPriority->addAction(mSetPriorityTimeCritical);

    // GoToThreadEntry
    mGoToThreadEntry = new QAction("Go to Thread Entry", this);
    connect(mGoToThreadEntry, SIGNAL(triggered()), this, SLOT(GoToThreadEntry()));

}

ThreadView::ThreadView(StdTable* parent) : StdTable(parent)
{
    int charwidth = getCharWidth();
    addColumnAt(8 + charwidth * sizeof(unsigned int) * 2, "Number", false);
    addColumnAt(8 + charwidth * sizeof(unsigned int) * 2, "ID", false);
    addColumnAt(8 + charwidth * sizeof(uint_t) * 2, "Entry", false);
    addColumnAt(8 + charwidth * sizeof(uint_t) * 2, "TEB", false);
#ifdef _WIN64
    addColumnAt(8 + charwidth * sizeof(uint_t) * 2, "RIP", false);
#else
    addColumnAt(8 + charwidth * sizeof(uint_t) * 2, "EIP", false);
#endif //_WIN64
    addColumnAt(8 + charwidth * 14, "Suspend Count", false);
    addColumnAt(8 + charwidth * 12, "Priority", false);
    addColumnAt(8 + charwidth * 16, "WaitReason", false);
    addColumnAt(8 + charwidth * 10, "LastError", false);
    addColumnAt(0, "Name", false);

    //setCopyMenuOnly(true);

    connect(Bridge::getBridge(), SIGNAL(updateThreads()), this, SLOT(updateThreadList()));
    connect(this, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));

    setupContextMenu();
}

void ThreadView::updateThreadList()
{
    THREADLIST threadList;
    memset(&threadList, 0, sizeof(THREADLIST));
    DbgGetThreadList(&threadList);
    setRowCount(threadList.count);
    for(int i = 0; i < threadList.count; i++)
    {
        if(!threadList.list[i].BasicInfo.ThreadNumber)
            setCellContent(i, 0, "Main");
        else
            setCellContent(i, 0, QString("%1").arg(threadList.list[i].BasicInfo.ThreadNumber, 0, 10));
        setCellContent(i, 1, QString("%1").arg(threadList.list[i].BasicInfo.dwThreadId, 0, 16).toUpper());
        setCellContent(i, 2, QString("%1").arg(threadList.list[i].BasicInfo.ThreadStartAddress, sizeof(int_t) * 2, 16, QChar('0')).toUpper());
        setCellContent(i, 3, QString("%1").arg(threadList.list[i].BasicInfo.ThreadLocalBase, sizeof(int_t) * 2, 16, QChar('0')).toUpper());
        setCellContent(i, 4, QString("%1").arg(threadList.list[i].ThreadCip, sizeof(int_t) * 2, 16, QChar('0')).toUpper());
        setCellContent(i, 5, QString().sprintf("%d", threadList.list[i].SuspendCount));
        QString priorityString;
        switch(threadList.list[i].Priority)
        {
        case _PriorityIdle:
            priorityString = "Idle";
            break;
        case _PriorityAboveNormal:
            priorityString = "AboveNormal";
            break;
        case _PriorityBelowNormal:
            priorityString = "BelowNormal";
            break;
        case _PriorityHighest:
            priorityString = "Highest";
            break;
        case _PriorityLowest:
            priorityString = "Lowest";
            break;
        case _PriorityNormal:
            priorityString = "Normal";
            break;
        case _PriorityTimeCritical:
            priorityString = "TimeCritical";
            break;
        default:
            priorityString = "Unknown";
            break;
        }
        setCellContent(i, 6, priorityString);
        QString waitReasonString;
        switch(threadList.list[i].WaitReason)
        {
        case _Executive:
            waitReasonString = "Executive";
            break;
        case _FreePage:
            waitReasonString = "FreePage";
            break;
        case _PageIn:
            waitReasonString = "PageIn";
            break;
        case _PoolAllocation:
            waitReasonString = "PoolAllocation";
            break;
        case _DelayExecution:
            waitReasonString = "DelayExecution";
            break;
        case _Suspended:
            waitReasonString = "Suspended";
            break;
        case _UserRequest:
            waitReasonString = "UserRequest";
            break;
        case _WrExecutive:
            waitReasonString = "WrExecutive";
            break;
        case _WrFreePage:
            waitReasonString = "WrFreePage";
            break;
        case _WrPageIn:
            waitReasonString = "WrPageIn";
            break;
        case _WrPoolAllocation:
            waitReasonString = "WrPoolAllocation";
            break;
        case _WrDelayExecution:
            waitReasonString = "WrDelayExecution";
            break;
        case _WrSuspended:
            waitReasonString = "WrSuspended";
            break;
        case _WrUserRequest:
            waitReasonString = "WrUserRequest";
            break;
        case _WrEventPair:
            waitReasonString = "WrEventPair";
            break;
        case _WrQueue:
            waitReasonString = "WrQueue";
            break;
        case _WrLpcReceive:
            waitReasonString = "WrLpcReceive";
            break;
        case _WrLpcReply:
            waitReasonString = "WrLpcReply";
            break;
        case _WrVirtualMemory:
            waitReasonString = "WrVirtualMemory";
            break;
        case _WrPageOut:
            waitReasonString = "WrPageOut";
            break;
        case _WrRendezvous:
            waitReasonString = "WrRendezvous";
            break;
        case _Spare2:
            waitReasonString = "Spare2";
            break;
        case _Spare3:
            waitReasonString = "Spare3";
            break;
        case _Spare4:
            waitReasonString = "Spare4";
            break;
        case _Spare5:
            waitReasonString = "Spare5";
            break;
        case _WrCalloutStack:
            waitReasonString = "WrCalloutStack";
            break;
        case _WrKernel:
            waitReasonString = "WrKernel";
            break;
        case _WrResource:
            waitReasonString = "WrResource";
            break;
        case _WrPushLock:
            waitReasonString = "WrPushLock";
            break;
        case _WrMutex:
            waitReasonString = "WrMutex";
            break;
        case _WrQuantumEnd:
            waitReasonString = "WrQuantumEnd";
            break;
        case _WrDispatchInt:
            waitReasonString = "WrDispatchInt";
            break;
        case _WrPreempted:
            waitReasonString = "WrPreempted";
            break;
        case _WrYieldExecution:
            waitReasonString = "WrYieldExecution";
            break;
        case _WrFastMutex:
            waitReasonString = "WrFastMutex";
            break;
        case _WrGuardedMutex:
            waitReasonString = "WrGuardedMutex";
            break;
        case _WrRundown:
            waitReasonString = "WrRundown";
            break;
        default:
            waitReasonString = "Unknown";
            break;
        }
        setCellContent(i, 7, waitReasonString);
        setCellContent(i, 8, QString("%1").arg(threadList.list[i].LastError, sizeof(unsigned int) * 2, 16, QChar('0')).toUpper());
        setCellContent(i, 9, threadList.list[i].BasicInfo.threadName);
    }
    if(threadList.count)
        BridgeFree(threadList.list);
    mCurrentThread = threadList.CurrentThread;
    reloadData();
}

QString ThreadView::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    QString ret = StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
    if(rowBase + rowOffset == mCurrentThread && !col)
    {
        painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("ThreadCurrentBackgroundColor")));
        painter->setPen(QPen(ConfigColor("ThreadCurrentColor"))); //white text
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, ret);
        ret = "";
    }
    return ret;
}

void ThreadView::doubleClickedSlot()
{
    QString threadId = getCellContent(getInitialSelection(), 1);
    DbgCmdExecDirect(QString("switchthread " + threadId).toUtf8().constData());
    emit showCpu();
}
