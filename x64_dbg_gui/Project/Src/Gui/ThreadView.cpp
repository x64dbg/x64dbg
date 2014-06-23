#include "ThreadView.h"

ThreadView::ThreadView(StdTable *parent) : StdTable(parent)
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    addColumnAt(8+charwidth*sizeof(unsigned int)*2, "Number", false);
    addColumnAt(8+charwidth*sizeof(unsigned int)*2, "ID", false);
    addColumnAt(8+charwidth*sizeof(uint_t)*2, "Entry", false);
    addColumnAt(8+charwidth*sizeof(uint_t)*2, "TEB", false);
#ifdef _WIN64
    addColumnAt(8+charwidth*sizeof(uint_t)*2, "RIP", false);
#else
    addColumnAt(8+charwidth*sizeof(uint_t)*2, "EIP", false);
#endif //_WIN64
    addColumnAt(8+charwidth*14, "Suspend Count", false);
    addColumnAt(8+charwidth*12, "Priority", false);
    addColumnAt(8+charwidth*16, "WaitReason", false);
    addColumnAt(8+charwidth*10, "LastError", false);
    addColumnAt(0, "Name", false);

    connect(Bridge::getBridge(), SIGNAL(updateThreads()), this, SLOT(updateThreadList()));
}

void ThreadView::updateThreadList()
{
    THREADLIST threadList;
    memset(&threadList, 0, sizeof(THREADLIST));
    DbgGetThreadList(&threadList);
    setRowCount(threadList.count);
    for(int i=0; i<threadList.count; i++)
    {
        if(!threadList.list[i].BasicInfo.ThreadNumber)
            setCellContent(i, 0, "Main");
        else
            setCellContent(i, 0, QString("%1").arg(threadList.list[i].BasicInfo.ThreadNumber, 0, 10));
        setCellContent(i, 1, QString("%1").arg(threadList.list[i].BasicInfo.dwThreadId, 0, 16).toUpper());
        setCellContent(i, 2, QString("%1").arg(threadList.list[i].BasicInfo.ThreadStartAddress, sizeof(int_t) * 2, 16, QChar('0')).toUpper());
        setCellContent(i, 3, QString("%1").arg(threadList.list[i].BasicInfo.ThreadLocalBase, sizeof(int_t) * 2, 16, QChar('0')).toUpper());
        setCellContent(i, 4, QString("%1").arg(threadList.list[i].ThreadCip, sizeof(int_t) * 2, 16, QChar('0')).toUpper());
        setCellContent(i, 5, QString("%1").arg(threadList.list[i].SuspendCount, 0, 10));
        QString priorityString;
        switch(threadList.list[i].Priority)
        {
        case _PriorityIdle:
            priorityString="Idle";
            break;
        case _PriorityAboveNormal:
            priorityString="AboveNormal";
            break;
        case _PriorityBelowNormal:
            priorityString="BelowNormal";
            break;
        case _PriorityHighest:
            priorityString="Highest";
            break;
        case _PriorityLowest:
            priorityString="Lowest";
            break;
        case _PriorityNormal:
            priorityString="Normal";
            break;
        case _PriorityTimeCritical:
            priorityString="TimeCritical";
            break;
        default:
            priorityString="Unknown";
            break;
        }
        setCellContent(i, 6, priorityString);
        QString waitReasonString;
        switch(threadList.list[i].WaitReason)
        {
        case _Executive:
            waitReasonString="Executive";
            break;
        case _FreePage:
            waitReasonString="FreePage";
            break;
        case _PageIn:
            waitReasonString="PageIn";
            break;
        case _PoolAllocation:
            waitReasonString="PoolAllocation";
            break;
        case _DelayExecution:
            waitReasonString="DelayExecution";
            break;
        case _Suspended:
            waitReasonString="Suspended";
            break;
        case _UserRequest:
            waitReasonString="UserRequest";
            break;
        case _WrExecutive:
            waitReasonString="WrExecutive";
            break;
        case _WrFreePage:
            waitReasonString="WrFreePage";
            break;
        case _WrPageIn:
            waitReasonString="WrPageIn";
            break;
        case _WrPoolAllocation:
            waitReasonString="WrPoolAllocation";
            break;
        case _WrDelayExecution:
            waitReasonString="WrDelayExecution";
            break;
        case _WrSuspended:
            waitReasonString="WrSuspended";
            break;
        case _WrUserRequest:
            waitReasonString="WrUserRequest";
            break;
        case _WrEventPair:
            waitReasonString="WrEventPair";
            break;
        case _WrQueue:
            waitReasonString="WrQueue";
            break;
        case _WrLpcReceive:
            waitReasonString="WrLpcReceive";
            break;
        case _WrLpcReply:
            waitReasonString="WrLpcReply";
            break;
        case _WrVirtualMemory:
            waitReasonString="WrVirtualMemory";
            break;
        case _WrPageOut:
            waitReasonString="WrPageOut";
            break;
        case _WrRendezvous:
            waitReasonString="WrRendezvous";
            break;
        case _Spare2:
            waitReasonString="Spare2";
            break;
        case _Spare3:
            waitReasonString="Spare3";
            break;
        case _Spare4:
            waitReasonString="Spare4";
            break;
        case _Spare5:
            waitReasonString="Spare5";
            break;
        case _WrCalloutStack:
            waitReasonString="WrCalloutStack";
            break;
        case _WrKernel:
            waitReasonString="WrKernel";
            break;
        case _WrResource:
            waitReasonString="WrResource";
            break;
        case _WrPushLock:
            waitReasonString="WrPushLock";
            break;
        case _WrMutex:
            waitReasonString="WrMutex";
            break;
        case _WrQuantumEnd:
            waitReasonString="WrQuantumEnd";
            break;
        case _WrDispatchInt:
            waitReasonString="WrDispatchInt";
            break;
        case _WrPreempted:
            waitReasonString="WrPreempted";
            break;
        case _WrYieldExecution:
            waitReasonString="WrYieldExecution";
            break;
        case _WrFastMutex:
            waitReasonString="WrFastMutex";
            break;
        case _WrGuardedMutex:
            waitReasonString="WrGuardedMutex";
            break;
        case _WrRundown:
            waitReasonString="WrRundown";
            break;
        default:
            waitReasonString="Unknown";
            break;
        }
        setCellContent(i, 7, waitReasonString);
        setCellContent(i, 8, QString("%1").arg(threadList.list[i].LastError, sizeof(unsigned int) * 2, 16, QChar('0')).toUpper());
        setCellContent(i, 9, threadList.list[i].BasicInfo.threadName);
    }
    if(threadList.count)
        BridgeFree(threadList.list);
    mCurrentThread=threadList.CurrentThread;
    reloadData();
}

QString ThreadView::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    QString ret=StdTable::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
    if(rowBase+rowOffset==mCurrentThread && !col)
    {
        painter->fillRect(QRect(x, y, w, h), QBrush(ConfigColor("ThreadCurrentBackgroundColor")));
        painter->setPen(QPen(ConfigColor("ThreadCurrentColor"))); //white text
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, ret);
        ret="";
    }
    return ret;
}
