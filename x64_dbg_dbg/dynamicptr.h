#pragma once

#include "memory.h"

template<typename T, bool ReadOnly = false>
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
        if(m_Modified)
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

    T* operator->()
    {
        // The user could modify our internal structure after
        // return
        m_Modified = true;

        // Read the external program data
        Update();
        return &m_InternalData;
    }

    T operator=(const T & rhs)
    {
        // This operation is only allowed with ReadOnly==false
        if(!ReadOnly)
        {
            // Otherwise sync it with the external program and read it again.
            // The external program can be messing with data at the same time.
            m_InternalData = rhs;
            Sync();
            Update();
            return m_InternalData;
        }

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
        if(!ReadOnly)
            MemWrite(m_InternalAddr, &m_InternalData, sizeof(T));

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