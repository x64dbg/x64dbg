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

class DiaLoadCallback : public IDiaLoadCallback2
{
    virtual HRESULT STDMETHODCALLTYPE NotifyDebugDir(
        /* [in] */ BOOL fExecutable,
        /* [in] */ DWORD cbData,
        /* [size_is][in] */ BYTE* pbData) override
    {
        GuiSymbolLogAdd(StringUtils::sprintf("[DIA] NotifyDebugDir: %s\n", StringUtils::ToHex(pbData, cbData).c_str()).c_str());
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE NotifyOpenDBG(
        /* [in] */ LPCOLESTR dbgPath,
        /* [in] */ HRESULT resultCode) override
    {
        GuiSymbolLogAdd(StringUtils::sprintf("[DIA] NotifyOpenDBG: %s, %08X\n", StringUtils::Utf16ToUtf8(dbgPath).c_str(), resultCode).c_str());
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE NotifyOpenPDB(
        /* [in] */ LPCOLESTR pdbPath,
        /* [in] */ HRESULT resultCode) override
    {
        GuiSymbolLogAdd(StringUtils::sprintf("[DIA] NotifyOpenPDB: %s, %08X\n", StringUtils::Utf16ToUtf8(pdbPath).c_str(), resultCode).c_str());
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE RestrictRegistryAccess(void) override { return S_OK; }

    virtual HRESULT STDMETHODCALLTYPE RestrictSymbolServerAccess(void) override { return S_OK; }

    virtual HRESULT STDMETHODCALLTYPE RestrictDBGAccess(void) override { return S_OK; }

    virtual HRESULT STDMETHODCALLTYPE RestrictOriginalPathAccess(void) override { return S_OK; }

    virtual HRESULT STDMETHODCALLTYPE RestrictReferencePathAccess(void) override { return S_OK; }

    virtual HRESULT STDMETHODCALLTYPE RestrictSystemRootAccess(void) override { return S_OK; }

    //TODO: properly implement IUnknown (https://msdn.microsoft.com/en-us/library/office/cc839627.aspx)
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID rid, _Outptr_ void** ppUnk) override
    {
        if(ppUnk == NULL)
        {
            return E_INVALIDARG;
        }
        if(rid == __uuidof(IDiaLoadCallback2))
            *ppUnk = (IDiaLoadCallback2*)this;
        else if(rid == __uuidof(IDiaLoadCallback))
            *ppUnk = (IDiaLoadCallback*)this;
        else if(rid == __uuidof(IUnknown))
            *ppUnk = (IUnknown*)this;
        else
            *ppUnk = NULL;
        if(*ppUnk != NULL)
        {
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return 1;
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        return 1;
    }
};

class DiaFileStream : public IStream
{
private:
    HANDLE _hFile;

public:
    DiaFileStream(HANDLE hFile)
    {
        _hFile = hFile;
    }

    ~DiaFileStream()
    {
        if(_hFile != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(_hFile);
        }
    }

public:
    HRESULT static OpenFile(LPCWSTR pName, IStream** ppStream)
    {
        HANDLE hFile = ::CreateFileW(pName, GENERIC_READ, FILE_SHARE_READ,
                                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if(hFile == INVALID_HANDLE_VALUE)
            return HRESULT_FROM_WIN32(GetLastError());

        DiaFileStream* out = new DiaFileStream(hFile);
        *ppStream = out;

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
            return S_OK;
        }
        else
            return E_NOINTERFACE;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return 1;
    }

    virtual ULONG STDMETHODCALLTYPE Release(void)
    {
        return 1;
    }

    virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead)
    {
        BOOL rc = ReadFile(_hFile, pv, cb, pcbRead, NULL);
        return (rc) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }

    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin,
                                           ULARGE_INTEGER* lpNewFilePointer)
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

    virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten) {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE Revert(void) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE Clone(IStream**) { return E_NOTIMPL; }
};

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
    hr = CoCreateInstance(__uuidof(DiaSource), NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (LPVOID*)&m_dataSource);
    if(testError(hr) || m_dataSource == nullptr)
    {
        if(hr == REGDB_E_CLASSNOTREG)
        {
            hr = NoRegCoCreate(L"msdia140.dll", __uuidof(DiaSource), __uuidof(IDiaDataSource), (LPVOID*)&m_dataSource);
            if(testError(hr))
                return false;
        }
        else
        {
            GuiSymbolLogAdd("Unable to initialize PDBDia Library.\n");
            return false;
        }
    }

    _wsplitpath_s(file, NULL, 0, fileDir, MAX_PATH, NULL, 0, fileExt, MAX_PATH);

    if(_wcsicmp(fileExt, L".pdb") == 0)
    {
        hr = DiaFileStream::OpenFile(file, &m_stream);
        if(testError(hr))
        {
            GuiSymbolLogAdd("Unable to open PDB file.\n");
            return false;
        }

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
        DiaLoadCallback callback;
        hr = m_dataSource->loadDataForExe(file, fileDir, &callback);
    }

    if(testError(hr))
    {
        if(hr != E_PDB_NOT_FOUND)
        {
            GuiSymbolLogAdd(StringUtils::sprintf("Unable to open PDB file - %08X\n", hr).c_str());
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
        ScopedDiaType<IDiaSymbol> globalSym;
        hr = m_session->get_globalScope(globalSym.ref());
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
                close();

                GuiSymbolLogAdd("PDB age is not matching.\n");
                return false;
            }

            // NOTE: For some reason this never matches, commented for now.
            // ^ 99% sure this should only be used for PDB v2.0 ('NB10' ones). v7.0 PDBs should be checked using (age+guid) only
            /*
            DWORD signature = 0;
            hr = globalSym->get_signature(&signature);
            if (!testError(hr) && validationData->signature != signature)
            {
                close();

                GuiSymbolLogAdd("PDB is not matching.\n");
                return false;
            }
            */

            GUID guid = {0};
            hr = globalSym->get_guid(&guid);
            if(!testError(hr) && memcmp(&guid, &validationData->guid, sizeof(GUID)) != 0)
            {
                close();

                GuiSymbolLogAdd("PDB guid is not matching.\n");
                return false;
            }
        }
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

    if(m_stream != nullptr)
    {
        delete(DiaFileStream*)m_stream;
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

bool PDBDiaFile::getFunctionLineNumbers(DWORD rva, ULONGLONG size, uint64_t imageBase, std::map<uint64_t, DiaLineInfo_t> & lines)
{
    HRESULT hr;
    DWORD lineNumber = 0;
    DWORD relativeVirtualAddress = 0;
    DWORD lineNumberEnd = 0;

    ScopedDiaType<IDiaEnumLineNumbers> lineNumbersEnum;
    hr = m_session->findLinesByRVA(rva, static_cast<DWORD>(size), lineNumbersEnum.ref());
    if(!SUCCEEDED(hr))
        return false;

    LONG lineCount = 0;
    hr = lineNumbersEnum->get_Count(&lineCount);
    if(!SUCCEEDED(hr))
        return false;

    if(lineCount == 0)
        return true;

    std::vector<IDiaLineNumber*> lineNumbers;
    lineNumbers.resize(lineCount);

    ULONG fetched = 0;
    hr = lineNumbersEnum->Next(lineCount, lineNumbers.data(), &fetched);
    for(LONG n = 0; n < fetched; n++)
    {
        ScopedDiaType<IDiaLineNumber> lineNumberInfo(lineNumbers[n]);

        ScopedDiaType<IDiaSourceFile> sourceFile;
        hr = lineNumberInfo->get_sourceFile(sourceFile.ref());
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

        DWORD segment = -1;
        hr = lineNumberInfo->get_addressSection(&segment);
        if(!SUCCEEDED(hr))
            continue;

        DWORD offset = -1;
        hr = lineNumberInfo->get_addressOffset(&offset);
        if(!SUCCEEDED(hr))
            continue;

        BSTR fileName = nullptr;
        hr = sourceFile->get_fileName(&fileName);
        if(!SUCCEEDED(hr))
            continue;

        DiaLineInfo_t lineInfo;
        lineInfo.fileName = StringUtils::Utf16ToUtf8(fileName);
        lineInfo.lineNumber = lineNumber;
        lineInfo.offset = offset;
        lineInfo.segment = segment;
        lineInfo.virtualAddress = relativeVirtualAddress;

        lines.emplace(lineInfo.virtualAddress, lineInfo);

        SysFreeString(fileName);
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
    ScopedDiaSymbol globalScope;
    IDiaSymbol* symbol = nullptr;
    ULONG celt = 0;
    HRESULT hr;
    DiaSymbol_t symbolInfo;
    bool res = true;

    hr = m_session->get_globalScope(globalScope.ref());
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
        ScopedDiaEnumSymbols enumSymbols;

        hr = globalScope->findChildren(SymTagCompiland, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);
                if(!enumerateCompilandScope(sym, context))
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
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                if(convertSymbolInfo(symbol, symbolInfo, context))
                {
                    ScopedDiaSymbol sym(symbol);
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
        ScopedDiaEnumSymbols enumSymbols;

        hr = globalScope->findChildren(SymTagFunction, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);
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
        ScopedDiaEnumSymbols enumSymbols;

        hr = globalScope->findChildren(SymTagData, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);
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
    hr = m_session->findSymbolByRVAEx(address, tag, &symbol, &disp);
    if(hr != S_OK)
        return false;

    ScopedDiaSymbol scopedSym(symbol);

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
        ScopedDiaEnumSymbols enumSymbols;

        hr = compiland->findChildren(SymTagFunction, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);

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
        ScopedDiaEnumSymbols enumSymbols;

        hr = compiland->findChildren(SymTagData, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);

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
        ScopedDiaEnumSymbols enumSymbols;
        hr = functionSym->findChildren(SymTagData, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);

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
        ScopedDiaEnumSymbols enumSymbols;
        hr = functionSym->findChildren(SymTagBlock, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);

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
        ScopedDiaEnumSymbols enumSymbols;
        hr = functionSym->findChildren(SymTagLabel, nullptr, nsNone, enumSymbols.ref());
        if(hr == S_OK)
        {
            while((hr = enumSymbols->Next(1, &symbol, &celt)) == S_OK && celt == 1)
            {
                ScopedDiaSymbol sym(symbol);

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

typedef char* pchar_t;
typedef const char* pcchar_t;
typedef char* (*GetParameter_t)(long n);
using Alloc_t = decltype(malloc);
using Free_t = decltype(free);

enum
{
    UNDNAME_COMPLETE = 0x0, //Enables full undecoration.
    UNDNAME_NO_LEADING_UNDERSCORES = 0x1, //Removes leading underscores from Microsoft extended keywords.
    UNDNAME_NO_MS_KEYWORDS = 0x2, //Disables expansion of Microsoft extended keywords.
    UNDNAME_NO_FUNCTION_RETURNS = 0x4, //Disables expansion of return type for primary declaration.
    UNDNAME_NO_ALLOCATION_MODEL = 0x8, //Disables expansion of the declaration model.
    UNDNAME_NO_ALLOCATION_LANGUAGE = 0x10, //Disables expansion of the declaration language specifier.
    UNDNAME_NO_MS_THISTYPE = 0x20, //NYI Disable expansion of MS keywords on the 'this' type for primary declaration.
    UNDNAME_NO_CV_THISTYPE = 0x40, //NYI Disable expansion of CV modifiers on the 'this' type for primary declaration/
    UNDNAME_NO_THISTYPE = 0x60, //Disables all modifiers on the this type.
    UNDNAME_NO_ACCESS_SPECIFIERS = 0x80, //Disables expansion of access specifiers for members.
    UNDNAME_NO_THROW_SIGNATURES = 0x100, //Disables expansion of "throw-signatures" for functions and pointers to functions.
    UNDNAME_NO_MEMBER_TYPE = 0x200, //Disables expansion of static or virtual members.
    UNDNAME_NO_RETURN_UDT_MODEL = 0x400, //Disables expansion of the Microsoft model for UDT returns.
    UNDNAME_32_BIT_DECODE = 0x800, //Undecorates 32-bit decorated names.
    UNDNAME_NAME_ONLY = 0x1000, //Gets only the name for primary declaration; returns just [scope::]name. Expands template params.
    UNDNAME_TYPE_ONLY = 0x2000, //Input is just a type encoding; composes an abstract declarator.
    UNDNAME_HAVE_PARAMETERS = 0x4000, //The real template parameters are available.
    UNDNAME_NO_ECSU = 0x8000, //Suppresses enum/class/struct/union.
    UNDNAME_NO_IDENT_CHAR_CHECK = 0x10000, //Suppresses check for valid identifier characters.
    UNDNAME_NO_PTR64 = 0x20000, //Does not include ptr64 in output.
};

#if _MSC_VER != 1800
#error unDNameEx is undocumented and possibly unsupported on your runtime! Uncomment this line if you understand the risks and want continue regardless...
#endif //_MSC_VER

//undname.cxx
extern "C" pchar_t __cdecl __unDNameEx(_Out_opt_z_cap_(maxStringLength) pchar_t outputString,
                                       pcchar_t name,
                                       int maxStringLength,    // Note, COMMA is leading following optional arguments
                                       Alloc_t pAlloc,
                                       Free_t pFree,
                                       GetParameter_t pGetParameter,
                                       unsigned long disableFlags
                                      );

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

    if(context.collectUndecoratedNames && !symbolInfo.name.empty() && symbolInfo.name.at(0) == '?')
    {
        //TODO: undocumented hack to have some kind of performance while undecorating names
        auto mymalloc = [](size_t size) { return emalloc(size, "convertSymbolInfo::undecoratedName"); };
        auto myfree = [](void* ptr) { return efree(ptr, "convertSymbolInfo::undecoratedName"); };
        symbolInfo.undecoratedName.resize(max(512, symbolInfo.name.length() * 2));
        if(!__unDNameEx((char*)symbolInfo.undecoratedName.data(),
                        symbolInfo.name.c_str(),
                        symbolInfo.undecoratedName.size(),
                        mymalloc,
                        myfree,
                        nullptr,
                        UNDNAME_COMPLETE))
        {
            symbolInfo.undecoratedName.clear();
        }
        else
        {
            symbolInfo.undecoratedName.resize(strlen(symbolInfo.undecoratedName.c_str()));
            if(symbolInfo.name == symbolInfo.undecoratedName)
                symbolInfo.undecoratedName = ""; //https://stackoverflow.com/a/18299315
            /*auto test = getSymbolUndecoratedNameString(symbol); //TODO: this does not appear to work very well
            if(!symbolInfo.undecoratedName.empty())
            {
                if(test != symbolInfo.undecoratedName)
                {
                    GuiSymbolLogAdd(StringUtils::sprintf("undecoration mismatch, msvcrt: \"%s\", DIA: \"%s\"\n",
                        symbolInfo.undecoratedName.c_str(),
                        test.c_str()).c_str());
                }
            }*/
        }
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

