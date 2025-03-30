#include "_global.h"
#include <comutil.h>
#include <windows.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>

#include "msdia/dia2.h"
#include "msdia/cvconst.h"
#include "msdia/diacreate.h"

#include "pdbdiafile.h"
#include "stringutils.h"
#include "console.h"
#include "LLVMDemangle/LLVMDemangle.h"

// NOTE: Necessary for MinGW builds
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(DiaSource, 0xe6756135, 0x1e65, 0x4d17, 0x85, 0x76, 0x61, 0x07, 0x61, 0x39, 0x8c, 0x3c)
__CRT_UUID_DECL(DiaSourceAlt, 0x91904831, 0x49ca, 0x4766, 0xb9, 0x5c, 0x25, 0x39, 0x7e, 0x2d, 0xd6, 0xdc)
__CRT_UUID_DECL(DiaStackWalker, 0xce4a85db, 0x5768, 0x475b, 0xa4, 0xe1, 0xc0, 0xbc, 0xa2, 0x11, 0x2a, 0x6b)
__CRT_UUID_DECL(IDiaLoadCallback, 0xC32ADB82, 0x73F4, 0x421b, 0x95, 0xD5, 0xA4, 0x70, 0x6E, 0xDF, 0x5D, 0xBE)
__CRT_UUID_DECL(IDiaLoadCallback2, 0x4688a074, 0x5a4d, 0x4486, 0xae, 0xa8, 0x7b, 0x90, 0x71, 0x1d, 0x9f, 0x7c)
__CRT_UUID_DECL(IDiaReadExeAtOffsetCallback, 0x587A461C, 0xB80B, 0x4f54, 0x91, 0x94, 0x50, 0x32, 0x58, 0x9A, 0x63, 0x19)
__CRT_UUID_DECL(IDiaReadExeAtRVACallback, 0x8E3F80CA, 0x7517, 0x432a, 0xBA, 0x07, 0x28, 0x51, 0x34, 0xAA, 0xEA, 0x8E)
__CRT_UUID_DECL(IDiaDataSource, 0x79F1BB5F, 0xB66E, 0x48e5, 0xB6, 0xA9, 0x15, 0x45, 0xC3, 0x23, 0xCA, 0x3D)
__CRT_UUID_DECL(IDiaEnumSymbols, 0xCAB72C48, 0x443B, 0x48f5, 0x9B, 0x0B, 0x42, 0xF0, 0x82, 0x0A, 0xB2, 0x9A)
__CRT_UUID_DECL(IDiaEnumSymbolsByAddr, 0x624B7D9C, 0x24EA, 0x4421, 0x9D, 0x06, 0x3B, 0x57, 0x74, 0x71, 0xC1, 0xFA)
__CRT_UUID_DECL(IDiaEnumSourceFiles, 0x10F3DBD9, 0x664F, 0x4469, 0xB8, 0x08, 0x94, 0x71, 0xC7, 0xA5, 0x05, 0x38)
__CRT_UUID_DECL(IDiaEnumInputAssemblyFiles, 0x1C7FF653, 0x51F7, 0x457E, 0x84, 0x19, 0xB2, 0x0F, 0x57, 0xEF, 0x7E, 0x4D)
__CRT_UUID_DECL(IDiaEnumLineNumbers, 0xFE30E878, 0x54AC, 0x44f1, 0x81, 0xBA, 0x39, 0xDE, 0x94, 0x0F, 0x60, 0x52)
__CRT_UUID_DECL(IDiaEnumInjectedSources, 0xD5612573, 0x6925, 0x4468, 0x88, 0x83, 0x98, 0xCD, 0xEC, 0x8C, 0x38, 0x4A)
__CRT_UUID_DECL(IDiaEnumSegments, 0xE8368CA9, 0x01D1, 0x419d, 0xAC, 0x0C, 0xE3, 0x12, 0x35, 0xDB, 0xDA, 0x9F)
__CRT_UUID_DECL(IDiaEnumSectionContribs, 0x1994DEB2, 0x2C82, 0x4b1d, 0xA5, 0x7F, 0xAF, 0xF4, 0x24, 0xD5, 0x4A, 0x68)
__CRT_UUID_DECL(IDiaEnumFrameData, 0x9FC77A4B, 0x3C1C, 0x44ed, 0xA7, 0x98, 0x6C, 0x1D, 0xEE, 0xA5, 0x3E, 0x1F)
__CRT_UUID_DECL(IDiaEnumDebugStreamData, 0x486943E8, 0xD187, 0x4a6b, 0xA3, 0xC4, 0x29, 0x12, 0x59, 0xFF, 0xF6, 0x0D)
__CRT_UUID_DECL(IDiaEnumDebugStreams, 0x08CBB41E, 0x47A6, 0x4f87, 0x92, 0xF1, 0x1C, 0x9C, 0x87, 0xCE, 0xD0, 0x44)
__CRT_UUID_DECL(IDiaAddressMap, 0xB62A2E7A, 0x067A, 0x4ea3, 0xB5, 0x98, 0x04, 0xC0, 0x97, 0x17, 0x50, 0x2C)
__CRT_UUID_DECL(IDiaSession, 0x2F609EE1, 0xD1C8, 0x4E24, 0x82, 0x88, 0x33, 0x26, 0xBA, 0xDC, 0xD2, 0x11)
__CRT_UUID_DECL(IDiaSymbol, 0xcb787b2f, 0xbd6c, 0x4635, 0xba, 0x52, 0x93, 0x31, 0x26, 0xbd, 0x2d, 0xcd)
__CRT_UUID_DECL(IDiaSourceFile, 0xA2EF5353, 0xF5A8, 0x4eb3, 0x90, 0xD2, 0xCB, 0x52, 0x6A, 0xCB, 0x3C, 0xDD)
__CRT_UUID_DECL(IDiaInputAssemblyFile, 0x3BFE56B0, 0x390C, 0x4863, 0x94, 0x30, 0x1F, 0x3D, 0x08, 0x3B, 0x76, 0x84)
__CRT_UUID_DECL(IDiaLineNumber, 0xB388EB14, 0xBE4D, 0x421d, 0xA8, 0xA1, 0x6C, 0xF7, 0xAB, 0x05, 0x70, 0x86)
__CRT_UUID_DECL(IDiaSectionContrib, 0x0CF4B60E, 0x35B1, 0x4c6c, 0xBD, 0xD8, 0x85, 0x4B, 0x9C, 0x8E, 0x38, 0x57)
__CRT_UUID_DECL(IDiaSegment, 0x0775B784, 0xC75B, 0x4449, 0x84, 0x8B, 0xB7, 0xBD, 0x31, 0x59, 0x54, 0x5B)
__CRT_UUID_DECL(IDiaInjectedSource, 0xAE605CDC, 0x8105, 0x4a23, 0xB7, 0x10, 0x32, 0x59, 0xF1, 0xE2, 0x61, 0x12)
__CRT_UUID_DECL(IDiaStackWalkFrame, 0x07C590C1, 0x438D, 0x4F47, 0xBD, 0xCD, 0x43, 0x97, 0xBC, 0x81, 0xAD, 0x75)
__CRT_UUID_DECL(IDiaFrameData, 0xA39184B7, 0x6A36, 0x42de, 0x8E, 0xEC, 0x7D, 0xF9, 0xF3, 0xF5, 0x9F, 0x33)
__CRT_UUID_DECL(IDiaImageData, 0xC8E40ED2, 0xA1D9, 0x4221, 0x86, 0x92, 0x3C, 0xE6, 0x61, 0x18, 0x4B, 0x44)
__CRT_UUID_DECL(IDiaTable, 0x4A59FB77, 0xABAC, 0x469b, 0xA3, 0x0B, 0x9E, 0xCC, 0x85, 0xBF, 0xEF, 0x14)
__CRT_UUID_DECL(IDiaEnumTables, 0xC65C2B0A, 0x1150, 0x4d7a, 0xAF, 0xCC, 0xE0, 0x5B, 0xF3, 0xDE, 0xE8, 0x1E)
__CRT_UUID_DECL(IDiaPropertyStorage, 0x9d416f9c, 0xe184, 0x45b2, 0xa4, 0xf0, 0xce, 0x51, 0x7f, 0x71, 0x9e, 0x9b)
__CRT_UUID_DECL(IDiaStackFrame, 0x5edbc96d, 0xcdd6, 0x4792, 0xaf, 0xbe, 0xcc, 0x89, 0x00, 0x7d, 0x96, 0x10)
__CRT_UUID_DECL(IDiaEnumStackFrames, 0xec9d461d, 0xce74, 0x4711, 0xa0, 0x20, 0x7d, 0x8f, 0x9a, 0x1d, 0xd2, 0x55)
__CRT_UUID_DECL(IDiaStackWalkHelper, 0x21F81B1B, 0xC5BB, 0x42A3, 0xBC, 0x4F, 0xCC, 0xBA, 0xA7, 0x5B, 0x9F, 0x19)
__CRT_UUID_DECL(IDiaStackWalker, 0x5485216b, 0xa54c, 0x469f, 0x96, 0x70, 0x52, 0xb2, 0x4d, 0x52, 0x29, 0xbb)
__CRT_UUID_DECL(IDiaStackWalkHelper2, 0x8222c490, 0x507b, 0x4bef, 0xb3, 0xbd, 0x41, 0xdc, 0xa7, 0xb5, 0x93, 0x4c)
__CRT_UUID_DECL(IDiaStackWalker2, 0x7c185885, 0xa015, 0x4cac, 0x94, 0x11, 0x0f, 0x4f, 0xb3, 0x9b, 0x1f, 0x3a)
#endif // __CRT_UUID_DECL

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

        try
        {
            *ppStream = new FileStream(hFile);
        }
        catch(const std::bad_alloc &)
        {
            CloseHandle(hFile);
        }

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

    HRESULT hr = REGDB_E_CLASSNOTREG;
    hr = NoRegCoCreate(L"msdia140.dll", __uuidof(DiaSource), __uuidof(IDiaDataSource), (LPVOID*)&m_dataSource);
    if(testError(hr) || m_dataSource == nullptr)
    {
        hr = CoCreateInstance(__uuidof(DiaSource), NULL, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), (LPVOID*)&m_dataSource);
    }
    if(testError(hr) || m_dataSource == nullptr)
    {
        GuiSymbolLogAdd("Unable to initialize PDBDia Library.\n");
        return false;
    }

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

    SysFreeString(str);

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
    SysFreeString(str);

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
        ULONG end = std::min((ULONG)lineCount, (step + 1) * bucket);

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
                SysFreeString(fileName);
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
    default:
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
        auto demangled = LLVMDemangle(symbolInfo.name.c_str());
        if(demangled && symbolInfo.name.compare(demangled) != 0)
            symbolInfo.undecoratedName = demangled;
        LLVMDemangleFree(demangled);
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

