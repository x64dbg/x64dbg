#pragma once

#include "memory.h"

template<typename T, bool ReadOnly = false, bool BreakOnFail = false>
class RemotePtr;

template<typename T, typename U>
RemotePtr<U> RemoteMemberPtr(uint Address, U T::*Member)
{
    // Calculate the offset from the member to the class base
    uint offset = ((char*) & ((T*)nullptr->*Member) - (char*)nullptr);

    return RemotePtr<U>(Address + offset);
}

template<typename T, bool ReadOnly, bool BreakOnFail>
class RemotePtr
{
public:
    explicit RemotePtr(uint Address)
    {
        Init(Address);
    }

    explicit RemotePtr(PVOID Address)
    {
        Init((uint)Address);
    }

    ~RemotePtr()
    {
        if(!ReadOnly && m_Modified)
            Sync();
    }

    T get()
    {
        // Read the external program data; no user edits
        Update();
        return m_InternalData;
    }

    size_t size()
    {
        return sizeof(T);
    }

    template<typename A = std::remove_pointer<T>, typename B = std::remove_pointer<T>>
    RemotePtr<A, ReadOnly, BreakOnFail> ptr(A * B::*Member)
    {
        // Calculate the offset from the member to the class base
        uint offset = ((char*) & ((B*)nullptr->*Member) - (char*)nullptr);

        // First the pointer is read
        auto ptr = RemotePtr<PVOID, ReadOnly, BreakOnFail>(m_InternalAddr + offset);

        // Now return the real data structure
        return RemotePtr<A, ReadOnly, BreakOnFail>(ptr.get());
    }

    template<typename A, typename B = std::remove_reference<T>>
    RemotePtr<A, ReadOnly, BreakOnFail> next(A B::*Member)
    {
        // Calculate the offset from the member to the class base
        uint offset = ((char*) & ((B*)nullptr->*Member) - (char*)nullptr);

        return RemotePtr<A, ReadOnly, BreakOnFail>(m_InternalAddr + offset);
    }

    T* operator->()
    {
        // The user could modify our internal structure after
        // return
        m_Modified = true;

        // Read the external program data; return a pointer
        Update();
        return &m_InternalData;
    }

    T operator=(const T & rhs)
    {
        // This operation is only allowed with ReadOnly==false
        if(!ReadOnly)
        {
            // Otherwise sync it with the external program.
            // The external program can be messing with data at the same time.
            m_InternalData = rhs;
            Sync();

            // Re-read data and then send it to the user
            return get();
        }

        if(BreakOnFail)
            __debugbreak();

        return rhs;
    }

    T operator()()
    {
        return get();
    }

private:
    inline void Update()
    {
        MemRead(m_InternalAddr, &m_InternalData, sizeof(T));
    }

    inline void Sync()
    {
        if(BreakOnFail)
        {
            if(ReadOnly)
                __debugbreak();

            if(!MemWrite(m_InternalAddr, &m_InternalData, sizeof(T)))
                __debugbreak();
        }

        m_Modified = false;
    }

    inline void Init(uint Address)
    {
        m_Modified = false;
        m_InternalAddr = Address;
        memset(&m_InternalData, 0, sizeof(T));
    }

    bool m_Modified;
    uint m_InternalAddr;
    T m_InternalData;
};