#ifndef _SYMBOLSOURCEPDB_H_
#define _SYMBOLSOURCEPDB_H_

#include "pdbdiafile.h"
#include "symbolsourcebase.h"
#include "sortedlru.h"

#include <thread>
#include <atomic>
#include <mutex>

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
    CRITICAL_SECTION _cs;
    DWORD64 _loadStart;

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
