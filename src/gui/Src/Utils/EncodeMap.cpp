#include "EncodeMap.h"

EncodeMap::EncodeMap(QObject* parent) : QObject(parent), mBase(0), mSize(0), mBuffer(nullptr)
{
}

EncodeMap::~EncodeMap()
{
    if(mBuffer)
        DbgReleaseEncodeTypeBuffer(mBuffer);
}

void EncodeMap::setMemoryRegion(duint addr)
{
    mBase = DbgMemFindBaseAddr(addr, &mSize);
    if(!mBase)
        return;

    if(mBuffer)
        DbgReleaseEncodeTypeBuffer(mBuffer);
    mBuffer = (byte*)DbgGetEncodeTypeBuffer(addr);
}

void EncodeMap::setDataType(duint va, ENCODETYPE type)
{
    setDataType(va, getEncodeTypeSize(type), type);
}

void EncodeMap::setDataType(duint va, duint size, ENCODETYPE type)
{
    DbgSetEncodeType(va, size, type);
    if(!mBuffer && va >= mBase && va < mBase + mSize)
        setMemoryRegion(va);
}

void EncodeMap::delRange(duint start, duint size)
{
    DbgDelEncodeTypeRange(start, size);
}

void EncodeMap::delSegment(duint va)
{
    DbgDelEncodeTypeSegment(va);
    if(mBuffer && va >= mBase && va < mBase + mSize)
    {
        mBuffer = nullptr;
        DbgReleaseEncodeTypeBuffer(mBuffer);
    }
}

ENCODETYPE EncodeMap::getDataType(duint addr)
{
    if(!mBuffer || !inRange(addr))
        return enc_unknown;

    return ENCODETYPE(mBuffer[addr - mBase]);
}

duint EncodeMap::getDataSize(duint addr, duint codesize)
{
    if(!mBuffer || !inRange(addr))
        return codesize;

    duint offset = addr - mBase;

    auto type = ENCODETYPE(mBuffer[offset]);

    auto datasize = getEncodeTypeSize(type);
    if(isCode(type))
        return codesize;
    else
        return datasize;
}
