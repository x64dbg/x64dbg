#pragma once

#include "Imports.h"
#include <map>
#include <functional>
#include <QMutex>
#include "MemoryPage.h"

typedef DWORD TRACEINDEX;

class TraceFileDump
{
public:
    struct Key
    {
        duint addr;
        TRACEINDEX index;
        friend bool operator <(const Key & a, const Key & b) noexcept
        {
            // order is inverted, highest address is less! We want to use lower_bound() to find last memory access index.
            return a.addr > b.addr || a.addr == b.addr && a.index > b.index;
        }
    };
    struct DumpRecord
    {
        unsigned char oldData;
        unsigned char newData;
        //unsigned char isWrite;
        //unsigned char isExecute;
    };

    TraceFileDump();
    ~TraceFileDump();
    void clear();
    inline void setEnabled() noexcept
    {
        enabled = true;
    }
    inline bool isEnabled() const noexcept
    {
        return enabled;
    }
    // Read a byte at "addr" at the moment given in "index"
    bool isValidReadPtr(duint addr) const;
    void getBytes(duint addr, duint size, TRACEINDEX index, void* buffer) const;
    std::vector<TRACEINDEX> getReferences(duint startAddr, duint endAddr) const;
    // Insert memory access records
    void addMemAccess(duint cip, unsigned char* opcode, int opcodeSize, duint* memAddr, const duint* oldMemory, const duint* newMemory, size_t count);
    // Find pattern
    void findAllMem(const unsigned char* data, const unsigned char* mask, size_t size, std::function<bool(duint, TRACEINDEX, TRACEINDEX)> matchFunction) const;
    inline void increaseIndex() noexcept
    {
        maxIndex++;
    }
    inline TRACEINDEX getMaxIndex() noexcept
    {
        return maxIndex;
    }
    // Find continuous memory areas (currently unused)
    // void findMemAreas();
    std::vector<std::pair<duint, duint>> memAreas;
private:
    std::map<Key, DumpRecord> dump;
    // maxIndex is the last index included here. As the debuggee steps there will be new data coming.
    TRACEINDEX maxIndex;
    bool enabled;
};

class TraceFileDumpMemoryPage : public MemoryPage
{
    Q_OBJECT
public:
    TraceFileDumpMemoryPage(TraceFileDump* dump, QObject* parent = nullptr);
    virtual bool read(void* parDest, dsint parRVA, duint parSize) const override;
    virtual bool write(const void* parDest, dsint parRVA, duint parSize) override;
    void setSelectedIndex(TRACEINDEX index);
    TRACEINDEX getSelectedIndex() const;
    bool isAvailable() const;
private:
    TraceFileDump* dump;
    TRACEINDEX selectedIndex = static_cast<TRACEINDEX>(0);
};
