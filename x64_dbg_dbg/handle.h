#ifndef _HANDLE_H
#define _HANDLE_H

#include <windows.h>

class Handle
{
public:
    Handle(HANDLE h = 0)
    {
        mHandle = h;
    }

    ~Handle()
    {
        DWORD dwFlags = 0;
        if(GetHandleInformation(mHandle, &dwFlags) && !(dwFlags & HANDLE_FLAG_PROTECT_FROM_CLOSE))
            CloseHandle(mHandle);
    }

    const HANDLE & operator=(const HANDLE & h)
    {
        return mHandle = h;
    }

    operator HANDLE & ()
    {
        return mHandle;
    }

    bool operator!() const
    {
        return (!mHandle || mHandle == INVALID_HANDLE_VALUE);
    }

    operator bool() const
    {
        return !this;
    }

private:
    HANDLE mHandle;
};

#endif //_HANDLE_H