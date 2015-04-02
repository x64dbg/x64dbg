#ifndef _THREADING_H
#define _THREADING_H

#include "_global.h"

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

/**
\brief Locks that can be used in the CriticalSectionLocker class.
*/
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
    LockSym,
    LockLast
};

/**
\brief A critical section locker.
*/
class CriticalSectionLocker
{
public:
    /**
    \brief Deinitialises the critical sections.
    */
    static void Deinitialize();

    /**
    \brief This initialized a new instance of CriticalSectionLocker and it enters the critical section.
    \param lock The critical section to enter.
    */
    CriticalSectionLocker(CriticalSectionLock lock);

    ~CriticalSectionLocker();

    /**
    \brief Unlocks the critical section lock.
    */
    void unlock();

    /**
    \brief Relocks the critical section lock.
    */
    void relock();

private:
    /**
    \brief Initializes the critical section objects.
    */
    static void Initialize();

    /**
    \brief Boolean indicating if the initialization was done.
    */
    static bool bInitDone;

    /**
    \brief The critical section objects used for locking.
    */
    static CRITICAL_SECTION locks[LockLast];

    /**
    \brief The lock.
    */
    CriticalSectionLock gLock;

    /**
    \brief true if locked, false otherwise.
    */
    bool Locked;
};

#endif // _THREADING_H
