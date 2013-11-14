#include "MemoryPage.h"

MemoryPage::MemoryPage(uint_t parBase, uint_t parSize, QObject *parent) : QObject(parent)
{
    mBase = 0;
    mSize = 0;
}


void MemoryPage::readOriginalMemory(byte_t* parDest, uint_t parRVA, uint_t parSize)
{
    Bridge::getBridge()->readProcessMemory(parDest, mBase + parRVA, parSize);
}


uint_t MemoryPage::getSize()
{
    return mSize;
}


uint_t MemoryPage::getBase()
{
    return mBase;
}


void MemoryPage::setAttributes(uint_t base, uint_t size)
{
    mBase = base;
    mSize = size;
}
