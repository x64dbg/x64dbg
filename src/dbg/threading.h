#ifndef _THREADING_H
#define _THREADING_H

#include "_global.h"

enum WAIT_ID
{
    WAITID_RUN,
    WAITID_LAST
};

//functions
void waitclear();
void wait(WAIT_ID id);
bool waitfor(WAIT_ID id, unsigned int Milliseconds);
void lock(WAIT_ID id);
void unlock(WAIT_ID id);
bool waitislocked(WAIT_ID id);
void waitinitialize();
void waitdeinitialize();

//
// THREAD SYNCHRONIZATION
//
// Win Vista and newer: (Faster) SRW locks used
// Win 2003 and older:  (Slower) Critical sections used
//
#define EXCLUSIVE_ACQUIRE(Index)     SectionLocker<Index, false> __ThreadLock
#define EXCLUSIVE_ACQUIRE_GUI(Index) SectionLocker<Index, false, true> __ThreadLock
#define EXCLUSIVE_REACQUIRE()        __ThreadLock.Lock()
#define EXCLUSIVE_RELEASE()          __ThreadLock.Unlock()

#define SHARED_ACQUIRE(Index)        SectionLocker<Index, true> __SThreadLock
#define SHARED_ACQUIRE_GUI(Index)    SectionLocker<Index, true, true> __SThreadLock
#define SHARED_REACQUIRE()           __SThreadLock.Lock()
#define SHARED_RELEASE()             __SThreadLock.Unlock()

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
    LockSym,
    LockCmdLine,
    LockDatabase,
    LockPluginList,
    LockPluginCallbackList,
    LockPluginCommandList,
    LockPluginMenuList,
    LockPluginExprfunctionList,
    LockPluginFormatfunctionList,
    LockSehCache,
    LockMnemonicHelp,
    LockTraceRecord,
    LockCrossReferences,
    LockDebugStartStop,
    LockArguments,
    LockEncodeMaps,
    LockCallstackCache,
    LockRunToUserCode,
    LockWatch,
    LockExpressionFunctions,
    LockHistory,
    LockSymbolCache,
    LockLineCache,
    LockTypeManager,
    LockModuleHashes,
    LockFormatFunctions,
    LockDllBreakpoints,
    LockHandleCache,

    // Number of elements in this enumeration. Must always be the last index.
    LockLast
};

template<typename T>
struct __declspec(align(64)) CacheAligned
{
    T value;

    T* operator&() { return &value; }
};

class SectionLockerGlobal
{
    template<SectionLock LockIndex, bool Shared, bool ProcessGuiEvents>
    friend class SectionLocker;

public:
    static void Initialize();
    static void Deinitialize();

private:
    template<SectionLock LockIndex, bool Shared, bool ProcessGuiEvents>
    static void AcquireLock()
    {
        auto threadId = GetCurrentThreadId();
        if(m_SRWLocks)
        {
            auto srwLock = &m_srwLocks[LockIndex];

            if(Shared)
            {
                if(m_exclusiveOwner[LockIndex].threadId == threadId)
                    return;

                if(ProcessGuiEvents && threadId == m_guiMainThreadId)
                {
                    while(!m_TryAcquireSRWLockShared(srwLock))
                        GuiProcessEvents();
                }
                else
                {
                    m_AcquireSRWLockShared(srwLock);
                }
                return;
            }

            if(m_exclusiveOwner[LockIndex].threadId == threadId)
            {
                assert(m_exclusiveOwner[LockIndex].count > 0);
                m_exclusiveOwner[LockIndex].count++;
                return;
            }

            if(ProcessGuiEvents && threadId == m_guiMainThreadId)
            {
                while(!m_TryAcquireSRWLockExclusive(srwLock))
                    GuiProcessEvents();
            }
            else
            {
                m_AcquireSRWLockExclusive(srwLock);
            }

            assert(m_exclusiveOwner[LockIndex].threadId == 0);
            assert(m_exclusiveOwner[LockIndex].count == 0);
            m_exclusiveOwner[LockIndex].threadId = threadId;
            m_exclusiveOwner[LockIndex].count = 1;
        }
        else
        {
            auto cr = &m_crLocks[LockIndex];
            if(ProcessGuiEvents && threadId == m_guiMainThreadId)
            {
                while(!TryEnterCriticalSection(cr))
                    GuiProcessEvents();
            }
            else
            {
                EnterCriticalSection(cr);
            }
        }
    }

    template<SectionLock LockIndex, bool Shared>
    static void ReleaseLock()
    {
        if(m_SRWLocks)
        {
            if(Shared)
            {
                if(m_exclusiveOwner[LockIndex].threadId == GetCurrentThreadId())
                    return;

                m_ReleaseSRWLockShared(&m_srwLocks[LockIndex]);
                return;
            }

            assert(m_exclusiveOwner[LockIndex].count && m_exclusiveOwner[LockIndex].threadId);
            m_exclusiveOwner[LockIndex].count--;

            if(m_exclusiveOwner[LockIndex].count == 0)
            {
                m_exclusiveOwner[LockIndex].threadId = 0;
                m_ReleaseSRWLockExclusive(&m_srwLocks[LockIndex]);
            }
        }
        else
        {
            LeaveCriticalSection(&m_crLocks[LockIndex]);
        }
    }

    typedef void (WINAPI* SRWLOCKFUNCTION)(PSRWLOCK SWRLock);
    typedef BOOLEAN(WINAPI* TRYSRWLOCKFUNCTION)(PSRWLOCK SWRLock);

    static bool m_Initialized;
    static bool m_SRWLocks;

    struct __declspec(align(64)) owner_info { DWORD threadId; size_t count; };
    static owner_info m_exclusiveOwner[SectionLock::LockLast];
    static CacheAligned<SRWLOCK> m_srwLocks[SectionLock::LockLast];
    static CacheAligned<CRITICAL_SECTION> m_crLocks[SectionLock::LockLast];
    static SRWLOCKFUNCTION m_InitializeSRWLock;
    static SRWLOCKFUNCTION m_AcquireSRWLockShared;
    static TRYSRWLOCKFUNCTION m_TryAcquireSRWLockShared;
    static SRWLOCKFUNCTION m_AcquireSRWLockExclusive;
    static TRYSRWLOCKFUNCTION m_TryAcquireSRWLockExclusive;
    static SRWLOCKFUNCTION m_ReleaseSRWLockShared;
    static SRWLOCKFUNCTION m_ReleaseSRWLockExclusive;
    static DWORD m_guiMainThreadId;
};

template<SectionLock LockIndex, bool Shared, bool ProcessGuiEvents = false>
class SectionLocker
{
public:
    SectionLocker()
    {
        m_LockCount = 0;
        Lock();
    }

    ~SectionLocker()
    {
        if(m_LockCount > 0)
            Unlock();

        // The lock count should be zero after destruction.
        assert(m_LockCount == 0);
    }

    inline void Lock()
    {
        Internal::AcquireLock<LockIndex, Shared, ProcessGuiEvents>();

        // We cannot recursively lock more than 255 times.
        assert(m_LockCount < 255);

        m_LockCount++;
    }

    inline void Unlock()
    {
        // Unlocking twice will cause undefined behaviour.
        assert(m_LockCount != 0);

        m_LockCount--;

        Internal::ReleaseLock<LockIndex, Shared>();
    }

protected:
    BYTE m_LockCount;

private:
    using Internal = SectionLockerGlobal;
};

#endif // _THREADING_H
