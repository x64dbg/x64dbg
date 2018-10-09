#include "symbolsourcedia.h"
#include "console.h"
#include "debugger.h"
#include <algorithm>

SymbolSourceDIA::SymbolSourceDIA()
    : _isOpen(false),
      _requiresShutdown(false),
      _loadCounter(0),
      _imageBase(0),
      _imageSize(0)
{
}

SymbolSourceDIA::~SymbolSourceDIA()
{
    SymbolSourceDIA::cancelLoading();
    if(_imageBase)
        GuiInvalidateSymbolSource(_imageBase);
}

static void SetWin10ThreadDescription(HANDLE threadHandle, const WString & name)
{
    typedef HRESULT(WINAPI * fnSetThreadDescription)(HANDLE hThread, PCWSTR lpThreadDescription);

    fnSetThreadDescription fp = (fnSetThreadDescription)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetThreadDescription");
    if(!fp)
        return; // Only available on windows 10.

    fp(threadHandle, name.c_str());
}

DWORD WINAPI SymbolSourceDIA::SymbolsThread(void* parameter)
{
    ((SymbolSourceDIA*)parameter)->loadSymbolsAsync();
    return 0;
}

DWORD WINAPI SymbolSourceDIA::SourceLinesThread(void* parameter)
{
    ((SymbolSourceDIA*)parameter)->loadSourceLinesAsync();
    return 0;
}

bool SymbolSourceDIA::loadPDB(const std::string & path, duint imageBase, duint imageSize, DiaValidationData_t* validationData)
{
    PDBDiaFile pdb; // Instance used for validation only.
    _isOpen = pdb.open(path.c_str(), 0, validationData);
#if 1 // Async loading.
    if(_isOpen)
    {
        _path = path;
        _imageSize = imageSize;
        _imageBase = imageBase;
        _requiresShutdown = false;
        _symbolsLoaded = false;
        _loadCounter.store(2);
        _symbolsThread = CreateThread(nullptr, 0, SymbolsThread, this, CREATE_SUSPENDED, nullptr);
        SetWin10ThreadDescription(_symbolsThread, L"SymbolsThread");
        _sourceLinesThread = CreateThread(nullptr, 0, SourceLinesThread, this, CREATE_SUSPENDED, nullptr);
        SetWin10ThreadDescription(_sourceLinesThread, L"SourceLinesThread");
        ResumeThread(_symbolsThread);
        ResumeThread(_sourceLinesThread);
    }
#endif
    return _isOpen;
}

bool SymbolSourceDIA::isOpen() const
{
    return _isOpen;
}

bool SymbolSourceDIA::isLoading() const
{
    return _loadCounter > 0;
}

bool SymbolSourceDIA::cancelLoading()
{
    _requiresShutdown.store(true);
    if(_symbolsThread)
    {
        WaitForSingleObject(_symbolsThread, INFINITE);
        CloseHandle(_symbolsThread);
        _symbolsThread = nullptr;
    }
    if(_sourceLinesThread)
    {
        WaitForSingleObject(_sourceLinesThread, INFINITE);
        CloseHandle(_sourceLinesThread);
        _sourceLinesThread = nullptr;
    }
    return true;
}

void SymbolSourceDIA::loadPDBAsync()
{
}

template<size_t Count>
static bool startsWith(const char* str, const char(&prefix)[Count])
{
    return strncmp(str, prefix, Count - 1) == 0;
}

bool SymbolSourceDIA::loadSymbolsAsync()
{
    ScopedDecrement ref(_loadCounter);

    PDBDiaFile pdb;

    if(!pdb.open(_path.c_str()))
    {
        return false;
    }

    DWORD lastUpdate = 0;
    DWORD loadStart = GetTickCount();

    PDBDiaFile::Query_t query;
    query.collectSize = true;
    query.collectUndecoratedNames = true;
    query.callback = [&](DiaSymbol_t & sym)->bool
    {
        if(_requiresShutdown)
            return false;

        if(sym.name.c_str()[0] == 0x7F)
            return true;

#define filter(prefix) if(startsWith(sym.name.c_str(), prefix)) return true
        filter("__imp__");
        filter("__imp_?");
        filter("_imp___");
        filter("__NULL_IMPORT_DESCRIPTOR");
        filter("__IMPORT_DESCRIPTOR_");
#undef filter

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
            symInfo.rva = sym.virtualAddress;
            symInfo.publicSymbol = sym.publicSymbol;

            // Check if we already have it inside, private symbols have priority over public symbols.
            // TODO: only use this map during initialization phase
            {
                ScopedSpinLock lock(_lockSymbols);

                auto it = _symAddrs.find(sym.virtualAddress);
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
                    _symAddrs.insert({ sym.virtualAddress, _symData.size() - 1 });
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

    bool res = pdb.enumerateLexicalHierarchy(query);

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
        _symbolsLoaded = true;
    }

    DWORD ms = GetTickCount() - loadStart;
    double secs = (double)ms / 1000.0;

    GuiSymbolLogAdd(StringUtils::sprintf("[%p] Loaded %d symbols in %.03fs\n", _imageBase, _symAddrs.size(), secs).c_str());

    GuiInvalidateSymbolSource(_imageBase);

    GuiUpdateAllViews();

    return true;
}

bool SymbolSourceDIA::loadSourceLinesAsync()
{
    ScopedDecrement ref(_loadCounter);

    PDBDiaFile pdb;

    if(!pdb.open(_path.c_str()))
    {
        return false;
    }

    DWORD lineLoadStart = GetTickCount();

    const size_t rangeSize = 1024 * 1024;

    for(duint rva = 0; rva < _imageSize; rva += rangeSize)
    {
        if(_requiresShutdown)
            return false;

        std::map<uint64_t, DiaLineInfo_t> lines;

        bool res = pdb.getFunctionLineNumbers(rva, rangeSize, _imageBase, lines);
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

            auto idx = findSourceFile(info.fileName);
            if(idx == -1)
            {
                idx = _sourceFiles.size();
                _sourceFiles.push_back(info.fileName);
            }
            lineInfo.sourceFileIdx = idx;

            _lockLines.lock();

            _linesData.push_back(lineInfo);
            _lines.insert({ lineInfo.rva, _linesData.size() - 1 });

            _lockLines.unlock();
        }

    }

    _sourceLines.resize(_sourceFiles.size());
    for(size_t i = 0; i < _linesData.size(); i++)
    {
        auto & line = _linesData[i];
        _sourceLines[line.sourceFileIdx].insert({ line.lineNumber, i });
    }

    if(_requiresShutdown)
        return false;

    DWORD ms = GetTickCount() - lineLoadStart;
    double secs = (double)ms / 1000.0;

    GuiSymbolLogAdd(StringUtils::sprintf("[%p] Loaded %d line infos in %.03fs\n", _imageBase, _lines.size(), secs).c_str());

    GuiUpdateAllViews();

    return true;
}

uint32_t SymbolSourceDIA::findSourceFile(const std::string & fileName) const
{
    //TODO: make this fast
    uint32_t idx = -1;
    for(uint32_t n = 0; n < _sourceFiles.size(); n++)
    {
        const String & str = _sourceFiles[n];
        if(stricmp(str.c_str(), fileName.c_str()) == 0)
        {
            idx = n;
            break;
        }
    }
    return idx;
}

bool SymbolSourceDIA::findSymbolExact(duint rva, SymbolInfo & symInfo)
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

bool SymbolSourceDIA::findSymbolExactOrLower(duint rva, SymbolInfo & symInfo)
{
    ScopedSpinLock lock(_lockSymbols);

    auto it = findExactOrLower(_symAddrs, rva);
    if(it != _symAddrs.end())
    {
        symInfo = _symData[it->second];
        symInfo.disp = (int32_t)(rva - symInfo.rva);
        return true;
    }

    return nullptr;
}

void SymbolSourceDIA::enumSymbols(const CbEnumSymbol & cbEnum)
{
    ScopedSpinLock lock(_lockSymbols);
    if(!_symbolsLoaded)
        return;

    for(auto & it : _symAddrs)
    {
        const SymbolInfo & sym = _symData[it.second];
        if(!cbEnum(sym))
        {
            break;
        }
    }
}

bool SymbolSourceDIA::findSourceLineInfo(duint rva, LineInfo & lineInfo)
{
    ScopedSpinLock lock(_lockLines);

    auto found = _lines.find(rva);
    if(found == _lines.end())
        return false;

    auto & cached = _linesData.at(found->second);
    lineInfo.rva = cached.rva;
    lineInfo.lineNumber = cached.lineNumber;
    lineInfo.disp = 0;
    lineInfo.size = 0;
    lineInfo.sourceFile = _sourceFiles[cached.sourceFileIdx];
    return true;
}

bool SymbolSourceDIA::findSourceLineInfo(const std::string & file, int line, LineInfo & lineInfo)
{
    ScopedSpinLock lock(_lockLines);

    auto sourceIdx = findSourceFile(file);
    if(sourceIdx == -1)
        return false;

    auto & lineMap = _sourceLines[sourceIdx];
    auto found = lineMap.find(line);
    if(found == lineMap.end())
        return false;

    auto & cached = _linesData.at(found->second);
    lineInfo.rva = cached.rva;
    lineInfo.lineNumber = cached.lineNumber;
    lineInfo.disp = 0;
    lineInfo.size = 0;
    lineInfo.sourceFile = _sourceFiles[cached.sourceFileIdx];
    return true;
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

bool SymbolSourceDIA::findSymbolByName(const std::string & name, SymbolInfo & symInfo, bool caseSensitive)
{
    ScopedSpinLock lock(_lockSymbols);
    if(!_symbolsLoaded)
        return false;

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

bool SymbolSourceDIA::findSymbolsByPrefix(const std::string & prefix, const std::function<bool(const SymbolInfo &)> & cbSymbol, bool caseSensitive)
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
    if(!_symbolsLoaded)
        return false;

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
