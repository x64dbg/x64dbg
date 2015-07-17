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
    // Special template part of this class, everything here
    // should not be touched
    template<bool Condition, typename U>
    using enableIf = typename std::enable_if<Condition, U>::type;

    template<typename U>
    using makePtrRemote = RemotePtr<U, ReadOnly, BreakOnFail>;

    template<typename U>
    struct isClass : std::integral_constant < bool, std::is_class<std::remove_pointer<U>>::value ||
                    std::is_class<std::remove_reference<U>>::value > {};

    template<typename U>
    struct isPtrClass : std::integral_constant < bool, !std::is_arithmetic<U>::value &&
            isClass<U>::value > {};

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

    template<typename A, typename B, typename C = std::remove_pointer<A>::type>
    enableIf<isPtrClass<A>::value, makePtrRemote<C>> next(A B::*Member)
    {
        // First the pointer is read
        auto ptr = RemotePtr<PVOID, ReadOnly, BreakOnFail>(m_InternalAddr + memberOffset(Member));

        // Now return the real data structure
        return makePtrRemote<C>(ptr.get());
    }

    template<typename A, typename B>
    enableIf<std::is_arithmetic<A>::value, makePtrRemote<A>> next(A B::*Member)
    {
        // Return direct value with adjusted offset
        return makePtrRemote<A>(m_InternalAddr + memberOffset(Member));
    }

    template<typename A, typename B = T>
    enableIf<isClass<T>::value, uint> memberOffset(A B::*Member)
    {
        return (char*) & ((std::remove_pointer<T>::type*)nullptr->*Member) - (char*)nullptr;
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