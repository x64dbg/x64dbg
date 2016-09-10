#include "handles.h"
#include "undocumented.h"
#include "exception.h"

typedef struct _OBJECT_NAME_INFORMATION
{
    UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

typedef struct _OBJECT_TYPE_INFORMATION
{
    UNICODE_STRING TypeName;
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
    ULONG ValidAccessMask;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    UCHAR TypeIndex; // since WINBLUE
    CHAR ReservedByte;
    ULONG PoolType;
    ULONG DefaultPagedPoolCharge;
    ULONG DefaultNonPagedPoolCharge;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

#define STATUS_INFO_LENGTH_MISMATCH 0xC0000004
#define STATUS_SUCCESS 0x00000000

#define SystemHandleInformation 16

#define ObjectNameInformation 1
#define ObjectTypeInformation 2

typedef enum _SYSTEM_HANDLE_FLAGS
{
    PROTECT_FROM_CLOSE = 1,
    INHERIT = 2
} SYSTEM_HANDLE_FLAGS;

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO // Size=16
{
    USHORT UniqueProcessId; // Size=2 Offset=0
    USHORT CreatorBackTraceIndex; // Size=2 Offset=2
    UCHAR ObjectTypeIndex; // Size=1 Offset=4
    UCHAR HandleAttributes; // Size=1 Offset=5 (SYSTEM_HANDLE_FLAGS)
    USHORT HandleValue; // Size=2 Offset=6
    PVOID Object; // Size=4 Offset=8
    ULONG GrantedAccess; // Size=4 Offset=12
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION // Size=20
{
    ULONG NumberOfHandles; // Size=4 Offset=0
    SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1]; // Size=16 Offset=4
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef NTSTATUS(NTAPI* ZWQUERYSYSTEMINFORMATION)(
    IN LONG SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

typedef NTSTATUS(NTAPI* ZWQUERYOBJECT)(
    IN HANDLE Handle OPTIONAL,
    IN LONG ObjectInformationClass,
    OUT PVOID ObjectInformation OPTIONAL,
    IN ULONG ObjectInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

bool HandlesEnum(duint pid, std::vector<HANDLEINFO> & handles)
{
    static auto ZwQuerySystemInformation = ZWQUERYSYSTEMINFORMATION(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwQuerySystemInformation"));
    if(!ZwQuerySystemInformation)
        return 0;
    Memory<PSYSTEM_HANDLE_INFORMATION> HandleInformation(16 * 1024, "_dbg_enumhandles");
    NTSTATUS ErrorCode = ERROR_SUCCESS;
    for(;;)
    {
        ErrorCode = ZwQuerySystemInformation(SystemHandleInformation, HandleInformation(), ULONG(HandleInformation.size()), nullptr);
        if(ErrorCode != STATUS_INFO_LENGTH_MISMATCH)
            break;
        HandleInformation.realloc(HandleInformation.size() * 2, "_dbg_enumhandles");
    }
    if(ErrorCode != STATUS_SUCCESS)
        return false;

    handles.reserve(HandleInformation()->NumberOfHandles);

    HANDLEINFO info;
    for(ULONG i = 0; i < HandleInformation()->NumberOfHandles; i++)
    {
        const auto & handle = HandleInformation()->Handles[i];
        if(handle.UniqueProcessId != pid)
            continue;
        info.Handle = handle.HandleValue;
        info.TypeNumber = handle.ObjectTypeIndex;
        info.GrantedAccess = handle.GrantedAccess;
        handles.push_back(info);
    }
    return true;
}

static DWORD WINAPI getNameThread(LPVOID lpParam)
{
    (*(std::function<void()>*)lpParam)();
    return 0;
}

bool HandlesGetName(HANDLE hProcess, HANDLE remoteHandle, String & name, String & typeName)
{
    static auto ZwQueryObject = ZWQUERYOBJECT(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "ZwQueryObject"));
    if(!ZwQueryObject)
        return false;
    HANDLE hLocalHandle;
    if(DuplicateHandle(hProcess, remoteHandle, GetCurrentProcess(), &hLocalHandle, 0, FALSE, 0))
    {
        ULONG ReturnSize = 0;
        if(ZwQueryObject(hLocalHandle, ObjectTypeInformation, nullptr, 0, &ReturnSize) == STATUS_INFO_LENGTH_MISMATCH)
        {
            ReturnSize += 0x2000;
            Memory<OBJECT_TYPE_INFORMATION*> objectTypeInfo(ReturnSize + sizeof(WCHAR) * 16, "_dbg_gethandlename:objectTypeInfo");
            if(ZwQueryObject(hLocalHandle, ObjectTypeInformation, objectTypeInfo(), ReturnSize, nullptr) == STATUS_SUCCESS)
                typeName = StringUtils::Utf16ToUtf8(objectTypeInfo()->TypeName.Buffer);
        }

        std::function<void()> getName = [&]()
        {
            if(ZwQueryObject(hLocalHandle, ObjectNameInformation, nullptr, 0, &ReturnSize) == STATUS_INFO_LENGTH_MISMATCH)
            {
                ReturnSize += 0x2000;
                Memory<OBJECT_NAME_INFORMATION*> objectNameInfo(ReturnSize + sizeof(WCHAR) * 16, "_dbg_gethandlename:objectNameInfo");
                if(ZwQueryObject(hLocalHandle, ObjectNameInformation, objectNameInfo(), ReturnSize, nullptr) == STATUS_SUCCESS)
                    name = StringUtils::Utf16ToUtf8(objectNameInfo()->Name.Buffer);
            }
        };

        auto hThread = CreateThread(nullptr, 0, getNameThread, &getName, 0, nullptr);
        auto result = WaitForSingleObject(hThread, 200);
        if(result != WAIT_OBJECT_0)
        {
            TerminateThread(hThread, 0);
            name = String(ErrorCodeToName(result));
        }
        else
            CloseHandle(hThread);

        CloseHandle(hLocalHandle);
    }
    else
        name = String(ErrorCodeToName(GetLastError()));
    return true;
}
