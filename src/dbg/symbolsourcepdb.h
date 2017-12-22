#ifndef _SYMBOLSOURCEPDB_H_
#define _SYMBOLSOURCEPDB_H_

#include "pdbdiafile.h"
#include "symbolsourcebase.h"
#include "sortedlru.h"

#include <thread>
#include <atomic>
#include <mutex>

class SpinLock
{
private:
    std::atomic_flag _locked;

public:
    SpinLock() { _locked.clear(); }
    void lock()
    {
        while(_locked.test_and_set(std::memory_order_acquire)) { ; }
    }
    void unlock()
    {
        _locked.clear(std::memory_order_release);
    }
};

class ScopedSpinLock
{
private:
    SpinLock & _lock;
public:
    ScopedSpinLock(SpinLock & lock) : _lock(lock) { _lock.lock(); }
    ~ScopedSpinLock() { _lock.unlock(); }
};

class SymbolSourcePDB : public SymbolSourceBase
{
    struct CachedLineInfo
    {
        uint32 rva;
        uint32 lineNumber;
        uint32 sourceFileIdx;
    };

    struct ScopedDecrement
    {
    private:
        std::atomic<duint> & _counter;
    public:
        ScopedDecrement(std::atomic<duint> & counter) : _counter(counter) {}
        ~ScopedDecrement() { _counter--; }
    };

private:
    PDBDiaFile _pdb;

    std::vector<SymbolInfo> _symData;

    struct AddrIndex
    {
        duint addr;
        size_t index;

        bool operator<(const AddrIndex & b) const
        {
            return addr < b.addr;
        }
    };
    std::vector<AddrIndex> _symAddrMap; //rva -> data index (sorted on rva)

    struct NameIndex
    {
        const char* name;
        size_t index;

        bool operator<(const NameIndex & b) const
        {
            return cmp(*this, b, false) < 0;
        }

        static int cmp(const NameIndex & a, const NameIndex & b, bool caseSensitive)
        {
            return (caseSensitive ? strcmp : hackicmp)(a.name, b.name);
        }
    };
    std::vector<NameIndex> _symNameMap; //name -> data index (sorted on name)

    //Symbol addresses to index in _symNames (TODO: refactor to std::vector)
    std::map<duint, size_t> _symAddrs;

    //std::map<duint, SymbolInfo> _sym;
    std::map<duint, CachedLineInfo> _lines;
    std::thread _symbolsThread;
    std::thread _sourceLinesThread;
    std::atomic<bool> _requiresShutdown;
    std::atomic<duint> _loadCounter;
    std::vector<String> _sourceFiles;
    duint _imageBase;
    duint _imageSize;
    SpinLock _lockSymbols;
    SpinLock _lockLines;

private:
    static int hackicmp(const char* s1, const char* s2)
    {
        unsigned char c1, c2;
        while((c1 = *s1++) == (c2 = *s2++))
            if(c1 == '\0')
                return 0;
        s1--, s2--;
        while((c1 = tolower(*s1++)) == (c2 = tolower(*s2++)))
            if(c1 == '\0')
                return 0;
        return c1 - c2;
    }

public:
    static bool isLibraryAvailable()
    {
        return PDBDiaFile::initLibrary();
    }

public:
    SymbolSourcePDB();

    virtual ~SymbolSourcePDB() override;

    virtual bool isOpen() const override;

    virtual bool isLoading() const override;

    virtual bool cancelLoading() override;

    virtual bool findSymbolExact(duint rva, SymbolInfo & symInfo) override;

    virtual bool findSymbolExactOrLower(duint rva, SymbolInfo & symInfo) override;

    virtual void enumSymbols(const CbEnumSymbol & cbEnum) override;

    virtual bool findSourceLineInfo(duint rva, LineInfo & lineInfo) override;

    virtual bool findSymbolByName(const std::string & name, SymbolInfo & symInfo, bool caseSensitive) override;

    virtual bool findSymbolsByPrefix(const std::string & prefix, std::vector<SymbolInfo> & symbols, bool caseSensitive) override;

public:
    bool loadPDB(const std::string & path, duint imageBase, duint imageSize);

private:
    void loadPDBAsync();
    bool loadSymbolsAsync(String path);
    bool loadSourceLinesAsync(String path);
};

#endif // _SYMBOLSOURCEPDB_H_
