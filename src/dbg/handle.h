#ifndef _HANDLE_H
#define _HANDLE_H

#include <windows.h>

class Handle
{
public:
    Handle(HANDLE h = nullptr) : mHandle(h) { }
    Handle(const Handle &) = delete;
    Handle(Handle &&) = delete;

    ~Handle()
    {
        Close();
    }

    void Close()
    {
        if(*this)
        {
            DWORD dwFlags = 0;
            if(GetHandleInformation(mHandle, &dwFlags) && !(dwFlags & HANDLE_FLAG_PROTECT_FROM_CLOSE))
                CloseHandle(mHandle);
            mHandle = INVALID_HANDLE_VALUE;
        }
    }

    operator HANDLE()
    {
        return mHandle;
    }

    explicit operator bool() const
    {
        return mHandle != nullptr && mHandle != INVALID_HANDLE_VALUE;
    }

private:
    HANDLE mHandle;
};

#endif //_HANDLE_H