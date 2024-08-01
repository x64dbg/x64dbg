#pragma once

#include "Imports.h"
#include <map>
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
        //unsigned char isWrite;
        //unsigned char isExecute;
    };

    TraceFileDump();
    ~TraceFileDump();
    void clear();
    inline void setEnabled()
    {
        enabled = true;
    }
    inline bool isEnabled() const
    {
        return enabled;
    }
    // Read a byte at "addr" at the moment given in "index"
    bool isValidReadPtr(duint addr) const;
    void getBytes(duint addr, duint size, TRACEINDEX index, void* buffer) const;
    std::vector<TRACEINDEX> getReferences(duint startAddr, duint endAddr) const;
    // Insert a memory access record
    //void addMemAccess(duint addr, DumpRecord record);
    void addMemAccess(duint addr, const void* oldData, const void* newData, size_t size);
    inline void increaseIndex()
    {
        maxIndex++;
    }
    inline TRACEINDEX getMaxIndex()
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
