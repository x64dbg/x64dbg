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

bool SymbolSourceDIA::loadPDB(const std::string & path, const std::string & modname, duint imageBase, duint imageSize, DiaValidationData_t* validationData)
{
    if(!validationData)
    {
        GuiSymbolLogAdd(StringUtils::sprintf("[%p, %s] Skipping PDB validation, expect invalid results!\n", imageBase, modname.c_str()).c_str());
    }
    PDBDiaFile pdb; // Instance used for validation only.
    _isOpen = pdb.open(path.c_str(), 0, validationData);
    if(_isOpen)
    {
        _path = path;
        _imageSize = imageSize;
        _modname = modname;
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

template<size_t Count>
static bool startsWith(const char* str, const char(&prefix)[Count])
{
    return strncmp(str, prefix, Count - 1) == 0;
}

bool SymbolSourceDIA::loadSymbolsAsync()
{
    ScopedDecrement ref(_loadCounter);

    GuiRepaintTableView();

    PDBDiaFile pdb;

    if(!pdb.open(_path.c_str()))
    {
        return false;
    }

    DWORD lastUpdate = 0;
    DWORD loadStart = GetTickCount();

    PDBDiaFile::Query_t query;
    query.collectSize = false;
    query.collectUndecoratedNames = true;
    query.callback = [&](DiaSymbol_t & sym) -> bool
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
            _symData.emplace_back();
            SymbolInfo & symInfo = _symData.back();
            symInfo.decoratedName = std::move(sym.name);
            symInfo.undecoratedName = std::move(sym.undecoratedName);
            symInfo.size = (duint)sym.size;
            symInfo.disp = sym.disp;
            symInfo.rva = (duint)sym.virtualAddress;
            symInfo.publicSymbol = sym.publicSymbol;
        }

        return true;
    };

    bool res = pdb.enumerateLexicalHierarchy(query);

    if(!res)
    {
        return false;
    }

    //handle symbol address sorting
    {
        _symAddrMap.resize(_symData.size());
        for(size_t i = 0; i < _symData.size(); i++)
        {
            AddrIndex addrIndex;
            addrIndex.addr = _symData[i].rva;
            addrIndex.index = i;
            _symAddrMap[i] = addrIndex;
        }
        std::sort(_symAddrMap.begin(), _symAddrMap.end(), [this](const AddrIndex & a, const AddrIndex & b)
        {
            // smaller
            if(a.addr < b.addr)
            {
                return true;
            }
            // bigger
            else if(a.addr > b.addr)
            {
                return false;
            }
            // equal
            else
            {
                // Check if we already have it inside, public symbols have priority over private symbols.
                return !_symData[a.index].publicSymbol < !_symData[b.index].publicSymbol;
            }
        });
    }

    //remove duplicate symbols from view
    if(!_symAddrMap.empty())
    {
        SymbolInfo* prev = nullptr;
        size_t insertPos = 0;
        for(size_t i = 0; i < _symAddrMap.size(); i++)
        {
            AddrIndex addrIndex = _symAddrMap[i];
            SymbolInfo & sym = _symData[addrIndex.index];
            if(prev && sym.rva == prev->rva && sym.decoratedName == prev->decoratedName && sym.undecoratedName == prev->undecoratedName)
            {
                sym.decoratedName.swap(String());
                sym.undecoratedName.swap(String());
                continue;
            }
            prev = &sym;

            _symAddrMap[insertPos++] = addrIndex;
        }
        _symAddrMap.resize(insertPos);
    }

    //handle symbol name sorting
    {
        _symNameMap.resize(_symAddrMap.size());
        for(size_t i = 0; i < _symAddrMap.size(); i++)
        {
            size_t symIndex = _symAddrMap[i].index;
            NameIndex nameIndex;
            nameIndex.name = _symData[symIndex].decoratedName.c_str(); //NOTE: DO NOT MODIFY decoratedName is any way!
            nameIndex.index = symIndex;
            _symNameMap[i] = nameIndex;
        }
        std::sort(_symNameMap.begin(), _symNameMap.end());
    }

    if(_requiresShutdown)
        return false;

    _symbolsLoaded = true;

    DWORD ms = GetTickCount() - loadStart;
    double secs = (double)ms / 1000.0;

    GuiSymbolLogAdd(StringUtils::sprintf("[%p, %s] Loaded %u symbols in %.03fs\n", _imageBase, _modname.c_str(), _symAddrMap.size(), secs).c_str());

    GuiInvalidateSymbolSource(_imageBase);

    GuiUpdateAllViews();

    return true;
}

bool SymbolSourceDIA::loadSourceLinesAsync()
{
    ScopedDecrement ref(_loadCounter);

    GuiRepaintTableView();

    PDBDiaFile pdb;

    if(!pdb.open(_path.c_str()))
    {
        return false;
    }

    DWORD lineLoadStart = GetTickCount();

    const uint32_t rangeSize = 1024 * 1024 * 10;

    std::vector<DiaLineInfo_t> lines;
    std::map<DWORD, String> files;

    if(!pdb.enumerateLineNumbers(0, uint32_t(_imageSize), lines, files, _requiresShutdown))
        return false;

    if(files.size() == 1)
    {
        GuiSymbolLogAdd(StringUtils::sprintf("[%p, %s] Since there is only one file, attempting line overflow detection (%ums)..\n", _imageBase, _modname.c_str(), GetTickCount() - lineLoadStart).c_str());

        // This is a super hack to adjust for the (undocumented) limit of 16777215 lines (unsigned 24 bits maximum).
        // It is unclear at this point if yasm/coff/link/pdb is causing this issue.
        // We can fix this because there is only a single source file and the returned result is sorted by *both* rva/line (IMPORTANT!).
        // For supporting multiple source files in the future we could need multiple 'lineOverflow' variables.
        uint32_t maxLine = 0, maxRva = 0, lineOverflows = 0;
        for(auto & line : lines)
        {
            if(_requiresShutdown)
                return false;

            uint32_t overflowValue = 0x1000000 * (lineOverflows + 1) - 1; //0xffffff, 0x1ffffff, 0x2ffffff, etc
            if((line.lineNumber & 0xfffff0) == 0 && (maxLine & 0xfffffff0) == (overflowValue & 0xfffffff0))  // allow 16 lines of play, perhaps there is a label/comment on line 0xffffff+1
            {
                GuiSymbolLogAdd(StringUtils::sprintf("[%p, %s] Line number overflow detected (%u -> %u), adjusting with hacks...\n", _imageBase, _modname.c_str(), maxLine, line.lineNumber).c_str());
                lineOverflows++;
            }

            line.lineNumber += lineOverflows * 0xffffff + lineOverflows;
            if(!(line.lineNumber > maxLine))
            {
                GuiSymbolLogAdd(StringUtils::sprintf("[%p, %s] The line information is not sorted by line (violated assumption)! lineNumber: %u, maxLine: %u\n", _imageBase, _modname.c_str(), line.lineNumber, maxLine).c_str());
            }
            maxLine = line.lineNumber;
            if(!(line.rva > maxRva))
            {
                GuiSymbolLogAdd(StringUtils::sprintf("[%p, %s] The line information is not sorted by rva (violated assumption)! rva: 0x%x, maxRva: 0x%x\n", _imageBase, _modname.c_str(), line.rva, maxRva).c_str());
            }
            maxRva = line.rva;
        }
    }

    _linesData.reserve(lines.size());
    _sourceFiles.reserve(files.size());
    _lineAddrMap.reserve(lines.size());

    struct SourceFileInfo
    {
        uint32_t sourceFileIndex;
        uint32_t lineCount;
    };
    std::map<DWORD, SourceFileInfo> sourceIdMap; //DIA sourceId -> sourceFileInfo
    for(const auto & line : lines)
    {
        if(_requiresShutdown)
            return false;

        _linesData.emplace_back();
        CachedLineInfo & lineInfo = _linesData.back();
        lineInfo.rva = line.rva;
        lineInfo.lineNumber = line.lineNumber;

        auto sourceFileId = line.sourceFileId;
        auto found = sourceIdMap.find(sourceFileId);
        if(found == sourceIdMap.end())
        {
            SourceFileInfo info;
            info.sourceFileIndex = uint32_t(_sourceFiles.size());
            info.lineCount = 0;
            _sourceFiles.push_back(files[sourceFileId]);
            found = sourceIdMap.insert({ sourceFileId, info }).first;
        }
        found->second.lineCount++;
        lineInfo.sourceFileIndex = found->second.sourceFileIndex;

        AddrIndex lineIndex;
        lineIndex.addr = lineInfo.rva;
        lineIndex.index = _linesData.size() - 1;
        _lineAddrMap.push_back(lineIndex);
    }

    // stable because first line encountered has to be found
    std::stable_sort(_lineAddrMap.begin(), _lineAddrMap.end());

    // perfectly allocate memory for line info vectors
    _sourceLines.resize(_sourceFiles.size());
    for(const auto & it : sourceIdMap)
        _sourceLines[it.second.sourceFileIndex].reserve(it.second.lineCount);

    // insert source line information
    for(size_t i = 0; i < _linesData.size(); i++)
    {
        auto & line = _linesData[i];
        LineIndex lineIndex;
        lineIndex.line = line.lineNumber;
        lineIndex.index = uint32_t(i);
        _sourceLines[line.sourceFileIndex].push_back(lineIndex);
    }

    // sort source line information
    for(size_t i = 0; i < _sourceLines.size(); i++)
    {
        auto & lineMap = _sourceLines[i];
        std::stable_sort(lineMap.begin(), lineMap.end());
    }

    if(_requiresShutdown)
        return false;

    _linesLoaded = true;

    DWORD ms = GetTickCount() - lineLoadStart;
    double secs = (double)ms / 1000.0;

    GuiSymbolLogAdd(StringUtils::sprintf("[%p, %s] Loaded %d line infos in %.03fs\n", _imageBase, _modname.c_str(), _linesData.size(), secs).c_str());

    GuiUpdateAllViews();

    return true;
}

uint32_t SymbolSourceDIA::findSourceFile(const std::string & fileName) const
{
    // Map the disk file name back to PDB file name
    std::string mappedFileName;
    if(!getSourceFileDiskToPdb(fileName, mappedFileName))
        mappedFileName = fileName;

    //TODO: make this fast
    uint32_t idx = -1;
    for(uint32_t n = 0; n < _sourceFiles.size(); n++)
    {
        const String & str = _sourceFiles[n];
        if(_stricmp(str.c_str(), mappedFileName.c_str()) == 0)
        {
            idx = n;
            break;
        }
    }
    return idx;
}

bool SymbolSourceDIA::findSymbolExact(duint rva, SymbolInfo & symInfo)
{
    if(!_symbolsLoaded)
        return false;

    if(SymbolSourceBase::isAddressInvalid(rva))
        return false;

    AddrIndex find;
    find.addr = rva;
    find.index = -1;
    auto it = binary_find(_symAddrMap.begin(), _symAddrMap.end(), find);

    if(it != _symAddrMap.end())
    {
        symInfo = _symData[it->index];
        return true;
    }

    if(isLoading() == false)
        markAdressInvalid(rva);

    return false;
}

bool SymbolSourceDIA::findSymbolExactOrLower(duint rva, SymbolInfo & symInfo)
{
    if(!_symbolsLoaded)
        return false;

    if(_symAddrMap.empty())
        return false;

    AddrIndex find;
    find.addr = rva;
    find.index = -1;
    auto it = [&]()
    {
        auto it = std::lower_bound(_symAddrMap.begin(), _symAddrMap.end(), find);
        // not found
        if(it == _symAddrMap.end())
            return --it;
        // exact match
        if(it->addr == rva)
            return it;
        // right now 'it' points to the first element bigger than rva
        return it == _symAddrMap.begin() ? _symAddrMap.end() : --it;
    }();

    if(it != _symAddrMap.end())
    {
        symInfo = _symData[it->index];
        symInfo.disp = (int32_t)(rva - symInfo.rva);
        return true;
    }

    return nullptr;
}

void SymbolSourceDIA::enumSymbols(const CbEnumSymbol & cbEnum)
{
    if(!_symbolsLoaded)
        return;

    for(auto & it : _symAddrMap)
    {
        const SymbolInfo & sym = _symData[it.index];
        if(!cbEnum(sym))
        {
            break;
        }
    }
}

bool SymbolSourceDIA::findSourceLineInfo(duint rva, LineInfo & lineInfo)
{
    if(!_linesLoaded)
        return false;

    AddrIndex find;
    find.addr = rva;
    find.index = -1;
    auto it = binary_find(_lineAddrMap.begin(), _lineAddrMap.end(), find);
    if(it == _lineAddrMap.end())
        return false;

    auto & cached = _linesData.at(it->index);
    lineInfo.rva = cached.rva;
    lineInfo.lineNumber = cached.lineNumber;
    lineInfo.disp = 0;
    lineInfo.size = 0;
    lineInfo.sourceFile = _sourceFiles[cached.sourceFileIndex];
    getSourceFilePdbToDisk(lineInfo.sourceFile, lineInfo.sourceFile);
    return true;
}

bool SymbolSourceDIA::findSourceLineInfo(const std::string & file, int line, LineInfo & lineInfo)
{
    if(!_linesLoaded)
        return false;

    auto sourceIdx = findSourceFile(file);
    if(sourceIdx == -1)
        return false;

    auto & lineMap = _sourceLines[sourceIdx];
    LineIndex find;
    find.line = line;
    find.index = -1;
    auto found = binary_find(lineMap.begin(), lineMap.end(), find);
    if(found == lineMap.end())
        return false;

    auto & cached = _linesData[found->index];
    lineInfo.rva = cached.rva;
    lineInfo.lineNumber = cached.lineNumber;
    lineInfo.disp = 0;
    lineInfo.size = 0;
    lineInfo.sourceFile = _sourceFiles[cached.sourceFileIndex];
    getSourceFilePdbToDisk(lineInfo.sourceFile, lineInfo.sourceFile);
    return true;
}

bool SymbolSourceDIA::findSymbolByName(const std::string & name, SymbolInfo & symInfo, bool caseSensitive)
{
    if(!_symbolsLoaded)
        return false;

    NameIndex found;
    if(!NameIndex::findByName(_symNameMap, name, found, caseSensitive))
        return false;
    symInfo = _symData[found.index];
    return true;
}

bool SymbolSourceDIA::findSymbolsByPrefix(const std::string & prefix, const std::function<bool(const SymbolInfo &)> & cbSymbol, bool caseSensitive)
{
    if(!_symbolsLoaded)
        return false;

    return NameIndex::findByPrefix(_symNameMap, prefix, [this, &cbSymbol](const NameIndex & index)
    {
        return cbSymbol(_symData[index.index]);
    }, caseSensitive);
}

std::string SymbolSourceDIA::loadedSymbolPath() const
{
    return _path;
}