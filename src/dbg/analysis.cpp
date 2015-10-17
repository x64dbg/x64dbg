#include "analysis.h"
#include "memory.h"

Analysis::Analysis(uint base, uint size)
{
    _base = base;
    _size = size;
    _data = new unsigned char[_size + MAX_DISASM_BUFFER];
    MemRead(_base, _data, _size);
}

Analysis::~Analysis()
{
    delete[] _data;
}

bool Analysis::IsValidAddress(uint addr)
{
    return addr >= _base && addr < _base + _size;
}

const unsigned char* Analysis::TranslateAddress(uint addr)
{
    return IsValidAddress(addr) ? _data + (addr - _base) : nullptr;
}