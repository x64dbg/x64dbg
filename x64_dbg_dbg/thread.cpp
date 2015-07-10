/**
 @file thread.cpp

 @brief Implements the thread class.
 */

#include "thread.h"
#include "memory.h"
#include "threading.h"

static std::unordered_map<DWORD, THREADINFO> threadList;

void ThreadCreate(CREATE_THREAD_DEBUG_INFO* CreateThread)
{
    THREADINFO curInfo;
    memset(&curInfo, 0, sizeof(THREADINFO));

    curInfo.ThreadNumber = ThreadGetCount();
    curInfo.Handle = CreateThread->hThread;
    curInfo.ThreadId = ((DEBUG_EVENT*)GetDebugData())->dwThreadId;
    curInfo.ThreadStartAddress = (uint)CreateThread->lpStartAddress;
    curInfo.ThreadLocalBase = (uint)CreateThread->lpThreadLocalBase;

    // The first thread (#0) is always the main program thread
    if(curInfo.ThreadNumber <= 0)
        strcpy_s(curInfo.threadName, "Main Thread");

    // Modify global thread list
    EXCLUSIVE_ACQUIRE(LockThreads);
    threadList.insert(std::make_pair(curInfo.ThreadId, curInfo));
    EXCLUSIVE_RELEASE();

    // Notify GUI
    GuiUpdateThreadView();
}

void ThreadExit(DWORD ThreadId)
{
    EXCLUSIVE_ACQUIRE(LockThreads);

    // Don't use a foreach loop here because of the iterator erase() call
    for(auto itr = threadList.begin(); itr != threadList.end(); itr++)
    {
        if(itr->first == ThreadId)
        {
            threadList.erase(itr);
            break;
        }
    }

    EXCLUSIVE_RELEASE();
    GuiUpdateThreadView();
}

void ThreadClear()
{
    // Clear the current array of threads
    EXCLUSIVE_ACQUIRE(LockThreads);
    threadList.clear();
    EXCLUSIVE_RELEASE();

    // Update the GUI's list
    GuiUpdateThreadView();
}

int ThreadGetCount()
{
    SHARED_ACQUIRE(LockThreads);
    return (int)threadList.size();
}

DWORD getLastErrorFromTeb(ULONG_PTR ThreadLocalBase)
{
    TEB teb;

    if(!ThreadGetTeb(ThreadLocalBase, &teb))
        return 0;

    return teb.LastErrorValue;
}

void ThreadGetList(THREADLIST* List)
{
    SHARED_ACQUIRE(LockThreads);

    //
    // This function converts a C++ std::vector to a C-style THREADLIST[].
    // Also assume BridgeAlloc zeros the returned buffer.
    //
    int count = (int)threadList.size();

    if(count <= 0)
        return;

    List->count = count;
    List->list = (THREADALLINFO*)BridgeAlloc(count * sizeof(THREADALLINFO));

    // Fill out the list data
    for(int i = 0; i < count; i++)
    {
        HANDLE threadHandle = threadList[i].Handle;

        // Get the debugger's current thread index
        if(threadHandle == hActiveThread)
            List->CurrentThread = i;

        memcpy(&List->list[i].BasicInfo, &threadList[i], sizeof(THREADINFO));

        List->list[i].ThreadCip = GetContextDataEx(threadHandle, UE_CIP);
        List->list[i].SuspendCount = ThreadGetSuspendCount(threadHandle);
        List->list[i].Priority = ThreadGetPriority(threadHandle);
        List->list[i].WaitReason = ThreadGetWaitReason(threadHandle);
        List->list[i].LastError = getLastErrorFromTeb(threadList[i].ThreadLocalBase);
    }
}

bool ThreadIsValid(DWORD ThreadId)
{
    SHARED_ACQUIRE(LockThreads);
    return threadList.find(ThreadId) != threadList.end();
}

bool ThreadGetTeb(uint TEBAddress, TEB* Teb)
{
    //
    // TODO: Keep a cached copy inside the vector
    //
    memset(Teb, 0, sizeof(TEB));

    return MemRead((void*)TEBAddress, Teb, sizeof(TEB), nullptr);
}

int ThreadGetSuspendCount(HANDLE Thread)
{
    //
    // Suspend a thread in order to get the previous suspension count
    // WARNING: This function is very bad (threads should not be randomly interrupted)
    //
    int suspendCount = (int)SuspendThread(Thread);

    if(suspendCount == -1)
        return 0;

    // Resume the thread's normal execution
    ResumeThread(Thread);

    return suspendCount;
}

THREADPRIORITY ThreadGetPriority(HANDLE Thread)
{
    return (THREADPRIORITY)GetThreadPriority(Thread);
}

THREADWAITREASON ThreadGetWaitReason(HANDLE Thread)
{
    UNREFERENCED_PARAMETER(Thread);

    // TODO: Implement this
    return _Executive;
}

DWORD ThreadGetLastError(DWORD ThreadId)
{
    SHARED_ACQUIRE(LockThreads);

    if(threadList.find(ThreadId) != threadList.end())
        return getLastErrorFromTeb(threadList[ThreadId].ThreadLocalBase);

    return 0;
}

bool ThreadSetName(DWORD ThreadId, const char* Name)
{
    EXCLUSIVE_ACQUIRE(LockThreads);

    // Modifies a variable (name), so an exclusive lock is required
    if(threadList.find(ThreadId) != threadList.end())
    {
        if(!Name)
            Name = "";

        strcpy_s(threadList[ThreadId].threadName, Name);
        return true;
    }

    return false;
}

HANDLE ThreadGetHandle(DWORD ThreadId)
{
    SHARED_ACQUIRE(LockThreads);

    if(threadList.find(ThreadId) != threadList.end())
        return threadList[ThreadId].Handle;

    return nullptr;
}

DWORD ThreadGetId(HANDLE Thread)
{
    SHARED_ACQUIRE(LockThreads);

    // Search for the ID in the local list
    for(auto & entry : threadList)
    {
        if(entry.second.Handle == Thread)
            return entry.first;
    }

    // Wasn't found, check with Windows
    return GetThreadId(Thread);
}

int ThreadSuspendAll()
{
    //
    // SuspendThread does not modify any internal variables
    //
    SHARED_ACQUIRE(LockThreads);

    int count = 0;
    for(auto & entry : threadList)
    {
        if(SuspendThread(entry.second.Handle) != -1)
            count++;
    }

    return count;
}

int ThreadResumeAll()
{
    //
    // ResumeThread does not modify any internal variables
    //
    SHARED_ACQUIRE(LockThreads);

    int count = 0;
    for(auto & entry : threadList)
    {
        if(ResumeThread(entry.second.Handle) != -1)
            count++;
    }

    return count;
}