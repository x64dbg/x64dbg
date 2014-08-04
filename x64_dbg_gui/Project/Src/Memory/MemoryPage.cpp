#include "MemoryPage.h"

MemoryPage::MemoryPage(uint_t parBase, uint_t parSize, QObject* parent) : QObject(parent)
{
    Q_UNUSED(parBase);
    Q_UNUSED(parSize);
    mBase = 0;
    mSize = 0;
}

bool MemoryPage::read(void* parDest, uint_t parRVA, uint_t parSize)
{
    return DbgMemRead(mBase + parRVA, (unsigned char*)parDest, parSize);
}

bool MemoryPage::read(byte_t* parDest, uint_t parRVA, uint_t parSize)
{
    return read((void*)parDest, parRVA, parSize);
}

bool MemoryPage::write(const void* parDest, uint_t parRVA, uint_t parSize)
{
    bool ret = DbgFunctions()->MemPatch(mBase + parRVA, (unsigned char*)parDest, parSize);
    GuiUpdatePatches();
    return ret;
}

bool MemoryPage::write(const byte_t* parDest, uint_t parRVA, uint_t parSize)
{
    return write((const void*)parDest, parRVA, parSize);
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
