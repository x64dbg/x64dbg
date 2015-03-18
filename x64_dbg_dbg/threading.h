#pragma once

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
#define EXCLUSIVE_ACQUIRE(Index)    ExclusiveSectionLocker __ThreadLock(SectionLock::##Index);
#define EXCLUSIVE_RELEASE()         __ThreadLock.Unlock();

#define SHARED_ACQUIRE(Index)       SharedSectionLocker __SThreadLock(SectionLock::##Index);
#define SHARED_RELEASE()            __SThreadLock.Unlock();

enum SectionLock
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

class ExclusiveSectionLocker
{
public:
    static void Initialize();
    static void Deinitialize();

    ExclusiveSectionLocker(SectionLock LockIndex);
    ~ExclusiveSectionLocker();

    void Lock();
    void Unlock();

private:
    static bool     m_Initialized;
    static SRWLOCK  m_Locks[SectionLock::LockLast];

protected:
    SRWLOCK*    m_Lock;
    BYTE        m_LockCount;
};

class SharedSectionLocker : public ExclusiveSectionLocker
{
public:
    SharedSectionLocker(SectionLock LockIndex);
    ~SharedSectionLocker();

    void Lock();
    void Unlock();
};