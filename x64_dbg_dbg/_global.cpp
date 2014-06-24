#include "_global.h"
#include "DeviceNameResolver\DeviceNameResolver.h"
#include <new>

HINSTANCE hInst;
char dbbasepath[deflen]="";
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
static char alloctrace[MAX_PATH]="";

void* emalloc(size_t size, const char* reason)
{
    unsigned char* a=(unsigned char*)GlobalAlloc(GMEM_FIXED, size);
    if(!a)
    {
        MessageBoxA(0, "Could not allocate memory", "Error", MB_ICONERROR);
        ExitProcess(1);
    }
    memset(a, 0, size);
    emalloc_count++;
    /*
    FILE* file=fopen(alloctrace, "a+");
    fprintf(file, "DBG%.5d:alloc:"fhex":%s:"fhex"\n", emalloc_count, a, reason, size);
    fclose(file);
    */
    return a;
}

void efree(void* ptr, const char* reason)
{
    emalloc_count--;
    /*
    FILE* file=fopen(alloctrace, "a+");
    fprintf(file, "DBG%.5d:efree:"fhex":%s\n", emalloc_count, ptr, reason);
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
    strcpy(alloctrace, file);
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

bool GetFileNameFromHandle(HANDLE hFile, char* szFileName)
{
    return PathFromFileHandleA(hFile, szFileName, MAX_PATH);
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