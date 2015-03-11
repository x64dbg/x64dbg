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

//
// THREAD SYNCHRONIZATION
//
// Better, but requires VISTA+
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa904937%28v=vs.85%29.aspx
//
#define EXCLUSIVE_ACQUIRE(Index)    CriticalSectionLocker __ThreadLock(Index, false);
#define EXCLUSIVE_RELEASE()         __ThreadLock.Unlock();

#define SHARED_ACQUIRE(Index)       CriticalSectionLocker __SThreadLock(Index, true);
#define SHARED_RELEASE()            __SThreadLock.Unlock();

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

    // This is defined because of a bug in the Windows 8.1 kernel;
    // Calling VirtualQuery/VirtualProtect/ReadProcessMemory can and will cause
    // a deadlock.
    // https://bitbucket.org/mrexodia/x64_dbg/issue/247/x64-dbg-bug-string-references-function
    LockWin8Workaround,

    LockLast
};

class CriticalSectionLocker
{
public:
    static void Initialize();
    static void Deinitialize();

    CriticalSectionLocker(CriticalSectionLock LockIndex, bool Shared);
    ~CriticalSectionLocker();

    void Unlock();
    void Lock(bool Shared);

private:
    static bool m_Initialized;
    static SRWLOCK m_Locks[LockLast];

    SRWLOCK* m_Lock;
    bool m_Shared;
    BYTE m_LockCount;
};

#endif // _THREADING_H
