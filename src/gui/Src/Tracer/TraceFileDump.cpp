#include <QMutexLocker>
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
    maxIndex = 0ull;
    dump.clear();
}

unsigned char TraceFileDump::getByte(Key location, bool & success) const
{
    auto it = dump.lower_bound(location);
    success = false;
    if(it != dump.end())
    {
        if(it->first.addr == location.addr)
        {
            success = true;
        }
        else if(it != dump.begin())
        {
            // try to get to next record which may hold the required data
            --it;
            if(it->first.addr == location.addr)
            {
                success = true;
            }
        }
    }
    if(!success)
    {
        return 0;
    }
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
    char failedTimes = 0;
    buffer.resize(size);
    for(duint i = 0; i < size; i++)
    {
        Key location = {addr + i, index};
        auto it = dump.lower_bound(location);
        bool success = false;
        if(it != dump.end())
        {
            if(it->first.addr == location.addr)
            {
                success = true;
            }
            else if(it != dump.begin())
            {
                // try to get to next record which may hold the required data
                --it;
                if(it->first.addr == location.addr)
                {
                    success = true;
                }
                else if(it->first.addr > location.addr) // this must be higher address than requested location, gap should be filled with zeroes
                {
                    duint gap_size = it->first.addr - location.addr;
                    gap_size = std::min(gap_size, size - i); // prevent overflow
                    memset(&buffer[i], 0, gap_size);
                    i += gap_size - 1;
                    continue;
                }
                //else
                //    GuiAddLogMessage("impossible!");
            }
        }
        if(!success)
        {
            buffer[i] = 0;
            continue;
        }
        if(location.index > it->first.index)
        {
            buffer[i] = it->second.newData;
        }
        else
        {
            buffer[i] = it->second.oldData; // Old data of new instruction is preferred
        }
        // Peek at next entries to see if we are lucky to have data for addr+i+1 easily, works for data only accessed once
        while(it != dump.begin() &&  i + 1 < size && failedTimes < 5)
        {
            --it;
            if(it->first.addr == addr + i + 1)
            {
                // At the right address, but still not sure whether the index is right
                if(it == dump.begin() || std::prev(it)->first.addr != addr + i + 1)
                {
                    // OK
                    ++i;
                    if(location.index > it->first.index)
                    {
                        buffer[i] = it->second.newData;
                    }
                    else
                    {
                        buffer[i] = it->second.oldData; // Old data of new instruction is preferred
                    }
                    failedTimes = 0;
                    continue;
                }
            }
            failedTimes++; // This trick doesn't work for frequently accessed memory areas, e.g call stack. Disable it quickly.
            break;
        }
        if(it == dump.begin() && i < size - 1)
        {
            // Nothing more, fill the rest with zeros and done
            memset(&buffer[i + 1], 0, size - i - 1);
            break;
        }
    }
    return buffer;
}

// find references to the memory address
std::vector<unsigned long long> TraceFileDump::getReferences(duint startAddr, duint endAddr) const
{
    std::vector<unsigned long long> index;
    if(endAddr < startAddr)
        std::swap(endAddr, startAddr);
    // find references to the memory address
    auto it = dump.lower_bound({endAddr, maxIndex + 1});
    while(it != dump.end() && it->first.addr >= startAddr && it->first.addr <= endAddr)
    {
        index.push_back(it->first.index);
        ++it;
    }
    if(index.empty())
        return index;
    // rearrange the array and remove duplicates
    std::sort(index.begin(), index.end());
    std::vector<unsigned long long> result;
    result.push_back(index[0]);
    for(size_t i = 1; i < index.size(); i++)
    {
        if(index[i] != result[result.size() - 1])
            result.push_back(index[i]);
    }
    return result;
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
        //records[i - 1].second.isWrite = 0; //TODO
        //records[i - 1].second.isExecute = 0;
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
        auto it = dump.lower_bound({addr - 1, maxIndex + 1});
        // try to find out if addr-1 is in the dump
        for(; it != dump.end(); it = dump.lower_bound({addr - 1, maxIndex + 1}))
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

// TraceFileDumpMemoryPage
TraceFileDumpMemoryPage::TraceFileDumpMemoryPage(QObject* parent) : MemoryPage(0x10000, 0x1000, parent)
{
    QMutexLocker locker(&lock);
    dump = nullptr;
}

void TraceFileDumpMemoryPage::setSelectedIndex(unsigned long long index)
{
    QMutexLocker locker(&lock);
    if(dump)
        selectedIndex = std::min(index, dump->getMaxIndex());
    else
        selectedIndex = 0ull;
}

void TraceFileDumpMemoryPage::setDumpObject(TraceFileDump* dump)
{
    QMutexLocker locker(&lock);
    this->dump = dump;
}

bool TraceFileDumpMemoryPage::isAvailable() const
{
    return !!this->dump;
}

unsigned long long TraceFileDumpMemoryPage::getSelectedIndex() const
{
    return selectedIndex;
}

bool TraceFileDumpMemoryPage::read(void* parDest, dsint parRVA, duint parSize) const
{
    QMutexLocker locker(&lock);
    if(!dump)
        return false;
    auto buffer = dump->getBytes(mBase + parRVA, parSize, selectedIndex);
    memcpy(parDest, buffer.data(), parSize);
    return true;
}

bool TraceFileDumpMemoryPage::write(const void* parDest, dsint parRVA, duint parSize)
{
    return false; // write is not supported
}
