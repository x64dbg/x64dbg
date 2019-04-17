#ifndef _SYMBOLSOURCEDIA_H_
#define _SYMBOLSOURCEDIA_H_

#include "_global.h"

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

class SymbolSourceDIA : public SymbolSourceBase
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

private: //symbols
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

private: //line info
    //TODO: make this source file stuff smarter
    std::vector<CachedLineInfo> _linesData;
    std::map<duint, size_t> _lines; //addr -> line
    std::vector<String> _sourceFiles; // uniqueId + name
    std::map<DWORD, uint32_t> _sourceIdMap; //uniqueId -> index in _sourceFiles
    std::vector<std::map<int, size_t>> _sourceLines; //uses index in _sourceFiles

private: //general
    HANDLE _symbolsThread = nullptr;
    HANDLE _sourceLinesThread = nullptr;
    std::atomic<bool> _requiresShutdown;
    std::atomic<duint> _loadCounter;

    bool _isOpen;
    std::string _path;
    std::string _modname;
    duint _imageBase;
    duint _imageSize;
    SpinLock _lockSymbols;
    bool _symbolsLoaded = false;
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
    SymbolSourceDIA();

    virtual ~SymbolSourceDIA() override;

    virtual bool isOpen() const override;

    virtual bool isLoading() const override;

    virtual bool cancelLoading() override;

    virtual bool findSymbolExact(duint rva, SymbolInfo & symInfo) override;

    virtual bool findSymbolExactOrLower(duint rva, SymbolInfo & symInfo) override;

    virtual void enumSymbols(const CbEnumSymbol & cbEnum) override;

    virtual bool findSourceLineInfo(duint rva, LineInfo & lineInfo) override;

    virtual bool findSourceLineInfo(const std::string & file, int line, LineInfo & lineInfo) override;

    virtual bool findSymbolByName(const std::string & name, SymbolInfo & symInfo, bool caseSensitive) override;

    virtual bool findSymbolsByPrefix(const std::string & prefix, const std::function<bool(const SymbolInfo &)> & cbSymbol, bool caseSensitive) override;

    virtual std::string loadedSymbolPath() const override;

public:
    bool loadPDB(const std::string & path, const std::string & modname, duint imageBase, duint imageSize, DiaValidationData_t* validationData);

private:
    void loadPDBAsync();
    bool loadSymbolsAsync();
    bool loadSourceLinesAsync();
    uint32_t findSourceFile(const std::string & fileName) const;

    static DWORD WINAPI SymbolsThread(void* parameter);
    static DWORD WINAPI SourceLinesThread(void* parameter);
};

#endif // _SYMBOLSOURCEPDB_H_
