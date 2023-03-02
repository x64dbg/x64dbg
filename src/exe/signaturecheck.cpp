#include <Windows.h>
#include <delayimp.h>
#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>

#include <string>

static wchar_t szModulePath[MAX_PATH];
static bool bPerformSignatureChecks = false;

// TODO: look at hijacking of wintrust (also old vulnerabilities)
// - https://www.bleepingcomputer.com/news/security/microsoft-code-sign-check-bypassed-to-drop-zloader-malware/
// - https://www.trustedsec.com/blog/object-overloading/
#pragma comment (lib, "wintrust")

// Source: https://learn.microsoft.com/en-us/windows/win32/seccrypto/example-c-program--verifying-the-signature-of-a-pe-file
bool VerifyEmbeddedSignature(LPCWSTR pwszSourceFile, bool checkRevocation = true)
{
    LONG lStatus;
    DWORD dwLastError;

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
    lStatus = WinVerifyTrust(
                  NULL,
                  &WVTPolicyGUID,
                  &WinTrustData);

    bool validSignature = false;
    switch(lStatus)
    {
    case ERROR_SUCCESS:
        /*
        Signed file:
            - Hash that represents the subject is trusted.

            - Trusted publisher without any verification errors.

            - UI was disabled in dwUIChoice. No publisher or
                time stamp chain errors.

            - UI was enabled in dwUIChoice and the user clicked
                "Yes" when asked to install and run the signed
                subject.
        */
        wprintf_s(L"The file \"%s\" is signed and the signature "
                  L"was verified.\n",
                  pwszSourceFile);
        validSignature = true;
        break;

    case TRUST_E_NOSIGNATURE:
        // The file was not signed or had a signature
        // that was not valid.

        // Get the reason for no signature.
        dwLastError = GetLastError();
        if(TRUST_E_NOSIGNATURE == dwLastError ||
                TRUST_E_SUBJECT_FORM_UNKNOWN == dwLastError ||
                TRUST_E_PROVIDER_UNKNOWN == dwLastError)
        {
            // The file was not signed.
            wprintf_s(L"The file \"%s\" is not signed.\n",
                      pwszSourceFile);
        }
        else
        {
            // The signature was not valid or there was an error
            // opening the file.
            wprintf_s(L"An unknown error occurred trying to "
                      L"verify the signature of the \"%s\" file.\n",
                      pwszSourceFile);
        }

        break;

    case TRUST_E_EXPLICIT_DISTRUST:
        // The hash that represents the subject or the publisher
        // is not allowed by the admin or user.
        wprintf_s(L"The signature is present, but specifically "
                  L"disallowed.\n");
        break;

    case TRUST_E_SUBJECT_NOT_TRUSTED:
        // The user clicked "No" when asked to install and run.
        wprintf_s(L"The signature is present, but not "
                  L"trusted.\n");
        break;

    case CRYPT_E_SECURITY_SETTINGS:
        /*
        The hash that represents the subject or the publisher
        was not explicitly trusted by the admin and the
        admin policy has disabled user trust. No signature,
        publisher or time stamp errors.
        */
        wprintf_s(L"CRYPT_E_SECURITY_SETTINGS - The hash "
                  L"representing the subject or the publisher wasn't "
                  L"explicitly trusted by the admin and admin policy "
                  L"has disabled user trust. No signature, publisher "
                  L"or timestamp errors.\n");
        break;

    default:
        // The UI was disabled in dwUIChoice or the admin policy
        // has disabled user trust. lStatus contains the
        // publisher or time stamp chain error.
        wprintf_s(L"Error is: 0x%x.\n",
                  lStatus);
        break;
    }

    // Any hWVTStateData must be released by a call with close.
    WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;

    lStatus = WinVerifyTrust(
                  NULL,
                  &WVTPolicyGUID,
                  &WinTrustData);

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

// https://devblogs.microsoft.com/oldnewthing/20170126-00/?p=95265
static FARPROC WINAPI delayHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
    // TODO: pre-parse the imports required and make sure there isn't a fake DLL being loaded
    if(dliNotify == dliNotePreLoadLibrary)
    {
        std::wstring fullDllPath = szModulePath;
        fullDllPath += L'\\';
        fullDllPath += Utf8ToUtf16(pdli->szDll);
        MessageBoxW(nullptr, fullDllPath.c_str(), L"dliNotePreLoadLibrary", MB_ICONERROR);
        if(bPerformSignatureChecks)
        {
            auto validSignature = VerifyEmbeddedSignature(fullDllPath.c_str());
            if(!validSignature)
            {
                MessageBoxW(nullptr, fullDllPath.c_str(), L"DLL does not have a valid signature", MB_ICONERROR | MB_SYSTEMMODAL);
                ExitProcess(TRUST_E_NOSIGNATURE);
            }
        }
        auto hModule = ::LoadLibraryW(fullDllPath.c_str());
        if(!hModule)
        {
            MessageBoxW(nullptr, fullDllPath.c_str(), L"DLL not found!", MB_ICONERROR | MB_SYSTEMMODAL);
            ExitProcess(ERROR_MOD_NOT_FOUND);
        }
        return (FARPROC)hModule;
    }
    return 0;
}

// Visual Studio 2015 Update 3 made this const per default
// https://dev.to/yumetodo/list-of-mscver-and-mscfullver-8nd
#if _MSC_FULL_VER >= 190024210
const
#endif // _MSC_FULL_VER
PfnDliHook __pfnDliNotifyHook2 = delayHook;

bool InitializeSignatureCheck()
{
    if(!GetModuleFileNameW(GetModuleHandleW(nullptr), szModulePath, _countof(szModulePath)))
        return false;
    bPerformSignatureChecks = VerifyEmbeddedSignature(szModulePath);
    bPerformSignatureChecks = true; // TODO: remove this
    auto ptr = wcsrchr(szModulePath, L'\\');
    if(ptr == nullptr)
        return false;
    *ptr = L'\0';
    // TODO: check signature
    // https://learn.microsoft.com/en-us/windows/win32/seccrypto/example-c-program--verifying-the-signature-of-a-pe-file
    return true;
}