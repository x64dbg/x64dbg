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

template<typename T>
class ScopedDiaType
{
private:
    T* _sym;
public:
    ScopedDiaType() : _sym(nullptr) {}
    ScopedDiaType(T* sym) : _sym(sym) {}
    ~ScopedDiaType()
    {
        if(_sym != nullptr)
        {
            _sym->Release();
        }
    }
    T** ref() { return &_sym; }
    T* operator->()
    {
        return _sym;
    }
    operator T* ()
    {
        return _sym;
    }
};

typedef ScopedDiaType<IDiaSymbol> ScopedDiaSymbol;
typedef ScopedDiaType<IDiaEnumSymbols> ScopedDiaEnumSymbols;

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
    ScopedDiaSymbol globalScope;
    IDiaSymbol* symbol = nullptr;
    ULONG celt = 0;
    HRESULT hr;
    DiaSymbol_t symbolInfo;
    bool res = true;

    hr = m_session->get_globalScope(globalScope.ref());
    if(hr != S_OK)
        return false;

    std::unordered_set<uint32_t> visited;

    uint32_t scopeId = getSymbolId(globalScope);
    visited.insert(scopeId);

    // Enumerate compilands.
    {
        ScopedDiaEnumSymbols enumSymbols;

        hr = globalScope->findChildren(SymTagCompiland, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);
                if(!enumerateCompilandScope(sym, callback, visited, collectUndecoratedNames))
                {
                    return false;
                }
            }
        }
    }

    // Enumerate publics.
    {
        ScopedDiaEnumSymbols enumSymbols;

        hr = globalScope->findChildren(SymTagPublicSymbol, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                if(convertSymbolInfo(symbol, symbolInfo, collectUndecoratedNames))
                {
                    ScopedDiaSymbol sym(symbol);
                    if(!callback(symbolInfo))
                    {
                        return false;
                    }
                }
            }
        }
    }

    // Enumerate global functions.
    {
        ScopedDiaEnumSymbols enumSymbols;

        hr = globalScope->findChildren(SymTagFunction, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);
                if(convertSymbolInfo(sym, symbolInfo, collectUndecoratedNames))
                {
                    if(!callback(symbolInfo))
                    {
                        return false;
                    }
                }
            }
        }
    }

    // Enumerate global data.
    {
        ScopedDiaEnumSymbols enumSymbols;

        hr = globalScope->findChildren(SymTagData, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);
                if(convertSymbolInfo(sym, symbolInfo, collectUndecoratedNames))
                {
                    if(!callback(symbolInfo))
                    {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

bool PDBDiaFile::findSymbolRVA(uint64_t address, DiaSymbol_t & sym, DiaSymbolType symType /*= DiaSymbolType::ANY*/)
{
    if(m_session == nullptr || m_dataSource == nullptr)
        return false;

    IDiaEnumSymbols* enumSymbols = nullptr;
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
        ScopedDiaSymbol scopedSym(symbol);

        sym.disp = disp;

        if(!convertSymbolInfo(scopedSym, sym, true))
            res = false;
        else
            res = true;
    }

    return res;
}

bool PDBDiaFile::enumerateCompilandScope(IDiaSymbol* compiland, std::function<bool(DiaSymbol_t &)> & callback, std::unordered_set<uint32_t> & visited, const bool collectUndecoratedNames)
{
    IDiaSymbol* symbol = nullptr;
    bool res = true;
    ULONG celt = 0;
    HRESULT hr;
    DWORD symTagType;

    DiaSymbol_t symbolInfo;

    uint32_t symId = getSymbolId(compiland);

    {
        ScopedDiaEnumSymbols enumSymbols;

        hr = compiland->findChildren(SymTagFunction, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);

                hr = sym->get_symTag(&symTagType);

                if(hr == S_OK)
                {
                    if(!processFunctionSymbol(sym, callback, visited, collectUndecoratedNames))
                    {
                        return false;
                    }
                }

            }
        }
    }

    {
        ScopedDiaEnumSymbols enumSymbols;

        hr = compiland->findChildren(SymTagData, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);

                hr = sym->get_symTag(&symTagType);

                if(hr == S_OK)
                {
                    if(convertSymbolInfo(sym, symbolInfo, collectUndecoratedNames))
                    {
                        if(!callback(symbolInfo))
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }

    {
        ScopedDiaEnumSymbols enumSymbols;

        hr = compiland->findChildren(SymTagBlock, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);

                hr = sym->get_symTag(&symTagType);

                if(hr == S_OK)
                {
                    if(convertSymbolInfo(sym, symbolInfo, collectUndecoratedNames))
                    {
                        if(!callback(symbolInfo))
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }

    {
        ScopedDiaEnumSymbols enumSymbols;

        hr = compiland->findChildren(SymTagLabel, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);

                hr = sym->get_symTag(&symTagType);

                if(hr == S_OK)
                {
                    if(convertSymbolInfo(sym, symbolInfo, collectUndecoratedNames))
                    {
                        if(!callback(symbolInfo))
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool PDBDiaFile::processFunctionSymbol(IDiaSymbol* functionSym, std::function<bool(DiaSymbol_t &)> & callback, std::unordered_set<uint32_t> & visited, const bool collectUndecoratedNames /*= false*/)
{
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

    {
        ScopedDiaEnumSymbols enumSymbols;
        hr = functionSym->findChildren(SymTagData, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);

                hr = sym->get_symTag(&symTagType);

                LocationType locType;
                sym->get_locationType((DWORD*)&locType);

                if(hr == S_OK && locType == LocIsStatic)
                {
                    if(convertSymbolInfo(sym, symbolInfo, collectUndecoratedNames))
                    {
                        if(!callback(symbolInfo))
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }

    {
        ScopedDiaEnumSymbols enumSymbols;
        hr = functionSym->findChildren(SymTagBlock, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);

                hr = sym->get_symTag(&symTagType);

                if(hr == S_OK)
                {
                    if(convertSymbolInfo(sym, symbolInfo, collectUndecoratedNames))
                    {
                        if(!callback(symbolInfo))
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }

    {
        ScopedDiaEnumSymbols enumSymbols;
        hr = functionSym->findChildren(SymTagLabel, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while(res == true && (hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);

                hr = sym->get_symTag(&symTagType);

                if(hr == S_OK)
                {
                    if(convertSymbolInfo(sym, symbolInfo, collectUndecoratedNames))
                    {
                        if(!callback(symbolInfo))
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool PDBDiaFile::resolveSymbolSize(IDiaSymbol* symbol, uint64_t & size, uint32_t symTag)
{
    bool res = true;

    HRESULT hr;
    uint64_t tempSize = -1;

    if(symTag == SymTagData)
    {
        ScopedDiaSymbol symType;
        hr = symbol->get_type(symType.ref());

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
        symbolInfo.type = DiaSymbolType::PUBLIC;
        symbolInfo.publicSymbol = true;
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

