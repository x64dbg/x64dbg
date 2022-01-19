#include "_global.h"
#include <comutil.h>
#include <windows.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>

#include "msdia/dia2.h"
#include "msdia/cvConst.h"
#include "msdia/diacreate.h"

#include "pdbdiafile.h"
#include "stringutils.h"
#include "console.h"
#include "symbolundecorator.h"

//Taken from: https://msdn.microsoft.com/en-us/library/ms752876(v=vs.85).aspx
class FileStream : public IStream
{
    FileStream(HANDLE hFile)
    {
        AddRef();
        _hFile = hFile;
    }

    ~FileStream()
    {
        if(_hFile != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(_hFile);
        }
    }

public:
    HRESULT static OpenFile(LPCWSTR pName, IStream** ppStream, bool fWrite)
    {
        HANDLE hFile = ::CreateFileW(pName, fWrite ? GENERIC_WRITE : GENERIC_READ, FILE_SHARE_READ,
                                     NULL, fWrite ? CREATE_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if(hFile == INVALID_HANDLE_VALUE)
            return HRESULT_FROM_WIN32(GetLastError());

        *ppStream = new FileStream(hFile);

        if(*ppStream == NULL)
            CloseHandle(hFile);

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppvObject)
    {
        if(iid == __uuidof(IUnknown)
                || iid == __uuidof(IStream)
                || iid == __uuidof(ISequentialStream))
        {
            *ppvObject = static_cast<IStream*>(this);
            AddRef();
            return S_OK;
        }
        else
            return E_NOINTERFACE;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return (ULONG)InterlockedIncrement(&_refcount);
    }

    virtual ULONG STDMETHODCALLTYPE Release(void)
    {
        ULONG res = (ULONG)InterlockedDecrement(&_refcount);
        if(res == 0)
            delete this;
        return res;
    }

    // ISequentialStream Interface
public:
    virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead)
    {
        BOOL rc = ReadFile(_hFile, pv, cb, pcbRead, NULL);
        return (rc) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }

    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten)
    {
        BOOL rc = WriteFile(_hFile, pv, cb, pcbWritten, NULL);
        return rc ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }

    // IStream Interface
public:
    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE Revert(void) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE Clone(IStream**) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin, ULARGE_INTEGER* lpNewFilePointer)
    {
        DWORD dwMoveMethod;

        switch(dwOrigin)
        {
        case STREAM_SEEK_SET:
            dwMoveMethod = FILE_BEGIN;
            break;
        case STREAM_SEEK_CUR:
            dwMoveMethod = FILE_CURRENT;
            break;
        case STREAM_SEEK_END:
            dwMoveMethod = FILE_END;
            break;
        default:
            return STG_E_INVALIDFUNCTION;
            break;
        }

        if(SetFilePointerEx(_hFile, liDistanceToMove, (PLARGE_INTEGER)lpNewFilePointer,
                            dwMoveMethod) == 0)
            return HRESULT_FROM_WIN32(GetLastError());
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag)
    {
        if(GetFileSizeEx(_hFile, (PLARGE_INTEGER)&pStatstg->cbSize) == 0)
            return HRESULT_FROM_WIN32(GetLastError());
        return S_OK;
    }

private:
    HANDLE _hFile;
    LONG _refcount = 0;
};

template<typename T>
class ScopedDiaType
{
private:
    T* _sym;
public:
    ScopedDiaType() : _sym(nullptr) {}
    ScopedDiaType(T* sym) : _sym(sym) {}
    ~ScopedDiaType() { if(_sym != nullptr) _sym->Release(); }
    T** ref() { return &_sym; }
    T** operator&() { return ref(); }
    T* operator->() { return _sym; }
    operator T* () { return _sym; }
    void Attach(T* sym) { _sym = sym; }
};

template<typename T>
using CComPtr = ScopedDiaType<T>;

PDBDiaFile::PDBDiaFile() :
    m_stream(nullptr),
    m_dataSource(nullptr),
    m_session(nullptr)
{
}

PDBDiaFile::~PDBDiaFile()
{
    if(isOpen())
        close();
}

bool PDBDiaFile::initLibrary()
{
    return SUCCEEDED(CoInitialize(nullptr));
}

bool PDBDiaFile::shutdownLibrary()
{
    CoUninitialize();
    return true;
}

bool PDBDiaFile::open(const char* file, uint64_t loadAddress, DiaValidationData_t* validationData)
{
    return open(StringUtils::Utf8ToUtf16(file).c_str(), loadAddress, validationData);
}

bool PDBDiaFile::open(const wchar_t* file, uint64_t loadAddress, DiaValidationData_t* validationData)
{
    if(!initLibrary())
        return false;

    if(isOpen())
    {
#if 1 // Enable for validation purpose.
        __debugbreak();
#endif
        return false;
    }

    wchar_t fileExt[MAX_PATH] = { 0 };
    wchar_t fileDir[MAX_PATH] = { 0 };

    HRESULT hr = REGDB_E_CLASSNOTREG;
    hr = NoRegCoCreate(L"msdia140.dll", __uuidof(DiaSourceAlt), __uuidof(IDiaDataSource), (LPVOID*)&m_dataSource);
    if(testError(hr) || m_dataSource == nullptr)
    {
        hr = CoCreateInstance(__uuidof(DiaSourceAlt), NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (LPVOID*)&m_dataSource);
    }
    if(testError(hr) || m_dataSource == nullptr)
    {
        GuiSymbolLogAdd("Unable to initialize PDBDia Library.\n");
        return false;
    }

    _wsplitpath_s(file, NULL, 0, fileDir, MAX_PATH, NULL, 0, fileExt, MAX_PATH);

    if(_wcsicmp(fileExt, L".pdb") == 0)
    {
        hr = FileStream::OpenFile(file, &m_stream, false);
        if(testError(hr))
        {
            GuiSymbolLogAdd("Unable to open PDB file.\n");
            return false;
        }
        /*std::vector<unsigned char> pdbData;
        if (!FileHelper::ReadAllData(StringUtils::Utf16ToUtf8(file), pdbData))
        {
            GuiSymbolLogAdd("Unable to open PDB file.\n");
            return false;
        }
        m_stream = SHCreateMemStream(pdbData.data(), pdbData.size());*/

        if(validationData != nullptr)
        {
            hr = m_dataSource->loadDataFromIStream(m_stream);
            if(hr == E_PDB_FORMAT)
            {
                GuiSymbolLogAdd("PDB uses an obsolete format.\n");
                return false;
            }
        }
        else
        {
            hr = m_dataSource->loadDataFromIStream(m_stream);
        }
    }
    else
    {
        // NOTE: Unsupported use with IStream.
        hr = m_dataSource->loadDataForExe(file, fileDir, nullptr);
    }

    if(testError(hr))
    {
        if(hr != E_PDB_NOT_FOUND)
        {
            GuiSymbolLogAdd(StringUtils::sprintf("Unable to open PDB file - %08X\n", hr).c_str());
        }
        else
        {
            GuiSymbolLogAdd(StringUtils::sprintf("Unknown error - %08X\n", hr).c_str());
        }
        return false;
    }

    hr = m_dataSource->openSession(&m_session);
    if(testError(hr) || m_session == nullptr)
    {
        GuiSymbolLogAdd(StringUtils::sprintf("Unable to create new PDBDia Session - %08X\n", hr).c_str());
        return false;
    }

    if(validationData != nullptr)
    {
        IDiaSymbol* globalSym = nullptr;
        hr = m_session->get_globalScope(&globalSym);
        if(testError(hr))
        {
            //??
        }
        else
        {
            DWORD age = 0;
            hr = globalSym->get_age(&age);
            if(!testError(hr) && validationData->age != age)
            {
                globalSym->Release();
                close();

                GuiSymbolLogAdd(StringUtils::sprintf("Validation error: PDB age is not matching (expected: %u, actual: %u).\n", validationData->age, age).c_str());
                return false;
            }

            if(validationData->signature != 0)
            {
                // PDB v2.0 ('NB10' ones) do not have a GUID and they use a signature and age
                DWORD signature = 0;
                hr = globalSym->get_signature(&signature);
                if(!testError(hr) && validationData->signature != signature)
                {
                    globalSym->Release();
                    close();

                    GuiSymbolLogAdd(StringUtils::sprintf("Validation error: PDB signature is not matching (expected: %08X, actual: %08X).\n",
                                                         signature, validationData->signature).c_str());
                    return false;
                }
            }
            else
            {
                // v7.0 PDBs should be checked using (age+guid) only
                GUID guid = { 0 };
                hr = globalSym->get_guid(&guid);
                if(!testError(hr) && memcmp(&guid, &validationData->guid, sizeof(GUID)) != 0)
                {
                    globalSym->Release();
                    close();

                    auto guidStr = [](const GUID & guid) -> String
                    {
                        // https://stackoverflow.com/a/22848342/1806760
                        char guid_string[37]; // 32 hex chars + 4 hyphens + null terminator
                        sprintf_s(guid_string,
                        "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                        guid.Data1, guid.Data2, guid.Data3,
                        guid.Data4[0], guid.Data4[1], guid.Data4[2],
                        guid.Data4[3], guid.Data4[4], guid.Data4[5],
                        guid.Data4[6], guid.Data4[7]);
                        return guid_string;
                    };

                    GuiSymbolLogAdd(StringUtils::sprintf("Validation error: PDB guid is not matching (expected: %s, actual: %s).\n",
                                                         guidStr(validationData->guid).c_str(), guidStr(guid).c_str()).c_str());
                    return false;
                }
            }
        }
        globalSym->Release();
    }

    if(loadAddress != 0)
    {
        m_session->put_loadAddress(loadAddress);
    }

    return true;
}

bool PDBDiaFile::isOpen() const
{
    return m_session != nullptr || m_dataSource != nullptr || m_stream != nullptr;
}

bool PDBDiaFile::close()
{
    if(m_session)
    {
        auto refcount = m_session->Release();
        if(refcount != 0)
            dprintf("Memory leaks in IDiaSession (refcount: %u)\n", refcount);
        m_session = nullptr;
    }

    if(m_dataSource)
    {
        auto refcount = m_dataSource->Release();
        if(refcount != 0)
            dprintf("Memory leaks in IDiaDataSource (refcount: %u)\n", refcount);
        m_dataSource = nullptr;
    }

    if(m_stream)
    {
        auto refcount = m_stream->Release();
        if(refcount != 0)
            dprintf("Memory leaks in IStream (refcount: %u)\n", refcount);
        m_stream = nullptr;
    }

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

    hr = sym->get_name(&str);
    if(hr != S_OK)
        return name;

    if(str != nullptr)
    {
        name = StringUtils::Utf16ToUtf8(str);
    }

    LocalFree(str - 2);

    return name;
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
    LocalFree(str - 2);

    return result;
}

bool PDBDiaFile::enumerateLineNumbers(uint32_t rva, uint32_t size, std::vector<DiaLineInfo_t> & lines, std::map<DWORD, std::string> & files, const std::atomic<bool> & cancelled)
{
    HRESULT hr;
    DWORD lineNumber = 0;
    DWORD relativeVirtualAddress = 0;
    DWORD lineNumberEnd = 0;

    CComPtr<IDiaEnumLineNumbers> lineNumbersEnum;
    hr = m_session->findLinesByRVA(rva, size, &lineNumbersEnum);
    if(!SUCCEEDED(hr))
        return false;

    LONG lineCount = 0;
    hr = lineNumbersEnum->get_Count(&lineCount);
    if(!SUCCEEDED(hr))
        return false;

    if(lineCount == 0)
        return true;

    lines.reserve(lines.size() + lineCount);

    const ULONG bucket = 10000;
    ULONG steps = lineCount / bucket + (lineCount % bucket != 0);
    for(ULONG step = 0; step < steps; step++)
    {
        ULONG begin = step * bucket;
        ULONG end = min((ULONG)lineCount, (step + 1) * bucket);

        if(cancelled)
            return false;

        std::vector<IDiaLineNumber*> lineNumbers;
        ULONG lineCountStep = end - begin;
        lineNumbers.resize(lineCountStep);

        ULONG fetched = 0;
        hr = lineNumbersEnum->Next((ULONG)lineNumbers.size(), lineNumbers.data(), &fetched);
        for(ULONG n = 0; n < fetched; n++)
        {
            if(cancelled)
            {
                for(ULONG m = n; m < fetched; m++)
                    lineNumbers[m]->Release();
                return false;
            }

            CComPtr<IDiaLineNumber> lineNumberInfo;
            lineNumberInfo.Attach(lineNumbers[n]);

            DWORD sourceFileId = 0;
            hr = lineNumberInfo->get_sourceFileId(&sourceFileId);
            if(!SUCCEEDED(hr))
                continue;

            if(!files.count(sourceFileId))
            {
                CComPtr<IDiaSourceFile> sourceFile;
                hr = lineNumberInfo->get_sourceFile(&sourceFile);
                if(!SUCCEEDED(hr))
                    continue;

                BSTR fileName = nullptr;
                hr = sourceFile->get_fileName(&fileName);
                if(!SUCCEEDED(hr))
                    continue;

                files.insert({ sourceFileId, StringUtils::Utf16ToUtf8(fileName) });
                LocalFree(fileName - 2);
            }

            DiaLineInfo_t lineInfo;
            lineInfo.sourceFileId = sourceFileId;

            hr = lineNumberInfo->get_lineNumber(&lineInfo.lineNumber);
            if(!SUCCEEDED(hr))
                continue;

            hr = lineNumberInfo->get_relativeVirtualAddress(&lineInfo.rva);
            if(!SUCCEEDED(hr))
                continue;

            lines.push_back(lineInfo);
        }
    }

    return true;
}

uint32_t getSymbolId(IDiaSymbol* sym)
{
    DWORD id;
    sym->get_symIndexId(&id);
    return id;
}

bool PDBDiaFile::enumerateLexicalHierarchy(const Query_t & query)
{
    CComPtr<IDiaSymbol> globalScope;
    IDiaSymbol* symbol = nullptr;
    ULONG celt = 0;
    HRESULT hr;
    DiaSymbol_t symbolInfo;
    bool res = true;

    hr = m_session->get_globalScope(&globalScope);
    if(hr != S_OK)
        return false;

    InternalQueryContext_t context;
    context.callback = query.callback;
    context.collectSize = query.collectSize;
    context.collectUndecoratedNames = query.collectUndecoratedNames;

    uint32_t scopeId = getSymbolId(globalScope);
    context.visited.insert(scopeId);

    // Enumerate compilands.
    {
        CComPtr<IDiaEnumSymbols> enumSymbols;

        hr = globalScope->findChildren(SymTagCompiland, nullptr, nsNone, &enumSymbols);
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                CComPtr<IDiaSymbol> sym(symbol);
                if(!enumerateCompilandScope(sym, context))
                {
                    return false;
                }
            }
        }
    }

    // Enumerate publics.
    {
        CComPtr<IDiaEnumSymbols> enumSymbols;

        hr = globalScope->findChildren(SymTagPublicSymbol, nullptr, nsNone, &enumSymbols);
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                CComPtr<IDiaSymbol> sym(symbol);
                if(convertSymbolInfo(symbol, symbolInfo, context))
                {
                    if(!context.callback(symbolInfo))
                    {
                        return false;
                    }
                }
            }
        }
    }

    // Enumerate global functions.
    {
        CComPtr<IDiaEnumSymbols> enumSymbols;

        hr = globalScope->findChildren(SymTagFunction, nullptr, nsNone, &enumSymbols);
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                CComPtr<IDiaSymbol> sym(symbol);
                if(convertSymbolInfo(sym, symbolInfo, context))
                {
                    if(!context.callback(symbolInfo))
                    {
                        return false;
                    }
                }
            }
        }
    }

    // Enumerate global data.
    {
        CComPtr<IDiaEnumSymbols> enumSymbols;

        hr = globalScope->findChildren(SymTagData, nullptr, nsNone, &enumSymbols);
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                CComPtr<IDiaSymbol> sym(symbol);
                if(convertSymbolInfo(sym, symbolInfo, context))
                {
                    if(!context.callback(symbolInfo))
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

    long disp = 0;
    hr = m_session->findSymbolByRVAEx((DWORD)address, tag, &symbol, &disp);
    if(hr != S_OK)
        return false;

    CComPtr<IDiaSymbol> scopedSym(symbol);

    sym.disp = disp;

    InternalQueryContext_t context;
    context.collectSize = true;
    context.collectUndecoratedNames = true;

    if(!convertSymbolInfo(scopedSym, sym, context))
        return false;

    return true;
}

bool PDBDiaFile::enumerateCompilandScope(IDiaSymbol* compiland, InternalQueryContext_t & context)
{
    IDiaSymbol* symbol = nullptr;
    ULONG celt = 0;
    HRESULT hr;
    DWORD symTagType;

    DiaSymbol_t symbolInfo;

    uint32_t symId = getSymbolId(compiland);

    {
        CComPtr<IDiaEnumSymbols> enumSymbols;

        hr = compiland->findChildren(SymTagFunction, nullptr, nsNone, &enumSymbols);
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                CComPtr<IDiaSymbol> sym(symbol);

                hr = sym->get_symTag(&symTagType);

                if(hr == S_OK)
                {
                    if(!processFunctionSymbol(sym, context))
                    {
                        return false;
                    }
                }

            }
        }
    }

    {
        CComPtr<IDiaEnumSymbols> enumSymbols;

        hr = compiland->findChildren(SymTagData, nullptr, nsNone, &enumSymbols);
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                CComPtr<IDiaSymbol> sym(symbol);

                hr = sym->get_symTag(&symTagType);

                if(hr == S_OK)
                {
                    if(convertSymbolInfo(sym, symbolInfo, context))
                    {
                        if(!context.callback(symbolInfo))
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }

    {
        CComPtr<IDiaEnumSymbols> enumSymbols;

        hr = compiland->findChildren(SymTagBlock, nullptr, nsNone, &enumSymbols);
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                CComPtr<IDiaSymbol> sym(symbol);

                hr = sym->get_symTag(&symTagType);

                if(hr == S_OK)
                {
                    if(convertSymbolInfo(sym, symbolInfo, context))
                    {
                        if(!context.callback(symbolInfo))
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }

    {
        CComPtr<IDiaEnumSymbols> enumSymbols;

        hr = compiland->findChildren(SymTagLabel, nullptr, nsNone, &enumSymbols);
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                CComPtr<IDiaSymbol> sym(symbol);

                hr = sym->get_symTag(&symTagType);

                if(hr == S_OK)
                {
                    if(convertSymbolInfo(sym, symbolInfo, context))
                    {
                        if(!context.callback(symbolInfo))
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

bool PDBDiaFile::processFunctionSymbol(IDiaSymbol* functionSym, InternalQueryContext_t & context)
{
    IDiaSymbol* symbol = nullptr;
    ULONG celt = 0;
    HRESULT hr;
    DWORD symTagType;

    uint32_t symId = getSymbolId(functionSym);
    if(context.visited.find(symId) != context.visited.end())
    {
        GuiSymbolLogAdd("Dupe\n");
        return true;
    }

    context.visited.insert(symId);

    DiaSymbol_t symbolInfo;
    if(convertSymbolInfo(functionSym, symbolInfo, context))
    {
        if(!context.callback(symbolInfo))
            return false;
    }

    {
        CComPtr<IDiaEnumSymbols> enumSymbols;
        hr = functionSym->findChildren(SymTagData, nullptr, nsNone, &enumSymbols);
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                CComPtr<IDiaSymbol> sym(symbol);

                hr = sym->get_symTag(&symTagType);

                LocationType locType;
                sym->get_locationType((DWORD*)&locType);

                if(hr == S_OK && locType == LocIsStatic)
                {
                    if(convertSymbolInfo(sym, symbolInfo, context))
                    {
                        if(!context.callback(symbolInfo))
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }

    {
        CComPtr<IDiaEnumSymbols> enumSymbols;
        hr = functionSym->findChildren(SymTagBlock, nullptr, nsNone, &enumSymbols);
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                CComPtr<IDiaSymbol> sym(symbol);

                hr = sym->get_symTag(&symTagType);

                if(hr == S_OK)
                {
                    if(convertSymbolInfo(sym, symbolInfo, context))
                    {
                        if(!context.callback(symbolInfo))
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }

    {
        CComPtr<IDiaEnumSymbols> enumSymbols;
        hr = functionSym->findChildren(SymTagLabel, nullptr, nsNone, &enumSymbols);
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                CComPtr<IDiaSymbol> sym(symbol);

                hr = sym->get_symTag(&symTagType);

                if(hr == S_OK)
                {
                    if(convertSymbolInfo(sym, symbolInfo, context))
                    {
                        if(!context.callback(symbolInfo))
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
    bool res = false;

    HRESULT hr;
    uint64_t tempSize = -1;

    if(symTag == SymTagData)
    {
        CComPtr<IDiaSymbol> symType;
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
    else if(symTag == SymTagFunction ||
            symTag == SymTagBlock)
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

bool PDBDiaFile::convertSymbolInfo(IDiaSymbol* symbol, DiaSymbol_t & symbolInfo, InternalQueryContext_t & context)
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

    if(context.collectUndecoratedNames && !symbolInfo.name.empty() && (symbolInfo.name.at(0) == '?' || symbolInfo.name.at(0) == '_' || symbolInfo.name.at(0) == '@'))
    {
        undecorateName(symbolInfo.name, symbolInfo.undecoratedName);
    }
    else
    {
        symbolInfo.undecoratedName = "";
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

    symbolInfo.size = -1;
    if(context.collectSize)
    {
        if(!resolveSymbolSize(symbol, symbolInfo.size, symTagType) || symbolInfo.size == 0)
        {
            symbolInfo.size = -1;
        }
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

