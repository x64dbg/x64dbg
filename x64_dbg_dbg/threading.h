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

#define EXCLUSIVE_ACQUIRE(Index)    CriticalSectionLocker __ThreadLock(Index);
#define EXCLUSIVE_RELEASE()         __ThreadLock.Unlock();

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
    static void Initialize();
    static void Deinitialize();

    CriticalSectionLocker(CriticalSectionLock LockIndex);
    ~CriticalSectionLocker();

    void Unlock();
    void Lock();
    bool TryLock();

private:
    static bool m_Initialized;
    static CRITICAL_SECTION m_Locks[LockLast];

    CRITICAL_SECTION* m_Section;
    BYTE m_LockCount;
};

#endif // _THREADING_H
