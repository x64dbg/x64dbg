#include "MemoryPage.h"

MemoryPage::MemoryPage(uint_t parBase, uint_t parSize, QObject* parent) : QObject(parent), mBase(0), mSize(0)
{
    Q_UNUSED(parBase);
    Q_UNUSED(parSize);
}

bool MemoryPage::read(void* parDest, uint_t parRVA, uint_t parSize) const
{
    return DbgMemRead(mBase + parRVA, reinterpret_cast<unsigned char*>(parDest), parSize);
}

bool MemoryPage::read(byte_t* parDest, uint_t parRVA, uint_t parSize) const
{
    return read(reinterpret_cast<void*>(parDest), parRVA, parSize);
}

bool MemoryPage::write(const void* parDest, uint_t parRVA, uint_t parSize)
{
    bool ret = DbgFunctions()->MemPatch(mBase + parRVA, reinterpret_cast<const unsigned char*>(parDest), parSize);
    GuiUpdatePatches();
    return ret;
}

bool MemoryPage::write(const byte_t* parDest, uint_t parRVA, uint_t parSize)
{
    return write(reinterpret_cast<const void*>(parDest), parRVA, parSize);
}

uint_t MemoryPage::getSize() const
{
    return mSize;
}

uint_t MemoryPage::getBase() const
{
    return mBase;
}

uint_t MemoryPage::va(int_t rva) const
{
    return mBase + rva;
}

void MemoryPage::setAttributes(uint_t base, uint_t size)
{
    mBase = base;
    mSize = size;
}
