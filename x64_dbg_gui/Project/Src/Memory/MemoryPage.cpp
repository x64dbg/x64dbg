#include "MemoryPage.h"

MemoryPage::MemoryPage(uint_t parBase, uint_t parSize, QObject *parent) : QObject(parent)
{
    Q_UNUSED(parBase);
    Q_UNUSED(parSize);
    mBase = 0;
    mSize = 0;
}


bool MemoryPage::read(byte_t* parDest, uint_t parRVA, uint_t parSize)
{
    return DbgMemRead(mBase + parRVA, parDest, parSize);
}


uint_t MemoryPage::getSize()
{
    return mSize;
}


uint_t MemoryPage::getBase()
{
    return mBase;
}

uint_t MemoryPage::va(int_t rva)
{
    return mBase + rva;
}

void MemoryPage::setAttributes(uint_t base, uint_t size)
{
    mBase = base;
    mSize = size;
}
