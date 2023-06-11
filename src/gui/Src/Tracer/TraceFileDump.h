#pragma once

#include "Imports.h"
#include <map>

class TraceFileDump
{
public:
    struct Key
    {
        duint addr;
        unsigned long long index;
        friend bool operator <(const Key & a, const Key & b)
        {
            // order is inverted, highest address is less! We want to use lower_bound() to find last memory access index.
            return a.addr > b.addr || a.addr == b.addr && a.index > b.index;
        }
    };
    struct DumpRecord
    {
        unsigned char oldData;
        unsigned char newData;
        unsigned char isWrite;
        unsigned char isExecute;
    };

    TraceFileDump();
    ~TraceFileDump();
    void clear();
    // Read a byte at "addr" at the moment given in "index"
    unsigned char getByte(Key location, bool & success) const;
    std::vector<unsigned char> getBytes(duint addr, duint size, unsigned long long index) const;
    std::vector<unsigned long long> getReferences(duint startAddr, duint endAddr) const;
    // Insert a memory access record
    //void addMemAccess(duint addr, DumpRecord record);
    void addMemAccess(duint addr, const void* oldData, const void* newData, size_t size);
    inline void increaseIndex()
    {
        maxIndex++;
    }
    inline unsigned long long getMaxIndex()
    {
        return maxIndex;
    }
    // Find continuous memory areas
    void findMemAreas();
    std::vector<std::pair<duint, duint>> memAreas;
private:
    std::map<Key, DumpRecord> dump;
    unsigned long long maxIndex;
};
