#include "MemoryPage.h"
#include <Bridge.h>

MemoryPage::MemoryPage(duint parBase, duint parSize, QObject* parent) : QObject(parent), mBase(0), mSize(0)
{
    Q_UNUSED(parBase);
    Q_UNUSED(parSize);
}

bool MemoryPage::read(void* parDest, dsint parRVA, duint parSize) const
{
    return DbgMemRead(mBase + parRVA, reinterpret_cast<unsigned char*>(parDest), parSize);
}

bool MemoryPage::write(const void* parDest, dsint parRVA, duint parSize)
{
    bool ret = DbgFunctions()->MemPatch(mBase + parRVA, reinterpret_cast<const unsigned char*>(parDest), parSize);
    GuiUpdatePatches();
    return ret;
}

duint MemoryPage::getSize() const
{
    return mSize;
}

duint MemoryPage::getBase() const
{
    return mBase;
}

duint MemoryPage::va(dsint rva) const
{
    return mBase + rva;
}

void MemoryPage::setAttributes(duint base, duint size)
{
    mBase = base;
    mSize = size;
}

bool MemoryPage::inRange(duint va) const
{
    return va >= mBase && va < mBase + mSize;
}
