#include "ntdll/ntdll.h"
#include <functional>
#include <algorithm>
#include "handles.h"
#include "exception.h"
#include "debugger.h"
#include "thread.h"
#include "threading.h"

static std::unordered_map<UCHAR, String> HandleTypeNames;
static std::unordered_map<HANDLE, String> HandleTypeCache;

static void HandleTypesEnum()
{
    Memory<POBJECT_TYPES_INFORMATION> TypesInformation(16 * 1024, "types");
    NTSTATUS status = STATUS_SUCCESS;
    for(;;)
    {
        status = NtQueryObject(nullptr, ObjectTypesInformation, TypesInformation(), ULONG(TypesInformation.size()), nullptr);
        if(status != STATUS_INFO_LENGTH_MISMATCH)
            break;
        TypesInformation.realloc(TypesInformation.size() * 2, "types");
    }
    if(status != STATUS_SUCCESS)
        return;

    auto TypeInfo = TypesInformation()->TypeInformation;
    HandleTypeNames.reserve(TypesInformation()->NumberOfTypes);
    for(ULONG i = 0; i < TypesInformation()->NumberOfTypes; i++)
    {
        auto wtypeName = WString(TypeInfo->TypeName.Buffer, TypeInfo->TypeName.Buffer + TypeInfo->TypeName.Length / 2);
        auto typeName = StringUtils::Utf16ToUtf8(wtypeName);
        auto typeIndex = i + 1;
        if(BridgeGetNtBuildNumber() >= 9600 /* Windows 8.1 */)
        {
            typeIndex = TypeInfo->TypeIndex;
        }
        HandleTypeNames.emplace((UCHAR)typeIndex, typeName);
        TypeInfo = (POBJECT_TYPE_INFORMATION)((char*)(TypeInfo + 1) + ALIGN_UP(TypeInfo->TypeName.MaximumLength, ULONG_PTR));
    }
}

// Enumerate all handles in the debuggee
bool HandlesEnum(std::vector<HANDLEINFO> & handles)
{
    if(HandleTypeNames.empty())
        HandleTypesEnum();

    duint pid;
    Memory<PSYSTEM_HANDLE_INFORMATION> HandleInformation(16 * 1024, "_dbg_enumhandles");
    NTSTATUS ErrorCode = ERROR_SUCCESS;
    pid = fdProcessInfo->dwProcessId;
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

    EXCLUSIVE_ACQUIRE(LockHandleCache);
    HandleTypeCache.clear();
    HANDLEINFO info;
    for(ULONG i = 0; i < HandleInformation()->NumberOfHandles; i++)
    {
        const auto & handle = HandleInformation()->Handles[i];
        if(handle.UniqueProcessId != pid)
            continue;
        info.Handle = handle.HandleValue;
        info.TypeNumber = handle.ObjectTypeIndex;
        info.GrantedAccess = handle.GrantedAccess;
        auto typeNameItr = HandleTypeNames.find(handle.ObjectTypeIndex);
        if(typeNameItr != HandleTypeNames.end())
            HandleTypeCache.emplace((HANDLE)handle.HandleValue, typeNameItr->second);
        handles.push_back(info);
    }
    return true;
}

static DWORD WINAPI getNameThread(LPVOID lpParam)
{
    (*(std::function<void()>*)lpParam)();
    return 0;
}

static String getProcessName(DWORD PID)
{
    wchar_t processName[MAX_PATH];
    std::string processNameUtf8;
    HANDLE hPIDProcess;
    hPIDProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, PID);
    if(hPIDProcess != NULL)
    {
        if(GetProcessImageFileNameW(hPIDProcess, processName, _countof(processName)) > 0)
        {
            processNameUtf8 = StringUtils::Utf16ToUtf8(processName);
        }
        CloseHandle(hPIDProcess);
    }
    return processNameUtf8;
}

// Get the name of a handle of debuggee
bool HandlesGetName(HANDLE remoteHandle, String & name, String & typeName)
{
    HANDLE hProcess = fdProcessInfo->hProcess;
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
            if(PID == 0) //The first time could fail because the process didn't specify query permissions.
            {
                HANDLE hLocalQueryHandle;
                if(DuplicateHandle(hProcess, remoteHandle, GetCurrentProcess(), &hLocalQueryHandle, PROCESS_QUERY_INFORMATION, FALSE, 0))
                {
                    PID = GetProcessId(hLocalQueryHandle);
                    CloseHandle(hLocalQueryHandle);
                }
            }

            if(PID > 0)
            {
                if(PID == fdProcessInfo->dwProcessId)
                {
                    name = StringUtils::sprintf("PID: %s (%s)", formatpidtid(PID).c_str(), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Debuggee")));
                }
                else
                {
                    std::string processName = getProcessName(PID);
                    if(processName.size() > 0)
                        name = StringUtils::sprintf("PID: %s (%s)", formatpidtid(PID).c_str(), processName.c_str());
                    else
                        name = StringUtils::sprintf("PID: %s", formatpidtid(PID).c_str());
                }
            }
        }
        else if(strcmp(typeName.c_str(), "Thread") == 0)
        {
            auto getTidPid = [](HANDLE hThread, DWORD & TID, DWORD & PID)
            {
                static auto pGetThreadId = (DWORD(__stdcall*)(HANDLE))GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetThreadId");
                static auto pGetProcessIdOfThread = (DWORD(__stdcall*)(HANDLE))GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetProcessIdOfThread");
                if(pGetThreadId != NULL && pGetProcessIdOfThread != NULL) //Vista or Server 2003 only
                {
                    TID = pGetThreadId(hThread);
                    PID = pGetProcessIdOfThread(hThread);
                }
                else //Windows XP
                {
                    THREAD_BASIC_INFORMATION threadInfo;
                    ULONG threadInfoSize = 0;
                    NTSTATUS isok = NtQueryInformationThread(hThread, ThreadBasicInformation, &threadInfo, sizeof(threadInfo), &threadInfoSize);
                    if(NT_SUCCESS(isok))
                    {
                        TID = (DWORD)(duint)threadInfo.ClientId.UniqueThread;
                        PID = (DWORD)(duint)threadInfo.ClientId.UniqueProcess;
                    }
                }
            };

            DWORD TID, PID;
            getTidPid(hLocalHandle, TID, PID);
            if(TID == 0 || PID == 0) //The first time could fail because the process didn't specify query permissions.
            {
                HANDLE hLocalQueryHandle;
                if(DuplicateHandle(hProcess, remoteHandle, GetCurrentProcess(), &hLocalQueryHandle, THREAD_QUERY_INFORMATION, FALSE, 0))
                {
                    getTidPid(hLocalQueryHandle, TID, PID);
                    CloseHandle(hLocalQueryHandle);
                }
            }

            if(TID > 0 && PID > 0)
            {
                // Check if the thread is in the debuggee
                if(PID == fdProcessInfo->dwProcessId)
                {
                    char ThreadName[MAX_THREAD_NAME_SIZE];
                    if(ThreadGetName(TID, ThreadName) && ThreadName[0] != 0)
                        name = StringUtils::sprintf("TID: %s (%s), PID: %s (%s)", formatpidtid(TID).c_str(), ThreadName, formatpidtid(PID).c_str(), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Debuggee")));
                    else
                        name = StringUtils::sprintf("TID: %s, PID: %s (%s)", formatpidtid(TID).c_str(), ThreadName, formatpidtid(PID).c_str(), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Debuggee")));
                }
                else
                {
                    std::string processName = getProcessName(PID);
                    if(processName.size() > 0)
                        name = StringUtils::sprintf("TID: %s, PID: %s (%s)", formatpidtid(TID).c_str(), formatpidtid(PID).c_str(), processName.c_str());
                    else
                        name = StringUtils::sprintf("TID: %s, PID: %s", formatpidtid(TID).c_str(), formatpidtid(PID).c_str());
                }
            }
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

    if(typeName.empty())
    {
        SHARED_ACQUIRE(LockHandleCache);
        auto itr = HandleTypeCache.find(remoteHandle);
        if(itr != HandleTypeCache.end())
            typeName = itr->second;
    }
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
    duint proc1, proc2;
    proc1 = GetClassLongPtrW(hWnd, GCLP_WNDPROC);
    proc2 = GetClassLongPtrA(hWnd, GCLP_WNDPROC);
    if(!DbgMemIsValidReadPtr(proc1))
        info.wndProc = proc2;
    else if(!DbgMemIsValidReadPtr(proc2))
        info.wndProc = proc1;
    else if(IsWindowUnicode(hWnd))
        info.wndProc = proc1;
    else
        info.wndProc = proc2;
    if(DbgFunctions()->ModGetParty(info.wndProc) != 0 || !DbgMemIsValidReadPtr(info.wndProc))
    {
        duint dlgproc1, dlgproc2;
        dlgproc1 = GetClassLongPtrW(hWnd, DWLP_DLGPROC);
        dlgproc2 = GetClassLongPtrA(hWnd, DWLP_DLGPROC);
        if(!DbgMemIsValidReadPtr(dlgproc1))
            dlgproc1 = dlgproc2;
        if(DbgMemIsValidReadPtr(dlgproc1))
        {
            info.wndProc = dlgproc1;
        }
    }
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
    memcpy(info.windowTitle, UTF8WindowTitle.c_str(), std::min(UTF8WindowTitle.size(), sizeof(info.windowTitle))); //Copy window title with repect to buffer size constraints
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
    memcpy(info.windowClass, UTF8WindowTitle.c_str(), std::min(UTF8WindowTitle.size(), sizeof(info.windowClass))); //Copy window class with repect to buffer size constraints
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

String LoadedAntiCheatDrivers()
{
    Memory<RTL_PROCESS_MODULES*> HandleInformation(0x1000, __FUNCTION__);
    NTSTATUS ErrorCode = ERROR_SUCCESS;
    for(;;)
    {
        ErrorCode = NtQuerySystemInformation(SystemModuleInformation, HandleInformation(), ULONG(HandleInformation.size()), nullptr);
        if(ErrorCode != STATUS_INFO_LENGTH_MISMATCH)
            break;
        HandleInformation.realloc(HandleInformation.size() * 2, __FUNCTION__);
    }
    if(ErrorCode != STATUS_SUCCESS)
        return {};
    const char* AntiCheatDrivers[] =
    {
        "EasyAntiCheat.sys",
        "EasyAntiCheat_EOS.sys",
    };
    std::unordered_set<String> DriverSet;
    for(auto & Driver : AntiCheatDrivers)
        DriverSet.insert(StringUtils::ToLower(Driver));
    String Result;
    auto Modules = HandleInformation();
    for(ULONG i = 0; i < Modules->NumberOfModules; i++)
    {
        const auto & Module = Modules->Modules[i];
        String DriverName = (char*)Module.FullPathName + Module.OffsetToFileName;
        if(DriverSet.count(StringUtils::ToLower(DriverName)))
        {
            if(!Result.empty())
                Result += '\n';
            Result += DriverName;
        }
    }
    return Result;
}