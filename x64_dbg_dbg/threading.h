#ifndef _THREADING_H
#define _THREADING_H

#include "_global.h"

//enums
enum WAIT_ID
{
    WAITID_RUN,
    WAITID_STOP,
    WAITID_USERDB
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
    LockLast
};

void CriticalSectionDeleteLocks();

class CriticalSectionLocker
{
public:
    CriticalSectionLocker(CriticalSectionLock lock);
    ~CriticalSectionLocker();
    void unlock();
    void relock();

private:
    CriticalSectionLock gLock;
};

#endif // _THREADING_H
