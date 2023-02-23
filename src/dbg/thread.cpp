/**
 @file thread.cpp

 @brief Implements the thread class.
 */

#include "thread.h"
#include "memory.h"
#include "threading.h"
#include "ntdll/ntdll.h"
#include "debugger.h"

static std::unordered_map<DWORD, THREADINFO> threadList;
static std::unordered_map<DWORD, THREADWAITREASON> threadWaitReasons;

// Function pointer for dynamic linking. Do not link statically for Windows XP compatibility.
// TODO: move this function definition out of thread.cpp
BOOL(WINAPI* QueryThreadCycleTime)(HANDLE ThreadHandle, PULONG64 CycleTime) = nullptr;

BOOL WINAPI QueryThreadCycleTimeUnsupported(HANDLE ThreadHandle, PULONG64 CycleTime)
{
    *CycleTime = 0;
    return TRUE;
}

void ThreadCreate(CREATE_THREAD_DEBUG_INFO* CreateThread)
{
    THREADINFO curInfo;
    memset(&curInfo, 0, sizeof(THREADINFO));

    curInfo.ThreadNumber = ThreadGetCount();
    curInfo.Handle = INVALID_HANDLE_VALUE;
    curInfo.ThreadId = ((DEBUG_EVENT*)GetDebugData())->dwThreadId;
    curInfo.ThreadStartAddress = (duint)CreateThread->lpStartAddress;
    curInfo.ThreadLocalBase = (duint)CreateThread->lpThreadLocalBase;

    // Duplicate the debug thread handle -> thread handle
    DuplicateHandle(GetCurrentProcess(), CreateThread->hThread, GetCurrentProcess(), &curInfo.Handle, 0, FALSE, DUPLICATE_SAME_ACCESS);

    typedef HRESULT(WINAPI * GETTHREADDESCRIPTION)(HANDLE hThread, PWSTR * ppszThreadDescription);
    static GETTHREADDESCRIPTION _GetThreadDescription = (GETTHREADDESCRIPTION)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetThreadDescription");
    PWSTR threadDescription = nullptr;

    // The first thread (#0) is always the main program thread
    if(curInfo.ThreadNumber <= 0)
        strcpy_s(curInfo.threadName, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Main Thread")));
    else if(_GetThreadDescription && SUCCEEDED(_GetThreadDescription(curInfo.Handle, &threadDescription)) && threadDescription)
    {
        if(threadDescription[0])
            strncpy_s(curInfo.threadName, StringUtils::Escape(StringUtils::Utf16ToUtf8(threadDescription)).c_str(), _TRUNCATE);
        LocalFree(threadDescription);
    }
    else
        curInfo.threadName[0] = 0;

    // Modify global thread list
    EXCLUSIVE_ACQUIRE(LockThreads);
    threadList.emplace(curInfo.ThreadId, curInfo);
    EXCLUSIVE_RELEASE();

    // Notify GUI
    GuiUpdateThreadView();
}

void ThreadExit(DWORD ThreadId)
{
    EXCLUSIVE_ACQUIRE(LockThreads);

    // Erase element using native functions
    auto itr = threadList.find(ThreadId);

    if(itr != threadList.end())
    {
        CloseHandle(itr->second.Handle);
        threadList.erase(itr);
    }

    EXCLUSIVE_RELEASE();
    GuiUpdateThreadView();
}

void ThreadClear()
{
    EXCLUSIVE_ACQUIRE(LockThreads);

    // Close all handles first
    for(auto & itr : threadList)
        CloseHandle(itr.second.Handle);

    // Empty the array
    threadList.clear();

    // Update the GUI's list
    EXCLUSIVE_RELEASE();
    GuiUpdateThreadView();
}

int ThreadGetCount()
{
    SHARED_ACQUIRE(LockThreads);
    return (int)threadList.size();
}

void ThreadGetList(THREADLIST* List)
{
    ASSERT_NONNULL(List);
    SHARED_ACQUIRE(LockThreads);

    //
    // This function converts a C++ std::unordered_map to a C-style THREADLIST[].
    // Also assume BridgeAlloc zeros the returned buffer.
    //
    List->count = (int)threadList.size();

    if(List->count == 0)
    {
        List->list = nullptr;
        return;
    }

    // Allocate C-style array
    List->list = (THREADALLINFO*)BridgeAlloc(List->count * sizeof(THREADALLINFO));

    // Fill out the list data
    int index = 0;

    // Unused thread exit time
    FILETIME threadExitTime;

    for(auto & itr : threadList)
    {
        HANDLE threadHandle = itr.second.Handle;

        // Get the debugger's active thread index
        if(threadHandle == hActiveThread)
            List->CurrentThread = index;

        memcpy(&List->list[index].BasicInfo, &itr.second, sizeof(THREADINFO));

        typedef HRESULT(WINAPI * GETTHREADDESCRIPTION)(HANDLE hThread, PWSTR * ppszThreadDescription);
        static GETTHREADDESCRIPTION _GetThreadDescription = (GETTHREADDESCRIPTION)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetThreadDescription");
        PWSTR threadDescription = nullptr;
        if(_GetThreadDescription && SUCCEEDED(_GetThreadDescription(threadHandle, &threadDescription)) && threadDescription)
        {
            // Thread name may have changed
            if(threadDescription[0])
                strncpy_s(List->list[index].BasicInfo.threadName, StringUtils::Escape(StringUtils::Utf16ToUtf8(threadDescription)).c_str(), _TRUNCATE);
            LocalFree(threadDescription);
        }

        List->list[index].ThreadCip = GetContextDataEx(threadHandle, UE_CIP);
        List->list[index].SuspendCount = ThreadGetSuspendCount(threadHandle);
        List->list[index].Priority = ThreadGetPriority(threadHandle);
        List->list[index].LastError = ThreadGetLastErrorTEB(itr.second.ThreadLocalBase);
        GetThreadTimes(threadHandle, &List->list[index].CreationTime, &threadExitTime, &List->list[index].KernelTime, &List->list[index].UserTime);
        List->list[index].Cycles = ThreadQueryCycleTime(threadHandle);
        index++;
    }

    // Get the wait reason for every thread in the list
    for(int i = 0; i < List->count; i++)
    {
        auto found = threadWaitReasons.find(List->list[i].BasicInfo.ThreadId);
        if(found != threadWaitReasons.end())
            List->list[i].WaitReason = found->second;
    }
}

void ThreadGetList(std::vector<THREADINFO> & list)
{
    SHARED_ACQUIRE(LockThreads);
    list.clear();
    list.reserve(threadList.size());
    for(const auto & thread : threadList)
        list.push_back(thread.second);
}

bool ThreadGetInfo(DWORD ThreadId, THREADINFO & info)
{
    SHARED_ACQUIRE(LockThreads);

    auto found = threadList.find(ThreadId);
    if(found == threadList.end())
        return false;

    info = found->second;
    return true;
}

bool ThreadIsValid(DWORD ThreadId)
{
    SHARED_ACQUIRE(LockThreads);
    return threadList.find(ThreadId) != threadList.end();
}

bool ThreadGetTib(duint TEBAddress, NT_TIB* Tib)
{
    // Calculate offset from structure member
    TEBAddress += offsetof(TEB, NtTib);

    memset(Tib, 0, sizeof(NT_TIB));
    return MemReadUnsafe(TEBAddress, Tib, sizeof(NT_TIB));
}

bool ThreadGetTeb(duint TEBAddress, TEB* Teb)
{
    memset(Teb, 0, sizeof(TEB));
    return MemReadUnsafe(TEBAddress, Teb, sizeof(TEB));
}

int ThreadGetSuspendCount(HANDLE Thread)
{
    // Query the suspend count. This only works on Windows 8.1 and later
    DWORD suspendCount;
    if(NT_SUCCESS(NtQueryInformationThread(Thread, ThreadSuspendCount, &suspendCount, sizeof(suspendCount), nullptr)))
    {
        return suspendCount;
    }

    //
    // Suspend a thread in order to get the previous suspension count
    // WARNING: This function is very bad (threads should not be randomly interrupted)
    //

    // Use NtSuspendThread, because there is no Win32 error for STATUS_SUSPEND_COUNT_EXCEEDED
    NTSTATUS status = NtSuspendThread(Thread, &suspendCount);
    if(status == STATUS_SUSPEND_COUNT_EXCEEDED)
        suspendCount = MAXCHAR; // If the thread is already at the max suspend count, KeSuspendThread raises an exception and never returns the count
    else if(!NT_SUCCESS(status))
        suspendCount = 0;

    // Resume the thread's normal execution
    if(NT_SUCCESS(status))
        ResumeThread(Thread);

    return suspendCount;
}

THREADPRIORITY ThreadGetPriority(HANDLE Thread)
{
    return (THREADPRIORITY)GetThreadPriority(Thread);
}

DWORD ThreadGetLastErrorTEB(ULONG_PTR ThreadLocalBase)
{
    // Get the offset for the TEB::LastErrorValue and read it
    DWORD lastError = 0;
    duint structOffset = ThreadLocalBase + offsetof(TEB, LastErrorValue);

    MemReadUnsafe(structOffset, &lastError, sizeof(DWORD));
    return lastError;
}

DWORD ThreadGetLastError(DWORD ThreadId)
{
    SHARED_ACQUIRE(LockThreads);

    if(threadList.find(ThreadId) != threadList.end())
        return ThreadGetLastErrorTEB(threadList[ThreadId].ThreadLocalBase);

    ASSERT_ALWAYS("Trying to get last error of a thread that doesn't exist!");
    return 0;
}

NTSTATUS ThreadGetLastStatusTEB(ULONG_PTR ThreadLocalBase)
{
    // Get the offset for the TEB::LastStatusValue and read it
    NTSTATUS lastStatus = 0;
    duint structOffset = ThreadLocalBase + offsetof(TEB, LastStatusValue);

    MemReadUnsafe(structOffset, &lastStatus, sizeof(NTSTATUS));
    return lastStatus;
}

NTSTATUS ThreadGetLastStatus(DWORD ThreadId)
{
    SHARED_ACQUIRE(LockThreads);

    if(threadList.find(ThreadId) != threadList.end())
        return ThreadGetLastStatusTEB(threadList[ThreadId].ThreadLocalBase);

    ASSERT_ALWAYS("Trying to get last status of a thread that doesn't exist!");
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

        strncpy_s(threadList[ThreadId].threadName, Name, _TRUNCATE);
        return true;
    }

    return false;
}

/**
@brief ThreadGetName Get the name of the thread.
@param ThreadId The id of the thread.
@param Name The returned name of the thread. Must be at least MAX_THREAD_NAME_SIZE size
@return True if the function succeeds. False otherwise.
*/
bool ThreadGetName(DWORD ThreadId, char* Name)
{
    SHARED_ACQUIRE(LockThreads);
    if(threadList.find(ThreadId) != threadList.end())
    {
        strcpy_s(Name, MAX_THREAD_NAME_SIZE, threadList[ThreadId].threadName);
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
    typedef DWORD (WINAPI * GETTHREADID)(HANDLE hThread);
    static GETTHREADID _GetThreadId = (GETTHREADID)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetThreadId");
    return _GetThreadId ? _GetThreadId(Thread) : 0;
}

int ThreadSuspendAll()
{
    // SuspendThread does not modify any internal variables
    SHARED_ACQUIRE(LockThreads);

    int count = 0;
    for(auto & entry : threadList)
    {
        if(SuspendThread(entry.second.Handle) != -1)
            count++;
        else
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to suspend thread 0x%X...\n"), entry.second.ThreadId);
    }

    return count;
}

int ThreadResumeAll()
{
    // ResumeThread does not modify any internal variables
    SHARED_ACQUIRE(LockThreads);

    int count = 0;
    for(auto & entry : threadList)
    {
        if(ResumeThread(entry.second.Handle) != -1)
            count++;
    }

    return count;
}

ULONG_PTR ThreadGetLocalBase(DWORD ThreadId)
{
    SHARED_ACQUIRE(LockThreads);
    auto found = threadList.find(ThreadId);
    return found != threadList.end() ? found->second.ThreadLocalBase : 0;
}

ULONG64 ThreadQueryCycleTime(HANDLE hThread)
{
    ULONG64 CycleTime;

    // Initialize function pointer
    if(QueryThreadCycleTime == nullptr)
    {
        QueryThreadCycleTime = (BOOL(WINAPI*)(HANDLE, PULONG64))GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "QueryThreadCycleTime");
        if(QueryThreadCycleTime == nullptr)
            QueryThreadCycleTime = QueryThreadCycleTimeUnsupported;
    }

    if(!QueryThreadCycleTime(hThread, &CycleTime))
        CycleTime = 0;
    return CycleTime;
}

void ThreadUpdateWaitReasons()
{
    ULONG size;
    if(NtQuerySystemInformation(SystemProcessInformation, NULL, 0, &size) != STATUS_INFO_LENGTH_MISMATCH)
        return;
    Memory<PSYSTEM_PROCESS_INFORMATION> systemProcessInfo(2 * size, "_dbg_threadwaitreason");
    NTSTATUS status = NtQuerySystemInformation(SystemProcessInformation, systemProcessInfo(), (ULONG)systemProcessInfo.size(), NULL);
    if(!NT_SUCCESS(status))
        return;

    PSYSTEM_PROCESS_INFORMATION process = systemProcessInfo();

    EXCLUSIVE_ACQUIRE(LockThreads);
    while(true)
    {
        for(ULONG thread = 0; thread < process->NumberOfThreads; ++thread)
        {
            auto tid = (DWORD)process->Threads[thread].ClientId.UniqueThread;
            if(threadList.count(tid))
                threadWaitReasons[tid] = (THREADWAITREASON)process->Threads[thread].WaitReason;
        }
        if(process->NextEntryOffset == 0) // Last entry
            break;
        process = (PSYSTEM_PROCESS_INFORMATION)((ULONG_PTR)process + process->NextEntryOffset);
    }
}