#include "signaturecheck.h"

#include <delayimp.h>
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>

#include <string>
#include <vector>

static wchar_t szApplicationDir[MAX_PATH];
static bool bPerformSignatureChecks = false;

//#define DEBUG_SIGNATURE_CHECKS

#ifdef DEBUG_SIGNATURE_CHECKS
static const wchar_t* gCheckedList[4096];
static size_t gCheckedListSize = 0;

static void debugMessage(const wchar_t* szMessage)
{
    wchar_t finalMessage[512] = L"[signaturecheck] ";
    wcsncat_s(finalMessage, szMessage, _TRUNCATE);
    OutputDebugStringW(finalMessage);
}
#endif // DEBUG_SIGNATURE_CHECKS

#pragma comment(lib, "wintrust")

// Source: https://learn.microsoft.com/en-us/windows/win32/seccrypto/example-c-program--verifying-the-signature-of-a-pe-file
static bool VerifyEmbeddedSignature(LPCWSTR pwszSourceFile, bool checkRevocation)
{
    // Initialize the WINTRUST_FILE_INFO structure.

    WINTRUST_FILE_INFO FileData;
    memset(&FileData, 0, sizeof(FileData));
    FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
    FileData.pcwszFilePath = pwszSourceFile;
    FileData.hFile = NULL;
    FileData.pgKnownSubject = NULL;

    /*
    WVTPolicyGUID specifies the policy to apply on the file
    WINTRUST_ACTION_GENERIC_VERIFY_V2 policy checks:

    1) The certificate used to sign the file chains up to a root
    certificate located in the trusted root certificate store. This
    implies that the identity of the publisher has been verified by
    a certification authority.

    2) In cases where user interface is displayed (which this example
    does not do), WinVerifyTrust will check for whether the
    end entity certificate is stored in the trusted publisher store,
    implying that the user trusts content from this publisher.

    3) The end entity certificate has sufficient permission to sign
    code, as indicated by the presence of a code signing EKU or no
    EKU.
    */

    GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA WinTrustData;

    // Initialize the WinVerifyTrust input data structure.

    // Default all fields to 0.
    memset(&WinTrustData, 0, sizeof(WinTrustData));

    WinTrustData.cbStruct = sizeof(WinTrustData);

    // Use default code signing EKU.
    WinTrustData.pPolicyCallbackData = NULL;

    // No data to pass to SIP.
    WinTrustData.pSIPClientData = NULL;

    // Disable WVT UI.
    WinTrustData.dwUIChoice = WTD_UI_NONE;

    // Check certificate revocations
    WinTrustData.fdwRevocationChecks = checkRevocation ? WTD_REVOKE_WHOLECHAIN : WTD_REVOKE_NONE;

    // Verify an embedded signature on a file.
    WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;

    // Verify action.
    WinTrustData.dwStateAction = WTD_STATEACTION_VERIFY;

    // Verification sets this value.
    WinTrustData.hWVTStateData = NULL;

    // Not used.
    WinTrustData.pwszURLReference = NULL;

    // This is not applicable if there is no UI because it changes
    // the UI to accommodate running applications instead of
    // installing applications.
    WinTrustData.dwUIContext = 0;

    // Set pFile.
    WinTrustData.pFile = &FileData;

    // WinVerifyTrust verifies signatures as specified by the GUID
    // and Wintrust_Data.
    auto lStatus = WinVerifyTrust(
                       NULL,
                       &WVTPolicyGUID,
                       &WinTrustData);

    bool validSignature = lStatus == ERROR_SUCCESS;

    // Any hWVTStateData must be released by a call with close.
    WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;

    auto lStatusClean = WinVerifyTrust(
                            NULL,
                            &WVTPolicyGUID,
                            &WinTrustData);

    SetLastError(lStatus);
    return validSignature;
}

static std::wstring Utf8ToUtf16(const char* str)
{
    std::wstring convertedString;
    if(!str || !*str)
        return convertedString;
    int requiredSize = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
    if(requiredSize > 0)
    {
        convertedString.resize(requiredSize - 1);
        if(!MultiByteToWideChar(CP_UTF8, 0, str, -1, (wchar_t*)convertedString.c_str(), requiredSize))
            convertedString.clear();
    }
    return convertedString;
}

static bool FileExists(const wchar_t* szFullPath)
{
    DWORD attrib = GetFileAttributesW(szFullPath);
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

SIGNATURE_EXPORT HMODULE LoadLibraryCheckedW(const wchar_t* szDll, bool allowFailure)
{
    std::wstring fullDllPath = szApplicationDir;
    fullDllPath += szDll;

#ifdef DEBUG_SIGNATURE_CHECKS
    debugMessage(L"LoadLibraryCheckedW");
    debugMessage(fullDllPath.c_str());
    auto checkedPath = (wchar_t*)malloc(sizeof(wchar_t) * (fullDllPath.size() + 1));
    wcscpy_s(checkedPath, fullDllPath.size() + 1, fullDllPath.c_str());
    gCheckedList[gCheckedListSize++] = checkedPath;
#endif // DEBUG_SIGNATURE_CHECKS

    if(allowFailure && !FileExists(fullDllPath.c_str()))
    {
#ifdef DEBUG_SIGNATURE_CHECKS
        debugMessage(L"^^^ DLL NOT FOUND ^^^");
#endif // DEBUG_SIGNATURE_CHECKS
        SetLastError(ERROR_MOD_NOT_FOUND);
        return 0;
    }

    if(bPerformSignatureChecks)
    {
        auto validSignature = VerifyEmbeddedSignature(fullDllPath.c_str(), false);
        if(!validSignature)
        {
#ifdef DEBUG_SIGNATURE_CHECKS
            wchar_t msg[128] = L"";
            wsprintfW(msg, L"^^^ INVALID SIGNATURE ^^^ -> %08X", GetLastError());
            debugMessage(msg);
#else
            MessageBoxW(nullptr, fullDllPath.c_str(), L"DLL does not have a valid signature", MB_ICONERROR | MB_SYSTEMMODAL);
            ExitProcess(TRUST_E_NOSIGNATURE);
#endif // DEBUG_SIGNATURE_CHECKS
        }
    }

    auto hModule = LoadLibraryW(fullDllPath.c_str());
    auto lastError = GetLastError();
    if(!allowFailure && !hModule)
    {
        MessageBoxW(nullptr, fullDllPath.c_str(), L"DLL not found!", MB_ICONERROR | MB_SYSTEMMODAL);
        ExitProcess(lastError);
    }
    return hModule;
}

SIGNATURE_EXPORT HMODULE LoadLibraryCheckedA(const char* szDll, bool allowFailure)
{
    return LoadLibraryCheckedW(Utf8ToUtf16(szDll).c_str(), allowFailure);
}

// https://devblogs.microsoft.com/oldnewthing/20170126-00/?p=95265
static FARPROC WINAPI delayHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
    // TODO: pre-parse the imports required and make sure there isn't a fake signed DLL being loaded
    if(dliNotify == dliNotePreLoadLibrary)
    {
        if(_stricmp(pdli->szDll, "wintrust.dll") == 0 || _stricmp(pdli->szDll, "user32.dll") == 0)
        {
            return 0;
        }

        return (FARPROC)LoadLibraryCheckedA(pdli->szDll, false);
    }
    return 0;
}

// Visual Studio 2015 Update 3 made this const per default
// https://dev.to/yumetodo/list-of-mscver-and-mscfullver-8nd
#if _MSC_FULL_VER >= 190024210
const
#endif // _MSC_FULL_VER
PfnDliHook __pfnDliNotifyHook2 = delayHook;

#ifdef DEBUG_SIGNATURE_CHECKS
typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef struct _LDR_DLL_UNLOADED_NOTIFICATION_DATA
{
    ULONG Flags;                    //Reserved.
    PUNICODE_STRING FullDllName;    //The full path name of the DLL module.
    PUNICODE_STRING BaseDllName;    //The base file name of the DLL module.
    PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
    ULONG SizeOfImage;              //The size of the DLL image, in bytes.
} LDR_DLL_UNLOADED_NOTIFICATION_DATA, * PLDR_DLL_UNLOADED_NOTIFICATION_DATA;

typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA
{
    ULONG Flags;                    //Reserved.
    PUNICODE_STRING FullDllName;    //The full path name of the DLL module.
    PUNICODE_STRING BaseDllName;    //The base file name of the DLL module.
    PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
    ULONG SizeOfImage;              //The size of the DLL image, in bytes.
} LDR_DLL_LOADED_NOTIFICATION_DATA, * PLDR_DLL_LOADED_NOTIFICATION_DATA;

typedef union _LDR_DLL_NOTIFICATION_DATA
{
    LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
    LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
} LDR_DLL_NOTIFICATION_DATA, * PLDR_DLL_NOTIFICATION_DATA, * const PCLDR_DLL_NOTIFICATION_DATA;

#define LDR_DLL_NOTIFICATION_REASON_LOADED 1
#define LDR_DLL_NOTIFICATION_REASON_UNLOADED 2

VOID CALLBACK LdrDllNotification(
    _In_     ULONG                       NotificationReason,
    _In_     PCLDR_DLL_NOTIFICATION_DATA NotificationData,
    _In_opt_ PVOID                       Context
);

using PLDR_DLL_NOTIFICATION_FUNCTION = decltype(&LdrDllNotification);

NTSTATUS NTAPI LdrRegisterDllNotification(
    _In_     ULONG                          Flags,
    _In_     PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction,
    _In_opt_ PVOID                          Context,
    _Out_    PVOID* Cookie
);

using PLDR_REGISTER_DLL_NOTIFICATION_FUNCTION = decltype(&LdrRegisterDllNotification);

PLDR_REGISTER_DLL_NOTIFICATION_FUNCTION pLdrRegisterDllNotification;

static VOID CALLBACK MyLdrDllNotification(
    _In_     ULONG                       NotificationReason,
    _In_     PCLDR_DLL_NOTIFICATION_DATA NotificationData,
    _In_opt_ PVOID                       Context
)
{
    if(NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED)
    {
        auto buffer = NotificationData->Loaded.FullDllName->Buffer;
        auto length = NotificationData->Loaded.FullDllName->Length;
        if(memcmp(buffer, szApplicationDir, wcslen(szApplicationDir)) == 0)
        {
            auto alreadyChecked = false;
            for(size_t i = 0; i < gCheckedListSize; i++)
            {
                auto checkedPath = gCheckedList[i];
                auto checkedLen = wcslen(checkedPath);
                if(length / 2 == checkedLen && memcmp(checkedPath, buffer, length) == 0)
                {
                    alreadyChecked = true;
                    break;
                }
            }
            if(alreadyChecked)
            {
                debugMessage(L"DllLoadNotification (checked)");
            }
            else
            {
                if(wcsstr(buffer, L"\\plugins\\"))
                    debugMessage(L"DllLoadNotification(plugin)");
                else
                    debugMessage(L"=== UNCHECKED === DllLoadNotification");
            }
            debugMessage(buffer);
        }
    }
}
#endif // DEBUG_SIGNATURE_CHECKS

#define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR    0x00000100
#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#define LOAD_LIBRARY_SEARCH_USER_DIRS       0x00000400
#define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS    0x00001000

typedef BOOL(WINAPI* pfnSetDefaultDllDirectories)(DWORD DirectoryFlags);
typedef BOOL(WINAPI* pfnSetDllDirectoryW)(LPCWSTR lpPathName);
typedef BOOL(WINAPI* pfnAddDllDirectory)(LPCWSTR lpPathName);

static pfnSetDefaultDllDirectories pSetDefaultDllDirectories;
static pfnSetDllDirectoryW pSetDllDirectoryW;
static pfnAddDllDirectory pAddDllDirectory;

bool InitializeSignatureCheck()
{
    if(!GetModuleFileNameW(GetModuleHandleW(nullptr), szApplicationDir, _countof(szApplicationDir)))
        return false;

    std::wstring executablePath = szApplicationDir;
    auto ptr = wcsrchr(szApplicationDir, L'\\');
    if(ptr == nullptr)
        return false;
    ptr[1] = L'\0';

    // Get system directory
    wchar_t szSystemDir[MAX_PATH] = L"";
    GetSystemDirectoryW(szSystemDir, _countof(szSystemDir));

    // This is generically fixing DLL hijacking by using the following search order
    // - System directory
    // - Application directory
    // References:
    // - https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-setdefaultdlldirectories
    // - https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-adddlldirectory
    // - https://medium.com/@1ndahous3/safe-code-pitfalls-dll-side-loading-winapi-and-c-73baaf48bdf5
    // - https://www.bleepingcomputer.com/news/security/microsoft-code-sign-check-bypassed-to-drop-zloader-malware/
    // - https://www.trustedsec.com/blog/object-overloading/
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    pSetDefaultDllDirectories = (pfnSetDefaultDllDirectories)GetProcAddress(hKernel32, "SetDefaultDllDirectories");
    pSetDllDirectoryW = (pfnSetDllDirectoryW)GetProcAddress(hKernel32, "SetDllDirectoryW");
    pAddDllDirectory = (pfnAddDllDirectory)GetProcAddress(hKernel32, "AddDllDirectory");
    if(pSetDefaultDllDirectories && pSetDllDirectoryW && pAddDllDirectory)
    {
        if(!pSetDllDirectoryW(L""))
            return false;

        // Thanks to daax for finding this order
        // > If more than one directory has been added, the order in which those directories are searched is unspecified.
        // Reversing ntdll shows that it is using a singly-linked list, so the order is the opposite in which you add it
        // Wine: https://github.com/wine-mirror/wine/blob/e796002ee61bf5dfb2718e8f4fb8fa928ccdc236/dlls/ntdll/loader.c#L4423-L4462
        if(!pAddDllDirectory(szApplicationDir))
            return false;
        if(!pAddDllDirectory(szSystemDir))
            return false;

        if(!pSetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_USER_DIRS))
            return false;
    }
    else
    {
        auto loadSystemDll = [&szSystemDir](const wchar_t* szName)
        {
            std::wstring fullDllPath;
            fullDllPath = szSystemDir;
            fullDllPath += L'\\';
            fullDllPath += szName;
            LoadLibraryW(fullDllPath.c_str());
        };

        // At least prevent DLL hijacking for wintrust.dll on XP/Old 7
        loadSystemDll(L"msasn1.dll");
        loadSystemDll(L"cryptsp.dll");
        loadSystemDll(L"cryptbase.dll");
        loadSystemDll(L"wintrust.dll");
        loadSystemDll(L"rsaenh.dll");
        loadSystemDll(L"psapi.dll");
    }

    bPerformSignatureChecks = VerifyEmbeddedSignature(executablePath.c_str(), false);

#ifdef DEBUG_SIGNATURE_CHECKS
    bPerformSignatureChecks = true;
    pLdrRegisterDllNotification = (PLDR_REGISTER_DLL_NOTIFICATION_FUNCTION)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrRegisterDllNotification");
    if(pLdrRegisterDllNotification == nullptr)
        return true;
    PVOID Cookie;
    auto status = pLdrRegisterDllNotification(0, MyLdrDllNotification, nullptr, &Cookie);
    if(status != 0)
        return false;
#endif // DEBUG_SIGNATURE_CHECKS

    if(bPerformSignatureChecks)
    {
        // Safely load the MSVC runtime DLLs (since they cannot be delay loaded)
        auto loadRuntimeDll = [](const wchar_t* szDll)
        {
            std::wstring fullDllPath = szApplicationDir;
            fullDllPath += L'\\';
            fullDllPath += szDll;
            if(FileExists(fullDllPath.c_str()))
                LoadLibraryCheckedW(szDll, true);
            else
                LoadLibraryW(szDll);
        };
        loadRuntimeDll(L"msvcr120.dll");
        loadRuntimeDll(L"msvcp120.dll");
    }

    return true;
}