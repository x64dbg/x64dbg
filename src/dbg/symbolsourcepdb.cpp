#include "symbolsourcepdb.h"
#include "console.h"
#include "debugger.h"

SymbolSourcePDB::SymbolSourcePDB()
    : _requiresShutdown(false),
      _imageBase(0),
      _loadCounter(0)
{
}

SymbolSourcePDB::~SymbolSourcePDB()
{
    if(isLoading() || _symbolsThread.joinable() || _sourceLinesThread.joinable())
    {
        cancelLoading();
    }

    if(_pdb.isOpen())
    {
        _pdb.close();
    }
}

bool SymbolSourcePDB::loadPDB(const std::string & path, duint imageBase, duint imageSize)
{
    if(!PDBDiaFile::initLibrary())
    {
        return false;
    }

    bool res = _pdb.open(path.c_str());
#if 1 // Async loading.
    if(res)
    {
        _imageSize = imageSize;
        _imageBase = imageBase;
        _requiresShutdown = false;
        _loadCounter.store(2);
        _symbolsThread = std::thread(&SymbolSourcePDB::loadSymbolsAsync, this, path);
        _sourceLinesThread = std::thread(&SymbolSourcePDB::loadSourceLinesAsync, this, path);
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
    return _loadCounter > 0;
}

bool SymbolSourcePDB::cancelLoading()
{
    _requiresShutdown.store(true);
    while(_loadCounter > 0)
    {
        Sleep(1);
    }

    if(_symbolsThread.joinable())
        _symbolsThread.join();

    if(_sourceLinesThread.joinable())
        _sourceLinesThread.join();

    return true;
}

void SymbolSourcePDB::loadPDBAsync()
{
}

bool SymbolSourcePDB::loadSymbolsAsync(String path)
{
    ScopedDecrement ref(_loadCounter);

    PDBDiaFile pdb;

    if(!pdb.open(path.c_str()))
    {
        return false;
    }

    DWORD lastUpdate = 0;
    DWORD loadStart = GetTickCount64();

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

            _lockSymbols.lock();

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

            _lockSymbols.unlock();

            DWORD curTick = GetTickCount();
            if(curTick - lastUpdate > 500)
            {
                GuiUpdateAllViews();
                lastUpdate = curTick;
            }
        }

        return true;
    }, true);

    if(!res)
    {
        return false;
    }

    DWORD64 ms = GetTickCount64() - loadStart;
    double secs = (double)ms / 1000.0;

    dprintf("Loaded %d symbols in %.03f\n", _sym.size(), secs);

    GuiUpdateAllViews();

    return true;
}


bool SymbolSourcePDB::loadSourceLinesAsync(String path)
{
    ScopedDecrement ref(_loadCounter);

    PDBDiaFile pdb;

    if(!pdb.open(path.c_str()))
    {
        return false;
    }

    dprintf("Loading Source lines...\n");

    DWORD64 lineLoadStart = GetTickCount64();

    const size_t rangeSize = 1024 * 1024;

    for(duint rva = 0; rva < _imageSize; rva += rangeSize)
    {
        if(_requiresShutdown)
            return false;

        std::map<uint64_t, DiaLineInfo_t> lines;

        bool res = _pdb.getFunctionLineNumbers(rva, rangeSize, _imageBase, lines);
        for(const auto & line : lines)
        {
            if(_requiresShutdown)
                return false;

            const auto & info = line.second;
            auto it = _lines.find(info.virtualAddress);
            if(it != _lines.end())
                continue;

            LineInfo lineInfo;
            lineInfo.addr = info.virtualAddress;
            lineInfo.disp = 0;
            lineInfo.lineNumber = info.lineNumber;
            lineInfo.sourceFile = info.fileName;

            _lockLines.lock();

            _lines.insert(std::make_pair(lineInfo.addr, lineInfo));

            _lockLines.unlock();
        }

    }

    if(_requiresShutdown)
        return false;

    DWORD64 ms = GetTickCount64() - lineLoadStart;
    double secs = (double)ms / 1000.0;

    dprintf("Loaded %d line infos in %.03f\n", _lines.size(), secs);

    GuiUpdateAllViews();

    return true;
}


bool SymbolSourcePDB::findSymbolExact(duint rva, SymbolInfo & symInfo)
{
    ScopedSpinLock lock(_lockSymbols);

    if(SymbolSourceBase::isAddressInvalid(rva))
        return false;

    auto it = _sym.find(rva);
    if(it != _sym.end())
    {
        symInfo = (*it).second;
        return true;
    }

#if 1
    if(isLoading() == false)
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
    ScopedSpinLock lock(_lockSymbols);

    auto it = findExactOrLower(_sym, rva);
    if(it != _sym.end())
    {
        symInfo = (*it).second;
        symInfo.disp = (int32_t)(rva - symInfo.addr);
        return true;
    }

    return nullptr;
}

void SymbolSourcePDB::enumSymbols(const CbEnumSymbol & cbEnum)
{
    ScopedSpinLock lock(_lockSymbols);

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
    if(isOpen() == false)
        return false;

    ScopedSpinLock lock(_lockLines);

    auto it = _lines.find(rva);
    if(it == _lines.end())
        return false;

    lineInfo = it->second;
    return true;
}