#ifndef _TASKTHREAD_H
#define _TASKTHREAD_H
#include "_global.h"
struct _SECURITY_ATTRIBUTES;

struct TaskThreadDetails;

class TaskThread
{
    TaskThreadDetails* details = 0;
public:
    typedef DWORD(WINAPI* TaskFunction_t)(void*);

    void WakeUp(void* arg = 0);
    template <typename T> void WakeUp(T arg = 0) { WakeUp((void*)arg); }

    TaskThread(TaskFunction_t,
               size_t minSleepTimeMs = 500,
               _SECURITY_ATTRIBUTES*  lpThreadAttributes = 0,
               SIZE_T                 dwStackSize = 0);
    ~TaskThread();
};
#endif // _TASKTHREAD_H
