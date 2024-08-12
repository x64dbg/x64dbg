#include <assert.h>
#include <QMutexLocker>
#include <thread>
#include <deque>
#include <array>
#include "Configuration.h"
#include "zydis_wrapper.h"
#include "TraceFileReader.h"
#include "TraceFileDump.h"
#include "StringUtil.h"

TraceFileDump::TraceFileDump()
{
    maxIndex = static_cast<TRACEINDEX>(0);
    enabled = false;
}

TraceFileDump::~TraceFileDump()
{
    typedef decltype(dump) T;
    if(dump.size() > 65536)
    {
        // Move this huge object to another thread so the dump can be closed quickly.
        T* alt_dump = new T(std::move(dump));
        std::thread cleaner([](T * alt_dump)
        {
            delete alt_dump; // This can be freeing several GB of memory.
        }, alt_dump);
        cleaner.detach(); // Continue to free memory
    }
}

void TraceFileDump::clear()
{
    maxIndex = static_cast<TRACEINDEX>(0);
    dump.clear();
}

bool TraceFileDump::isValidReadPtr(duint address) const
{
    if(!isEnabled())
        return false;

    Key location = {address, maxIndex + 1};
    auto it = dump.lower_bound(location);
    if(it != dump.end())
    {
        if(it->first.addr == address)
        {
            return true;
        }
    }
    return false;
}

void TraceFileDump::getBytes(duint addr, duint size, TRACEINDEX index, void* buffer) const
{
    if(!isEnabled())
        return;

    unsigned char* ptr = (unsigned char*)buffer;
    char failedTimes = 0;
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
                    memset(&ptr[i], 0, gap_size);
                    i += gap_size - 1;
                    continue;
                }
                //else
                //    GuiAddLogMessage("impossible!");
            }
        }
        if(!success)
        {
            ptr[i] = 0;
            continue;
        }
        if(location.index > it->first.index)
        {
            ptr[i] = it->second.newData;
        }
        else
        {
            ptr[i] = it->second.oldData; // Old data of new instruction is preferred
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
                        ptr[i] = it->second.newData;
                    }
                    else
                    {
                        ptr[i] = it->second.oldData; // Old data of new instruction is preferred
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
            memset(&ptr[i + 1], 0, size - i - 1);
            break;
        }
    }
}

// find references to the memory address
std::vector<TRACEINDEX> TraceFileDump::getReferences(duint startAddr, duint endAddr) const
{
    if(!isEnabled())
        return {};

    std::vector<TRACEINDEX> index;
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
    std::vector<TRACEINDEX> result;
    result.push_back(index[0]);
    for(size_t i = 1; i < index.size(); i++)
    {
        if(index[i] != result[result.size() - 1])
            result.push_back(index[i]);
    }
    return result;
}

// Insert memory access records
void TraceFileDump::addMemAccess(duint cip, unsigned char* opcode, int opcodeSize, duint* memAddr, const duint* oldMemory, const duint* newMemory, size_t count)
{
    std::array < std::pair<Key, DumpRecord>, MAX_DISASM_BUFFER + MAX_MEMORY_OPERANDS* sizeof(duint) > records;
    // This function used stack memory allocation instead of heap allocation for performance, can't handle more than MAX_MEMORY_OPERANDS elements.
    assert(count <= MAX_MEMORY_OPERANDS);
    int base;
    for(size_t j = 0; j < count; j++)
    {
        base = j * sizeof(duint);
        // insert in the correct order
        for(size_t i = sizeof(duint); i > 0; i--)
        {
            auto b = base + sizeof(duint) - i;
            records[base + i - 1].first.addr = memAddr[j] + b;
            records[base + i - 1].first.index = maxIndex;
            records[base + i - 1].second.oldData = ((const unsigned char*)oldMemory)[b];
            records[base + i - 1].second.newData = ((const unsigned char*)newMemory)[b];
            //records[i - 1].second.isWrite = 0; //TODO
            //records[i - 1].second.isExecute = 0;
        }
    }
    // Always add opcode into dump
    base = count * sizeof(duint);
    for(int i = opcodeSize; i > 0; i--)
    {
        auto b = opcodeSize - i;
        records[base + i - 1].first.addr = cip + b;
        records[base + i - 1].first.index = maxIndex;
        records[base + i - 1].second.oldData = opcode[b];
        records[base + i - 1].second.newData = opcode[b];
        //records[i - 1].second.isWrite = 0; //TODO
        //records[i - 1].second.isExecute = 1;
    }
    // Insert all the records at once
    dump.insert(records.begin(), std::next(records.begin(), count * sizeof(duint) + opcodeSize));
}

// Find continuous memory areas. It is done separate from adding memory accesses because the number of addresses is less than that of memory accesses
// TODO: We also need another findMemAreas() which only updates incrementally, when the user steps while tracing
// TODO: User interface (UI)
//void TraceFileDump::findMemAreas()
//{
//memAreas.clear();
//if(dump.empty())
//return;
//duint addr = dump.begin()->first.addr; //highest address
//duint end = addr;
//duint start;
//// find first access to addr
//do
//{
//auto it = dump.lower_bound({addr - 1, maxIndex + 1});
//// try to find out if addr-1 is in the dump
//for(; it != dump.end(); it = dump.lower_bound({addr - 1, maxIndex + 1}))
//{
//if(it->first.addr != addr - 1)
//break;
//addr--;
//}
//// addr-1 is not in the dump, insert the memory area
//start = addr;
//memAreas.push_back(std::make_pair(start, end));
//// get to next lowest address
//if(it != dump.end())
//{
//addr = it->first.addr;
//end = addr;
//}
//else
//{
//break;
//}
//}
//while(true);
//}

void TraceFileDump::findAllMem(const unsigned char* data, const unsigned char* mask, size_t size, std::function<bool(duint, TRACEINDEX, TRACEINDEX)> matchFunction) const
{
    typedef struct PARTIALMATCH
    {
        duint address;
        TRACEINDEX startIndex;
        TRACEINDEX endIndex;
        PARTIALMATCH(duint address = 0, TRACEINDEX startIndex = 0, TRACEINDEX endIndex = 0)
        {
            this->address = address;
            this->startIndex = startIndex;
            this->endIndex = endIndex;
        }
    } PARTIALMATCH;
    bool abortSearching = false;
    // Trim the trailing wildcards
    size_t trimmed = 0;
    while(mask[trimmed] == 0 && trimmed < size)
    {
        trimmed++;
    }
    if(trimmed >= size)
    {
        // Searching for just wildcards?
        return;
    }
    size -= trimmed;
    const unsigned char* firstChar = data + trimmed;
    const unsigned char* firstMask = mask + trimmed;
    std::deque<PARTIALMATCH> partialMatches; // Queue for partial matches
    std::vector<std::pair<unsigned char, TRACEINDEX>> values; // First is value, second is end index
    auto it = dump.rbegin();
    duint prevAddress = it->first.addr;
    // Due to the inverted order, the reverse iterator is used
    do
    {
        // Get a set of values at current address
        const duint address = it->first.addr;
        if(address > prevAddress + 1)
        {
            // A data gap
            size_t matchCount = partialMatches.size();
            for(size_t i = 0; i < matchCount; i++)
            {
                auto m = partialMatches.front();
                partialMatches.pop_front();
                bool noRequeue = false;
                for(duint gapAddress = prevAddress + 1; gapAddress < address; gapAddress++)
                {
                    const unsigned char currentMask = firstMask[gapAddress - m.address];
                    const unsigned char currentChar = firstChar[gapAddress - m.address] & currentMask;
                    if(currentChar == 0)
                    {
                        if(gapAddress - m.address + 1 == size)
                        {
                            // match success
                            noRequeue = true;
                            abortSearching = !matchFunction(m.address - trimmed, m.startIndex, m.endIndex);
                            if(abortSearching)
                                break;
                        }
                    }
                    else
                    {
                        // match failed - pattern is nonzero in the gap
                        noRequeue = true;
                        break;
                    }
                }
                // TODO: We don't queue new partial matches here, so patterns starting with "00" cannot be found when "00" is in the gap.
                if(abortSearching)
                    break;
                if(!noRequeue)
                    partialMatches.emplace_back(m);
            }
            if(abortSearching)
                break;
        }
        prevAddress = address;
        values.clear();
        do
        {
            // Ignore memory accesses that don't change (afterwards if one section matches then adjacent sections will not match)
            if(values.empty() || values.back().first != it->second.oldData)
            {
                values.emplace_back(std::pair<unsigned char, TRACEINDEX>(it->second.oldData, it->first.index));
            }
            else if(values.back().first == it->second.oldData)
            {
                // update index
                values.back().second = it->first.index;
            }
            ++it;
            if(it == dump.rend())
                break;
        }
        while(it->first.addr == address);
        if(values.back().first != std::prev(it)->second.newData)
        {
            values.emplace_back(std::pair<unsigned char, TRACEINDEX>(std::prev(it)->second.newData, maxIndex));
        }
        else
        {
            // update index
            values.back().second = maxIndex;
        }
        if(size > 1) // When searching for more than a byte
        {
            size_t matchCount = partialMatches.size();
            for(size_t i = 0; i < matchCount; i++)
            {
                auto m = partialMatches.front();
                const unsigned char currentMask = firstMask[address - m.address];
                const unsigned char currentChar = firstChar[address - m.address] & currentMask;
                // Remove this entry and add updated ones.
                // One entry may fork into multiple entries, for example when a byte is changed from a match to not match and back.
                partialMatches.pop_front();
                if(values.size() == 1)
                {
                    // The value doesn't change
                    auto & value = values[0];
                    if((value.first & currentMask) == currentChar)
                    {
                        if(address - m.address + 1 >= size)
                        {
                            // match success
                            abortSearching = !matchFunction(m.address - trimmed, m.startIndex, m.endIndex);
                            if(abortSearching)
                                break;
                        }
                        else
                        {
                            // prepare for next
                            partialMatches.emplace_back(m);
                        }
                    }
                }
                else
                {
                    for(TRACEINDEX idx = 1; idx < values.size(); idx++)
                    {
                        // Between valueBefore.second and valueAfter.second, the value of memory is valueAfter.first
                        auto & valueBefore = values[idx - 1];
                        auto & valueAfter = values[idx];
                        // value matches and last occurence of value overlaps
                        if((valueAfter.first & currentMask) == currentChar && (valueBefore.second < m.endIndex && valueAfter.second > m.startIndex))
                        {
                            TRACEINDEX startIndex = m.startIndex;
                            TRACEINDEX endIndex = m.endIndex;
                            if(startIndex < valueBefore.second)
                                startIndex = valueBefore.second;
                            if(endIndex > valueAfter.second)
                                endIndex = valueAfter.second;
                            if(address - m.address + 1 >= size)
                            {
                                // match success
                                abortSearching = !matchFunction(m.address - trimmed, m.startIndex, m.endIndex);
                                if(abortSearching)
                                    break;
                            }
                            else
                            {
                                // prepare for next
                                partialMatches.emplace_back(m.address, startIndex, endIndex);
                            }
                        }
                    }
                    if(abortSearching)
                        break;
                    if((values[0].first & currentMask) == currentChar && values[0].second > m.startIndex)
                    {
                        TRACEINDEX endIndex = m.endIndex;
                        if(endIndex > values[0].second)
                            endIndex = values[0].second;
                        if(address - m.address + 1 >= size)
                        {
                            // match success
                            abortSearching = !matchFunction(m.address - trimmed, m.startIndex, m.endIndex);
                            if(abortSearching)
                                break;
                        }
                        else
                        {
                            // prepare for next
                            partialMatches.emplace_back(m.address, m.startIndex, endIndex);
                        }
                    }
                }
            }
            if(abortSearching)
                break;
        }
        // Check last character
        for(size_t idx = 0; idx < values.size(); idx++)
        {
            if((values[idx].first & firstMask[0]) == (firstChar[0] & firstMask[0]))
            {
                // Create a partial match entry
                PARTIALMATCH m;
                m.address = address;
                if(idx > 0)
                    m.startIndex = values[idx - 1].second;
                else
                    m.startIndex = 0;
                m.endIndex = values[idx].second;
                if(size == 1)
                {
                    // Just searching for a byte
                    abortSearching = !matchFunction(m.address - trimmed, m.startIndex, m.endIndex);
                    if(abortSearching)
                        break;
                }
                else
                {
                    idx++; // Because this one matches and next one must change value, the new value cannot be a match.
                    partialMatches.emplace_back(m);
                }
            }
        }
    }
    while(it != dump.rend());
}

// TraceFileDumpMemoryPage
TraceFileDumpMemoryPage::TraceFileDumpMemoryPage(TraceFileDump* dump, QObject* parent) : MemoryPage(0x10000, 0x1000, parent)
{
    this->dump = dump;
}

void TraceFileDumpMemoryPage::setSelectedIndex(TRACEINDEX index)
{
    if(dump)
        selectedIndex = std::min(index, dump->getMaxIndex());
    else
        selectedIndex = static_cast<TRACEINDEX>(0);
}

bool TraceFileDumpMemoryPage::isAvailable() const
{
    return !!this->dump;
}

TRACEINDEX TraceFileDumpMemoryPage::getSelectedIndex() const
{
    return selectedIndex;
}

bool TraceFileDumpMemoryPage::read(void* parDest, dsint parRVA, duint parSize) const
{
    if(!dump)
        return false;
    dump->getBytes(mBase + parRVA, parSize, selectedIndex, parDest);
    return true;
}

bool TraceFileDumpMemoryPage::write(const void* parDest, dsint parRVA, duint parSize)
{
    Q_UNUSED(parDest);
    Q_UNUSED(parRVA);
    Q_UNUSED(parSize);
    return false; // write is not supported
}
