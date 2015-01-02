#ifndef _THREADING_H
#define _THREADING_H

#include "_global.h"

//enums
enum WAIT_ID
{
    WAITID_RUN,
    WAITID_STOP,
    WAITID_LAST
};

//functions
void waitclear();
void wait(WAIT_ID id);
void lock(WAIT_ID id);
void unlock(WAIT_ID id);
bool waitislocked(WAIT_ID id);

enum CriticalSectionLock
{
    LockMemoryPages,
    LockVariables,
    LockModules,
    LockComments,
    LockLabels,
    LockBookmarks,
    LockFunctions,
    LockLoops,
    LockBreakpoints,
    LockPatches,
    LockThreads,
    LockDprintf,
    LockLast
};

class CriticalSectionLocker
{
public:
    static void Deinitialize();
    CriticalSectionLocker(CriticalSectionLock lock);
    ~CriticalSectionLocker();
    void unlock();
    void relock();

private:
    static void Initialize();
    static bool bInitDone;
    static CRITICAL_SECTION locks[LockLast];

    CriticalSectionLock gLock;
    bool Locked;
};

#endif // _THREADING_H
