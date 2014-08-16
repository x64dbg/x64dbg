#include <stdio.h>
#include <windows.h>
#include <string>
#include <shlwapi.h>

enum arch
{
    notfound,
    invalid,
    x32,
    x64
};

static bool FileExists(const char* file)
{
    DWORD attrib = GetFileAttributes(file);
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

static arch GetFileArchitecture(const char* szFileName)
{
    arch retval = notfound;
    HANDLE hFile = CreateFileA(szFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
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

//Original code by Aurel from http://www.codeguru.com/cpp/w-p/win32/article.php/c1427/A-Simple-Win32-CommandLine-Parser.htm
static void commandlinefree(int argc, char** argv)
{
    for(int i = 0; i < argc; i++)
        free(argv[i]);
    free(argv);
}

static char** commandlineparse(int* argc)
{
    if(!argc)
        return NULL;
    LPWSTR wcCommandLine = GetCommandLineW();
    LPWSTR* argw = CommandLineToArgvW(wcCommandLine, argc);
    char** argv = (char**)malloc(sizeof(void*) * (*argc + 1));
    for(int i = 0; i < *argc; i++)
    {
        int bufSize = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, argw[i], -1, NULL, 0, NULL, NULL);
        argv[i] = (char*)malloc(bufSize + 1);
        WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, argw[i], bufSize, argv[i], bufSize * sizeof(char), NULL, NULL);
    }
    LocalFree(argw);
    return argv;
}

static bool BrowseFileOpen(HWND owner, const char* filter, const char* defext, char* filename, int filename_size, const char* init_dir)
{
    OPENFILENAME ofstruct;
    memset(&ofstruct, 0, sizeof(ofstruct));
    ofstruct.lStructSize = sizeof(ofstruct);
    ofstruct.hwndOwner = owner;
    ofstruct.hInstance = GetModuleHandleA(0);
    ofstruct.lpstrFilter = filter;
    ofstruct.lpstrFile = filename;
    ofstruct.nMaxFile = filename_size - 1;
    ofstruct.lpstrInitialDir = init_dir;
    ofstruct.lpstrDefExt = defext;
    ofstruct.Flags = OFN_EXTENSIONDIFFERENT | OFN_HIDEREADONLY | OFN_NONETWORKBUTTON;
    return !!GetOpenFileNameA(&ofstruct);
}

#define SHELLEXT_EXE_KEY "exefile\\shell\\Debug with x64_dbg\\Command"
#define SHELLEXT_DLL_KEY "dllfile\\shell\\Debug with x64_dbg\\Command"

void RegisterShellExtension(const char* key, const char* command)
{
    HKEY hKey;
    if(RegCreateKeyA(HKEY_CLASSES_ROOT, key, &hKey) != ERROR_SUCCESS)
    {
        MessageBoxA(0, "RegCreateKeyA failed!", "Running as Admin?", MB_ICONERROR);
        return;
    }
    if(RegSetValueExA(hKey, 0, 0, REG_EXPAND_SZ, (LPBYTE)command, strlen(command) + 1) != ERROR_SUCCESS)
        MessageBoxA(0, "RegSetValueExA failed!", "Running as Admin?", MB_ICONERROR);
    RegCloseKey(hKey);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    CoInitialize(NULL); //fixed some crash
    //Get INI file path
    char szModulePath[MAX_PATH] = "";
    if(!GetModuleFileNameA(0, szModulePath, MAX_PATH))
    {
        MessageBoxA(0, "Error getting module path!", "Error", MB_ICONERROR | MB_SYSTEMMODAL);
        return 0;
    }
    char szIniPath[MAX_PATH] = "";
    strcpy(szIniPath, szModulePath);
    char szCurrentDir[MAX_PATH] = "";
    strcpy(szCurrentDir, szModulePath);
    int len = (int)strlen(szCurrentDir);
    while(szCurrentDir[len] != '\\' && len)
        len--;
    if(len)
        szCurrentDir[len] = '\0';
    len = (int)strlen(szIniPath);
    while(szIniPath[len] != '.' && szIniPath[len] != '\\' && len)
        len--;
    if(szIniPath[len] == '\\')
        strcat(szIniPath, ".ini");
    else
        strcpy(&szIniPath[len], ".ini");

    //Load settings
    bool bDoneSomething = false;
    char sz32Path[MAX_PATH] = "";
    if(!GetPrivateProfileStringA("Launcher", "x32_dbg", "", sz32Path, MAX_PATH, szIniPath))
    {
        strcpy(sz32Path, szCurrentDir);
        PathAppendA(sz32Path, "x32\\x32_dbg.exe");
        if(FileExists(sz32Path))
        {
            WritePrivateProfileStringA("Launcher", "x32_dbg", sz32Path, szIniPath);
            bDoneSomething = true;
        }
    }
    char sz32Dir[MAX_PATH] = "";
    strcpy(sz32Dir, sz32Path);
    len = (int)strlen(sz32Dir);
    while(sz32Dir[len] != '\\' && len)
        len--;
    if(len)
        sz32Dir[len] = '\0';
    char sz64Path[MAX_PATH] = "";
    if(!GetPrivateProfileStringA("Launcher", "x64_dbg", "", sz64Path, MAX_PATH, szIniPath))
    {
        strcpy(sz64Path, szCurrentDir);
        PathAppendA(sz64Path, "x64\\x64_dbg.exe");
        if(FileExists(sz64Path))
        {
            WritePrivateProfileStringA("Launcher", "x64_dbg", sz64Path, szIniPath);
            bDoneSomething = true;
        }
    }
    char sz64Dir[MAX_PATH] = "";
    strcpy(sz64Dir, sz64Path);
    len = (int)strlen(sz64Dir);
    while(sz64Dir[len] != '\\' && len)
        len--;
    if(len)
        sz64Dir[len] = '\0';

    //Handle command line
    int argc = 0;
    char** argv = commandlineparse(&argc);
    if(argc <= 1) //no arguments -> set configuration
    {
        if(!FileExists(sz32Path) && BrowseFileOpen(0, "x32_dbg.exe\0x32_dbg.exe\0\0", 0, sz32Path, MAX_PATH, szCurrentDir))
        {
            WritePrivateProfileStringA("Launcher", "x32_dbg", sz32Path, szIniPath);
            bDoneSomething = true;
        }
        if(!FileExists(sz64Path) && BrowseFileOpen(0, "x64_dbg.exe\0x64_dbg.exe\0\0", 0, sz64Path, MAX_PATH, szCurrentDir))
        {
            WritePrivateProfileStringA("Launcher", "x64_dbg", sz64Path, szIniPath);
            bDoneSomething = true;
        }
        if(MessageBoxA(0, "Do you want to register a shell extension?", "Question", MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            char szLauncherCommand[MAX_PATH] = "";
            sprintf_s(szLauncherCommand, "\"%s\" \"%%1\"", szModulePath);
            RegisterShellExtension(SHELLEXT_EXE_KEY, szLauncherCommand);
            RegisterShellExtension(SHELLEXT_DLL_KEY, szLauncherCommand);
        }
        if(bDoneSomething)
            MessageBoxA(0, "New configuration written!", "Done!", MB_ICONINFORMATION);
    }
    if(argc == 2) //one argument -> execute debugger
    {
        std::string cmdLine = "\"";
        cmdLine += argv[1];
        cmdLine += "\"";
        switch(GetFileArchitecture(argv[1]))
        {
        case x32:
            if(sz32Path[0])
                ShellExecuteA(0, "open", sz32Path, cmdLine.c_str(), sz32Dir, SW_SHOWNORMAL);
            else
                MessageBoxA(0, "Path to x32_dbg not specified in launcher configuration...", "Error!", MB_ICONERROR);
            break;

        case x64:
            if(sz64Path[0])
                ShellExecuteA(0, "open", sz64Path, cmdLine.c_str(), sz64Dir, SW_SHOWNORMAL);
            else
                MessageBoxA(0, "Path to x64_dbg not specified in launcher configuration...", "Error!", MB_ICONERROR);
            break;

        case invalid:
            MessageBoxA(0, argv[1], "Invalid PE File!", MB_ICONERROR);
            break;

        case notfound:
            MessageBoxA(0, argv[1], "File not found or in use!", MB_ICONERROR);
            break;
        }
    }
    commandlinefree(argc, argv);
    return 0;
}
