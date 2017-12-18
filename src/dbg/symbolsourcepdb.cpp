#include "symbolsourcepdb.h"
#include "console.h"
#include "debugger.h"

SymbolSourcePDB::SymbolSourcePDB()
    : _isLoading(false),
      _requiresShutdown(false),
      _imageBase(0)
{
}

SymbolSourcePDB::~SymbolSourcePDB()
{
    if(_isLoading || _loadThread.joinable())
    {
        cancelLoading();
    }

    if(_pdb.isOpen())
    {
        _pdb.close();
    }
}

bool SymbolSourcePDB::loadPDB(const std::string & path, duint imageBase)
{
    if(!PDBDiaFile::initLibrary())
    {
        return false;
    }

    bool res = _pdb.open(path.c_str());
#if 1 // Async loading.
    if(res)
    {
        _imageBase = imageBase;
        _isLoading = true;
        _requiresShutdown = false;
        _loadStart = GetTickCount64();
        _loadThread = std::thread(&SymbolSourcePDB::loadPDBAsync, this);
    }
#endif
    return res;
}

bool SymbolSourcePDB::isOpen() const
{
    return _pdb.isOpen();
}

bool SymbolSourcePDB::isLoading() const
{
    return _isLoading;
}

bool SymbolSourcePDB::cancelLoading()
{
    if(_isLoading || _loadThread.joinable())
    {
        _requiresShutdown.store(true);
        _loadThread.join();
    }
    else
    {
        return false;
    }
    return true;
}

void SymbolSourcePDB::loadPDBAsync()
{
    DWORD lastUpdate = 0;

    bool res = _pdb.enumerateLexicalHierarchy([&](DiaSymbol_t & sym)->bool
    {
        if(_requiresShutdown)
            return false;

        if(sym.type == DiaSymbolType::PUBLIC ||
        sym.type == DiaSymbolType::FUNCTION ||
        sym.type == DiaSymbolType::LABEL ||
        sym.type == DiaSymbolType::DATA)
        {
            SymbolInfo symInfo;
            symInfo.decoratedName = sym.name;
            symInfo.undecoratedName = sym.undecoratedName;
            symInfo.size = sym.size;
            symInfo.disp = sym.disp;
            symInfo.addr = sym.virtualAddress;
            symInfo.publicSymbol = sym.publicSymbol;

            _lock.lock();

            // Check if we already have it inside, private symbols have priority over public symbols.
            auto it = _sym.find(symInfo.addr);
            if(it != _sym.end())
            {
                if(it->second.publicSymbol == true && symInfo.publicSymbol == false)
                {
                    // Replace.
                    it->second = symInfo;
                }
            }
            else
            {
                _sym.insert(std::make_pair(symInfo.addr, symInfo));
            }

            _lock.unlock();

            DWORD curTick = GetTickCount();
            if(curTick - lastUpdate > 500)
            {
                GuiUpdateAllViews();
                lastUpdate = curTick;
            }
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
    ScopedSpinLock lock(_lock);

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
    ScopedSpinLock lock(_lock);

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
    ScopedSpinLock lock(_lock);

    for(auto & it : _sym)
    {
        const SymbolInfo & sym = it.second;
        if(!cbEnum(sym))
        {
            break;
        }
    }
}

bool SymbolSourcePDB::findSourceLineInfo(duint rva, LineInfo & lineInfo)
{
    std::map<uint64_t, DiaLineInfo_t> lines;

    if(!_pdb.getFunctionLineNumbers(rva, 1, 0, lines))
        return false;

    if(lines.empty())
        return false;

    // Unhandled case, requires refactoring to query ranges instead of single lines.
    if(lines.size() > 1)
    {
        return false;
    }

    const auto & info = (*lines.begin()).second;
    lineInfo.addr = rva;
    lineInfo.sourceFile = info.fileName;
    lineInfo.lineNumber = info.lineNumber;

    return true;
}