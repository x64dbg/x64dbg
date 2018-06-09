#include "handles.h"
#include "ntdll/ntdll.h"
#include "exception.h"
#include "debugger.h"
#include <functional>

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
    Memory<PSYSTEM_HANDLE_INFORMATION> HandleInformation(16 * 1024, "_dbg_enumhandles");
    NTSTATUS ErrorCode = ERROR_SUCCESS;
    for(;;)
    {
        ErrorCode = NtQuerySystemInformation(SystemHandleInformation, HandleInformation(), ULONG(HandleInformation.size()), nullptr);
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
    HANDLE hLocalHandle;
    if(DuplicateHandle(hProcess, remoteHandle, GetCurrentProcess(), &hLocalHandle, 0, FALSE, DUPLICATE_SAME_ACCESS)) //Needs privileges for PID/TID retrival
    {
        ULONG ReturnSize = 0;
        if(NtQueryObject(hLocalHandle, ObjectTypeInformation, nullptr, 0, &ReturnSize) == STATUS_INFO_LENGTH_MISMATCH)
        {
            ReturnSize += 0x2000;
            Memory<OBJECT_TYPE_INFORMATION*> objectTypeInfo(ReturnSize + sizeof(WCHAR) * 16, "_dbg_gethandlename:objectTypeInfo");
            if(NtQueryObject(hLocalHandle, ObjectTypeInformation, objectTypeInfo(), ReturnSize, nullptr) == STATUS_SUCCESS)
                typeName = StringUtils::Utf16ToUtf8(objectTypeInfo()->TypeName.Buffer);
        }

        std::function<void()> getName = [&]()
        {
            if(NtQueryObject(hLocalHandle, ObjectNameInformation, nullptr, 0, &ReturnSize) == STATUS_INFO_LENGTH_MISMATCH)
            {
                ReturnSize += 0x2000;
                Memory<OBJECT_NAME_INFORMATION*> objectNameInfo(ReturnSize + sizeof(WCHAR) * 16, "_dbg_gethandlename:objectNameInfo");
                if(NtQueryObject(hLocalHandle, ObjectNameInformation, objectNameInfo(), ReturnSize, nullptr) == STATUS_SUCCESS)
                    name = StringUtils::Utf16ToUtf8(objectNameInfo()->Name.Buffer);
            }
        };

        name.clear();
        if(strcmp(typeName.c_str(), "Process") == 0)
        {
            DWORD PID = GetProcessId(hLocalHandle); //Windows XP SP1
            if(PID > 0)
                name = StringUtils::sprintf("PID = %X", PID);
        }
        else if(strcmp(typeName.c_str(), "Thread") == 0)
        {
            DWORD TID = 0;
            DWORD PID = 0;
            DWORD(__stdcall * pGetThreadId)(HANDLE);
            DWORD(__stdcall * pGetProcessIdOfThread)(HANDLE);
            pGetThreadId = (DWORD(__stdcall*)(HANDLE))GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetThreadId");
            pGetProcessIdOfThread = (DWORD(__stdcall*)(HANDLE))GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetProcessIdOfThread");
            if(pGetThreadId != NULL && pGetProcessIdOfThread != NULL)
            {
                TID = pGetThreadId(hLocalHandle); //Vista or Server 2003 only
                PID = pGetProcessIdOfThread(hLocalHandle); //Vista or Server 2003 only
            }
            else //Windows XP
            {
                THREAD_BASIC_INFORMATION threadInfo;
                ULONG threadInfoSize = 0;
                NTSTATUS isok = NtQueryInformationThread(hLocalHandle, ThreadBasicInformation, &threadInfo, sizeof(threadInfo), &threadInfoSize);
                if(NT_SUCCESS(isok))
                {
                    TID = (DWORD)threadInfo.ClientId.UniqueThread;
                    PID = (DWORD)threadInfo.ClientId.UniqueProcess;
                }
            }
            if(TID > 0 && PID > 0)
                name = StringUtils::sprintf("TID = %X, PID = %X", TID, PID);
        }
        if(name.empty())
        {
            HANDLE hThread;
            hThread = CreateThread(nullptr, 0, getNameThread, &getName, 0, nullptr);
            auto result = WaitForSingleObject(hThread, 200);
            if(result != WAIT_OBJECT_0)
            {
                TerminateThread(hThread, 0);
                name = String(ErrorCodeToName(result));
            }
            else
                CloseHandle(hThread);
        }
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