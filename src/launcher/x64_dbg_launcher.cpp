#define UNICODE
#include <stdio.h>
#include <windows.h>
#include <string>
#include <shlwapi.h>
#include <objbase.h>
#include <shlobj.h>
#include <atlcomcli.h>

#include "../exe/LoadResourceString.h"

typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
typedef BOOL(WINAPI* LPFN_Wow64DisableWow64FsRedirection)(PVOID);
typedef BOOL(WINAPI* LPFN_Wow64RevertWow64FsRedirection)(PVOID);


LPFN_Wow64DisableWow64FsRedirection _Wow64DisableRedirection = NULL;
LPFN_Wow64RevertWow64FsRedirection _Wow64RevertRedirection = NULL;

enum arch
{
    notfound,
    invalid,
    x32,
    x64
};

static bool FileExists(const TCHAR* file)
{
    auto attrib = GetFileAttributes(file);
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

static arch GetFileArchitecture(const TCHAR* szFileName)
{
    auto retval = notfound;
    auto hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        unsigned char data[0x1000];
        DWORD read = 0;
        auto fileSize = GetFileSize(hFile, nullptr);
        auto readSize = DWORD(sizeof(data));
        if(readSize > fileSize)
            readSize = fileSize;
        if(ReadFile(hFile, data, readSize, &read, nullptr))
        {
            retval = invalid;
            auto pdh = PIMAGE_DOS_HEADER(data);
            if(pdh->e_magic == IMAGE_DOS_SIGNATURE && size_t(pdh->e_lfanew) < readSize)
            {
                auto pnth = PIMAGE_NT_HEADERS(data + pdh->e_lfanew);
                if(pnth->Signature == IMAGE_NT_SIGNATURE)
                {
                    if(pnth->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)  //x32
                        retval = x32;
                    else if(pnth->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)  //x64
                        retval = x64;
                }
            }
        }
        CloseHandle(hFile);
    }
    return retval;
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

static BOOL isWoW64()
{

    LPFN_ISWOW64PROCESS fnIsWow64Process;
    BOOL isWoW64 = FALSE;

    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

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

static TCHAR* GetDesktopPath()
{
    static TCHAR path[MAX_PATH + 1];
    if(SHGetSpecialFolderPath(HWND_DESKTOP, path, CSIDL_DESKTOPDIRECTORY, FALSE))
        return path;
    return nullptr;
}

const wchar_t* SHELLEXT_EXE_KEY = L"exefile\\shell\\Debug with x64dbg\\Command";
const wchar_t* SHELLEXT_ICON_EXE_KEY = L"exefile\\shell\\Debug with x64dbg";
const wchar_t* SHELLEXT_DLL_KEY = L"dllfile\\shell\\Debug with x64dbg\\Command";
const wchar_t* SHELLEXT_ICON_DLL_KEY = L"dllfile\\shell\\Debug with x64dbg";

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
            _tmakepath(path, nullptr, GetDesktopPath(), szNameOfLink, TEXT("lnk"));
            CComBSTR tmp(path);
            hRes = ppf->Save(tmp, TRUE);
        }
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

struct RedirectWow
{
    PVOID oldValue = NULL;
    RedirectWow() {}
    bool DisableRedirect()
    {
        if(!_Wow64DisableRedirection(&oldValue))
        {
            return false;
        }
        return true;
    }
    ~RedirectWow()
    {
        if(oldValue != NULL)
        {
            if(!_Wow64RevertRedirection(oldValue))
                //Error occured here. Ignore or reset? (does it matter at this point?)
                MessageBox(nullptr, TEXT("Error in Reverting Redirection"), TEXT("Error"), MB_OK | MB_ICONERROR);
        }
    }
};

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    //Initialize COM
    CoInitialize(nullptr);


    //Get INI file path
    TCHAR szModulePath[MAX_PATH] = TEXT("");
    if(!GetModuleFileName(nullptr, szModulePath, MAX_PATH))
    {
        MessageBox(nullptr, LoadResString(IDS_ERRORGETTINGMODULEPATH), LoadResString(IDS_ERROR), MB_ICONERROR | MB_SYSTEMMODAL);
        return 0;
    }
    TCHAR szIniPath[MAX_PATH] = TEXT("");
    _tcscpy_s(szIniPath, szModulePath);
    TCHAR szCurrentDir[MAX_PATH] = TEXT("");
    _tcscpy_s(szCurrentDir, szModulePath);
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
        _tcscpy(&szIniPath[len], TEXT(".ini"));
    CreateUnicodeFile(szIniPath);

    //Load settings
    auto bDoneSomething = false;
    TCHAR sz32Path[MAX_PATH] = TEXT("");
    if(!GetPrivateProfileString(TEXT("Launcher"), TEXT("x32dbg"), TEXT(""), sz32Path, MAX_PATH, szIniPath))
    {
        _tcscpy_s(sz32Path, szCurrentDir);
        PathAppend(sz32Path, TEXT("x32\\x32dbg.exe"));
        if(FileExists(sz32Path))
        {
            WritePrivateProfileString(TEXT("Launcher"), TEXT("x32dbg"), sz32Path, szIniPath);
            bDoneSomething = true;
        }
    }

    TCHAR sz32Dir[MAX_PATH] = TEXT("");
    _tcscpy_s(sz32Dir, sz32Path);
    PathRemoveFileSpec(sz32Dir);

    TCHAR sz64Path[MAX_PATH] = TEXT("");
    if(!GetPrivateProfileString(TEXT("Launcher"), TEXT("x64dbg"), TEXT(""), sz64Path, MAX_PATH, szIniPath))
    {
        _tcscpy_s(sz64Path, szCurrentDir);
        PathAppend(sz64Path, TEXT("x64\\x64dbg.exe"));
        if(FileExists(sz64Path))
        {
            WritePrivateProfileString(TEXT("Launcher"), TEXT("x64dbg"), sz64Path, szIniPath);
            bDoneSomething = true;
        }
    }

    TCHAR sz64Dir[MAX_PATH] = TEXT("");
    _tcscpy_s(sz64Dir, sz64Path);
    PathRemoveFileSpec(sz64Dir);

    //Handle command line
    auto argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if(argc <= 1)  //no arguments -> set configuration
    {
        if(!FileExists(sz32Path) && BrowseFileOpen(nullptr, TEXT("x32dbg.exe\0x32dbg.exe\0\0"), nullptr, sz32Path, MAX_PATH, szCurrentDir))
        {
            WritePrivateProfileString(TEXT("Launcher"), TEXT("x32dbg"), sz32Path, szIniPath);
            bDoneSomething = true;
        }
        if(!FileExists(sz64Path) && BrowseFileOpen(nullptr, TEXT("x64dbg.exe\0x64dbg.exe\0\0"), nullptr, sz64Path, MAX_PATH, szCurrentDir))
        {
            WritePrivateProfileString(TEXT("Launcher"), TEXT("x64dbg"), sz64Path, szIniPath);
            bDoneSomething = true;
        }
        if(MessageBox(nullptr, LoadResString(IDS_ASKSHELLEXT), LoadResString(IDS_QUESTION), MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            TCHAR szLauncherCommand[MAX_PATH] = TEXT("");
            _stprintf_s(szLauncherCommand, _countof(szLauncherCommand), TEXT("\"%s\" \"%%1\""), szModulePath);
            TCHAR szIconCommand[MAX_PATH] = TEXT("");
            _stprintf_s(szIconCommand, _countof(szIconCommand), TEXT("\"%s\",0"), szModulePath);
            if(RegisterShellExtension(SHELLEXT_EXE_KEY, szLauncherCommand))
                AddShellIcon(SHELLEXT_ICON_EXE_KEY, szIconCommand, LoadResString(IDS_SHELLEXTDBG));
            if(RegisterShellExtension(SHELLEXT_DLL_KEY, szLauncherCommand))
                AddShellIcon(SHELLEXT_ICON_DLL_KEY, szIconCommand, LoadResString(IDS_SHELLEXTDBG));
        }
        if(MessageBox(nullptr, LoadResString(IDS_ASKDESKTOPSHORTCUT), LoadResString(IDS_QUESTION), MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            AddDesktopShortcut(sz32Path, TEXT("x32dbg"));
            if(isWoW64())
                AddDesktopShortcut(sz64Path, TEXT("x64dbg"));
        }
        if(bDoneSomething)
            MessageBox(nullptr, LoadResString(IDS_NEWCFGWRITTEN), LoadResString(IDS_DONE), MB_ICONINFORMATION);
    }
    else if(argc >= 2)  //one or more arguments -> execute debugger
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
        if(PathIsRelative(argv[1]))  //resolve the full path if a relative path is specified
        {
            GetCurrentDirectory(_countof(szPath), szPath);
            PathAppend(szPath, argv[1]);
        }
        else if(!ResolveShortcut(nullptr, argv[1], szPath, _countof(szPath)))  //attempt to resolve the shortcut path
            _tcscpy_s(szPath, argv[1]); //fall back to the origin full path

        std::wstring cmdLine;
        cmdLine.push_back(L'\"');
        cmdLine += szPath;
        cmdLine.push_back(L'\"');
        if(argc > 2)  //forward any commandline parameters
        {
            cmdLine += L" \"";
            for(auto i = 2; i < argc; i++)
            {
                if(i > 2)
                    cmdLine.push_back(L' ');
                cmdLine.push_back(L'\"');
                cmdLine += argv[i];
                cmdLine.push_back(L'\"');
            }
            cmdLine += L"\"";
        }

        if(canDisableRedirect)
            rWow.DisableRedirect();

        switch(GetFileArchitecture(szPath))
        {
        case x32:
            if(sz32Path[0])
                ShellExecute(nullptr, TEXT("open"), sz32Path, cmdLine.c_str(), sz32Dir, SW_SHOWNORMAL);
            else
                MessageBox(nullptr, LoadResString(IDS_INVDPATH32), LoadResString(IDS_ERROR), MB_ICONERROR);
            break;

        case x64:
            if(sz64Path[0])
                ShellExecute(nullptr, TEXT("open"), sz64Path, cmdLine.c_str(), sz64Dir, SW_SHOWNORMAL);
            else
                MessageBox(nullptr, LoadResString(IDS_INVDPATH64), LoadResString(IDS_ERROR), MB_ICONERROR);
            break;

        case invalid:
            MessageBox(nullptr, argv[1], LoadResString(IDS_INVDPE), MB_ICONERROR);
            break;

        case notfound:
            MessageBox(nullptr, argv[1], LoadResString(IDS_FILEERR), MB_ICONERROR);
            break;
        }
    }
    LocalFree(argv);
    return 0;
}
