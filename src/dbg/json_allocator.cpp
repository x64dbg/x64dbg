#include "json_allocator.h"
#include "_global.h"

class Heap
{
    char* mBase = nullptr;
    size_t mSize = 0;
    size_t mIndex = 0;
    int mCount = 0;

public:
    void* Alloc(size_t size)
    {
        if(mIndex + size > mSize)
            return emalloc(size, "Heap:ptr");
        auto ptr = mBase + mIndex;
        mIndex += size;
        mCount++;
        return ptr;
    }

    void Free(void* ptr)
    {
        if(size_t(ptr) >= size_t(mBase) && size_t(ptr) - size_t(mBase) < mSize)
            mCount--;
        else
            efree(ptr, "Heap:ptr");
    }

    void CreateHeap(size_t size)
    {
        mBase = (char*)VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        mSize = size;
        mIndex = 0;
        mCount = 0;
    }

    void DestroyHeap()
    {
        if(mCount)
            __debugbreak();
        mIndex = 0;
        //VirtualFree(mBase, 0, MEM_RELEASE);
    }
};

static volatile DWORD cheapThread = 0;
static Heap cheapHeap;

void* json_malloc(size_t size)
{
    if(cheapThread == GetCurrentThreadId())
        return cheapHeap.Alloc(size);
#ifdef ENABLE_MEM_TRACE
    return emalloc(size, "json:ptr");
#else
    return emalloc(size);
#endif
}

void json_free(void* ptr)
{
    if(cheapThread == GetCurrentThreadId())
    {
        cheapHeap.Free(ptr);
        return;
    }
#ifdef ENABLE_MEM_TRACE
    efree(ptr, "json:ptr");
#else
    efree(ptr);
#endif
}

void json_reserve_cheap(size_t size)
{
    if(cheapThread)
        return;
    //if(InterlockedCompareExchange(&cheapThread, GetCurrentThreadId(), 0) == 0)
    cheapHeap.CreateHeap(size);
    cheapThread = GetCurrentThreadId();
}

void json_free_cheap()
{
    if(cheapThread != GetCurrentThreadId())
        return;
    //if(InterlockedCompareExchange(&cheapThread, 0, GetCurrentThreadId()) == GetCurrentThreadId())
    cheapHeap.DestroyHeap();
    //cheapThread = 0;
}
