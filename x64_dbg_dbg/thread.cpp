/**
 @file thread.cpp

 @brief Implements the thread class.
 */

#include "thread.h"
#include "console.h"
#include "undocumented.h"
#include "memory.h"
#include "threading.h"

/**
 @brief List of threads.
 */

static std::vector<THREADINFO> threadList;

/**
 @brief The thread number.
 */

static int threadNum;

/**
 @brief The current thread.
 */

static int currentThread;

/**
 @fn void threadcreate(CREATE_THREAD_DEBUG_INFO* CreateThread)

 @brief Threadcreates the given create thread.

 @param [in,out] CreateThread If non-null, the create thread.
 */

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

/**
 @fn void threadexit(DWORD dwThreadId)

 @brief Threadexits the given double-word thread identifier.

 @param dwThreadId Identifier for the thread.
 */

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

/**
 @fn void threadclear()

 @brief Threadclears this object.
 */

void threadclear()
{
    threadNum = 0;
    CriticalSectionLocker locker(LockThreads);
    std::vector<THREADINFO>().swap(threadList);
    locker.unlock(); //prevent possible deadlocks
    GuiUpdateThreadView();
}

/**
 @fn static THREADWAITREASON GetThreadWaitReason(DWORD dwThreadId)

 @brief Gets thread wait reason.

 @param dwThreadId Identifier for the thread.

 @return The thread wait reason.
 */

static THREADWAITREASON GetThreadWaitReason(DWORD dwThreadId)
{
    return _Executive;
}

/**
 @fn static DWORD GetThreadLastError(uint tebAddress)

 @brief Gets thread last error.

 @param tebAddress The teb address.

 @return The thread last error.
 */

static DWORD GetThreadLastError(uint tebAddress)
{
    TEB teb;
    memset(&teb, 0, sizeof(TEB));
    if(!memread(fdProcessInfo->hProcess, (void*)tebAddress, &teb, sizeof(TEB), 0))
        return 0;
    return teb.LastErrorValue;
}

/**
 @fn void threadgetlist(THREADLIST* list)

 @brief Threadgetlists the given list.

 @param [in,out] list If non-null, the list.
 */

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

/**
 @fn bool threadisvalid(DWORD dwThreadId)

 @brief Threadisvalids the given double-word thread identifier.

 @param dwThreadId Identifier for the thread.

 @return true if it succeeds, false if it fails.
 */

bool threadisvalid(DWORD dwThreadId)
{
    CriticalSectionLocker locker(LockThreads);
    for(unsigned int i = 0; i < threadList.size(); i++)
        if(threadList.at(i).dwThreadId == dwThreadId)
            return true;
    return false;
}

/**
 @fn bool threadsetname(DWORD dwThreadId, const char* name)

 @brief Threadsetnames.

 @param dwThreadId Identifier for the thread.
 @param name       The name.

 @return true if it succeeds, false if it fails.
 */

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

/**
 @fn HANDLE threadgethandle(DWORD dwThreadId)

 @brief Threadgethandles the given double-word thread identifier.

 @param dwThreadId Identifier for the thread.

 @return The handle of the.
 */

HANDLE threadgethandle(DWORD dwThreadId)
{
    CriticalSectionLocker locker(LockThreads);
    for(unsigned int i = 0; i < threadList.size(); i++)
        if(threadList.at(i).dwThreadId == dwThreadId)
            return threadList.at(i).hThread;
    return 0;
}

/**
 @fn DWORD threadgetid(HANDLE hThread)

 @brief Threadgetids the given h thread.

 @param hThread Handle of the thread.

 @return A DWORD.
 */

DWORD threadgetid(HANDLE hThread)
{
    CriticalSectionLocker locker(LockThreads);
    for(unsigned int i = 0; i < threadList.size(); i++)
        if(threadList.at(i).hThread == hThread)
            return threadList.at(i).dwThreadId;
    return 0;
}