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
    addColumnAt(0, "", false);

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
        case PriorityIdle:
            priorityString="Idle";
        break;
        case PriorityAboveNormal:
            priorityString="AboveNormal";
        break;
        case PriorityBelowNormal:
            priorityString="BelowNormal";
        break;
        case PriorityHighest:
            priorityString="Highest";
        break;
        case PriorityLowest:
            priorityString="Lowest";
        break;
        case PriorityNormal:
            priorityString="Normal";
        break;
        case PriorityTimeCritical:
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
        case Executive:
            waitReasonString="Executive";
        break;
        case FreePage:
            waitReasonString="FreePage";
        break;
        case PageIn:
            waitReasonString="PageIn";
        break;
        case PoolAllocation:
            waitReasonString="PoolAllocation";
        break;
        case DelayExecution:
            waitReasonString="DelayExecution";
        break;
        case Suspended:
            waitReasonString="Suspended";
        break;
        case UserRequest:
            waitReasonString="UserRequest";
        break;
        case WrExecutive:
            waitReasonString="WrExecutive";
        break;
        case WrFreePage:
            waitReasonString="WrFreePage";
        break;
        case WrPageIn:
            waitReasonString="WrPageIn";
        break;
        case WrPoolAllocation:
            waitReasonString="WrPoolAllocation";
        break;
        case WrDelayExecution:
            waitReasonString="WrDelayExecution";
        break;
        case WrSuspended:
            waitReasonString="WrSuspended";
        break;
        case WrUserRequest:
            waitReasonString="WrUserRequest";
        break;
        case WrEventPair:
            waitReasonString="WrEventPair";
        break;
        case WrQueue:
            waitReasonString="WrQueue";
        break;
        case WrLpcReceive:
            waitReasonString="WrLpcReceive";
        break;
        case WrLpcReply:
            waitReasonString="WrLpcReply";
        break;
        case WrVirtualMemory:
            waitReasonString="WrVirtualMemory";
        break;
        case WrPageOut:
            waitReasonString="WrPageOut";
        break;
        case WrRendezvous:
            waitReasonString="WrRendezvous";
        break;
        case Spare2:
            waitReasonString="Spare2";
        break;
        case Spare3:
            waitReasonString="Spare3";
        break;
        case Spare4:
            waitReasonString="Spare4";
        break;
        case Spare5:
            waitReasonString="Spare5";
        break;
        case WrCalloutStack:
            waitReasonString="WrCalloutStack";
        break;
        case WrKernel:
            waitReasonString="WrKernel";
        break;
        case WrResource:
            waitReasonString="WrResource";
        break;
        case WrPushLock:
            waitReasonString="WrPushLock";
        break;
        case WrMutex:
            waitReasonString="WrMutex";
        break;
        case WrQuantumEnd:
            waitReasonString="WrQuantumEnd";
        break;
        case WrDispatchInt:
            waitReasonString="WrDispatchInt";
        break;
        case WrPreempted:
            waitReasonString="WrPreempted";
        break;
        case WrYieldExecution:
            waitReasonString="WrYieldExecution";
        break;
        case WrFastMutex:
            waitReasonString="WrFastMutex";
        break;
        case WrGuardedMutex:
            waitReasonString="WrGuardedMutex";
        break;
        case WrRundown:
            waitReasonString="WrRundown";
        break;
        default:
            waitReasonString="Unknown";
        break;
        }
        setCellContent(i, 7, waitReasonString);
        setCellContent(i, 8, QString("%1").arg(threadList.list[i].LastError, sizeof(unsigned int) * 2, 16, QChar('0')).toUpper());
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
        painter->save();
        painter->fillRect(QRect(x, y, w, h), QBrush(QColor("#000000")));
        painter->setPen(QPen(QColor("#FFFFFF"))); //white text
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, ret);
        painter->restore();
        ret="";
    }
    return ret;
}
