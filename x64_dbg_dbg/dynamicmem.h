#pragma once

template<typename T>
class Memory
{
public:
    Memory(const char* Reason = "Memory:???")
    {
        m_Ptr		= nullptr;
        m_Size		= 0;
        m_Reason	= Reason;
    }

    Memory(size_t Size, const char* Reason = "Memory:???")
    {
        m_Ptr		= reinterpret_cast<T>(emalloc(Size));
        m_Size		= Size;
        m_Reason	= Reason;

        memset(m_Ptr, 0, Size);
    }

    ~Memory()
    {
		if (m_Ptr)
			efree(m_Ptr);
    }

    T realloc(size_t Size, const char* Reason = "Memory:???")
    {
        m_Ptr		= reinterpret_cast<T>(erealloc(m_Ptr, Size));
        m_Size		= Size;
        m_Reason	= Reason;

        return (T)memset(m_Ptr, 0, m_Size);
    }

	size_t size()
	{
		return m_Size;
	}

    template<typename U>
    operator U()
    {
        return (U)m_Ptr;
    }

    operator T()
    {
        return m_Ptr;
    }

    T operator()()
    {
        return m_Ptr;
    }

	template<typename U>
	T operator+(const U& Other)
	{
		return m_Ptr + Other;
	}

private:
    T			m_Ptr;
    size_t		m_Size;
    const char* m_Reason;
};