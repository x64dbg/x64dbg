#include <comutil.h>
#include <windows.h>
#include <thread>
#include <atomic>
#include <algorithm>

#include "msdia/dia2.h"
#include "msdia/cvConst.h"
#include "msdia/diacreate.h"

#include "pdbdiafile.h"
#include "stringutils.h"

volatile LONG PDBDiaFile::m_sbInitialized = 0;

PDBDiaFile::PDBDiaFile() :
    m_dataSource(nullptr),
    m_session(nullptr)
{
}

PDBDiaFile::~PDBDiaFile()
{
}

bool PDBDiaFile::initLibrary()
{
    if(m_sbInitialized == 1)
        return true;

    LONG isInitialized = InterlockedCompareExchange(&m_sbInitialized, 1, 0);
    if(isInitialized != 0)
        return false;

    HRESULT hr = CoInitialize(nullptr);
#ifdef _DEBUG
    assert(SUCCEEDED(hr));
#endif
    return true;
}

bool PDBDiaFile::shutdownLibrary()
{
    LONG isInitialized = InterlockedCompareExchange(&m_sbInitialized, 0, 1);
    if(isInitialized != 1)
        return false;

    CoUninitialize();
    return true;
}

bool PDBDiaFile::open(const char* file, uint64_t loadAddress, DiaValidationData_t* validationData)
{
    wchar_t buf[1024];

    mbstowcs_s(nullptr, buf, file, 1024);

    return open(buf, loadAddress, validationData);
}

bool PDBDiaFile::open(const wchar_t* file, uint64_t loadAddress, DiaValidationData_t* validationData)
{
    wchar_t fileExt[MAX_PATH] = { 0 };
    wchar_t fileDir[MAX_PATH] = { 0 };

    HRESULT hr;
    hr = CoCreateInstance(__uuidof(DiaSource), NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (LPVOID*)&m_dataSource);
    if(testError(hr) || m_dataSource == nullptr)
    {
        if(hr == REGDB_E_CLASSNOTREG)
        {
            hr = NoRegCoCreate(L"msdia100.dll", __uuidof(DiaSource), __uuidof(IDiaDataSource), (LPVOID*)&m_dataSource);
            if(testError(hr))
            {
                hr = NoRegCoCreate(L"msdia90.dll", __uuidof(DiaSource), __uuidof(IDiaDataSource), (LPVOID*)&m_dataSource);
                if(testError(hr))
                {
                    hr = NoRegCoCreate(L"msdia80.dll", __uuidof(DiaSource), __uuidof(IDiaDataSource), (LPVOID*)&m_dataSource);
                    if(testError(hr))
                        return false;
                }
            }
        }
        else
        {
            printf("Unable to initialize PDBDia Library.\n");
            return false;
        }
    }

    _wsplitpath_s(file, NULL, 0, fileDir, MAX_PATH, NULL, 0, fileExt, MAX_PATH);

    if(_wcsicmp(fileExt, L".pdb") == 0)
    {
        if(validationData != nullptr)
        {
            hr = m_dataSource->loadAndValidateDataFromPdb(file, (GUID*)validationData->guid, validationData->signature, validationData->age);
            if((hr == E_PDB_INVALID_SIG) || (hr == E_PDB_INVALID_AGE))
            {
                printf("PDB is not matching.\n");
                return false;
            }
            else if(hr == E_PDB_FORMAT)
            {
                printf("PDB uses an obsolete format.\n");
                return false;
            }
        }
        else
        {
            hr = m_dataSource->loadDataFromPdb(file);
        }
    }
    else
    {
        hr = m_dataSource->loadDataForExe(file, fileDir, NULL);
    }

    if(testError(hr))
    {
        printf("Unable to open PDB file - %08X\n", hr);
        return false;
    }

    hr = m_dataSource->openSession(&m_session);
    if(testError(hr) || m_session == nullptr)
    {
        printf("Unable to create new PDBDia Session - %08X\n", hr);
        return false;
    }

    if(loadAddress != 0)
    {
        m_session->put_loadAddress(loadAddress);
    }

    return true;
}

bool PDBDiaFile::isOpen() const
{
    return m_session != nullptr && m_dataSource != nullptr;
}

bool PDBDiaFile::close()
{
    if(m_dataSource == nullptr)
        return false;
    if(m_session == nullptr)
        return false;

    m_session->Release();
    m_dataSource->Release();
    m_session = nullptr;
    m_dataSource = nullptr;
    m_symbols.clear();

    return true;
}

bool PDBDiaFile::testError(HRESULT hr)
{
    if(FAILED(hr))
    {
        return true;
    }
    return false;
}

bool PDBDiaFile::collectSymbols()
{
    std::deque<IDiaSymbol*> scopes;

    IDiaSymbol* scope = nullptr;
    HRESULT hr = m_session->get_globalScope(&scope);
    if(hr != S_OK)
        return false;

    scopes.push_back(scope);

    while(!scopes.empty())
    {
        IDiaSymbol* scope = scopes.front();
        scopes.pop_front();

        if(!collectSymbols(scope, &scopes))
        {
            scope->Release();
            return false;
        }

        scope->Release();
    }

    if(!collectSectionContribs())
        return false;

    std::sort(m_symbols.begin(), m_symbols.end(), [](DiaSymbol_t & a, DiaSymbol_t & b)->bool
    {
        return a.virtualAddress < b.virtualAddress;
    });

    return true;
}

bool PDBDiaFile::collectSymbols(IDiaSymbol* scope, std::deque<IDiaSymbol*>* scopes)
{
    static enum SymTagEnum enumTypes[] =
    {
        SymTagCompiland,
        SymTagData,
        SymTagFunction,
        SymTagPublicSymbol,
        SymTagLabel,
        SymTagThunk,
        //SymTagNull,
    };

    for(uint32_t i = 0; i < _countof(enumTypes); i++)
    {
        if(!collectSymbolsByTag(scope, enumTypes[i], scopes))
            return false;
    }

    return true;
}

bool PDBDiaFile::collectSymbolsByTag(IDiaSymbol* scope, enum SymTagEnum symTag, std::deque<IDiaSymbol*>* scopes)
{
    IDiaSymbol* symbol;
    IDiaEnumSymbols* enumSymbols;
    ULONG celt;
    LONG count;
    BOOL tempBool;
    DWORD actualSymTag;
    HRESULT hr;
    bool releaseSymbol;

    hr = scope->findChildren(symTag, nullptr, nsNone, &enumSymbols);
    if(hr != S_OK)
    {
        return true; // Doesn't have any childreen.
    }

    if(!enumSymbols)
    {
        return false;
    }

    hr = enumSymbols->get_Count(&count);
    if(hr == S_OK)
    {
        if(count == 0)
        {
            enumSymbols->Release();
            return true;
        }

        if(m_symbols.size() + count > m_symbols.capacity())
            m_symbols.reserve((m_symbols.capacity() + count) << 2);
    }

    while(enumSymbols->Next(1, &symbol, &celt) == S_OK && symbol != nullptr)
    {
        releaseSymbol = true;

        hr = symbol->get_symTag(&actualSymTag);
        if(hr != S_OK)
            return false;

#ifdef _DEBUG
        //assert(actualSymTag == symTag);
#endif

        DiaSymbol_t diaSymbol;
        diaSymbol.reachable = DiaReachableType::UNKNOWN;
        diaSymbol.returnable = DiaReturnableType::UNKNOWN;
        diaSymbol.convention = DiaCallingConvention::UNKNOWN;
        diaSymbol.perfectSize = false;
        diaSymbol.publicSymbol = true;

        switch(actualSymTag)
        {
        case SymTagCompiland:
        {
            releaseSymbol = false;
            scopes->emplace_back(symbol);
        }
        break;
        case SymTagPublicSymbol:
        {
            releaseSymbol = false;
            scopes->emplace_back(symbol);

            // By default we assume this is data.
            diaSymbol.type = DiaSymbolType::DATA;

            hr = symbol->get_virtualAddress(&diaSymbol.virtualAddress);
            if(hr != S_OK)
                break;

            hr = symbol->get_function(&tempBool);
            if(hr == S_OK && tempBool == TRUE)
                diaSymbol.type = DiaSymbolType::FUNCTION;

            hr = symbol->get_code(&tempBool);
            if(hr == S_OK && tempBool == TRUE && diaSymbol.type != DiaSymbolType::FUNCTION)
                diaSymbol.type = DiaSymbolType::CODE;

            hr = symbol->get_addressSection((DWORD*)&diaSymbol.segment);
            if(hr != S_OK)
                diaSymbol.segment = -1; // Unknown?

            hr = symbol->get_addressOffset((DWORD*)&diaSymbol.offset);
            if(hr != S_OK)
                diaSymbol.offset = -1; // Unknown?

            hr = symbol->get_length(&diaSymbol.size);
            if(hr != S_OK)
                diaSymbol.size = -1; // Unknown?

            // NOTE: This seems to be always failing, are these information simply missing?
            DWORD callingConvention = 0;
            hr = symbol->get_callingConvention(&callingConvention);
            if(hr == S_OK)
            {
                switch(callingConvention)
                {
                case CV_CALL_NEAR_C:
                    diaSymbol.convention = DiaCallingConvention::CALL_NEAR_C;
                    break;
                case CV_CALL_NEAR_FAST:
                    diaSymbol.convention = DiaCallingConvention::CALL_NEAR_FAST;
                    break;
                case CV_CALL_NEAR_STD:
                    diaSymbol.convention = DiaCallingConvention::CALL_NEAR_STD;
                    break;
                case CV_CALL_NEAR_SYS:
                    diaSymbol.convention = DiaCallingConvention::CALL_NEAR_SYS;
                    break;
                case CV_CALL_THISCALL:
                    diaSymbol.convention = DiaCallingConvention::CALL_THISCALL;
                    break;
                case CV_CALL_CLRCALL:
                    diaSymbol.convention = DiaCallingConvention::UNKNOWN;
                    break;
                }
            }

            diaSymbol.name = getSymbolNameString(symbol);

            m_symbols.emplace_back(diaSymbol);
        }
        break;
        case SymTagFunction:
        {
            releaseSymbol = false;
            scopes->push_back(symbol);

            diaSymbol.type = DiaSymbolType::FUNCTION;

            hr = symbol->get_virtualAddress(&diaSymbol.virtualAddress);
            if(hr != S_OK)
                break;

            hr = symbol->get_addressSection((DWORD*)&diaSymbol.segment);
            if(hr != S_OK)
                diaSymbol.segment = -1; // Unknown?

            hr = symbol->get_addressOffset((DWORD*)&diaSymbol.offset);
            if(hr != S_OK)
                diaSymbol.offset = -1; // Unknown?

            hr = symbol->get_length(&diaSymbol.size);
            if(hr != S_OK)
                diaSymbol.size = -1; // Unknown?
            else
                diaSymbol.perfectSize = true;

            hr = symbol->get_noReturn(&tempBool);
            if(hr == S_OK && tempBool == TRUE)
                diaSymbol.returnable = DiaReturnableType::NOTRETURNABLE;
            else if(hr == S_OK && tempBool == FALSE)
                diaSymbol.returnable = DiaReturnableType::RETURNABLE;

            hr = symbol->get_notReached(&tempBool);
            if(hr == S_OK && tempBool == TRUE)
                diaSymbol.reachable = DiaReachableType::NOTREACHABLE;
            else if(hr == S_OK && tempBool == FALSE)
                diaSymbol.reachable = DiaReachableType::REACHABLE;

            // NOTE: This seems to be always failing, are these information simply missing?
            DWORD callingConvention = 0;
            hr = symbol->get_callingConvention(&callingConvention);
            if(hr == S_OK)
            {
                switch(callingConvention)
                {
                case CV_CALL_NEAR_C:
                    diaSymbol.convention = DiaCallingConvention::CALL_NEAR_C;
                    break;
                case CV_CALL_NEAR_FAST:
                    diaSymbol.convention = DiaCallingConvention::CALL_NEAR_FAST;
                    break;
                case CV_CALL_NEAR_STD:
                    diaSymbol.convention = DiaCallingConvention::CALL_NEAR_STD;
                    break;
                case CV_CALL_NEAR_SYS:
                    diaSymbol.convention = DiaCallingConvention::CALL_NEAR_SYS;
                    break;
                case CV_CALL_THISCALL:
                    diaSymbol.convention = DiaCallingConvention::CALL_THISCALL;
                    break;
                case CV_CALL_CLRCALL:
                    diaSymbol.convention = DiaCallingConvention::UNKNOWN;
                    break;
                }
            }

            diaSymbol.name = getSymbolNameString(symbol);
            diaSymbol.publicSymbol = false;

            m_symbols.emplace_back(diaSymbol);
        }
        break;
        case SymTagData:
        {
            diaSymbol.type = DiaSymbolType::DATA;

            hr = symbol->get_addressSection((DWORD*)&diaSymbol.segment);
            if(hr != S_OK)
                diaSymbol.segment = -1; // Unknown?

            hr = symbol->get_addressOffset((DWORD*)&diaSymbol.offset);
            if(hr != S_OK)
                diaSymbol.offset = -1; // Unknown?

            hr = symbol->get_virtualAddress(&diaSymbol.virtualAddress);
            if(hr != S_OK)
                break;

            hr = symbol->get_length(&diaSymbol.size);
            if(hr != S_OK)
                diaSymbol.size = -1; // Unknown?
            else
                diaSymbol.perfectSize = true;

            if(diaSymbol.size == -1)
            {
                // Try over type.
                IDiaSymbol* symType = nullptr;

                hr = symbol->get_type(&symType);
                if(hr == S_OK && symType != nullptr)
                {

                    symType->Release();
                }
            }

            diaSymbol.name = getSymbolNameString(symbol);
            diaSymbol.publicSymbol = false;

            m_symbols.emplace_back(diaSymbol);
        }
        break;
        case SymTagLabel:
        {
            diaSymbol.type = DiaSymbolType::LABEL;

            diaSymbol.size = -1; // Labels don't have any size.
            diaSymbol.name = getSymbolNameString(symbol);
            diaSymbol.publicSymbol = false;

            hr = symbol->get_addressSection((DWORD*)&diaSymbol.segment);
            if(hr != S_OK)
                diaSymbol.segment = -1; // Unknown?

            hr = symbol->get_addressOffset((DWORD*)&diaSymbol.offset);
            if(hr != S_OK)
                diaSymbol.offset = -1; // Unknown?

            hr = symbol->get_virtualAddress(&diaSymbol.virtualAddress);
            if(hr != S_OK)
                break;

            m_symbols.emplace_back(diaSymbol);
        }
        break;
        }

        if(releaseSymbol)
            symbol->Release();
    }

    enumSymbols->Release();

    return true;
}

std::string PDBDiaFile::getSymbolNameString(IDiaSymbol* sym)
{
    HRESULT hr;
    BSTR str = nullptr;

    std::string name;
    std::string res;

    hr = sym->get_name(&str);
    if(hr != S_OK)
        return name;

    if(str != nullptr)
    {
        name = StringUtils::Utf16ToUtf8(str);
    }

    res = name;
    SysFreeString(str);

    size_t pos = res.find('(');
    if(pos != std::string::npos)
    {
        res = res.substr(0, pos);
    }

    return res;
}

std::string PDBDiaFile::getSymbolUndecoratedNameString(IDiaSymbol* sym)
{
    HRESULT hr;
    BSTR str = nullptr;
    std::string name;
    std::string result;

    hr = sym->get_undecoratedName(&str);
    if(hr != S_OK)
    {
        return name;
    }

    if(str != nullptr)
    {
        name = StringUtils::Utf16ToUtf8(str);
    }

    result = name;
    SysFreeString(str);

    return result;
}

HRESULT GetTable(IDiaSession* pSession, REFIID iid, void** ppUnk)
{
    IDiaEnumTables* pEnumTables;

    if(FAILED(pSession->getEnumTables(&pEnumTables)))
    {
        printf("GetTable() getEnumTables failed\n");
        return E_FAIL;
    }

    IDiaTable* pTable;
    ULONG celt = 0;

    while(SUCCEEDED(pEnumTables->Next(1, &pTable, &celt)) && (celt == 1))
    {
        // There's only one table that matches the given IID
        if(SUCCEEDED(pTable->QueryInterface(iid, (void**)ppUnk)))
        {
            pTable->Release();
            pEnumTables->Release();
            return S_OK;
        }

        pTable->Release();
    }

    pEnumTables->Release();

    return E_FAIL;
}

bool PDBDiaFile::collectSectionContribs()
{
    IDiaEnumSectionContribs* enumSectionContribs;

    if(FAILED(GetTable(m_session, __uuidof(IDiaEnumSectionContribs), (void**)&enumSectionContribs)))
        return false;

    IDiaSectionContrib* symbol;
    ULONG celt = 0;
    HRESULT hr;

    LONG enumCount = 0;
    hr = enumSectionContribs->get_Count(&enumCount);
    if(hr == S_OK)
    {
        if(m_symbols.size() + enumCount > m_symbols.capacity())
            m_symbols.reserve((m_symbols.size() + enumCount) << 2);
    }

    while(SUCCEEDED(enumSectionContribs->Next(1, &symbol, &celt)) && celt == 1)
    {
        DiaSymbol_t diaSymbol;
        diaSymbol.type = DiaSymbolType::SECTIONCONTRIB;
        diaSymbol.perfectSize = false;
        diaSymbol.reachable = DiaReachableType::UNKNOWN;
        diaSymbol.returnable = DiaReturnableType::UNKNOWN;
        diaSymbol.convention = DiaCallingConvention::UNKNOWN;

        hr = symbol->get_addressSection((DWORD*)&diaSymbol.segment);
        if(hr != S_OK)
            diaSymbol.segment = -1;

        hr = symbol->get_addressOffset((DWORD*)&diaSymbol.offset);
        if(hr != S_OK)
            diaSymbol.offset = -1;

        hr = symbol->get_virtualAddress(&diaSymbol.virtualAddress);
        if(hr != S_OK)
        {
            symbol->Release();
            continue;
        }

        DWORD len = 0;
        hr = symbol->get_length(&len);
        if(hr != S_OK)
            diaSymbol.size = -1;
        else
            diaSymbol.size = len;

        m_symbols.emplace_back(diaSymbol);

        symbol->Release();
    }

    enumSectionContribs->Release();

    return true;
}

const std::vector<DiaSymbol_t> & PDBDiaFile::getSymbols()
{
    return m_symbols;
}

bool PDBDiaFile::getFunctionLineNumbers(DWORD segment, DWORD offset, ULONGLONG size, uint64_t imageBase, std::map<uint64_t, LineInfo_t> & lines)
{
    HRESULT hr;
    IDiaEnumLineNumbers* lineNumbers = nullptr;
    IDiaLineNumber* lineNumberInfo = nullptr;
    IDiaSourceFile* sourceFile = nullptr;
    ULONG fetched = 0;
    DWORD lineNumber = 0;
    DWORD relativeVirtualAddress = 0;
    DWORD lineNumberEnd = 0;

    if(segment != 0 && offset != 0 && size > 0)
    {
        hr = m_session->findLinesByAddr(segment, offset, static_cast<DWORD>(size), &lineNumbers);
        if(!SUCCEEDED(hr))
            return false;

        LONG lineCount = 0;
        hr = lineNumbers->get_Count(&lineCount);
        if(!SUCCEEDED(hr))
            return false;

        for(LONG n = 0; n < lineCount; n++)
        {
            hr = lineNumbers->Item(n, &lineNumberInfo);
            if(!SUCCEEDED(hr))
                continue;

            hr = lineNumberInfo->get_sourceFile(&sourceFile);
            if(!SUCCEEDED(hr))
                continue;

            hr = lineNumberInfo->get_lineNumber(&lineNumber);
            if(!SUCCEEDED(hr))
                continue;

            hr = lineNumberInfo->get_relativeVirtualAddress(&relativeVirtualAddress);
            if(!SUCCEEDED(hr))
                continue;

            hr = lineNumberInfo->get_lineNumberEnd(&lineNumberEnd);
            if(!SUCCEEDED(hr))
                continue;

            BSTR fileName = nullptr;
            hr = sourceFile->get_fileName(&fileName);
            if(!SUCCEEDED(hr))
                continue;

            LineInfo_t lineInfo;
            lineInfo.fileName = StringUtils::Utf16ToUtf8(fileName);
            lineInfo.lineNumber = lineNumber;
            lineInfo.offset = 0;
            lineInfo.segment = segment;
            lineInfo.virtualAddress = imageBase + relativeVirtualAddress;

            lines.emplace(lineInfo.virtualAddress, lineInfo);

            sourceFile->Release();
            lineNumberInfo->Release();
        }

        lineNumbers->Release();
    }

    return true;
}

uint32_t getSymbolId(IDiaSymbol* sym)
{
    DWORD id;
    sym->get_symIndexId(&id);
    return id;
}

bool PDBDiaFile::enumerateLexicalHierarchy(std::function<bool(DiaSymbol_t &)> callback, const bool collectUndecoratedNames)
{
    IDiaEnumSymbols* enumSymbols = nullptr;
    IDiaSymbol* globalScope = nullptr;
    IDiaSymbol* symbol = nullptr;
    ULONG celt = 0;
    HRESULT hr;
    DiaSymbol_t symbolInfo;
    bool res = true;

    hr = m_session->get_globalScope(&globalScope);
    if(hr != S_OK)
        return false;

    std::unordered_set<uint32_t> visited;

    uint32_t scopeId = getSymbolId(globalScope);
    visited.insert(scopeId);

    // Enumerate compilands.
    hr = globalScope->findChildren(SymTagCompiland, nullptr, nsNone, &enumSymbols);
    if(hr == S_OK)
    {
        while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
        {
            if(!enumerateCompilandScope(symbol, callback, visited, collectUndecoratedNames))
            {
                res = false;
            }

            symbol->Release();
        }
        enumSymbols->Release();
    }
    if(!res)
        return res;

    // Enumerate publics.
    hr = globalScope->findChildren(SymTagPublicSymbol, nullptr, nsNone, &enumSymbols);
    if(hr == S_OK)
    {
        while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
        {
            if(convertSymbolInfo(symbol, symbolInfo, collectUndecoratedNames))
            {
                if(!callback(symbolInfo))
                    res = false;
            }
            symbol->Release();
        }
        enumSymbols->Release();
    }

    if(!res)
        return res;

    // Enumerate global functions.
    hr = globalScope->findChildren(SymTagFunction, nullptr, nsNone, &enumSymbols);
    if(hr == S_OK)
    {
        while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
        {
            if(convertSymbolInfo(symbol, symbolInfo, collectUndecoratedNames))
            {
                if(!callback(symbolInfo))
                    res = false;
            }
            symbol->Release();
        }
        enumSymbols->Release();
    }

    if(!res)
        return res;

    // Enumerate global data.
    hr = globalScope->findChildren(SymTagData, nullptr, nsNone, &enumSymbols);
    if(hr == S_OK)
    {
        while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
        {
            if(convertSymbolInfo(symbol, symbolInfo, collectUndecoratedNames))
            {
                if(!callback(symbolInfo))
                    res = false;
            }
            symbol->Release();
        }
        enumSymbols->Release();
    }

    globalScope->Release();

    return res;
}

bool PDBDiaFile::findSymbolRVA(uint64_t address, DiaSymbol_t & sym, DiaSymbolType symType /*= DiaSymbolType::ANY*/)
{
    if(m_session == nullptr || m_dataSource == nullptr)
        return false;

    IDiaEnumSymbols* enumSymbols = nullptr;
    IDiaSymbol* globalScope = nullptr;
    IDiaSymbol* symbol = nullptr;
    ULONG celt = 0;
    HRESULT hr;
    enum SymTagEnum tag = SymTagNull;

    switch(symType)
    {
    case DiaSymbolType::BLOCK:
        tag = SymTagBlock;
        break;
    case DiaSymbolType::FUNCTION:
        tag = SymTagFunction;
        break;
    case DiaSymbolType::LABEL:
        tag = SymTagLabel;
        break;
    case DiaSymbolType::PUBLIC:
        tag = SymTagPublicSymbol;
        break;
    }

    bool res = false;

    long disp = 0;
    hr = m_session->findSymbolByRVAEx(address, tag, &symbol, &disp);
    if(hr == S_OK)
    {
        sym.disp = disp;

        if(!convertSymbolInfo(symbol, sym, true))
            res = false;
        else
            res = true;

        symbol->Release();
    }

    return res;
}

bool PDBDiaFile::enumerateCompilandScope(IDiaSymbol* compiland, std::function<bool(DiaSymbol_t &)> & callback, std::unordered_set<uint32_t> & visited, const bool collectUndecoratedNames)
{
    IDiaEnumSymbols* enumSymbols = nullptr;
    IDiaSymbol* symbol = nullptr;
    bool res = true;
    ULONG celt = 0;
    HRESULT hr;
    DWORD symTagType;

    DiaSymbol_t symbolInfo;

    uint32_t symId = getSymbolId(compiland);

    hr = compiland->findChildren(SymTagFunction, nullptr, nsNone, &enumSymbols);
    if(hr == S_OK)
    {
        while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
        {
            hr = symbol->get_symTag(&symTagType);

            if(hr == S_OK)
            {
                if(!processFunctionSymbol(symbol, callback, visited, collectUndecoratedNames))
                {
                    res = false;
                }
            }

            symbol->Release();
        }
        enumSymbols->Release();
    }
    if(!res)
        return res;

    hr = compiland->findChildren(SymTagData, nullptr, nsNone, &enumSymbols);
    if(hr == S_OK)
    {
        while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
        {
            hr = symbol->get_symTag(&symTagType);

            if(hr == S_OK)
            {
                if(convertSymbolInfo(symbol, symbolInfo, collectUndecoratedNames))
                {
                    if(!callback(symbolInfo))
                        res = false;
                }
            }

            symbol->Release();
        }
        enumSymbols->Release();
    }
    if(!res)
        return res;

    hr = compiland->findChildren(SymTagBlock, nullptr, nsNone, &enumSymbols);
    if(hr == S_OK)
    {
        while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
        {
            hr = symbol->get_symTag(&symTagType);

            if(hr == S_OK)
            {
                if(convertSymbolInfo(symbol, symbolInfo, collectUndecoratedNames))
                {
                    if(!callback(symbolInfo))
                        res = false;
                }
            }

            symbol->Release();
        }
        enumSymbols->Release();
    }
    if(!res)
        return res;

    hr = compiland->findChildren(SymTagLabel, nullptr, nsNone, &enumSymbols);
    if(hr == S_OK)
    {
        while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
        {
            hr = symbol->get_symTag(&symTagType);

            if(hr == S_OK)
            {
                if(convertSymbolInfo(symbol, symbolInfo, collectUndecoratedNames))
                {
                    if(!callback(symbolInfo))
                        res = false;
                }
            }

            symbol->Release();
        }
        enumSymbols->Release();
    }

    return res;
}

bool PDBDiaFile::processFunctionSymbol(IDiaSymbol* functionSym, std::function<bool(DiaSymbol_t &)> & callback, std::unordered_set<uint32_t> & visited, const bool collectUndecoratedNames /*= false*/)
{
    IDiaEnumSymbols* enumSymbols = nullptr;
    IDiaSymbol* symbol = nullptr;
    ULONG celt = 0;
    HRESULT hr;
    DWORD symTagType;
    bool res = true;

    uint32_t symId = getSymbolId(functionSym);
    if(visited.find(symId) != visited.end())
    {
        printf("Dupe\n");
        return true;
    }

    visited.insert(symId);

    DiaSymbol_t symbolInfo;
    if(convertSymbolInfo(functionSym, symbolInfo, collectUndecoratedNames))
    {
        if(!callback(symbolInfo))
            return false;
    }

    hr = functionSym->findChildren(SymTagData, nullptr, nsNone, &enumSymbols);
    if(hr == S_OK)
    {
        while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
        {
            hr = symbol->get_symTag(&symTagType);

            LocationType locType;
            symbol->get_locationType((DWORD*)&locType);

            if(hr == S_OK && locType == LocIsStatic)
            {
                if(convertSymbolInfo(symbol, symbolInfo, collectUndecoratedNames))
                {
                    if(!callback(symbolInfo))
                        res = false;
                }
            }

            symbol->Release();
        }
        enumSymbols->Release();
    }
    if(!res)
        return res;

    hr = functionSym->findChildren(SymTagBlock, nullptr, nsNone, &enumSymbols);
    if(hr == S_OK)
    {
        while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
        {
            hr = symbol->get_symTag(&symTagType);

            if(hr == S_OK)
            {
                if(convertSymbolInfo(symbol, symbolInfo, collectUndecoratedNames))
                {
                    if(!callback(symbolInfo))
                        res = false;
                }
            }

            symbol->Release();
        }
        enumSymbols->Release();
    }

    hr = functionSym->findChildren(SymTagLabel, nullptr, nsNone, &enumSymbols);
    if(hr == S_OK)
    {
        while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
        {
            hr = symbol->get_symTag(&symTagType);

            if(hr == S_OK)
            {
                if(convertSymbolInfo(symbol, symbolInfo, collectUndecoratedNames))
                {
                    if(!callback(symbolInfo))
                        res = false;
                }
            }

            symbol->Release();
        }
        enumSymbols->Release();
    }

    return res;
}

bool PDBDiaFile::resolveSymbolSize(IDiaSymbol* symbol, uint64_t & size, uint32_t symTag)
{
    bool res = true;

    IDiaSymbol* symType = nullptr;
    HRESULT hr;
    uint64_t tempSize = -1;

    if(symTag == SymTagData)
    {
        hr = symbol->get_type(&symType);
        if(hr == S_OK && symType != nullptr)
        {
            DWORD symTagType = 0;
            hr = symType->get_symTag(&symTagType);

            switch(symTagType)
            {
            case SymTagFunctionType:
            {
                hr = symbol->get_length(&tempSize);
                if(hr == S_OK)
                {
                    size = tempSize;
                }
                else
                    res = false;
            }
            break;
            case SymTagPointerType:
            case SymTagArrayType:
            case SymTagUDT:
            {
                hr = symType->get_length(&tempSize);
                if(hr == S_OK)
                    size = tempSize;
                else
                    res = false;
            }
            break;
            case SymTagNull:
            {
                hr = symType->get_length(&tempSize);
                if(hr == S_OK)
                {
                    size = tempSize;
                }
                else
                {
                    hr = symbol->get_length(&tempSize);
                    if(hr == S_OK)
                        size = tempSize;
                    else
                        res = false;
                }
            }
            break;
            default:
            {
                // Native type.
                hr = symType->get_length(&tempSize);
                if(hr == S_OK)
                    size = tempSize;
                else
                    res = false;
            }
            break;
            }

            symType->Release();
        }

        // One last attempt.
        if(res == false || size == 0 || size == -1)
        {
            hr = symbol->get_length(&tempSize);
            if(hr == S_OK)
            {
                size = tempSize;
                res = true;
            }
            else
                res = false;
        }

    }
    else
    {
        hr = symbol->get_length(&tempSize);
        if(hr == S_OK)
        {
            size = tempSize;
        }
        else
            res = false;
    }

    return res;
}

bool PDBDiaFile::convertSymbolInfo(IDiaSymbol* symbol, DiaSymbol_t & symbolInfo, const bool collectUndecoratedNames)
{
    HRESULT hr;
    DWORD symTagType;

    // Default all values.
    symbolInfo.reachable = DiaReachableType::UNKNOWN;
    symbolInfo.returnable = DiaReturnableType::UNKNOWN;
    symbolInfo.convention = DiaCallingConvention::UNKNOWN;
    symbolInfo.size = -1;
    symbolInfo.offset = -1;
    symbolInfo.segment = -1;
    symbolInfo.disp = 0;
    symbolInfo.virtualAddress = -1;
    symbolInfo.perfectSize = false;
    symbolInfo.publicSymbol = false;

    hr = symbol->get_symTag(&symTagType);
    if(hr != S_OK)
        return false;

    symbolInfo.name = getSymbolNameString(symbol);

    if(collectUndecoratedNames)
    {
        symbolInfo.undecoratedName = getSymbolUndecoratedNameString(symbol);
    }

    hr = symbol->get_addressSection((DWORD*)&symbolInfo.segment);
    if(hr != S_OK)
    {
        return false;
    }

    hr = symbol->get_addressOffset((DWORD*)&symbolInfo.offset);
    if(hr != S_OK)
    {
        return false;
    }

    hr = symbol->get_virtualAddress(&symbolInfo.virtualAddress);
    if(hr != S_OK)
    {
        // NOTE: At this point we could still lookup the address over the executable.
        return false;
    }

    if(symbolInfo.virtualAddress == symbolInfo.offset)
    {
        return false;
    }

    symbolInfo.size = 0;

    if(!resolveSymbolSize(symbol, symbolInfo.size, symTagType) || symbolInfo.size == 0)
    {
        symbolInfo.size = -1;
    }

    switch(symTagType)
    {
    case SymTagPublicSymbol:
        symbolInfo.type = DiaSymbolType::LABEL;
        break;
    case SymTagFunction:
        symbolInfo.type = DiaSymbolType::FUNCTION;
        break;
    case SymTagData:
        symbolInfo.type = DiaSymbolType::DATA;
        break;
    case SymTagLabel:
        symbolInfo.type = DiaSymbolType::LABEL;
        break;
    case SymTagBlock:
        symbolInfo.type = DiaSymbolType::BLOCK;
        break;
    }

    return true;

}

