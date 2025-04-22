#include "File.h"

File::File(duint virtualBase, const QString &fileName)
    : mVirtualBase(virtualBase)
{
    mFile = new QFile(fileName, this);
    if(!mFile->open(QIODevice::ReadOnly))
    {
        throw std::runtime_error("Failed to open file: " + fileName.toUtf8().toStdString());
    }
    mSize = mFile->size();
}

File::~File()
{
    if(mFile != nullptr)
    {
        mFile->close();
        delete mFile;
        mFile = nullptr;
    }
}

bool File::read(duint addr, void *dest, duint size)
{
    if(addr < mVirtualBase || addr + size > mVirtualBase + mSize) {
        return false;
    }

    auto offset = addr - mVirtualBase;
    mFile->seek(offset);
    return mFile->read((char*)dest, size) == size;
}

bool File::getRange(duint addr, duint &base, duint &size)
{
    if(!isValidPtr(addr))
    {
        return false;
    }

    base = mVirtualBase;
    size = mSize;
    return true;
}

bool File::isCodePtr(duint addr)
{
    return false;
}

bool File::isValidPtr(duint addr)
{
    return addr >= mVirtualBase && addr < mVirtualBase + mSize;
}
