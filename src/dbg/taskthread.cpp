#include "taskthread.h"
#include <thread>

struct TaskThreadDetails
{
    HANDLE threadHandle;
    bool active = true;
    void* arg = 0;
    TaskThread::TaskFunction_t fn;
    CRITICAL_SECTION access;
    HANDLE wakeup;

    size_t minSleepTimeMs;
    size_t wakeups = 0;
    size_t execs = 0;
    TaskThreadDetails()
    {
        wakeup = CreateSemaphore(0, 0, 1, 0);
        InitializeCriticalSection(&access);
    }
    ~TaskThreadDetails()
    {
        EnterCriticalSection(&access);
        active = false;
        LeaveCriticalSection(&access);
        ReleaseSemaphore(wakeup, 1, 0);

        WaitForSingleObject(threadHandle, INFINITE);
        CloseHandle(threadHandle);
        DeleteCriticalSection(&access);
        CloseHandle(wakeup);
    }
    void WakeUp(void* _arg)
    {
        wakeups++;
        EnterCriticalSection(&access);
        arg = _arg;
        LeaveCriticalSection(&access);
        // This will fail if it's redundant, which is what we want.
        ReleaseSemaphore(wakeup, 1, 0);
    }

    void Loop()
    {
        void* argLatch = 0;
        while(active)
        {
            WaitForSingleObject(wakeup, INFINITE);

            EnterCriticalSection(&access);
            argLatch = arg;
            LeaveCriticalSection(&access);

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
