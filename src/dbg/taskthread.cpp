#include <Windows.h>
#include "taskthread.h"
#include <mutex>
#include <condition_variable>
#include <thread>

struct TaskThreadDetails
{
    HANDLE threadHandle;
    bool active = true;
    bool pending = false;
    void* arg = 0;
    TaskThread::TaskFunction_t fn;
    CRITICAL_SECTION access;
    CONDITION_VARIABLE wakeup;

    size_t minSleepTimeMs;
    size_t wakeups = 0;
    size_t execs = 0;
    TaskThreadDetails()
    {
        InitializeCriticalSection(&access);
        InitializeConditionVariable(&wakeup);
    }
    ~TaskThreadDetails()
    {
        active = false;
        pending = true;
        {
            EnterCriticalSection(&access);
            WakeConditionVariable(&wakeup);
            LeaveCriticalSection(&access);
        }
        WaitForSingleObject(threadHandle, INFINITE);
        CloseHandle(threadHandle);
    }
    void WakeUp(void* _arg)
    {
        EnterCriticalSection(&access);
        wakeups++;
        arg = _arg;
        pending = true;
        WakeConditionVariable(&wakeup);
        LeaveCriticalSection(&access);
    }

    void Loop()
    {
        void* argLatch = 0;
        while(active)
        {
            {
                EnterCriticalSection(&access);
                while(pending == false)
                    SleepConditionVariableCS(&wakeup, &access, INFINITE);
                argLatch = arg;
                pending = false;
                LeaveCriticalSection(&access);
            }

            if(active)
            {
                fn(argLatch);
                std::this_thread::sleep_for(std::chrono::milliseconds(minSleepTimeMs));
                execs++;
            }
        }
    }
};

static DWORD WINAPI __task_loop(void* ptr)
{
    ((TaskThreadDetails*)ptr)->Loop();
    return 0;
}

void TaskThread::WakeUp(void* arg)
{
    details->WakeUp(arg);
}
TaskThread::TaskThread(TaskFunction_t fn,
                       size_t minSleepTimeMs,
                       _SECURITY_ATTRIBUTES*  lpThreadAttributes,
                       SIZE_T                 dwStackSize)
{
    details = new TaskThreadDetails();
    details->fn = fn;
    details->minSleepTimeMs = minSleepTimeMs;
    details->threadHandle = CreateThread(lpThreadAttributes, dwStackSize, __task_loop, details, 0, 0);
}
TaskThread::~TaskThread()
{
    delete details;
    details = 0;
}
