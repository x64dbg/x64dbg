#include "TraceFileDump.h"
#include "StringUtil.h"

TraceFileDump::TraceFileDump()
{
    maxIndex = 0ull;
}

TraceFileDump::~TraceFileDump()
{

}

void TraceFileDump::clear()
{
    maxIndex = 0;
    dump.clear();
}

unsigned char TraceFileDump::getByte(Key location, bool & success) const
{
    auto it = dump.lower_bound(location);
    if(it != dump.end())
    {
        if(it->first.addr == location.addr)
        {
            goto found;
        }
        else if(it != dump.begin())
        {
            // try to get to next record which may hold the required data
            --it;
            if(it->first.addr == location.addr)
            {
                goto found;
            }
        }
    }
    success = false;
    return 0;
found:
    success = true;
    if(location.index > it->first.index)
    {
        return it->second.newData;
    }
    else
    {
        return it->second.oldData;
    }
}

std::vector<unsigned char> TraceFileDump::getBytes(duint addr, duint size, unsigned long long index) const
{
    std::vector<unsigned char> buffer;
    buffer.resize(size);
    for(duint i = 0; i < size; i++)
    {
        bool success;
        buffer[i] = getByte({addr + i, index}, success);
    }
    return buffer;
}

//void TraceFileDump::addMemAccess(duint addr, DumpRecord record)
//{
//    Key location = {addr, maxIndex};
//    dump.insert(std::make_pair(location, record));
//}

void TraceFileDump::addMemAccess(duint addr, const void* oldData, const void* newData, size_t size)
{
    std::vector<std::pair<Key, DumpRecord>> records;
    records.resize(size);
    // insert in the correct order
    for(size_t i = size; i > 0; i--)
    {
        auto b = size - i;
        records[i - 1].first.addr = addr + b;
        records[i - 1].first.index = maxIndex;
        records[i - 1].second.oldData = ((const unsigned char*)oldData)[b];
        records[i - 1].second.newData = ((const unsigned char*)newData)[b];
        records[i - 1].second.isWrite = 0; //TODO
        records[i - 1].second.isExecute = 0;
    }
    dump.insert(records.begin(), records.end());
}

// Find continuous memory areas. It is done separate from adding memory accesses because the number of addresses is less than that of memory accesses
// TODO: We also need another findMemAreas() which only updates incrementally, when the user steps while tracing
void TraceFileDump::findMemAreas()
{
    memAreas.clear();
    if(dump.empty())
        return;
    duint addr = dump.begin()->first.addr; //highest address
    duint end = addr;
    duint start;
    // find first access to addr
    do
    {
        auto it = dump.lower_bound({addr - 1, maxIndex});
        // try to find out if addr-1 is in the dump
        for(; it != dump.end(); it = dump.lower_bound({addr - 1, maxIndex}))
        {
            if(it->first.addr != addr - 1)
                break;
            addr--;
        }
        // addr-1 is not in the dump, insert the memory area
        start = addr;
        memAreas.push_back(std::make_pair(start, end));
        // get to next lowest address
        if(it != dump.end())
        {
            addr = it->first.addr;
            end = addr;
        }
        else
        {
            break;
        }
    }
    while(true);
}
