/**
 @file _global.cpp

 @brief Implements the global class.
 */

#include "_global.h"
#include <new>

/**
 @brief The instance.
 */

HINSTANCE hInst;

/**
 @brief The dbbasepath[deflen].
 */

char dbbasepath[deflen] = "";

/**
 @brief The dbpath[ 3*deflen].
 */

char dbpath[3 * deflen] = "";

/**
 @fn void* emalloc(size_t size)

 @brief Emallocs the given size.

 @param size The size.

 @return null if it fails, else a void*.
 */

/**
 @fn void* erealloc(void* ptr, size_t size)

 @brief Ereallocs.

 @param [in,out] ptr If non-null, the pointer.
 @param size         The size.

 @return null if it fails, else a void*.
 */

/**
 @fn void efree(void* ptr)

 @brief Efrees the given pointer.

 @param [in,out] ptr If non-null, the pointer.
 */

/**
 @brief Number of emallocs.
 */

static int emalloc_count = 0;

/**
 @brief The alloctrace[ maximum path].
 */

static char alloctrace[MAX_PATH] = "";

/**
 @fn void* emalloc(size_t size, const char* reason)

 @brief Emallocs.

 @param size   The size.
 @param reason The reason.

 @return null if it fails, else a void*.
 */

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

/**
 @fn void* erealloc(void* ptr, size_t size, const char* reason)

 @brief Ereallocs.

 @param [in,out] ptr If non-null, the pointer.
 @param size         The size.
 @param reason       The reason.

 @return null if it fails, else a void*.
 */

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

/**
 @fn void efree(void* ptr, const char* reason)

 @brief Efrees.

 @param [in,out] ptr If non-null, the pointer.
 @param reason       The reason.
 */

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

/**
 @fn int memleaks()

 @brief Gets the memleaks.

 @return An int.
 */

int memleaks()
{
    return emalloc_count;
}

/**
 @fn void setalloctrace(const char* file)

 @brief Setalloctraces the given file.

 @param file The file.
 */

void setalloctrace(const char* file)
{
    strcpy_s(alloctrace, file);
}

/**
 @fn bool arraycontains(const char* cmd_list, const char* cmd)

 @brief Arraycontains.

 @param cmd_list List of commands.
 @param cmd      The command.

 @return true if it succeeds, false if it fails.
 */

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

/**
 @fn bool scmp(const char* a, const char* b)

 @brief Scmps.

 @param a The const char* to process.
 @param b The const char* to process.

 @return true if it succeeds, false if it fails.
 */

bool scmp(const char* a, const char* b)
{
    if(_stricmp(a, b))
        return false;
    return true;
}

/**
 @fn void formathex(char* string)

 @brief Formathexes the given string.

 @param [in,out] string If non-null, the string.
 */

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

/**
 @fn void formatdec(char* string)

 @brief Formatdecs the given string.

 @param [in,out] string If non-null, the string.
 */

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

/**
 @fn bool FileExists(const char* file)

 @brief Queries if a given file exists.

 @param file The file.

 @return true if it succeeds, false if it fails.
 */

bool FileExists(const char* file)
{
    DWORD attrib = GetFileAttributesW(StringUtils::Utf8ToUtf16(file).c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

/**
 @fn bool DirExists(const char* dir)

 @brief Queries if a given dir exists.

 @param dir The dir.

 @return true if it succeeds, false if it fails.
 */

bool DirExists(const char* dir)
{
    DWORD attrib = GetFileAttributesW(StringUtils::Utf8ToUtf16(dir).c_str());
    return (attrib == FILE_ATTRIBUTE_DIRECTORY);
}

/**
 @fn bool GetFileNameFromHandle(HANDLE hFile, char* szFileName)

 @brief Gets file name from handle.

 @param hFile               Handle of the file.
 @param [in,out] szFileName If non-null, filename of the file.

 @return true if it succeeds, false if it fails.
 */

bool GetFileNameFromHandle(HANDLE hFile, char* szFileName)
{
    wchar_t wszFileName[MAX_PATH] = L"";
    if(!PathFromFileHandleW(hFile, wszFileName, sizeof(wszFileName)))
        return false;
    strcpy_s(szFileName, MAX_PATH, StringUtils::Utf16ToUtf8(wszFileName).c_str());
    return true;
}

/**
 @fn bool settingboolget(const char* section, const char* name)

 @brief Settingboolgets.

 @param section The section.
 @param name    The name.

 @return true if it succeeds, false if it fails.
 */

bool settingboolget(const char* section, const char* name)
{
    uint setting;
    if(!BridgeSettingGetUint(section, name, &setting))
        return false;
    if(setting)
        return true;
    return false;
}

/**
 @fn arch GetFileArchitecture(const char* szFileName)

 @brief Gets file architecture.

 @param szFileName Filename of the file.

 @return The file architecture.
 */

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

/**
 @fn bool IsWow64()

 @brief Query if this object is wow 64.

 @return true if wow 64, false if not.
 */

bool IsWow64()
{
    BOOL bIsWow64Process = FALSE;
    //x64_dbg supports WinXP SP3 and later only, so ignore the GetProcAddress crap :D
    IsWow64Process(GetCurrentProcess(), &bIsWow64Process);
    return !!bIsWow64Process;
}