#include "cmd-thread-control.h"
#include "variable.h"
#include "memory.h"
#include "value.h"
#include "debugger.h"
#include "console.h"
#include "label.h"
#include "historycontext.h"
#include "thread.h"

bool cbDebugCreatethread(int argc, char* argv[])
{
    if(argc < 2)
        return false;
    duint Entry = 0;
    duint Argument = 0;
    if(!valfromstring(argv[1], &Entry))
        return false;
    if(!MemIsCodePage(Entry, false))
        return false;
    if(argc > 2)
    {
        if(!valfromstring(argv[2], &Argument))
            return false;
    }
    DWORD ThreadId = 0;
    auto hThread = CreateRemoteThread(fdProcessInfo->hProcess, nullptr, 0, LPTHREAD_START_ROUTINE(Entry), LPVOID(Argument), 0, &ThreadId);
    if(!hThread)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Create thread failed!"));
        return false;
    }
    else
    {
        CloseHandle(hThread);
        char label[MAX_LABEL_SIZE];
        if(!LabelGet(Entry, label))
            label[0] = 0;
#ifdef _WIN64
        dprintf(QT_TRANSLATE_NOOP("DBG", "Thread %s created at %s %p(Argument=%llX)\n"), formatpidtid(ThreadId).c_str(), label, Entry, Argument);
#else //x86
        dprintf(QT_TRANSLATE_NOOP("DBG", "Thread %s created at %s %p(Argument=%X)\n"), formatpidtid(ThreadId).c_str(), label, Entry, Argument);
#endif
        varset("$result", ThreadId, false);
        return true;
    }
}

bool cbDebugSwitchthread(int argc, char* argv[])
{
    duint threadid = fdProcessInfo->dwThreadId; //main thread
    if(argc > 1)
        if(!valfromstring(argv[1], &threadid, false))
            return false;
    if(!ThreadIsValid((DWORD)threadid)) //check if the thread is valid
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid thread %s\n"), formatpidtid((DWORD)threadid).c_str());
        return false;
    }
    //switch thread
    if(ThreadGetId(hActiveThread) != threadid)
    {
        hActiveThread = ThreadGetHandle((DWORD)threadid);
        HistoryClear();
        DebugUpdateGuiAsync(GetContextDataEx(hActiveThread, UE_CIP), true);
        dputs(QT_TRANSLATE_NOOP("DBG", "Thread switched!"));
    }
    return true;
}

bool cbDebugSuspendthread(int argc, char* argv[])
{
    duint threadid = fdProcessInfo->dwThreadId;
    if(argc > 1)
        if(!valfromstring(argv[1], &threadid, false))
            return false;
    if(!ThreadIsValid((DWORD)threadid)) //check if the thread is valid
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid thread %s\n"), formatpidtid((DWORD)threadid).c_str());
        return false;
    }
    //suspend thread
    if(SuspendThread(ThreadGetHandle((DWORD)threadid)) == -1)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error suspending thread"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Thread suspended"));
    GuiUpdateAllViews();
    return true;
}

bool cbDebugResumethread(int argc, char* argv[])
{
    duint threadid = fdProcessInfo->dwThreadId;
    if(argc > 1)
        if(!valfromstring(argv[1], &threadid, false))
            return false;
    if(!ThreadIsValid((DWORD)threadid)) //check if the thread is valid
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid thread %s\n"), formatpidtid((DWORD)threadid).c_str());
        return false;
    }
    //resume thread
    if(ResumeThread(ThreadGetHandle((DWORD)threadid)) == -1)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error resuming thread"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Thread resumed!"));
    GuiUpdateAllViews();
    return true;
}

bool cbDebugKillthread(int argc, char* argv[])
{
    duint threadid = fdProcessInfo->dwThreadId;
    if(argc > 1)
        if(!valfromstring(argv[1], &threadid, false))
            return false;
    duint exitcode = 0;
    if(argc > 2)
        if(!valfromstring(argv[2], &exitcode, false))
            return false;
    if(!ThreadIsValid((DWORD)threadid)) //check if the thread is valid
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid thread %s\n"), formatpidtid((DWORD)threadid).c_str());
        return false;
    }
    //terminate thread
    if(TerminateThread(ThreadGetHandle((DWORD)threadid), (DWORD)exitcode) != 0)
    {
        GuiUpdateAllViews();
        dputs(QT_TRANSLATE_NOOP("DBG", "Thread terminated"));
        return true;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Error terminating thread!"));
    return false;
}

bool cbDebugSuspendAllThreads(int argc, char* argv[])
{
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d/%d thread(s) suspended\n"), ThreadSuspendAll(), ThreadGetCount());

    GuiUpdateAllViews();
    return true;
}

bool cbDebugResumeAllThreads(int argc, char* argv[])
{
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d/%d thread(s) resumed\n"), ThreadResumeAll(), ThreadGetCount());

    GuiUpdateAllViews();
    return true;
}

bool cbDebugSetPriority(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint threadid;
    if(!valfromstring(argv[1], &threadid, false))
        return false;
    duint priority;
    if(!valfromstring(argv[2], &priority))
    {
        if(_strcmpi(argv[2], "Normal") == 0)
            priority = THREAD_PRIORITY_NORMAL;
        else if(_strcmpi(argv[2], "AboveNormal") == 0)
            priority = THREAD_PRIORITY_ABOVE_NORMAL;
        else if(_strcmpi(argv[2], "TimeCritical") == 0)
            priority = THREAD_PRIORITY_TIME_CRITICAL;
        else if(_strcmpi(argv[2], "Idle") == 0)
            priority = THREAD_PRIORITY_IDLE;
        else if(_strcmpi(argv[2], "BelowNormal") == 0)
            priority = THREAD_PRIORITY_BELOW_NORMAL;
        else if(_strcmpi(argv[2], "Highest") == 0)
            priority = THREAD_PRIORITY_HIGHEST;
        else if(_strcmpi(argv[2], "Lowest") == 0)
            priority = THREAD_PRIORITY_LOWEST;
        else
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Unknown priority value, read the help!"));
            return false;
        }
    }
    else
    {
        switch((int)priority) //check if the priority value is valid
        {
        case THREAD_PRIORITY_NORMAL:
        case THREAD_PRIORITY_ABOVE_NORMAL:
        case THREAD_PRIORITY_TIME_CRITICAL:
        case THREAD_PRIORITY_IDLE:
        case THREAD_PRIORITY_BELOW_NORMAL:
        case THREAD_PRIORITY_HIGHEST:
        case THREAD_PRIORITY_LOWEST:
            break;
        default:
            dputs(QT_TRANSLATE_NOOP("DBG", "Unknown priority value, read the help!"));
            return false;
        }
    }
    if(!ThreadIsValid((DWORD)threadid)) //check if the thread is valid
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid thread %s\n"), formatpidtid((DWORD)threadid).c_str());
        return false;
    }
    //set thread priority
    if(SetThreadPriority(ThreadGetHandle((DWORD)threadid), (int)priority) == 0)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error setting thread priority"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Thread priority changed!"));
    GuiUpdateAllViews();
    return true;
}

bool cbDebugSetthreadname(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint threadid;
    if(!valfromstring(argv[1], &threadid, false))
        return false;
    THREADINFO info;
    if(!ThreadGetInfo((DWORD)threadid, info))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid thread %s\n"), formatpidtid((DWORD)threadid).c_str());
        return false;
    }
    auto newname = argc > 2 ? argv[2] : "";
    if(!ThreadSetName((DWORD)threadid, newname))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to change the name for thread %s\n"), formatpidtid((DWORD)threadid).c_str());
        return false;
    }
    if(!*info.threadName)
        dprintf(QT_TRANSLATE_NOOP("DBG", "Thread name set to \"%s\"!\n"), newname);
    else
        dprintf(QT_TRANSLATE_NOOP("DBG", "Thread name changed from \"%s\" to \"%s\"!\n"), info.threadName, newname);
    GuiUpdateAllViews();
    return true;
}