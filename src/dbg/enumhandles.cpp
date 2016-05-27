#include "_global.h"
#include "debugger.h"
#include "TitanEngine\TitanEngine.h"

struct SYSTEM_HANDLE_INFORMATION
{
    ULONG ProcessId;
    UCHAR ObjectTypeNumber;
    UCHAR Flags;
    USHORT Handle;
    PVOID Object;
    DWORD GrantedAccess;
};

struct OBJECT_TYPE_INFORMATION
{
    UNICODE_STRING Name;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG TotalPagedPoolUsage;
    ULONG TotalNonPagedPoolUsage;
    ULONG TotalNamePoolUsage;
    ULONG TotalHandleTableUsage;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG HighWaterPagedPoolUsage;
    ULONG HighWaterNonPagedPoolUsage;
    ULONG HighWaterNamePoolUsage;
    ULONG HighWaterHandleTableUsage;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccess;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    USHORT MaintainTypeList;
    DWORD PoolType;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
};

struct MYHANDLES
{
    DWORD_PTR HandleCount;
    SYSTEM_HANDLE_INFORMATION Handles[1];
};

#ifdef _WIN64
DWORD (*NtQuerySystemInformation)(DWORD SystemInfoClass, void* SystemInfo, DWORD SystemInfoSize, DWORD* ReturnedSize) = nullptr;
#else //x86
DWORD(__stdcall* NtQuerySystemInformation)(DWORD SystemInfoClass, void* SystemInfo, DWORD SystemInfoSize, DWORD* ReturnedSize) = nullptr;
#endif //_WIN64
#ifdef _WIN64
DWORD (*NtQueryObject)(HANDLE ObjectHandle, ULONG ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength) = nullptr;
#else //x86
DWORD(__stdcall* NtQueryObject)(HANDLE ObjectHandle, ULONG ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength) = nullptr;
#endif //_WIN64

extern "C" DLL_EXPORT long _dbg_enumhandles(duint* handles, unsigned char* typeNumbers, unsigned int* grantedAccess, unsigned int maxcount)
{
    MYHANDLES* myhandles = (MYHANDLES*)emalloc(16384, "_dbg_enumhandles");
    DWORD size = 16384;
    DWORD errcode = 0xC0000004;
    if(NtQuerySystemInformation == nullptr)
        *(FARPROC*)&NtQuerySystemInformation = GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQuerySystemInformation");
    while(errcode == 0xC0000004)
    {
        errcode = NtQuerySystemInformation(16, myhandles, size, &size);
        if(errcode == 0xC0000004)
        {
            myhandles = (MYHANDLES*)erealloc(myhandles, size + 16384, "_dbg_enumhandles");
            size += 16384;
        }
        else
        {
            break;
        }
    }
    if(errcode != 0)
    {
        efree(myhandles, "_dbg_enumhandles");
        return 0;
    }
    else
    {
        unsigned int j = 0;
        for(unsigned int i = 0; i < myhandles->HandleCount; i++)
        {
            DWORD pid = fdProcessInfo->dwProcessId;
            if(myhandles->Handles[i].ProcessId == pid)
            {
                handles[j] = myhandles->Handles[j].Handle;
                typeNumbers[j] = myhandles->Handles[j].ObjectTypeNumber;
                grantedAccess[j] = myhandles->Handles[j].GrantedAccess;
                if(++j == maxcount) break;
            }
        }
        efree(myhandles, "_dbg_enumhandles");
        return j;
    }
}

extern "C" DLL_EXPORT bool _dbg_gethandlename(char* name, char* typeName, size_t buffersize, duint remotehandle)
{
    HANDLE hLocalHandle;
    if(typeName && DuplicateHandle(fdProcessInfo->hProcess, (HANDLE)remotehandle, GetCurrentProcess(), &hLocalHandle, DUPLICATE_SAME_ACCESS, FALSE, 0))
    {
        OBJECT_TYPE_INFORMATION* objectTypeInfo = (OBJECT_TYPE_INFORMATION*)emalloc(128, "_dbg_gethandlename");
        if(NtQueryObject == nullptr)
            *(FARPROC*)&NtQueryObject = GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryObject");
        if(NtQueryObject(hLocalHandle, 2, objectTypeInfo, 128, NULL) >= 0)
            strcpy_s(typeName, buffersize, StringUtils::Utf16ToUtf8(objectTypeInfo->Name.Buffer).c_str());
        efree(objectTypeInfo, "_dbg_gethandlename");
        CloseHandle(hLocalHandle);
    }
    wchar_t* buffer;
    buffer = (wchar_t*)HandlerGetHandleNameW(fdProcessInfo->hProcess, fdProcessInfo->dwProcessId, (HANDLE)remotehandle, false);
    if(buffer)
    {
        strcpy_s(name, buffersize, StringUtils::Utf16ToUtf8(buffer).c_str());
        VirtualFree(buffer, 0, MEM_RELEASE);
        return true;
    }
    return true;
}
