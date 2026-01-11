#pragma once

#include <type_traits>

template<typename T>
class Memory
{
    static_assert(std::is_pointer<T>::value, "Memory template parameter must be a pointer type");
public:
    //
    // This class guarantees that the returned allocated memory
    // will always be zeroed
    //
    explicit Memory(const char* Reason = "Memory:???")
    {
        m_Ptr = nullptr;
        m_Size = 0;
#ifdef ENABLE_MEM_TRACE
        m_Reason = Reason;
#endif //ENABLE_MEM_TRACE
    }

    explicit Memory(size_t Size, const char* Reason = "Memory:???")
    {
        m_Ptr = reinterpret_cast<T>(emalloc(Size, Reason));
        m_Size = Size;
#ifdef ENABLE_MEM_TRACE
        m_Reason = Reason;
#endif //ENABLE_MEM_TRACE

        memset(m_Ptr, 0, Size);
    }

    ~Memory()
    {
        if(m_Ptr)
#ifdef ENABLE_MEM_TRACE
            efree(m_Ptr, m_Reason);
#else
            efree(m_Ptr);
#endif //ENABLE_MEM_TRACE
    }

    T realloc(size_t Size, const char* Reason = "Memory:???")
    {
        m_Ptr = reinterpret_cast<T>(erealloc(m_Ptr, Size, Reason));
        m_Size = Size;
#ifdef ENABLE_MEM_TRACE
        m_Reason = Reason;
#endif //ENABLE_MEM_TRACE

        memset(m_Ptr, 0, m_Size);
        return m_Ptr;
    }

    size_t size() const
    {
        return m_Size;
    }

    T operator()()
    {
        return m_Ptr;
    }

private:
    T           m_Ptr;
    size_t      m_Size;
#ifdef ENABLE_MEM_TRACE
    const char* m_Reason;
#endif //ENABLE_MEM_TRACE
};