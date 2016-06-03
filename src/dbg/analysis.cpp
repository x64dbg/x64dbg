#include "analysis.h"
#include "memory.h"

Analysis::Analysis(duint base, duint size)
{
    mBase = base;
    mSize = size;
    mData = new unsigned char[mSize + MAX_DISASM_BUFFER];
    MemRead(mBase, mData, mSize);
}

Analysis::~Analysis()
{
    delete[] mData;
}