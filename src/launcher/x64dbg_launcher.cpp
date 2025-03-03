#define UNICODE
#include <stdio.h>
#include <windows.h>
#include <string>
#include <queue>
#include <shlwapi.h>
#include <objbase.h>
#pragma warning(push)
#pragma warning(disable:4091)
#include <shlobj.h>
#pragma warning(pop)
#include <atlcomcli.h>

#include "../exe/resource.h"
#include "../exe/LoadResourceString.h"
#include "../exe/icon.h"
#include "../dbg/GetPeArch.h"

static bool FileExists(const TCHAR* file)
{
    auto attrib = GetFileAttributes(file);
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

static bool BrowseFileOpen(HWND owner, const TCHAR* filter, const TCHAR* defext, TCHAR* filename, int filename_size, const TCHAR* init_dir)
{
    OPENFILENAME ofstruct;
    memset(&ofstruct, 0, sizeof(ofstruct));
    ofstruct.lStructSize = sizeof(ofstruct);
    ofstruct.hwndOwner = owner;
    ofstruct.hInstance = GetModuleHandleW(nullptr);
    ofstruct.lpstrFilter = filter;
    ofstruct.lpstrFile = filename;
    ofstruct.nMaxFile = filename_size - 1;
    ofstruct.lpstrInitialDir = init_dir;
    ofstruct.lpstrDefExt = defext;
    ofstruct.Flags = OFN_EXTENSIONDIFFERENT | OFN_HIDEREADONLY | OFN_NONETWORKBUTTON;
    return !!GetOpenFileName(&ofstruct);
}

typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
typedef BOOL(WINAPI* LPFN_Wow64DisableWow64FsRedirection)(PVOID);
typedef BOOL(WINAPI* LPFN_Wow64RevertWow64FsRedirection)(PVOID);

LPFN_Wow64DisableWow64FsRedirection _Wow64DisableRedirection = NULL;
LPFN_Wow64RevertWow64FsRedirection _Wow64RevertRedirection = NULL;

static BOOL isWoW64()
{
    BOOL isWoW64 = FALSE;

    static auto fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

    if(NULL != fnIsWow64Process)
    {
        if(!fnIsWow64Process(GetCurrentProcess(), &isWoW64))
        {
            return FALSE;
        }
    }
    return isWoW64;
}

static BOOL isWowRedirectionSupported()
{
    BOOL bRedirectSupported = FALSE;

    _Wow64DisableRedirection = (LPFN_Wow64DisableWow64FsRedirection)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Wow64DisableWow64FsRedirection");
    _Wow64RevertRedirection = (LPFN_Wow64RevertWow64FsRedirection)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Wow64RevertWow64FsRedirection");

    if(!_Wow64DisableRedirection || !_Wow64RevertRedirection)
        return bRedirectSupported;
    else
        return !bRedirectSupported;
}

struct RedirectWow
{
    PVOID oldValue = NULL;

    bool DisableRedirect()
    {
        return !!_Wow64DisableRedirection(&oldValue);
    }

    ~RedirectWow()
    {
        if(oldValue != NULL)
        {
            if(!_Wow64RevertRedirection(oldValue))
                //Error occurred here. Ignore or reset? (does it matter at this point?)
                MessageBox(nullptr, TEXT("Error in Reverting Redirection"), TEXT("Error"), MB_OK | MB_ICONERROR);
        }
    }
};

static TCHAR* GetDesktopPath()
{
    static TCHAR path[MAX_PATH + 1];
    if(SHGetSpecialFolderPath(HWND_DESKTOP, path, CSIDL_DESKTOPDIRECTORY, FALSE))
        return path;
    return nullptr;
}

static HRESULT AddDesktopShortcut(TCHAR* szPathOfFile, const TCHAR* szNameOfLink)
{
    HRESULT hRes = NULL;

    //Get the working directory
    TCHAR pathFile[MAX_PATH + 1];
    _tcscpy_s(pathFile, szPathOfFile);
    PathRemoveFileSpec(pathFile);

    CComPtr<IShellLink> psl;
    hRes = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if(SUCCEEDED(hRes))
    {
        CComPtr<IPersistFile> ppf;

        psl->SetPath(szPathOfFile);
        psl->SetDescription(LoadResString(IDS_SHORTCUTDESC));
        psl->SetIconLocation(szPathOfFile, 0);
        psl->SetWorkingDirectory(pathFile);

        hRes = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if(SUCCEEDED(hRes))
        {
            TCHAR path[MAX_PATH + 1] = TEXT("");
            _tmakepath_s(path, nullptr, GetDesktopPath(), szNameOfLink, TEXT("lnk"));
            CComBSTR tmp(path);
            hRes = ppf->Save(tmp, TRUE);
        }
    }
    return hRes;
}

static HRESULT RemoveDesktopShortcut(const TCHAR* szNameOfLink)
{
    HRESULT hRes = S_OK;

    TCHAR szDesktopPath[MAX_PATH + 1] = TEXT("");
    _tmakepath_s(szDesktopPath, nullptr, GetDesktopPath(), szNameOfLink, TEXT("lnk"));

    if(FileExists(szDesktopPath))
    {
        if(!DeleteFile(szDesktopPath))
        {
            MessageBox(nullptr, TEXT("Failed to delete the desktop shortcut."), TEXT("Error"), MB_ICONERROR);
            hRes = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        hRes = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    return hRes;
}

static bool RegisterShellExtension(const TCHAR* key, const TCHAR* command)
{
    HKEY hKey;
    auto result = true;
    if(RegCreateKey(HKEY_CLASSES_ROOT, key, &hKey) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGCREATEKEYFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
        return false;
    }
    if(RegSetValueEx(hKey, nullptr, 0, REG_EXPAND_SZ, LPBYTE(command), (_tcslen(command) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGSETVALUEEXFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
        result = false;
    }
    RegCloseKey(hKey);
    return result;
}

static bool UnregisterShellExtension(const TCHAR* key)
{
    if(SHDeleteKey(HKEY_CLASSES_ROOT, key) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGDELETEKEYFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR | MB_OK);
        return false;
    }
    return true;
}

static void AddShellIcon(const TCHAR* key, const TCHAR* icon, const TCHAR* title)
{
    HKEY pKey;
    if(RegOpenKeyEx(HKEY_CLASSES_ROOT, key, 0, KEY_ALL_ACCESS, &pKey) != ERROR_SUCCESS)
        MessageBox(nullptr, LoadResString(IDS_REGOPENKEYFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
    if(RegSetValueEx(pKey, L"Icon", 0, REG_SZ, LPBYTE(icon), (_tcslen(icon) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS)
        MessageBox(nullptr, LoadResString(IDS_REGSETVALUEEXFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
    if(RegSetValueEx(pKey, nullptr, 0, REG_SZ, LPBYTE(title), (_tcslen(title) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS)
        MessageBox(nullptr, LoadResString(IDS_REGSETVALUEEXFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
    RegCloseKey(pKey);
}

static void CreateUnicodeFile(const TCHAR* file)
{
    //Taken from: http://www.codeproject.com/Articles/9071/Using-Unicode-in-INI-files
    if(FileExists(file))
        return;

    // UTF16-LE BOM(FFFE)
    WORD wBOM = 0xFEFF;
    auto hFile = CreateFile(file, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if(hFile == INVALID_HANDLE_VALUE)
        return;
    DWORD written = 0;
    WriteFile(hFile, &wBOM, sizeof(WORD), &written, nullptr);
    CloseHandle(hFile);
}

//Taken from: http://www.cplusplus.com/forum/windows/64088/
static bool ResolveShortcut(HWND hwnd, const TCHAR* szShortcutPath, TCHAR* szResolvedPath, size_t nSize)
{
    if(!szResolvedPath)
        return SUCCEEDED(E_INVALIDARG);

    //Get a pointer to the IShellLink interface.
    CComPtr<IShellLink> psl;
    auto hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if(SUCCEEDED(hres))
    {
        //Get a pointer to the IPersistFile interface.
        CComPtr<IPersistFile> ppf;
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if(SUCCEEDED(hres))
        {
            //Load the shortcut.
            CComBSTR tmp(szShortcutPath);
            hres = ppf->Load(tmp, STGM_READ);

            if(SUCCEEDED(hres))
            {
                //Resolve the link.
                hres = psl->Resolve(hwnd, 0);

                if(SUCCEEDED(hres))
                {
                    //Get the path to the link target.
                    TCHAR szGotPath[MAX_PATH] = { 0 };
                    hres = psl->GetPath(szGotPath, _countof(szGotPath), nullptr, SLGP_SHORTPATH);

                    if(SUCCEEDED(hres))
                    {
                        _tcscpy_s(szResolvedPath, nSize, szGotPath);
                    }
                }
            }
        }
    }
    return SUCCEEDED(hres);
}

static void AddDBFileTypeIcon(TCHAR* sz32Path, TCHAR* sz64Path)
{
    HKEY hKeyCreatedx32;
    HKEY hKeyCreatedx64;
    HKEY hKeyCreatedIconx32;
    HKEY hKeyCreatedIconx64;
    LPCWSTR dbx32key = L".dd32";
    LPCWSTR dbx64key = L".dd64";
    LPCWSTR db_desc = L"x64dbg_db";

    // file type key created
    if(RegCreateKey(HKEY_CLASSES_ROOT, dbx32key, &hKeyCreatedx32) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGCREATEKEYFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
        return;
    }
    if(RegCreateKey(HKEY_CLASSES_ROOT, dbx64key, &hKeyCreatedx64) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGCREATEKEYFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
        return;
    }

    // file type desc
    if(RegSetValueEx(hKeyCreatedx32, nullptr, 0, REG_SZ, LPBYTE(db_desc), (_tcslen(db_desc) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGSETVALUEEXFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
        return;
    }
    if(RegSetValueEx(hKeyCreatedx64, nullptr, 0, REG_SZ, LPBYTE(db_desc), (_tcslen(db_desc) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGSETVALUEEXFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
        return;
    }

    // file type key icon created
    if(RegCreateKey(hKeyCreatedx32, L"DefaultIcon", &hKeyCreatedIconx32) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGCREATEKEYFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
        return;
    }
    if(RegCreateKey(hKeyCreatedx64, L"DefaultIcon", &hKeyCreatedIconx64) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGCREATEKEYFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
        return;
    }

    // file type key icon path
    if(RegSetValueEx(hKeyCreatedIconx32, nullptr, 0, REG_SZ, LPBYTE(sz32Path), (_tcslen(sz32Path) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGSETVALUEEXFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
        return;
    }
    if(RegSetValueEx(hKeyCreatedIconx64, nullptr, 0, REG_SZ, LPBYTE(sz64Path), (_tcslen(sz64Path) + 1) * sizeof(TCHAR)) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGSETVALUEEXFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
        return;
    }

    RegCloseKey(hKeyCreatedx32);
    RegCloseKey(hKeyCreatedx64);
    RegCloseKey(hKeyCreatedIconx32);
    RegCloseKey(hKeyCreatedIconx64);

    // refresh icons cache
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    return;
}

static void RemoveDBFileTypeIcon()
{
    LPCWSTR dbx32key = L".dd32";
    LPCWSTR dbx64key = L".dd64";

    if(RegDeleteKey(HKEY_CLASSES_ROOT, L".dd32\\DefaultIcon") != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGDELETEKEYFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
    }

    if(RegDeleteKey(HKEY_CLASSES_ROOT, dbx32key) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGDELETEKEYFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
    }

    if(RegDeleteKey(HKEY_CLASSES_ROOT, L".dd64\\DefaultIcon") != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGDELETEKEYFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
    }

    if(RegDeleteKey(HKEY_CLASSES_ROOT, dbx64key) != ERROR_SUCCESS)
    {
        MessageBox(nullptr, LoadResString(IDS_REGDELETEKEYFAIL), LoadResString(IDS_ASKADMIN), MB_ICONERROR);
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

static TCHAR szApplicationDir[MAX_PATH] = TEXT("");
static TCHAR szCurrentDir[MAX_PATH] = TEXT("");
static TCHAR sz32Path[MAX_PATH] = TEXT("");
static TCHAR sz32Dir[MAX_PATH] = TEXT("");
static TCHAR sz64Path[MAX_PATH] = TEXT("");
static TCHAR sz64Dir[MAX_PATH] = TEXT("");

static void restartInstall()
{
    OSVERSIONINFO osvi;
    memset(&osvi, 0, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);
    auto operation = osvi.dwMajorVersion >= 6 ? TEXT("runas") : TEXT("open");
    ShellExecute(nullptr, operation, szApplicationDir, TEXT("::install"), szCurrentDir, SW_SHOWNORMAL);
}

static INT_PTR CALLBACK DlgLauncher(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
        HANDLE hIcon;
        hIcon = LoadIconW(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON1));
        SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SetDlgItemText(hwndDlg, IDC_BUTTONINSTALL, LoadResString(IDS_SETUP));
        EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON32), *sz32Dir != TEXT('\0'));
        EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON64), *sz64Dir != TEXT('\0') && isWoW64());
    }
    return TRUE;

    case WM_CLOSE:
    {
        EndDialog(hwndDlg, 0);
    }
    return TRUE;

    case WM_COMMAND:
    {
        switch(LOWORD(wParam))
        {
        case IDC_BUTTON32:
        {
            EndDialog(hwndDlg, 0);
            ShellExecute(nullptr, TEXT("open"), sz32Path, TEXT(""), sz32Dir, SW_SHOWNORMAL);
        }
        return TRUE;

        case IDC_BUTTON64:
        {
            EndDialog(hwndDlg, 0);
            ShellExecute(nullptr, TEXT("open"), sz64Path, TEXT(""), sz64Dir, SW_SHOWNORMAL);
        }
        return TRUE;

        case IDC_BUTTONINSTALL:
        {
            EndDialog(hwndDlg, 0);
            restartInstall();
        }
        return TRUE;
        }
    }
    break;
    }
    return FALSE;
}

static bool convertNumber(const wchar_t* str, unsigned long & result, int radix)
{
    errno = 0;
    wchar_t* end;
    result = wcstoul(str, &end, radix);
    if(!result && end == str)
        return false;
    if(result == ULLONG_MAX && errno)
        return false;
    if(*end)
        return false;
    return true;
}

static bool parseId(const wchar_t* str, unsigned long & result)
{
    int radix = 10;
    if(!wcsncmp(str, L"0x", 2))
        radix = 16, str += 2;
    return convertNumber(str, result, radix);
}

const wchar_t* SHELLEXT_EXE_KEY = L"exefile\\shell\\Debug with x64dbg\\Command";
const wchar_t* SHELLEXT_ICON_EXE_KEY = L"exefile\\shell\\Debug with x64dbg";
const wchar_t* SHELLEXT_DLL_KEY = L"dllfile\\shell\\Debug with x64dbg\\Command";
const wchar_t* SHELLEXT_ICON_DLL_KEY = L"dllfile\\shell\\Debug with x64dbg";


INT_PTR CALLBACK DlgConfigurations(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_INITDIALOG:
    {
        HANDLE hIcon;
        hIcon = LoadIconW(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON1));
        SendMessageW(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

        // Set the state of checkboxes
        CheckDlgButton(hDlg, IDC_CHECK_SHELLEXT, BST_CHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_DESKTOPSHORTCUT, BST_CHECKED);
        CheckDlgButton(hDlg, IDC_CHECK_ICON, BST_CHECKED);

        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        // Retrieve the state of checkboxes
        BOOL bShellExt = IsDlgButtonChecked(hDlg, IDC_CHECK_SHELLEXT) == BST_CHECKED;
        BOOL bDesktopShortcut = IsDlgButtonChecked(hDlg, IDC_CHECK_DESKTOPSHORTCUT) == BST_CHECKED;
        BOOL bIcon = IsDlgButtonChecked(hDlg, IDC_CHECK_ICON) == BST_CHECKED;

        if(LOWORD(wParam) == IDYES)
        {

            // Perform actions based on checkbox states
            if(bShellExt)
            {
                TCHAR szLauncherCommand[MAX_PATH] = TEXT("");
                _stprintf_s(szLauncherCommand, _countof(szLauncherCommand), TEXT("\"%s\" \"%%1\""), szApplicationDir);

                TCHAR szIconCommand[MAX_PATH] = TEXT("");
                _stprintf_s(szIconCommand, _countof(szIconCommand), TEXT("\"%s\",0"), szApplicationDir);

                if(RegisterShellExtension(SHELLEXT_EXE_KEY, szLauncherCommand))
                    AddShellIcon(SHELLEXT_ICON_EXE_KEY, szIconCommand, LoadResString(IDS_SHELLEXTDBG));

                if(RegisterShellExtension(SHELLEXT_DLL_KEY, szLauncherCommand))
                    AddShellIcon(SHELLEXT_ICON_DLL_KEY, szIconCommand, LoadResString(IDS_SHELLEXTDBG));
            }

            if(bDesktopShortcut)
            {
                AddDesktopShortcut(sz32Path, TEXT("x32dbg"));
                if(isWoW64())
                    AddDesktopShortcut(sz64Path, TEXT("x64dbg"));
            }

            if(bIcon)
            {
                AddDBFileTypeIcon(sz32Path, sz64Path);
            }

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        else if(LOWORD(wParam) == IDNO)
        {
            if(bShellExt)
            {
                UnregisterShellExtension(SHELLEXT_EXE_KEY);
                UnregisterShellExtension(SHELLEXT_ICON_EXE_KEY);
                UnregisterShellExtension(SHELLEXT_DLL_KEY);
                UnregisterShellExtension(SHELLEXT_ICON_DLL_KEY);
            }

            if(bDesktopShortcut)
            {
                RemoveDesktopShortcut(TEXT("x32dbg"));
                if(isWoW64())
                    RemoveDesktopShortcut(TEXT("x64dbg"));
            }

            if(bIcon)
            {
                RemoveDBFileTypeIcon();
            }

            EndDialog(hDlg, IDNO);
            return (INT_PTR)TRUE;
        }
        else if(LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

static void deleteZoneData(const std::wstring & rootDir)
{
    std::wstring tempPath;
    std::queue<std::wstring> queue;
    queue.push(rootDir);
    while(!queue.empty())
    {
        auto dir = queue.front();
        queue.pop();
        WIN32_FIND_DATAW foundData;
        HANDLE hSearch = FindFirstFileW((dir + L"\\*").c_str(), &foundData);
        if(hSearch == INVALID_HANDLE_VALUE)
        {
            continue;
        }
        do
        {
            if((foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
            {
                if(wcscmp(foundData.cFileName, L".") != 0 && wcscmp(foundData.cFileName, L"..") != 0)
                    queue.push(dir + L"\\" + foundData.cFileName);
            }
            else
            {
                tempPath = dir + L"\\" + foundData.cFileName + L":Zone.Identifier";
                DeleteFileW(tempPath.c_str());
            }
        }
        while(FindNextFileW(hSearch, &foundData));
        FindClose(hSearch);
    }
}

typedef BOOL(WINAPI* LPFN_SetProcessDpiAwarenessContext)(int);

static void EnableHiDPI()
{
    // Windows 10 Build 1607
    LPFN_SetProcessDpiAwarenessContext SetProcessDpiAwarenessContext = (LPFN_SetProcessDpiAwarenessContext)GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "SetProcessDpiAwarenessContext");
    if(SetProcessDpiAwarenessContext != nullptr)
    {
        // Windows 10 Build 1703
        SetProcessDpiAwarenessContext(-4);  //DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
    }
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    EnableHiDPI();
    InitCommonControls();

    //Initialize COM
    CoInitialize(nullptr);

    //Get INI file path
    if(!GetModuleFileName(nullptr, szApplicationDir, MAX_PATH))
    {
        MessageBox(nullptr, LoadResString(IDS_ERRORGETTINGMODULEPATH), LoadResString(IDS_ERROR), MB_ICONERROR | MB_SYSTEMMODAL);
        return 0;
    }
    TCHAR szIniPath[MAX_PATH] = TEXT("");
    _tcscpy_s(szIniPath, szApplicationDir);
    _tcscpy_s(szCurrentDir, szApplicationDir);
    auto len = int(_tcslen(szCurrentDir));
    while(szCurrentDir[len] != TEXT('\\') && len)
        len--;
    if(len)
        szCurrentDir[len] = TEXT('\0');
    len = int(_tcslen(szIniPath));
    while(szIniPath[len] != TEXT('.') && szIniPath[len] != TEXT('\\') && len)
        len--;
    if(szIniPath[len] == TEXT('\\'))
        _tcscat_s(szIniPath, TEXT(".ini"));
    else
        _tcscpy_s(&szIniPath[len], _countof(szIniPath) - len, TEXT(".ini"));
    CreateUnicodeFile(szIniPath);

    //Load settings
    auto bDoneSomething = false;
    TCHAR szTempPath[MAX_PATH] = TEXT("");
    if(!GetPrivateProfileString(TEXT("Launcher"), TEXT("x32dbg"), TEXT(""), szTempPath, MAX_PATH, szIniPath))
    {
        _tcscpy_s(sz32Path, szCurrentDir);
        PathAppend(sz32Path, TEXT("x32\\x32dbg.exe"));
        if(FileExists(sz32Path))
        {
            WritePrivateProfileString(TEXT("Launcher"), TEXT("x32dbg"), TEXT("x32\\x32dbg.exe"), szIniPath);
            bDoneSomething = true;
        }
    }
    else
    {
        if(PathIsRelative(szTempPath))
        {
            _tcscpy_s(sz32Path, szCurrentDir);
            PathAppend(sz32Path, szTempPath);
        }
        else
            _tcscpy_s(sz32Path, szTempPath);
    }

    _tcscpy_s(sz32Dir, sz32Path);
    PathRemoveFileSpec(sz32Dir);

    if(!GetPrivateProfileString(TEXT("Launcher"), TEXT("x64dbg"), TEXT(""), szTempPath, MAX_PATH, szIniPath))
    {
        _tcscpy_s(sz64Path, szCurrentDir);
        PathAppend(sz64Path, TEXT("x64\\x64dbg.exe"));
        if(FileExists(sz64Path))
        {
            WritePrivateProfileString(TEXT("Launcher"), TEXT("x64dbg"), TEXT("x64\\x64dbg.exe"), szIniPath);
            bDoneSomething = true;
        }
    }
    else
    {
        if(PathIsRelative(szTempPath))
        {
            _tcscpy_s(sz64Path, szCurrentDir);
            PathAppend(sz64Path, szTempPath);
        }
        else
            _tcscpy_s(sz64Path, szTempPath);
    }

    _tcscpy_s(sz64Dir, sz64Path);
    PathRemoveFileSpec(sz64Dir);

    //Functions to load the relevant debugger with a command line
    auto load32 = [](const wchar_t* cmdLine)
    {
        if(sz32Path[0])
            ShellExecute(nullptr, TEXT("open"), sz32Path, cmdLine, sz32Dir, SW_SHOWNORMAL);
        else
            MessageBox(nullptr, LoadResString(IDS_INVDPATH32), LoadResString(IDS_ERROR), MB_ICONERROR);
    };
    auto load64 = [](const wchar_t* cmdLine)
    {
        if(sz64Path[0])
            ShellExecute(nullptr, TEXT("open"), sz64Path, cmdLine, sz64Dir, SW_SHOWNORMAL);
        else
            MessageBox(nullptr, LoadResString(IDS_INVDPATH64), LoadResString(IDS_ERROR), MB_ICONERROR);
    };

    unsigned long pid = 0, id2 = 0;
    auto loadPid = [&](const wchar_t* cmdLine)
    {
        if(isWoW64())
        {
            auto hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if(hProcess)
            {
                BOOL bWow64Process = FALSE;
                if(IsWow64Process(hProcess, &bWow64Process) && bWow64Process)
                    load32(cmdLine);
                else
                    load64(cmdLine);
                CloseHandle(hProcess);
            }
            else
                load64(cmdLine);
        }
        else
            load32(cmdLine);
    };

    OutputDebugStringW(L"[x96dbg] Command line:");
    OutputDebugStringW(GetCommandLineW());

    //Handle command line
    auto argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    // If x64dbg is not found, perform installation
    if(bDoneSomething)
    {
        restartInstall();
        return 0;
    }

    if(argc <= 1) //no arguments -> launcher dialog
    {
        if(!FileExists(sz32Path) && BrowseFileOpen(nullptr, TEXT("x32dbg.exe\0x32dbg.exe\0*.exe\0*.exe\0\0"), nullptr, sz32Path, MAX_PATH, szCurrentDir))
        {
            WritePrivateProfileString(TEXT("Launcher"), TEXT("x32dbg"), sz32Path, szIniPath);
            bDoneSomething = true;
        }
        if(isWoW64() && !FileExists(sz64Path) && BrowseFileOpen(nullptr, TEXT("x64dbg.exe\0x64dbg.exe\0*.exe\0*.exe\0\0"), nullptr, sz64Path, MAX_PATH, szCurrentDir))
        {
            WritePrivateProfileString(TEXT("Launcher"), TEXT("x64dbg"), sz64Path, szIniPath);
            bDoneSomething = true;
        }
        DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_DIALOGLAUNCHER), 0, DlgLauncher);
    }
    else if(argc == 2 && !wcscmp(argv[1], L"::install")) //set configuration
    {
        if(!FileExists(sz32Path) && BrowseFileOpen(nullptr, TEXT("x32dbg.exe\0x32dbg.exe\0*.exe\0*.exe\0\0"), nullptr, sz32Path, MAX_PATH, szCurrentDir))
        {
            WritePrivateProfileString(TEXT("Launcher"), TEXT("x32dbg"), sz32Path, szIniPath);
            bDoneSomething = true;
        }
        if(isWoW64() && !FileExists(sz64Path) && BrowseFileOpen(nullptr, TEXT("x64dbg.exe\0x64dbg.exe\0*.exe\0*.exe\0\0"), nullptr, sz64Path, MAX_PATH, szCurrentDir))
        {
            WritePrivateProfileString(TEXT("Launcher"), TEXT("x64dbg"), sz64Path, szIniPath);
            bDoneSomething = true;
        }
        deleteZoneData(szCurrentDir);
        deleteZoneData(szCurrentDir + std::wstring(L"\\..\\pluginsdk"));

        INT_PTR result = DialogBox(hInstance, MAKEINTRESOURCE(IDD_OPTIONS_DIALOG), nullptr, DlgConfigurations);
    }
    else if(argc == 3 && !wcscmp(argv[1], L"-p") && parseId(argv[2], pid)) //-p PID
    {
        wchar_t cmdLine[32] = L"";
        wsprintfW(cmdLine, L"-p %u", pid);
        loadPid(cmdLine);
    }
    else if(argc == 5 && !wcscmp(argv[1], L"-p") && !wcscmp(argv[3], L"-tid") && parseId(argv[2], pid) && parseId(argv[4], id2)) //-p PID -tid TID
    {
        wchar_t cmdLine[32] = L"";
        wsprintfW(cmdLine, L"-p %u -tid %u", pid, id2);
        loadPid(cmdLine);
    }
    else if(argc == 5 && !wcscmp(argv[1], L"-p") && !wcscmp(argv[3], L"-e") && parseId(argv[2], pid) && parseId(argv[4], id2)) //-p PID -e EVENT
    {
        wchar_t cmdLine[32] = L"";
        wsprintfW(cmdLine, L"-a %u -e %u", pid, id2);
        loadPid(cmdLine);
    }
    else if(argc >= 2) //one or more arguments -> execute debugger
    {
        BOOL canDisableRedirect = FALSE;
        RedirectWow rWow;
        //check for redirection and disable it.
        if(isWoW64())
        {
            if(isWowRedirectionSupported())
            {
                canDisableRedirect = TRUE;
            }
        }

        TCHAR szPath[MAX_PATH] = TEXT("");
        if(PathIsRelative(argv[1])) //resolve the full path if a relative path is specified (TODO: honor the PATH environment variable)
        {
            GetCurrentDirectory(_countof(szPath), szPath);
            PathAppend(szPath, argv[1]);
        }
        else if(!ResolveShortcut(nullptr, argv[1], szPath, _countof(szPath))) //attempt to resolve the shortcut path
            _tcscpy_s(szPath, argv[1]); //fall back to the origin full path

        std::wstring cmdLine, escaped;
        cmdLine.push_back(L'\"');
        cmdLine += szPath;
        cmdLine.push_back(L'\"');
        if(argc > 2) //forward any commandline parameters
        {
            cmdLine += L" \"";
            for(auto i = 2; i < argc; i++)
            {
                if(i > 2)
                    cmdLine.push_back(L' ');

                escaped.clear();
                auto len = wcslen(argv[i]);
                for(size_t j = 0; j < len; j++)
                {
                    if(argv[i][j] == L'\"')
                        escaped.push_back(L'\"');
                    escaped.push_back(argv[i][j]);
                }

                cmdLine += escaped;
            }
            cmdLine += L"\"";
        }
        else //empty command line
        {
            cmdLine += L" \"\"";
        }

        //append current working directory
        TCHAR szCurDir[MAX_PATH] = TEXT("");
        GetCurrentDirectory(_countof(szCurDir), szCurDir);
        cmdLine += L" \"";
        cmdLine += szCurDir;
        cmdLine += L"\"";

        if(canDisableRedirect)
            rWow.DisableRedirect();

        //MessageBoxW(0, cmdLine.c_str(), L"x96dbg", MB_SYSTEMMODAL);
        //MessageBoxW(0, GetCommandLineW(), L"GetCommandLineW", MB_SYSTEMMODAL);
        //MessageBoxW(0, szCurDir, L"GetCurrentDirectory", MB_SYSTEMMODAL);

        switch(GetPeArch(szPath))
        {
        case PeArch::Native86:
        case PeArch::Dotnet86:
        case PeArch::DotnetAnyCpuPrefer32:
            load32(cmdLine.c_str());
            break;
        case PeArch::Native64:
        case PeArch::Dotnet64:
            load64(cmdLine.c_str());
            break;
        case PeArch::DotnetAnyCpu:
            if(isWoW64())
                load64(cmdLine.c_str());
            else
                load32(cmdLine.c_str());
            break;
        case PeArch::Invalid:
            if(FileExists(szPath))
                MessageBox(nullptr, LoadResString(IDS_INVDPE), argv[1], MB_ICONERROR);
            else
                MessageBox(nullptr, LoadResString(IDS_FILEERR), argv[1], MB_ICONERROR);
            break;
        default:
            __debugbreak();
        }
    }
    LocalFree(argv);
    return 0;
}
