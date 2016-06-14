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



bool EncodeMap::isRangeConflict(duint offset, duint size, duint codesize)
{
    if(codesize > size)
        return true;
    size = std::min(size, codesize);
    if(size <= 0)
        return false;
    ENCODETYPE type = (ENCODETYPE)mBuffer[offset];
    for(int i = 1 + offset; i < size + offset; i++)
    {
        if((ENCODETYPE)mBuffer[i] != enc_unknown)
            return true;
    }

    return false;
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

duint EncodeMap::getEncodeTypeSize(ENCODETYPE type)
{
    switch(type)
    {
    case enc_byte:
        return 1;
    case enc_word:
        return 2;
    case enc_dword:
        return 4;
    case enc_fword:
        return 6;
    case enc_qword:
        return 8;
    case enc_oword:
        return 16;
    case enc_mmword:
        return 16;
    case enc_xmmword:
        return 32;
    case enc_ymmword:
        return 64;
    case enc_real4:
        return 4;
    case enc_real8:
        return 8;
    case enc_real10:
        return 10;
    case enc_ascii:
        return 1;
    case enc_unicode:
        return 2;
    default:
        return 1;
    }
}


ENCODETYPE EncodeMap::getDataType(duint addr, duint codesize)
{
    if(!mBuffer || addr - mBase >= mSize)
        return ENCODETYPE::enc_unknown;

    duint offset = addr - mBase;
    ENCODETYPE type = (ENCODETYPE)mBuffer[offset];
    if(type == enc_unknown && isRangeConflict(offset, mSize - offset, codesize))
        return enc_byte;
    else
        return type;

    return ENCODETYPE::enc_unknown;
}

duint EncodeMap::getDataSize(duint addr, duint codesize)
{

    if(!mBuffer || addr - mBase >= mSize)
        return codesize;

    duint offset = addr - mBase;

    ENCODETYPE type = (ENCODETYPE)mBuffer[offset];

    duint datasize = getEncodeTypeSize(type);
    if(type == enc_unknown || type == enc_code)
    {
        if(isRangeConflict(offset, mSize - offset, codesize) || codesize == 0)
            return datasize;
        else
            return codesize;
    }
    else if(type == enc_ascii || type == enc_unicode)
    {
        duint totalsize = 0;
        for(int i = offset; i < mSize; i += datasize)
        {
            if(mBuffer[i] == type)
                totalsize += datasize;
            else
                break;
        }
        return totalsize;
    }
    else
        return datasize;

    return codesize;
}
