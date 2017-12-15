#include "symbolsourcepdb.h"
#include "console.h"
#include "debugger.h"

class ScopedCriticalSection
{
private:
    CRITICAL_SECTION* _cs;
public:
    ScopedCriticalSection(CRITICAL_SECTION* cs) : _cs(cs) { EnterCriticalSection(cs); }
    ~ScopedCriticalSection() { LeaveCriticalSection(_cs); }
};

SymbolSourcePDB::SymbolSourcePDB()
    : _isLoading(false),
      _requiresShutdown(false)
{
    InitializeCriticalSection(&_cs);
}

SymbolSourcePDB::~SymbolSourcePDB()
{
    if(_isLoading)
    {
        _requiresShutdown = true;
        if(_loadThread.joinable())
            _loadThread.join();
    }

    if(_pdb.isOpen())
    {
        _pdb.close();
    }

    DeleteCriticalSection(&_cs);
}

bool SymbolSourcePDB::loadPDB(const std::string & path, duint imageBase)
{
    if(!PDBDiaFile::initLibrary())
    {
        return false;
    }

#if 1
    bool res = _pdb.open(path.c_str());
    if(res)
    {
        _isLoading = true;
        _requiresShutdown = false;
        _loadStart = GetTickCount64();
        _loadThread = std::thread(&SymbolSourcePDB::loadPDBAsync, this);
    }
    return res;
#else
    return _pdb.open(path.c_str());
#endif
}

bool SymbolSourcePDB::isOpen() const
{
    return _pdb.isOpen();
}

bool SymbolSourcePDB::isLoading() const
{
    return _isLoading;
}

void SymbolSourcePDB::loadPDBAsync()
{
    bool res = _pdb.enumerateLexicalHierarchy([&](DiaSymbol_t & sym)->bool
    {
        if(_requiresShutdown)
            return false;

        if(sym.type == DiaSymbolType::FUNCTION ||
        sym.type == DiaSymbolType::LABEL ||
        sym.type == DiaSymbolType::DATA)
        {
            SymbolInfo symInfo;
            symInfo.decoratedName = sym.name;
            symInfo.undecoratedName = sym.undecoratedName;
            symInfo.size = sym.size;
            symInfo.disp = sym.disp;
            symInfo.addr = sym.virtualAddress;

            EnterCriticalSection(&_cs);
            _sym.insert(std::make_pair(symInfo.addr, symInfo));

            if(_sym.size() % 1000 == 0)
            {
                GuiUpdateAllViews();
            }
            LeaveCriticalSection(&_cs);
        }

        return true;
    }, true);

    _isLoading = false;

    if(!res)
        return;

    DWORD64 ms = GetTickCount64() - _loadStart;
    double secs = (double)ms / 1000.0;

    dprintf("Loaded %d symbols in %.03f\n", _sym.size(), secs);

    GuiUpdateAllViews();
}

bool SymbolSourcePDB::findSymbolExact(duint rva, SymbolInfo & symInfo)
{
    ScopedCriticalSection lock(&_cs);

    if(SymbolSourceBase::isAddressInvalid(rva))
        return false;

    auto it = _sym.find(rva);
    if(it != _sym.end())
    {
        symInfo = (*it).second;
        return true;
    }
    /*
    DiaSymbol_t sym;

    if (_pdb.findSymbolRVA(rva, sym) && sym.disp == 0)
    {
        symInfo.addr = rva;
        symInfo.disp = 0;
        symInfo.size = sym.size;
        symInfo.decoratedName = sym.undecoratedName;
        symInfo.undecoratedName = sym.undecoratedName;
        symInfo.valid = true;

        _symbols.insert(rva, symInfo);

        return true;
    }
    */
#if 1
    if(_isLoading == false)
        markAdressInvalid(rva);
#endif

    return false;
}

template <typename A, typename B>
typename A::iterator findExactOrLower(A & ctr, const B key)
{
    if(ctr.empty())
        return ctr.end();

    auto itr = ctr.lower_bound(key);

    if(itr == ctr.begin() && (*itr).first != key)
        return ctr.end();
    else if(itr == ctr.end() || (*itr).first != key)
        return --itr;

    return itr;
}

bool SymbolSourcePDB::findSymbolExactOrLower(duint rva, SymbolInfo & symInfo)
{
    ScopedCriticalSection lock(&_cs);

    auto it = findExactOrLower(_sym, rva);
    if(it != _sym.end())
    {
        symInfo = (*it).second;
        symInfo.disp = (int32_t)(rva - symInfo.addr);
        return true;
    }

    /*
    DiaSymbol_t sym;
    if (_pdb.findSymbolRVA(rva, sym))
    {
        symInfo.addr = rva;
        symInfo.disp = sym.disp;
        symInfo.size = sym.size;
        symInfo.decoratedName = sym.undecoratedName;
        symInfo.undecoratedName = sym.undecoratedName;
        symInfo.valid = true;

        return true;
    }
    */

    return nullptr;
}

void SymbolSourcePDB::enumSymbols(const CbEnumSymbol & cbEnum)
{
    ScopedCriticalSection lock(&_cs);
    for(auto & it : _sym)
        if(!cbEnum(it.second))
            break;
}

