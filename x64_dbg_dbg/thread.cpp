#include "thread.h"
#include "console.h"
#include "undocumented.h"
#include "memory.h"
#include "threading.h"

static std::vector<THREADINFO> threadList;
static int threadNum;
static int currentThread;

void threadcreate(CREATE_THREAD_DEBUG_INFO* CreateThread)
{
    THREADINFO curInfo;
    curInfo.ThreadNumber = threadNum;
    curInfo.hThread = CreateThread->hThread;
    curInfo.dwThreadId = ((DEBUG_EVENT*)GetDebugData())->dwThreadId;
    curInfo.ThreadStartAddress = (uint)CreateThread->lpStartAddress;
    curInfo.ThreadLocalBase = (uint)CreateThread->lpThreadLocalBase;
    *curInfo.threadName = '\0';
    if(!threadNum)
        strcpy(curInfo.threadName, "Main Thread");
    CriticalSectionLocker locker(LockThreads);
    threadList.push_back(curInfo);
    threadNum++;
    locker.unlock(); //prevent possible deadlocks
    GuiUpdateThreadView();
}

void threadexit(DWORD dwThreadId)
{
    CriticalSectionLocker locker(LockThreads);
    for(unsigned int i = 0; i < threadList.size(); i++)
        if(threadList.at(i).dwThreadId == dwThreadId)
        {
            threadList.erase(threadList.begin() + i);
            break;
        }
    locker.unlock(); //prevent possible deadlocks
    GuiUpdateThreadView();
}

void threadclear()
{
    threadNum = 0;
    CriticalSectionLocker locker(LockThreads);
    std::vector<THREADINFO>().swap(threadList);
    locker.unlock(); //prevent possible deadlocks
    GuiUpdateThreadView();
}

static THREADWAITREASON GetThreadWaitReason(DWORD dwThreadId)
{
    //TODO: implement this
    return _Executive;
}

static DWORD GetThreadLastError(uint tebAddress)
{
    TEB teb;
    memset(&teb, 0, sizeof(TEB));
    if(!memread(fdProcessInfo->hProcess, (void*)tebAddress, &teb, sizeof(TEB), 0))
        return 0;
    return teb.LastErrorValue;
}

void threadgetlist(THREADLIST* list)
{
    CriticalSectionLocker locker(LockThreads);
    int count = (int)threadList.size();
    list->count = count;
    if(!count)
        return;
    list->list = (THREADALLINFO*)BridgeAlloc(count * sizeof(THREADALLINFO));
    for(int i = 0; i < count; i++)
    {
        if(hActiveThread == threadList.at(i).hThread)
            currentThread = i;
        memset(&list->list[i], 0, sizeof(THREADALLINFO));
        memcpy(&list->list[i].BasicInfo, &threadList.at(i), sizeof(THREADINFO));
        HANDLE hThread = list->list[i].BasicInfo.hThread;
        list->list[i].ThreadCip = GetContextDataEx(hThread, UE_CIP);
        list->list[i].SuspendCount = SuspendThread(hThread);
        ResumeThread(hThread);
        list->list[i].Priority = (THREADPRIORITY)GetThreadPriority(list->list[i].BasicInfo.hThread);
        list->list[i].WaitReason = GetThreadWaitReason(list->list[i].BasicInfo.dwThreadId);
        list->list[i].LastError = GetThreadLastError(list->list[i].BasicInfo.ThreadLocalBase);
    }
    list->CurrentThread = currentThread;
}

bool threadisvalid(DWORD dwThreadId)
{
    CriticalSectionLocker locker(LockThreads);
    for(unsigned int i = 0; i < threadList.size(); i++)
        if(threadList.at(i).dwThreadId == dwThreadId)
            return true;
    return false;
}

bool threadsetname(DWORD dwThreadId, const char* name)
{
    CriticalSectionLocker locker(LockThreads);
    for(unsigned int i = 0; i < threadList.size(); i++)
        if(threadList.at(i).dwThreadId == dwThreadId)
        {
            if(name)
                strcpy_s(threadList.at(i).threadName, name);
            else
                *threadList.at(i).threadName = '\0';
        }
    return false;
}

HANDLE threadgethandle(DWORD dwThreadId)
{
    CriticalSectionLocker locker(LockThreads);
    for(unsigned int i = 0; i < threadList.size(); i++)
        if(threadList.at(i).dwThreadId == dwThreadId)
            return threadList.at(i).hThread;
    return 0;
}

DWORD threadgetid(HANDLE hThread)
{
    CriticalSectionLocker locker(LockThreads);
    for(unsigned int i = 0; i < threadList.size(); i++)
        if(threadList.at(i).hThread == hThread)
            return threadList.at(i).dwThreadId;
    return 0;
}

int threadgetcount()
{
    return (int)threadList.size();
}

int threadsuspendall()
{
    CriticalSectionLocker locker(LockThreads);
    int count = 0;
    for(unsigned int i = 0; i < threadList.size(); i++)
        if(SuspendThread(threadList.at(i).hThread) != -1)
            count++;
    return count;
}

int threadresumeall()
{
    CriticalSectionLocker locker(LockThreads);
    int count = 0;
    for(unsigned int i = 0; i < threadList.size(); i++)
        if(ResumeThread(threadList.at(i).hThread) != -1)
            count++;
    return count;
}