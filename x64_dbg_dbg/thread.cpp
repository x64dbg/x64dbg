#include "thread.h"
#include "console.h"

static std::vector<THREADINFO> threadList;
static int threadNum;
static int currentThread;

void threadcreate(CREATE_THREAD_DEBUG_INFO* CreateThread)
{
    THREADINFO curInfo;
    curInfo.ThreadNumber=threadNum;
    curInfo.hThread=CreateThread->hThread;
    curInfo.dwThreadId=((DEBUG_EVENT*)GetDebugData())->dwThreadId;
    curInfo.ThreadStartAddress=(uint)CreateThread->lpStartAddress;
    curInfo.ThreadLocalBase=(uint)CreateThread->lpThreadLocalBase;
    threadList.push_back(curInfo);
    threadNum++;
    GuiUpdateThreadView();
}

void threadexit(DWORD dwThreadId)
{
    for(int i=0; i<threadList.size(); i++)
        if(threadList.at(i).dwThreadId==dwThreadId)
        {
            threadList.erase(threadList.begin()+i);
            break;
        }
    GuiUpdateThreadView();
}

void threadclear()
{
    threadNum=0;
    std::vector<THREADINFO>().swap(threadList);
    GuiUpdateThreadView();
}

static THREADWAITREASON GetThreadWaitReason(DWORD dwThreadId)
{
    return Executive;
}

void threadgetlist(THREADLIST* list)
{
    int count=threadList.size();
    list->count=count;
    if(!count)
        return;
    list->list=(THREADALLINFO*)BridgeAlloc(count*sizeof(THREADALLINFO));
    for(int i=0; i<count; i++)
    {
        if(((DEBUG_EVENT*)GetDebugData())->dwThreadId==threadList.at(i).dwThreadId)
            currentThread=i;
        memset(&list->list[i], 0, sizeof(THREADALLINFO));
        memcpy(&list->list[i].BasicInfo, &threadList.at(i), sizeof(THREADINFO));
        HANDLE hThread=list->list[i].BasicInfo.hThread;
        list->list[i].ThreadCip=GetContextDataEx(hThread, UE_CIP);
        list->list[i].SuspendCount=SuspendThread(hThread);
        ResumeThread(hThread);
        list->list[i].Priority=(THREADPRIORITY)GetThreadPriority(list->list[i].BasicInfo.hThread);
        list->list[i].WaitReason=GetThreadWaitReason(list->list[i].BasicInfo.dwThreadId);
    }
    list->CurrentThread=currentThread;
}