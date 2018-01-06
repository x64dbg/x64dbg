#include "symbolsourcepdb.h"
#include "console.h"
#include "debugger.h"
#include <algorithm>

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

static void SetThreadDescription(std::thread & thread, WString name)
{
    typedef HRESULT(WINAPI * fnSetThreadDescription)(HANDLE hThread, PCWSTR lpThreadDescription);

    fnSetThreadDescription fp = (fnSetThreadDescription)GetProcAddress(GetModuleHandleA("kernel32.dll"), "SetThreadDescription");
    if(!fp)
        return; // Only available on windows 10.

    HANDLE handle = static_cast<HANDLE>(thread.native_handle());
    fp(handle, name.c_str());
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
        SetThreadDescription(_symbolsThread, L"SymbolsThread");
        _sourceLinesThread = std::thread(&SymbolSourcePDB::loadSourceLinesAsync, this, path);
        SetThreadDescription(_symbolsThread, L"SourceLinesThread");
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

    PDBDiaFile::Query_t query;
    query.collectSize = true;
    query.collectUndecoratedNames = true;
    query.callback = [&](DiaSymbol_t & sym)->bool
    {
        if(_requiresShutdown)
            return false;

        if(sym.type == DiaSymbolType::PUBLIC ||
        sym.type == DiaSymbolType::FUNCTION ||
        sym.type == DiaSymbolType::LABEL ||
        sym.type == DiaSymbolType::DATA) //TODO: properly handle import thunks + empty names + line symbols
        {
            SymbolInfo symInfo;
            symInfo.decoratedName = sym.name;
            symInfo.undecoratedName = sym.undecoratedName;
            symInfo.size = sym.size;
            symInfo.disp = sym.disp;
            symInfo.addr = sym.virtualAddress;
            symInfo.publicSymbol = sym.publicSymbol;

            // Check if we already have it inside, private symbols have priority over public symbols.
            // TODO: only use this map during initialization phase
            {
                ScopedSpinLock lock(_lockSymbols);

                auto it = _symAddrs.find(symInfo.addr);
                if(it != _symAddrs.end())
                {
                    if(_symData[it->second].publicSymbol == true && symInfo.publicSymbol == false)
                    {
                        // Replace.
                        _symData[it->second] = symInfo;
                    }
                }
                else
                {
                    _symData.push_back(symInfo);
                    _symAddrs.insert({ symInfo.addr, _symData.size() - 1 });
                }
            }

            //TODO: perhaps this shouldn't be done...
            DWORD curTick = GetTickCount();
            if(curTick - lastUpdate > 500)
            {
                GuiUpdateAllViews();
                lastUpdate = curTick;
            }
        }

        return true;
    };

    bool res = _pdb.enumerateLexicalHierarchy(query);

    if(!res)
    {
        return false;
    }

    //TODO: gracefully handle temporary storage (the spin lock will now starve the GUI while sorting)
    {
        ScopedSpinLock lock(_lockSymbols);

        //TODO: actually do something with this map
        _symAddrMap.reserve(_symAddrs.size());
        for(auto & it : _symAddrs)
        {
            AddrIndex addrIndex;
            addrIndex.addr = it.first;
            addrIndex.index = it.second;
            _symAddrMap.push_back(addrIndex);
        }
        std::sort(_symAddrMap.begin(), _symAddrMap.end());

        //handle symbol name sorting
        _symNameMap.resize(_symData.size());
        for(size_t i = 0; i < _symData.size(); i++)
        {
            NameIndex nameIndex;
            nameIndex.index = i;
            nameIndex.name = _symData.at(i).decoratedName.c_str(); //NOTE: DO NOT MODIFY decoratedName is any way!
            _symNameMap[i] = nameIndex;
        }
        std::sort(_symNameMap.begin(), _symNameMap.end());
    }

    DWORD64 ms = GetTickCount64() - loadStart;
    double secs = (double)ms / 1000.0;

    dprintf("Loaded %d symbols in %.03f\n", _symAddrs.size(), secs);

    //TODO: make beautiful
    ListInfo blub;
    blub.count = _symAddrs.size();
    blub.size = blub.count * sizeof(void*);
    std::vector<SymbolInfo*> fuck;
    fuck.resize(_symAddrs.size());
    size_t i = 0;
    for(auto it = _symAddrs.begin(); it != _symAddrs.end(); ++it)
        fuck[i++] = &_symData[it->second];
    blub.data = fuck.data();
    GuiSetModuleSymbols(_imageBase, &blub);

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

            CachedLineInfo lineInfo;
            lineInfo.rva = info.virtualAddress;
            lineInfo.lineNumber = info.lineNumber;

            uint32_t idx = -1;
            for(uint32_t n = 0; n < _sourceFiles.size(); n++)
            {
                const String & str = _sourceFiles[n];
                size_t size = str.size();
                if(size != info.fileName.size())
                    continue;
                if(str[0] != info.fileName[0])
                    continue;
                if(str[size - 1] != info.fileName[size - 1])
                    continue;
                if(str != info.fileName)
                    continue;
                idx = n;
                break;
            }
            if(idx == -1)
            {
                idx = _sourceFiles.size();
                _sourceFiles.push_back(info.fileName);
            }
            lineInfo.sourceFileIdx = idx;

            _lockLines.lock();

            _lines.insert(std::make_pair(lineInfo.rva, lineInfo));

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

    auto it = _symAddrs.find(rva);
    if(it != _symAddrs.end())
    {
        symInfo = _symData[it->second];
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

    auto it = findExactOrLower(_symAddrs, rva);
    if(it != _symAddrs.end())
    {
        symInfo = _symData[it->second];
        symInfo.disp = (int32_t)(rva - symInfo.addr);
        return true;
    }

    return nullptr;
}

void SymbolSourcePDB::enumSymbols(const CbEnumSymbol & cbEnum)
{
    ScopedSpinLock lock(_lockSymbols);

    for(auto & it : _symAddrs)
    {
        const SymbolInfo & sym = _symData[it.second];
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
    if(it != _lines.end())
    {
        const CachedLineInfo & cached = it->second;
        lineInfo.lineNumber = cached.lineNumber;
        lineInfo.disp = 0;
        lineInfo.size = 0;
        lineInfo.sourceFile = _sourceFiles[cached.sourceFileIdx];
        return true;
    }

    return false;
}

//http://en.cppreference.com/w/cpp/algorithm/lower_bound
template<class ForwardIt, class T, class Compare = std::less<>>
ForwardIt binary_find(ForwardIt first, ForwardIt last, const T & value, Compare comp = {})
{
    // Note: BOTH type T and the type after ForwardIt is dereferenced
    // must be implicitly convertible to BOTH Type1 and Type2, used in Compare.
    // This is stricter than lower_bound requirement (see above)

    first = std::lower_bound(first, last, value, comp);
    return first != last && !comp(value, *first) ? first : last;
}

bool SymbolSourcePDB::findSymbolByName(const std::string & name, SymbolInfo & symInfo, bool caseSensitive)
{
    ScopedSpinLock lock(_lockSymbols);

    NameIndex find;
    find.name = name.c_str();
    auto found = binary_find(_symNameMap.begin(), _symNameMap.end(), find);
    if(found != _symNameMap.end())
    {
        do
        {
            if(find.cmp(*found, find, caseSensitive) == 0)
            {
                symInfo = _symData.at(found->index);
                return true;
            }
            ++found;
        }
        while(found != _symNameMap.end() && find.cmp(find, *found, false) == 0);
    }
    return false;
}

bool SymbolSourcePDB::findSymbolsByPrefix(const std::string & prefix, const std::function<bool(const SymbolInfo &)> & cbSymbol, bool caseSensitive)
{
    struct PrefixCmp
    {
        PrefixCmp(size_t n) : n(n) { }

        bool operator()(const NameIndex & a, const NameIndex & b)
        {
            return cmp(a, b, false) < 0;
        }

        int cmp(const NameIndex & a, const NameIndex & b, bool caseSensitive)
        {
            return (caseSensitive ? strncmp : _strnicmp)(a.name, b.name, n);
        }

    private:
        size_t n;
    } prefixCmp(prefix.size());

    ScopedSpinLock lock(_lockSymbols);

    NameIndex find;
    find.name = prefix.c_str();
    auto found = binary_find(_symNameMap.begin(), _symNameMap.end(), find, prefixCmp);
    if(found == _symNameMap.end())
        return false;

    bool result = false;
    for(; found != _symNameMap.end() && prefixCmp.cmp(find, *found, false) == 0; ++found)
    {
        if(!caseSensitive || prefixCmp.cmp(find, *found, true) == 0)
        {
            result = true;
            if(!cbSymbol(_symData.at(found->index)))
                break;
        }
    }

    return result;
}
