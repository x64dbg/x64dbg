#ifndef _DYNAMICMEM_H
#define _DYNAMICMEM_H

class Memory
{
public:
    Memory(size_t size, const char* reason = "Memory:???")
    {
        this->realloc(size, reason);
    }

    ~Memory()
    {
        efree(mPtr, mReason);
    }

    Memory realloc(size_t size, const char* reason = "Memory:???")
    {
        mSize = size;
        mPtr = erealloc(mPtr, size, reason);
        mReason = reason;
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
        return static_cast<T*>(mPtr);
    }

private:
    void* mPtr;
    size_t mSize;
    const char* mReason;
};

#endif //_DYNAMICMEM_H