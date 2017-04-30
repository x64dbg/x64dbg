#include "handles.h"
#include "undocumented.h"
#include "exception.h"
#include "debugger.h"
#include <functional>

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

/**
\brief Get information about a window
*/
static WINDOW_INFO getWindowInfo(HWND hWnd)
{
    WINDOW_INFO info;
    memset(&info, 0, sizeof(info));
    if(IsWindow(hWnd) != TRUE) //Not a window
    {
        return info;
    }
    info.handle = (duint)hWnd; //Get Window Handle
    GetWindowRect(hWnd, &info.position); //Get Window Rect
    info.style = GetWindowLong(hWnd, GWL_STYLE); //Get Window Style
    info.styleEx = GetWindowLong(hWnd, GWL_EXSTYLE); //Get Window Stye ex
    info.wndProc = (IsWindowUnicode(hWnd) ? GetClassLongPtrW : GetClassLongPtrA)(hWnd, GCLP_WNDPROC); //Get Window Proc (thanks to ThunderCls!)
    info.enabled = IsWindowEnabled(hWnd) == TRUE;
    info.parent = (duint)GetParent(hWnd); //Get Parent Window
    info.threadId = GetWindowThreadProcessId(hWnd, nullptr); //Get Window Thread Id
    wchar_t limitedbuffer[256];
    limitedbuffer[255] = 0;
    GetWindowTextW(hWnd, limitedbuffer, 256);
    if(limitedbuffer[255] != 0) //Window title too long. Add "..." to the end of buffer.
    {
        if(limitedbuffer[252] < 0xDC00 || limitedbuffer[252] > 0xDFFF) //protect the last surrogate of UTF-16 surrogate pair
            limitedbuffer[252] = L'.';
        limitedbuffer[253] = L'.';
        limitedbuffer[254] = L'.';
        limitedbuffer[255] = 0;
    }
    auto UTF8WindowTitle = StringUtils::Utf16ToUtf8(limitedbuffer);
    memcpy(info.windowTitle, UTF8WindowTitle.c_str(), min(UTF8WindowTitle.size(), sizeof(info.windowTitle))); //Copy window title with repect to buffer size constraints
    GetClassNameW(hWnd, limitedbuffer, 256);
    if(limitedbuffer[255] != 0) //Window class too long. Add "..." to the end of buffer.
    {
        if(limitedbuffer[252] < 0xDC00 || limitedbuffer[252] > 0xDFFF) //protect the last surrogate of UTF-16 surrogate pair
            limitedbuffer[252] = L'.';
        limitedbuffer[253] = L'.';
        limitedbuffer[254] = L'.';
        limitedbuffer[255] = 0;
    }
    UTF8WindowTitle = StringUtils::Utf16ToUtf8(limitedbuffer);
    memcpy(info.windowClass, UTF8WindowTitle.c_str(), min(UTF8WindowTitle.size(), sizeof(info.windowClass))); //Copy window class with repect to buffer size constraints
    return info;
}

static BOOL CALLBACK getWindowInfoCallback(HWND hWnd, LPARAM lParam)
{
    std::vector<WINDOW_INFO>* windowInfo = reinterpret_cast<std::vector<WINDOW_INFO>*>(lParam);
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    if(pid == fdProcessInfo->dwProcessId)
    {
        windowInfo->push_back(getWindowInfo(hWnd));
    }
    return TRUE;
}

/**
\brief Enumerates the window and return a list of all the windows owned by the debuggee (currently only top level windows)
*/
bool HandlesEnumWindows(std::vector<WINDOW_INFO> & windowsList)
{
    std::vector<WINDOW_INFO> childWindowsList;
    EnumWindows(getWindowInfoCallback, (LPARAM)&windowsList);
    auto i = windowsList.begin();
    for(auto i = windowsList.cbegin(); i != windowsList.cend(); ++i)
    {
        EnumChildWindows((HWND)i->handle, getWindowInfoCallback, (LPARAM)&childWindowsList);
    }
    for(auto i = childWindowsList.cbegin(); i != childWindowsList.cend(); ++i)
    {
        windowsList.push_back(*i);
    }
    return true;
}

/**
\brief Enumerates the heap and return a list of all the heaps in the debuggee
*/
bool HandlesEnumHeaps(std::vector<HEAPINFO> & heapList)
{
    // Slow and official method to enum all heap blocks.
    /*
    HEAPLIST32 hl;
    Handle hHeapSnap = CreateToolhelp32Snapshot(TH32CS_SNAPHEAPLIST, fdProcessInfo->dwProcessId);

    hl.dwSize = sizeof(HEAPLIST32);

    if(hHeapSnap == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    if(Heap32ListFirst(hHeapSnap, &hl))
    {
        do
        {
            HEAPENTRY32 he;
            ZeroMemory(&he, sizeof(HEAPENTRY32));
            he.dwSize = sizeof(HEAPENTRY32);

            if(Heap32First(&he, fdProcessInfo->dwProcessId, hl.th32HeapID))
            {
                do
                {
                    HEAPINFO heapInfo;
                    memset(&heapInfo, 0, sizeof(heapInfo));
                    heapInfo.addr = he.dwAddress;
                    heapInfo.size = he.dwBlockSize;
                    heapInfo.flags = he.dwFlags;
                    heapList.push_back(heapInfo);

                    he.dwSize = sizeof(HEAPENTRY32);
                }
                while(Heap32Next(&he));
            }
            hl.dwSize = sizeof(HEAPLIST32);
        }
        while(Heap32ListNext(hHeapSnap, &hl));
    }
    else
    {
        return false;
    }

    return true;
    */
    return false;
}