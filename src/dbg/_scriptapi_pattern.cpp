#include "_scriptapi_pattern.h"
#include "patternfind.h"
#include "memory.h"

SCRIPT_EXPORT duint Script::Pattern::Find(unsigned char* data, duint datasize, const char* pattern)
{
    return patternfind(data, datasize, pattern);
}

SCRIPT_EXPORT duint Script::Pattern::FindMem(duint start, duint size, const char* pattern)
{
    Memory<unsigned char*> data(size, "Script::Pattern::FindMem::data");
    if(!MemRead(start, data(), size))
        return -1;
    auto found = Pattern::Find(data(), data.size(), pattern);
    return found == -1 ? 0 : found + start;
}

SCRIPT_EXPORT void Script::Pattern::Write(unsigned char* data, duint datasize, const char* pattern)
{
    patternwrite(data, datasize, pattern);
}

SCRIPT_EXPORT void Script::Pattern::WriteMem(duint start, duint size, const char* pattern)
{
    Memory<unsigned char*> data(size, "Script::Pattern::WriteMem::data");
    if(!MemRead(start, data(), data.size()))
        return;
    patternwrite(data(), data.size(), pattern);
    MemWrite(start, data(), data.size());
}

SCRIPT_EXPORT bool Script::Pattern::SearchAndReplace(unsigned char* data, duint datasize, const char* searchpattern, const char* replacepattern)
{
    return patternsnr(data, datasize, searchpattern, replacepattern);
}

SCRIPT_EXPORT bool Script::Pattern::SearchAndReplaceMem(duint start, duint size, const char* searchpattern, const char* replacepattern)
{
    Memory<unsigned char*> data(size, "Script::Pattern::SearchAndReplaceMem::data");
    if(!MemRead(start, data(), size))
        return false;
    auto found = patternfind(data(), data.size(), searchpattern);
    if(found == -1)
        return false;
    patternwrite(data() + found, data.size() - found, replacepattern);
    MemWrite((start + found), data() + found, data.size() - found);
    return true;
}