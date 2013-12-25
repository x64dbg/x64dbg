#include "_global.h"
#include <new>

HINSTANCE hInst;
char sqlitedb_basedir[deflen]="";
char dbpath[3*deflen]="";

void* emalloc(size_t size)
{
    return emalloc(size, "emalloc:???");
}

void efree(void* ptr)
{
    efree(ptr, "efree:???");
}

static int emalloc_count=0;

void* emalloc(size_t size, const char* reason)
{
    unsigned char* a=new (std::nothrow)unsigned char[size+0x1000];
    if(!a)
    {
        MessageBoxA(0, "Could not allocate memory", "Error", MB_ICONERROR);
        ExitProcess(1);
    }
    memset(a, 0, size);
    emalloc_count++;
    FILE* file=fopen("alloctrace.txt", "a+");
    fprintf(file, "DBG%.5d:alloc:"fhex":%s:"fhex"\n", emalloc_count, a, reason, size);
    fclose(file);
    return a;
}

void efree(void* ptr, const char* reason)
{
    emalloc_count--;
    FILE* file=fopen("alloctrace.txt", "a+");
    fprintf(file, "DBG%.5d:efree:"fhex":%s\n", emalloc_count, ptr, reason);
    fclose(file);
    delete[] (unsigned char*)ptr;
}

bool arraycontains(const char* cmd_list, const char* cmd)
{
    //TODO: fix this function a little
    if(!cmd_list or !cmd)
        return false;
    char temp[deflen]="";
    strcpy(temp, cmd_list);
    int len=strlen(cmd_list);
    if(len>=deflen)
        return false;
    for(int i=0; i<len; i++)
        if(temp[i]==1)
            temp[i]=0;
    if(!_stricmp(temp, cmd))
        return true;
    for(int i=strlen(temp); i<len; i++)
    {
        if(!temp[i])
        {
            if(!_stricmp(temp+i+1, cmd))
                return true;
            i+=strlen(temp+i+1);
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
    int len=strlen(string);
    _strupr(string);
    char* new_string=(char*)emalloc(len+1, "formathex:new_string");
    memset(new_string, 0, len+1);
    for(int i=0,j=0; i<len; i++)
        if(isxdigit(string[i]))
            j+=sprintf(new_string+j, "%c", string[i]);
    strcpy(string, new_string);
    efree(new_string, "formathex:new_string");
}

void formatdec(char* string)
{
    int len=strlen(string);
    _strupr(string);
    char* new_string=(char*)emalloc(len+1, "formatdec:new_string");
    memset(new_string, 0, len+1);
    for(int i=0,j=0; i<len; i++)
        if(isdigit(string[i]))
            j+=sprintf(new_string+j, "%c", string[i]);
    strcpy(string, new_string);
    efree(new_string, "formatdec:new_string");
}

bool FileExists(const char* file)
{
    DWORD attrib=GetFileAttributes(file);
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool DirExists(const char* dir)
{
    DWORD attrib=GetFileAttributes(dir);
    return (attrib==FILE_ATTRIBUTE_DIRECTORY);
}

bool DevicePathToPath(const char* devicepath, char* path, size_t path_size)
{
    if(!devicepath or !path)
        return false;
    char curDrive[3]=" :";
    char curDevice[MAX_PATH]="";
    for(char drive='C'; drive<='Z'; drive++)
    {
        *curDrive=drive;
        if(!QueryDosDeviceA(curDrive, curDevice, MAX_PATH))
            continue;
        size_t curDevice_len=strlen(curDevice);
        if(!_strnicmp(devicepath, curDevice, curDevice_len)) //we match the device
        {
            if(strlen(devicepath)-curDevice_len>=path_size)
                return false;
            sprintf(path, "%s%s", curDrive, devicepath+curDevice_len);
            return true;
        }
    }
    return false;
}

bool PathToDevicePath(const char* path, char* devicepath, size_t devicepath_size)
{
    if(!path or path[1]!=':' or !devicepath)
        return false;
    char curDrive[3]=" :";
    char curDevice[MAX_PATH]="";
    *curDrive=*path;
    if(!QueryDosDeviceA(curDrive, curDevice, MAX_PATH))
        return false;
    if(strlen(path)-2+strlen(curDevice)>=devicepath_size)
        return false;
    sprintf(devicepath, "%s%s", curDevice, path+2);
    return true;
}

bool GetFileNameFromHandle(HANDLE hFile, char* szFileName)
{
    if(!GetFileSize(hFile, 0))
        return false;
    HANDLE hFileMap=CreateFileMappingA(hFile, 0, PAGE_READONLY, 0, 1, 0);
    if(!hFileMap)
        return false;
    void* pFileMap=MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);
    if(!pFileMap)
    {
        CloseHandle(hFileMap);
        return false;
    }
    char szMappedName[MAX_PATH]="";
    if(GetMappedFileNameA(GetCurrentProcess(), pFileMap, szMappedName, MAX_PATH))
    {
        DevicePathToPath(szMappedName, szFileName, MAX_PATH);
        UnmapViewOfFile(pFileMap);
        CloseHandle(hFileMap);
        return true;
    }
    UnmapViewOfFile(pFileMap);
    CloseHandle(hFileMap);
    return false;
}
