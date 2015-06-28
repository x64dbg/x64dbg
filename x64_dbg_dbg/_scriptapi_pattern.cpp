#include "_scriptapi_pattern.h"
#include "patternfind.h"
#include "memory.h"

duint Script::Pattern::Find(unsigned char* data, duint datasize, const char* pattern)
{
    return patternfind(data, datasize, pattern);
}

duint Script::Pattern::FindMem(duint start, duint size, const char* pattern)
{
    Memory<unsigned char*> data(size, "Script::Pattern::FindMem::data");
    if(!MemRead((void*)start, data(), size, nullptr))
        return -1;
    return Pattern::Find(data(), data.size(), pattern) + start;
}

void Script::Pattern::Write(unsigned char* data, duint datasize, const char* pattern)
{
    patternwrite(data, datasize, pattern);
}

void Script::Pattern::WriteMem(duint start, duint size, const char* pattern)
{
    Memory<unsigned char*> data(size, "Script::Pattern::WriteMem::data");
    if(!MemRead((void*)start, data(), data.size(), nullptr))
        return;
    patternwrite(data(), data.size(), pattern);
    MemWrite((void*)start, data(), data.size(), nullptr);
}

bool Script::Pattern::SearchAndReplace(unsigned char* data, duint datasize, const char* searchpattern, const char* replacepattern)
{
    return patternsnr(data, datasize, searchpattern, replacepattern);
}

bool Script::Pattern::SearchAndReplaceMem(duint start, duint size, const char* searchpattern, const char* replacepattern)
{
    Memory<unsigned char*> data(size, "Script::Pattern::SearchAndReplaceMem::data");
    if(!MemRead((void*)start, data(), size, nullptr))
        return false;
    duint found = patternfind(data(), data.size(), searchpattern);
    if(found == -1)
        return false;
    patternwrite(data() + found, data.size() - found, replacepattern);
    MemWrite((void*)(start + found), data() + found, data.size() - found, nullptr);
    return true;
}