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
private:
    PDBDiaFile _pdb;
    SortedLRU<duint, SymbolInfo> _symbols;
    std::map<duint, SymbolInfo> _sym;
    std::thread _loadThread;
    std::atomic<bool> _isLoading;
    std::atomic<bool> _requiresShutdown;
    std::atomic_flag _isWriting;
    SpinLock _lock;
    DWORD64 _loadStart;
    std::atomic_flag _locked;

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

public:
    bool loadPDB(const std::string & path, duint imageBase);

private:
    void loadPDBAsync();
};

#endif // _SYMBOLSOURCEPDB_H_
