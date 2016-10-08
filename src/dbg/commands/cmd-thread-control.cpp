#include "cmd-thread-control.h"
#include "variable.h"
#include "memory.h"
#include "value.h"
#include "debugger.h"
#include "console.h"
#include "label.h"
#include "historycontext.h"
#include "thread.h"

CMDRESULT cbDebugCreatethread(int argc, char* argv[])
{
    if(argc < 2)
        return STATUS_ERROR;
    duint Entry = 0;
    duint Argument = 0;
    if(!valfromstring(argv[1], &Entry))
        return STATUS_ERROR;
    if(!MemIsCodePage(Entry, false))
        return STATUS_ERROR;
    if(argc > 2)
    {
        if(!valfromstring(argv[2], &Argument))
            return STATUS_ERROR;
    }
    DWORD ThreadId = 0;
    if(ThreaderCreateRemoteThread(Entry, true, reinterpret_cast<LPVOID>(Argument), &ThreadId) != 0)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Create thread failed!"));
        return STATUS_ERROR;
    }
    else
    {
        char label[MAX_LABEL_SIZE];
        if(!LabelGet(Entry, label))
            label[0] = 0;
#ifdef _WIN64
        dprintf(QT_TRANSLATE_NOOP("DBG", "Thread %X created at %s %p(Argument=%llX)\n"), ThreadId, label, Entry, Argument);
#else //x86
        dprintf(QT_TRANSLATE_NOOP("DBG", "Thread %X created at %s %p(Argument=%X)\n"), ThreadId, label, Entry, Argument);
#endif
        varset("$result", ThreadId, false);
        return STATUS_CONTINUE;
    }
}

CMDRESULT cbDebugSwitchthread(int argc, char* argv[])
{
    duint threadid = fdProcessInfo->dwThreadId; //main thread
    if(argc > 1)
        if(!valfromstring(argv[1], &threadid, false))
            return STATUS_ERROR;
    if(!ThreadIsValid((DWORD)threadid))  //check if the thread is valid
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid thread %X\n"), DWORD(threadid));
        return STATUS_ERROR;
    }
    //switch thread
    if(ThreadGetId(hActiveThread) != threadid)
    {
        hActiveThread = ThreadGetHandle((DWORD)threadid);
        HistoryClear();
        DebugUpdateGuiAsync(GetContextDataEx(hActiveThread, UE_CIP), true);
        dputs(QT_TRANSLATE_NOOP("DBG", "Thread switched!"));
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSuspendthread(int argc, char* argv[])
{
    duint threadid = fdProcessInfo->dwThreadId;
    if(argc > 1)
        if(!valfromstring(argv[1], &threadid, false))
            return STATUS_ERROR;
    if(!ThreadIsValid((DWORD)threadid))  //check if the thread is valid
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid thread %X\n"), DWORD(threadid));
        return STATUS_ERROR;
    }
    //suspend thread
    if(SuspendThread(ThreadGetHandle((DWORD)threadid)) == -1)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error suspending thread"));
        return STATUS_ERROR;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Thread suspended"));
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugResumethread(int argc, char* argv[])
{
    duint threadid = fdProcessInfo->dwThreadId;
    if(argc > 1)
        if(!valfromstring(argv[1], &threadid, false))
            return STATUS_ERROR;
    if(!ThreadIsValid((DWORD)threadid))  //check if the thread is valid
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid thread %X\n"), DWORD(threadid));
        return STATUS_ERROR;
    }
    //resume thread
    if(ResumeThread(ThreadGetHandle((DWORD)threadid)) == -1)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error resuming thread"));
        return STATUS_ERROR;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Thread resumed!"));
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugKillthread(int argc, char* argv[])
{
    duint threadid = fdProcessInfo->dwThreadId;
    if(argc > 1)
        if(!valfromstring(argv[1], &threadid, false))
            return STATUS_ERROR;
    duint exitcode = 0;
    if(argc > 2)
        if(!valfromstring(argv[2], &exitcode, false))
            return STATUS_ERROR;
    if(!ThreadIsValid((DWORD)threadid))  //check if the thread is valid
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid thread %X\n"), DWORD(threadid));
        return STATUS_ERROR;
    }
    //terminate thread
    if(TerminateThread(ThreadGetHandle((DWORD)threadid), (DWORD)exitcode) != 0)
    {
        GuiUpdateAllViews();
        dputs(QT_TRANSLATE_NOOP("DBG", "Thread terminated"));
        return STATUS_CONTINUE;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Error terminating thread!"));
    return STATUS_ERROR;
}

CMDRESULT cbDebugSuspendAllThreads(int argc, char* argv[])
{
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d/%d thread(s) suspended\n"), ThreadSuspendAll(), ThreadGetCount());

    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugResumeAllThreads(int argc, char* argv[])
{
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d/%d thread(s) resumed\n"), ThreadResumeAll(), ThreadGetCount());

    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetPriority(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Not enough arguments!"));
        return STATUS_ERROR;
    }
    duint threadid;
    if(!valfromstring(argv[1], &threadid, false))
        return STATUS_ERROR;
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
            return STATUS_ERROR;
        }
    }
    else
    {
        switch(priority)  //check if the priority value is valid
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
            return STATUS_ERROR;
        }
    }
    if(!ThreadIsValid((DWORD)threadid))  //check if the thread is valid
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid thread %X\n"), DWORD(threadid));
        return STATUS_ERROR;
    }
    //set thread priority
    if(SetThreadPriority(ThreadGetHandle((DWORD)threadid), (int)priority) == 0)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error setting thread priority"));
        return STATUS_ERROR;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Thread priority changed!"));
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetthreadname(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Not enough arguments!"));
        return STATUS_ERROR;
    }
    duint threadid;
    if(!valfromstring(argv[1], &threadid, false))
        return STATUS_ERROR;
    THREADINFO info;
    if(!ThreadGetInfo(DWORD(threadid), info))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid thread %X\n"), DWORD(threadid));
        return STATUS_ERROR;
    }
    auto newname = argc > 2 ? argv[2] : "";
    if(!ThreadSetName(DWORD(threadid), newname))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to change the name for thread %X\n"), DWORD(threadid));
        return STATUS_ERROR;
    }
    if(!*info.threadName)
        dprintf(QT_TRANSLATE_NOOP("DBG", "Thread name set to \"%s\"!\n"), newname);
    else
        dprintf(QT_TRANSLATE_NOOP("DBG", "Thread name changed from \"%s\" to \"%s\"!\n"), info.threadName, newname);
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}