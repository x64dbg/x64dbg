#include "_global.h"
#include <objbase.h>
#include <shlobj.h>
#include <new>

HINSTANCE hInst;
char dbbasepath[deflen] = "";
char dbpath[3 * deflen] = "";

static int emalloc_count = 0;
static char alloctrace[MAX_PATH] = "";

void* emalloc(size_t size, const char* reason)
{
    unsigned char* a = (unsigned char*)GlobalAlloc(GMEM_FIXED, size);
    if(!a)
    {
        MessageBoxA(0, "Could not allocate memory", "Error", MB_ICONERROR);
        ExitProcess(1);
    }
    memset(a, 0, size);
    emalloc_count++;
    /*
    FILE* file=fopen(alloctrace, "a+");
    fprintf(file, "DBG%.5d:  alloc:"fhex":%s:"fhex"\n", emalloc_count, a, reason, size);
    fclose(file);
    */
    return a;
}

void* erealloc(void* ptr, size_t size, const char* reason)
{
    if(!ptr)
        return emalloc(size, reason);
    unsigned char* a = (unsigned char*)GlobalReAlloc(ptr, size, 0);
    if(!a)
    {
        MessageBoxA(0, "Could not reallocate memory", "Error", MB_ICONERROR);
        ExitProcess(1);
    }
    memset(a, 0, size);
    /*
    FILE* file=fopen(alloctrace, "a+");
    fprintf(file, "DBG%.5d:realloc:"fhex":%s:"fhex"\n", emalloc_count, a, reason, size);
    fclose(file);
    */
    return a;
}

void efree(void* ptr, const char* reason)
{
    emalloc_count--;
    /*
    FILE* file=fopen(alloctrace, "a+");
    fprintf(file, "DBG%.5d:   free:"fhex":%s\n", emalloc_count, ptr, reason);
    fclose(file);
    */
    GlobalFree(ptr);
}

int memleaks()
{
    return emalloc_count;
}

void setalloctrace(const char* file)
{
    strcpy_s(alloctrace, file);
}

bool arraycontains(const char* cmd_list, const char* cmd)
{
    //TODO: fix this function a little
    if(!cmd_list or !cmd)
        return false;
    char temp[deflen] = "";
    strcpy_s(temp, cmd_list);
    int len = (int)strlen(cmd_list);
    if(len >= deflen)
        return false;
    for(int i = 0; i < len; i++)
        if(temp[i] == 1)
            temp[i] = 0;
    if(!_stricmp(temp, cmd))
        return true;
    for(int i = (int)strlen(temp); i < len; i++)
    {
        if(!temp[i])
        {
            if(!_stricmp(temp + i + 1, cmd))
                return true;
            i += (int)strlen(temp + i + 1);
        }
    }
    return false;
}

bool scmp(const char* a, const char* b)
{
    if(_stricmp(a, b))
        return false;
    return true;
}

void formathex(char* string)
{
    int len = (int)strlen(string);
    _strupr(string);
    Memory<char*> new_string(len + 1, "formathex:new_string");
    memset(new_string, 0, len + 1);
    for(int i = 0, j = 0; i < len; i++)
        if(isxdigit(string[i]))
            j += sprintf(new_string + j, "%c", string[i]);
    strcpy(string, new_string);
}

void formatdec(char* string)
{
    int len = (int)strlen(string);
    _strupr(string);
    Memory<char*> new_string(len + 1, "formatdec:new_string");
    memset(new_string, 0, len + 1);
    for(int i = 0, j = 0; i < len; i++)
        if(isdigit(string[i]))
            j += sprintf(new_string + j, "%c", string[i]);
    strcpy(string, new_string);
}

bool FileExists(const char* file)
{
    DWORD attrib = GetFileAttributesW(StringUtils::Utf8ToUtf16(file).c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool DirExists(const char* dir)
{
    DWORD attrib = GetFileAttributesW(StringUtils::Utf8ToUtf16(dir).c_str());
    return (attrib == FILE_ATTRIBUTE_DIRECTORY);
}

bool GetFileNameFromHandle(HANDLE hFile, char* szFileName)
{
    wchar_t wszFileName[MAX_PATH] = L"";
    if(!PathFromFileHandleW(hFile, wszFileName, sizeof(wszFileName)))
        return false;
    strcpy_s(szFileName, MAX_PATH, StringUtils::Utf16ToUtf8(wszFileName).c_str());
    return true;
}

bool settingboolget(const char* section, const char* name)
{
    uint setting;
    if(!BridgeSettingGetUint(section, name, &setting))
        return false;
    if(setting)
        return true;
    return false;
}

arch GetFileArchitecture(const char* szFileName)
{
    arch retval = notfound;
    HANDLE hFile = CreateFileW(StringUtils::Utf8ToUtf16(szFileName).c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
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

bool IsWow64()
{
    BOOL bIsWow64Process = FALSE;
    //x64_dbg supports WinXP SP3 and later only, so ignore the GetProcAddress crap :D
    IsWow64Process(GetCurrentProcess(), &bIsWow64Process);
    return !!bIsWow64Process;
}

//Taken from: http://www.cplusplus.com/forum/windows/64088/
bool ResolveShortcut(HWND hwnd, const wchar_t* szShortcutPath, char* szResolvedPath, size_t nSize)
{
    if(szResolvedPath == NULL)
        return SUCCEEDED(E_INVALIDARG);

    //Initialize COM stuff
    CoInitialize(NULL);

    //Get a pointer to the IShellLink interface.
    IShellLink* psl = NULL;
    HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if(SUCCEEDED(hres))
    {
        //Get a pointer to the IPersistFile interface.
        IPersistFile* ppf = NULL;
        hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
        if(SUCCEEDED(hres))
        {
            //Load the shortcut.
            hres = ppf->Load(szShortcutPath, STGM_READ);

            if(SUCCEEDED(hres))
            {
                //Resolve the link.
                hres = psl->Resolve(hwnd, 0);

                if(SUCCEEDED(hres))
                {
                    //Get the path to the link target.
                    char szGotPath[MAX_PATH] = {0};
                    hres = psl->GetPath(szGotPath, _countof(szGotPath), NULL, SLGP_SHORTPATH);

                    if(SUCCEEDED(hres))
                    {
                        strcpy_s(szResolvedPath, nSize, szGotPath);
                    }
                }
            }

            //Release the pointer to the IPersistFile interface.
            ppf->Release();
        }

        //Release the pointer to the IShellLink interface.
        psl->Release();
    }

    //Uninitialize COM stuff
    CoUninitialize();
    return SUCCEEDED(hres);
}