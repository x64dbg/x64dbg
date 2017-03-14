#include "analysis.h"
#include "memory.h"
#include "debugger.h"

//Workaround for modules that have holes between sections
static void DumbMemRead(duint BaseAddress, void* Buffer, duint Size)
{
    if(!MemIsCanonicalAddress(BaseAddress) || !Buffer || !Size)
        return;

    duint offset = 0;
    duint requestedSize = Size;
    duint sizeLeftInFirstPage = PAGE_SIZE - (BaseAddress & (PAGE_SIZE - 1));
    duint readSize = min(sizeLeftInFirstPage, requestedSize);

    while(readSize)
    {
        SIZE_T bytesRead = 0;
        MemoryReadSafe(fdProcessInfo->hProcess, (PVOID)(BaseAddress + offset), (PBYTE)Buffer + offset, readSize, &bytesRead);
        offset += readSize;
        requestedSize -= readSize;
        readSize = min(PAGE_SIZE, requestedSize);
    }
}

Analysis::Analysis(duint base, duint size)
{
    mBase = base;
    mSize = size;
    mData = new unsigned char[mSize + MAX_DISASM_BUFFER];
    memset(mData, 0xCC, mSize + MAX_DISASM_BUFFER);
    DumbMemRead(base, mData, size);
}

Analysis::~Analysis()
{
    delete[] mData;
}