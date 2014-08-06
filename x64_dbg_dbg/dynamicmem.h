#ifndef _DYNAMICMEM_H
#define _DYNAMICMEM_H

class Memory
{
    typedef std::vector<char> MemoryVector;

public:
    Memory(size_t size, const char* reason = "Memory:???")
    {
        this->realloc(size, reason);
    }

    ~Memory()
    {
        efree(mPtr, "Memory:free");
    }

    Memory realloc(size_t size, const char* reason = "Memory:???")
    {
        mSize = size;
        mPtr = erealloc(mPtr, size, reason);
        return *this;
    }

    size_t size()
    {
        return mSize;
    }

    //return a typeless pointer
    template<class T>
    operator T* ()
    {
        return static_cast<T*>(mem);
    }

private:
    void* mPtr;
    size_t mSize;
};

#endif //_DYNAMICMEM_H