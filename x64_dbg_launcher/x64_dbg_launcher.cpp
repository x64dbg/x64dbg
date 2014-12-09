/**
 @file x64_dbg_launcher.cpp

 @brief Implements the 64 debug launcher class.
 */

#include <stdio.h>
#include <windows.h>
#include <string>
#include <shlwapi.h>

/**
 @enum arch

 @brief Values that represent arch.
 */

enum arch
{
    /**
     @property notfound, invalid, x32, x64 }

     @brief Gets the.

     @return The.
     */

    notfound,

    /**
     @property invalid, x32, x64 }

     @brief Gets the.

     @return The.
     */

    invalid,

    /**
     @property x32, x64 }

     @brief Gets the.

     @return The.
     */

    x32,

    /**
     @brief .
     */

    x64
};

/**
 @fn static bool FileExists(const wchar_t* file)

 @brief Queries if a given file exists.

 @param file The file.

 @return true if it succeeds, false if it fails.
 */

static bool FileExists(const wchar_t* file)
{
    DWORD attrib = GetFileAttributesW(file);
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

/**
 @fn static arch GetFileArchitecture(const wchar_t* szFileName)

 @brief Gets file architecture.

 @param szFileName Filename of the file.

 @return The file architecture.
 */

static arch GetFileArchitecture(const wchar_t* szFileName)
{
    arch retval = notfound;
    HANDLE hFile = CreateFileW(szFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        unsigned char data[0x1000];
        DWORD read = 0;
        DWORD fileSize = GetFileSize(hFile, 0);
        DWORD readSize = sizeof(data);
        if(readSize > fileSize)
            readSize = fileSize;
        if(ReadFile(hFile, data, readSize, &read, 0))
        {
            retval = invalid;
            IMAGE_DOS_HEADER* pdh = (IMAGE_DOS_HEADER*)data;
            if(pdh->e_magic == IMAGE_DOS_SIGNATURE && (size_t)pdh->e_lfanew < readSize)
            {
                IMAGE_NT_HEADERS* pnth = (IMAGE_NT_HEADERS*)(data + pdh->e_lfanew);
                if(pnth->Signature == IMAGE_NT_SIGNATURE)
                {
                    if(pnth->FileHeader.Machine == IMAGE_FILE_MACHINE_I386) //x32
                        retval = x32;
                    else if(pnth->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) //x64
                        retval = x64;
                }
            }
        }
        CloseHandle(hFile);
    }
    return retval;
}

/**
 @fn static bool BrowseFileOpen(HWND owner, const wchar_t* filter, const wchar_t* defext, wchar_t* filename, int filename_size, const wchar_t* init_dir)

 @brief Queries if a given browse file open.

 @param owner             Handle of the owner.
 @param filter            Specifies the filter.
 @param defext            The defext.
 @param [in,out] filename If non-null, filename of the file.
 @param filename_size     Size of the filename.
 @param init_dir          The initialise dir.

 @return true if it succeeds, false if it fails.
 */

static bool BrowseFileOpen(HWND owner, const wchar_t* filter, const wchar_t* defext, wchar_t* filename, int filename_size, const wchar_t* init_dir)
{
    OPENFILENAMEW ofstruct;
    memset(&ofstruct, 0, sizeof(ofstruct));
    ofstruct.lStructSize = sizeof(ofstruct);
    ofstruct.hwndOwner = owner;
    ofstruct.hInstance = GetModuleHandleW(0);
    ofstruct.lpstrFilter = filter;
    ofstruct.lpstrFile = filename;
    ofstruct.nMaxFile = filename_size - 1;
    ofstruct.lpstrInitialDir = init_dir;
    ofstruct.lpstrDefExt = defext;
    ofstruct.Flags = OFN_EXTENSIONDIFFERENT | OFN_HIDEREADONLY | OFN_NONETWORKBUTTON;
    return !!GetOpenFileNameW(&ofstruct);
}

/**
 @def SHELLEXT_EXE_KEY

 @brief A macro that defines shellext executable key.
 */

#define SHELLEXT_EXE_KEY L"exefile\\shell\\Debug with x64_dbg\\Command"

/**
 @def SHELLEXT_DLL_KEY

 @brief A macro that defines shellext DLL key.
 */

#define SHELLEXT_DLL_KEY L"dllfile\\shell\\Debug with x64_dbg\\Command"

/**
 @fn static void RegisterShellExtension(const wchar_t* key, const wchar_t* command)

 @brief Registers the shell extension.

 @param key     The key.
 @param command The command.
 */

static void RegisterShellExtension(const wchar_t* key, const wchar_t* command)
{
    HKEY hKey;
    if(RegCreateKeyW(HKEY_CLASSES_ROOT, key, &hKey) != ERROR_SUCCESS)
    {
        MessageBoxW(0, L"RegCreateKeyA failed!", L"Running as Admin?", MB_ICONERROR);
        return;
    }
    if(RegSetValueExW(hKey, 0, 0, REG_EXPAND_SZ, (LPBYTE)command, (wcslen(command) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
        MessageBoxW(0, L"RegSetValueExA failed!", L"Running as Admin?", MB_ICONERROR);
    RegCloseKey(hKey);
}

static void CreateUnicodeFile(const wchar_t* file)
{
    //Taken from: http://www.codeproject.com/Articles/9071/Using-Unicode-in-INI-files
    if(FileExists(file))
        return;

    // UTF16-LE BOM(FFFE)
    WORD wBOM = 0xFEFF;
    HANDLE hFile = CreateFileW(file, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return;
    DWORD written = 0;
    WriteFile(hFile, &wBOM, sizeof(WORD), &written, NULL);
    CloseHandle(hFile);
}

/**
 @fn int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)

 @brief Window main.

 @param hInstance     The instance.
 @param hPrevInstance The previous instance.
 @param lpCmdLine     The command line.
 @param nShowCmd      The show command.

 @return An APIENTRY.
 */

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    CoInitialize(NULL); //fixed some crash
    //Get INI file path
    wchar_t szModulePath[MAX_PATH] = L"";
    if(!GetModuleFileNameW(0, szModulePath, MAX_PATH))
    {
        MessageBoxW(0, L"Error getting module path!", L"Error", MB_ICONERROR | MB_SYSTEMMODAL);
        return 0;
    }
    wchar_t szIniPath[MAX_PATH] = L"";
    wcscpy_s(szIniPath, szModulePath);
    wchar_t szCurrentDir[MAX_PATH] = L"";
    wcscpy_s(szCurrentDir, szModulePath);
    int len = (int)wcslen(szCurrentDir);
    while(szCurrentDir[len] != L'\\' && len)
        len--;
    if(len)
        szCurrentDir[len] = L'\0';
    len = (int)wcslen(szIniPath);
    while(szIniPath[len] != L'.' && szIniPath[len] != L'\\' && len)
        len--;
    if(szIniPath[len] == L'\\')
        wcscat_s(szIniPath, L".ini");
    else
        wcscpy(&szIniPath[len], L".ini");
    CreateUnicodeFile(szIniPath);

    //Load settings
    bool bDoneSomething = false;
    wchar_t sz32Path[MAX_PATH] = L"";
    if(!GetPrivateProfileStringW(L"Launcher", L"x32_dbg", L"", sz32Path, MAX_PATH, szIniPath))
    {
        wcscpy_s(sz32Path, szCurrentDir);
        PathAppendW(sz32Path, L"x32\\x32_dbg.exe");
        if(FileExists(sz32Path))
        {
            WritePrivateProfileStringW(L"Launcher", L"x32_dbg", sz32Path, szIniPath);
            bDoneSomething = true;
        }
    }
    wchar_t sz32Dir[MAX_PATH] = L"";
    wcscpy_s(sz32Dir, sz32Path);
    len = (int)wcslen(sz32Dir);
    while(sz32Dir[len] != L'\\' && len)
        len--;
    if(len)
        sz32Dir[len] = L'\0';
    wchar_t sz64Path[MAX_PATH] = L"";
    if(!GetPrivateProfileStringW(L"Launcher", L"x64_dbg", L"", sz64Path, MAX_PATH, szIniPath))
    {
        wcscpy_s(sz64Path, szCurrentDir);
        PathAppendW(sz64Path, L"x64\\x64_dbg.exe");
        if(FileExists(sz64Path))
        {
            WritePrivateProfileStringW(L"Launcher", L"x64_dbg", sz64Path, szIniPath);
            bDoneSomething = true;
        }
    }
    wchar_t sz64Dir[MAX_PATH] = L"";
    wcscpy_s(sz64Dir, sz64Path);
    len = (int)wcslen(sz64Dir);
    while(sz64Dir[len] != L'\\' && len)
        len--;
    if(len)
        sz64Dir[len] = L'\0';

    //Handle command line
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if(argc <= 1) //no arguments -> set configuration
    {
        if(!FileExists(sz32Path) && BrowseFileOpen(0, L"x32_dbg.exe\0x32_dbg.exe\0\0", 0, sz32Path, MAX_PATH, szCurrentDir))
        {
            WritePrivateProfileStringW(L"Launcher", L"x32_dbg", sz32Path, szIniPath);
            bDoneSomething = true;
        }
        if(!FileExists(sz64Path) && BrowseFileOpen(0, L"x64_dbg.exe\0x64_dbg.exe\0\0", 0, sz64Path, MAX_PATH, szCurrentDir))
        {
            WritePrivateProfileStringW(L"Launcher", L"x64_dbg", sz64Path, szIniPath);
            bDoneSomething = true;
        }
        if(MessageBoxW(0, L"Do you want to register a shell extension?", L"Question", MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            wchar_t szLauncherCommand[MAX_PATH] = L"";
            swprintf_s(szLauncherCommand, _countof(szLauncherCommand), L"\"%s\" \"%%1\"", szModulePath);
            RegisterShellExtension(SHELLEXT_EXE_KEY, szLauncherCommand);
            RegisterShellExtension(SHELLEXT_DLL_KEY, szLauncherCommand);
        }
        if(bDoneSomething)
            MessageBoxW(0, L"New configuration written!", L"Done!", MB_ICONINFORMATION);
    }
    if(argc == 2) //one argument -> execute debugger
    {
        std::wstring cmdLine = L"\"";
        cmdLine += argv[1];
        cmdLine += L"\"";
        switch(GetFileArchitecture(argv[1]))
        {
        case x32:
            if(sz32Path[0])
                ShellExecuteW(0, L"open", sz32Path, cmdLine.c_str(), sz32Dir, SW_SHOWNORMAL);
            else
                MessageBoxW(0, L"Path to x32_dbg not specified in launcher configuration...", L"Error!", MB_ICONERROR);
            break;

        case x64:
            if(sz64Path[0])
                ShellExecuteW(0, L"open", sz64Path, cmdLine.c_str(), sz64Dir, SW_SHOWNORMAL);
            else
                MessageBoxW(0, L"Path to x64_dbg not specified in launcher configuration...", L"Error!", MB_ICONERROR);
            break;

        case invalid:
            MessageBoxW(0, argv[1], L"Invalid PE File!", MB_ICONERROR);
            break;

        case notfound:
            MessageBoxW(0, argv[1], L"File not found or in use!", MB_ICONERROR);
            break;
        }
    }
    LocalFree(argv);
    return 0;
}
