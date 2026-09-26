#ifndef _HANDLE_H
#define _HANDLE_H

#include <windows.h>
#include "TitanEngine/TitanEngine.h"

class Handle
{
public:
    Handle(HANDLE h = nullptr) : mHandle(h) { }
    Handle(const Handle &) = delete;
    Handle(Handle && o)
    {
        mHandle = o.mHandle;
        o.mHandle = nullptr;
    }

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

class TitanHandle
{
public:
    TitanHandle(HANDLE handle = nullptr) : mHandle(handle) { }
    TitanHandle(const TitanHandle&) = delete;
    TitanHandle(TitanHandle&& other) noexcept : mHandle(other.mHandle)
    {
        other.mHandle = nullptr;
    }

    ~TitanHandle()
    {
        if(*this)
            TitanCloseHandle(mHandle);
    }

    operator HANDLE() const
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