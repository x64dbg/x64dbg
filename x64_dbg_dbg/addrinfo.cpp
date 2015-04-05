/**
 @file addrinfo.cpp

 @brief Implements the addrinfo class.
 */

#include "addrinfo.h"
#include "debugger.h"
#include "console.h"
#include "memory.h"
#include "breakpoint.h"
#include "threading.h"
#include "symbolinfo.h"
#include "murmurhash.h"
#include "lz4\lz4file.h"
#include "patches.h"
#include "module.h"
#include "comment.h"
#include "label.h"
#include "bookmark.h"
#include "function.h"
#include "loop.h"

//database functions
void dbsave()
{
    dprintf("saving database...");
    DWORD ticks = GetTickCount();
    JSON root = json_object();
    CommentCacheSave(root);
    LabelCacheSave(root);
    BookmarkCacheSave(root);
    FunctionCacheSave(root);
    LoopCacheSave(root);
    BpCacheSave(root);
    WString wdbpath = StringUtils::Utf8ToUtf16(dbpath);
    if(json_object_size(root))
    {
        FILE* jsonFile = 0;
        if(_wfopen_s(&jsonFile, wdbpath.c_str(), L"wb"))
        {
            dputs("failed to open database file for editing!");
            json_decref(root); //free root
            return;
        }
        if(json_dumpf(root, jsonFile, JSON_INDENT(4)) == -1)
        {
            dputs("couldn't write JSON to database file...");
            json_decref(root); //free root
            return;
        }
        fclose(jsonFile);
        if(!settingboolget("Engine", "DisableCompression"))
            LZ4_compress_fileW(wdbpath.c_str(), wdbpath.c_str());
    }
    else //remove database when nothing is in there
        DeleteFileW(wdbpath.c_str());
    dprintf("%ums\n", GetTickCount() - ticks);
    json_decref(root); //free root
}

void dbload()
{
    // If the file doesn't exist, there is no DB to load
    if(!FileExists(dbpath))
        return;

    dprintf("Loading database...");
    DWORD ticks = GetTickCount();

    // Multi-byte (UTF8) file path converted to UTF16
    WString databasePathW = StringUtils::Utf8ToUtf16(dbpath);

    // Decompress the file if compression was enabled
    bool useCompression     = !settingboolget("Engine", "DisableCompression");
    LZ4_STATUS lzmaStatus   = LZ4_INVALID_ARCHIVE;
    {
        lzmaStatus = LZ4_decompress_fileW(databasePathW.c_str(), databasePathW.c_str());

        // Check return code
        if(useCompression && lzmaStatus != LZ4_SUCCESS && lzmaStatus != LZ4_INVALID_ARCHIVE)
        {
            dputs("\nInvalid database file!");
            return;
        }
    }

    // Open the file for reading by the JSON parser
    FILE* jsonFile      = nullptr;
    long jsonFileSize   = 0;

    if(_wfopen_s(&jsonFile, databasePathW.c_str(), L"rb"))
    {
        dputs("\nFailed to open database file!");
        return;
    }

    // Get the current file size
    fseek(jsonFile, 0, SEEK_END);
    jsonFileSize = ftell(jsonFile);
    fseek(jsonFile, 0, SEEK_SET);

    // Verify that the file size is greater than 0.
    // This corrects a bug when a file exists, but there is no data inside.
    JSON root = nullptr;

    if(jsonFileSize > 0)
        root = json_loadf(jsonFile, 0, 0);

    // Release the file handle and re-compress
    fclose(jsonFile);

    if(lzmaStatus != LZ4_INVALID_ARCHIVE && useCompression)
        LZ4_compress_fileW(databasePathW.c_str(), databasePathW.c_str());

    // Validate JSON load status
    if(!root)
    {
        dputs("\nInvalid database file (JSON)!");
        return;
    }

    // Finally load all structures
    CommentCacheLoad(root);
    LabelCacheLoad(root);
    BookmarkCacheLoad(root);
    FunctionCacheLoad(root);
    LoopCacheLoad(root);
    BpCacheLoad(root);

    // Free root
    json_decref(root);
    dprintf("%ums\n", GetTickCount() - ticks);
}

void dbclose()
{
    dbsave();
    CommentClear();
    LabelClear();
    BookmarkClear();
    FunctionClear();
    LoopClear();
    BpClear();
    PatchClear();
}

///api functions
bool apienumexports(uint base, EXPORTENUMCALLBACK cbEnum)
{
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQueryEx(fdProcessInfo->hProcess, (const void*)base, &mbi, sizeof(mbi));
    uint size = mbi.RegionSize;
    Memory<void*> buffer(size, "apienumexports:buffer");
    if(!MemRead((void*)base, buffer, size, 0))
        return false;
    IMAGE_NT_HEADERS* pnth = (IMAGE_NT_HEADERS*)((uint)buffer + GetPE32DataFromMappedFile((ULONG_PTR)buffer, 0, UE_PE_OFFSET));
    uint export_dir_rva = pnth->OptionalHeader.DataDirectory[0].VirtualAddress;
    uint export_dir_size = pnth->OptionalHeader.DataDirectory[0].Size;
    IMAGE_EXPORT_DIRECTORY export_dir;
    memset(&export_dir, 0, sizeof(export_dir));
    MemRead((void*)(export_dir_rva + base), &export_dir, sizeof(export_dir), 0);
    unsigned int NumberOfNames = export_dir.NumberOfNames;
    if(!export_dir.NumberOfFunctions or !NumberOfNames) //no named exports
        return false;
    char modname[MAX_MODULE_SIZE] = "";
    ModNameFromAddr(base, modname, true);
    uint original_name_va = export_dir.Name + base;
    char original_name[deflen] = "";
    memset(original_name, 0, sizeof(original_name));
    MemRead((void*)original_name_va, original_name, deflen, 0);
    char* AddrOfFunctions_va = (char*)(export_dir.AddressOfFunctions + base);
    char* AddrOfNames_va = (char*)(export_dir.AddressOfNames + base);
    char* AddrOfNameOrdinals_va = (char*)(export_dir.AddressOfNameOrdinals + base);
    for(DWORD i = 0; i < NumberOfNames; i++)
    {
        DWORD curAddrOfName = 0;
        MemRead(AddrOfNames_va + sizeof(DWORD)*i, &curAddrOfName, sizeof(DWORD), 0);
        char* cur_name_va = (char*)(curAddrOfName + base);
        char cur_name[deflen] = "";
        memset(cur_name, 0, deflen);
        MemRead(cur_name_va, cur_name, deflen, 0);
        WORD curAddrOfNameOrdinals = 0;
        MemRead(AddrOfNameOrdinals_va + sizeof(WORD)*i, &curAddrOfNameOrdinals, sizeof(WORD), 0);
        DWORD curFunctionRva = 0;
        MemRead(AddrOfFunctions_va + sizeof(DWORD)*curAddrOfNameOrdinals, &curFunctionRva, sizeof(DWORD), 0);

        if(curFunctionRva >= export_dir_rva and curFunctionRva < export_dir_rva + export_dir_size)
        {
            char forwarded_api[deflen] = "";
            memset(forwarded_api, 0, deflen);
            MemRead((void*)(curFunctionRva + base), forwarded_api, deflen, 0);
            int len = (int)strlen(forwarded_api);
            int j = 0;
            while(forwarded_api[j] != '.' and j < len)
                j++;
            if(forwarded_api[j] == '.')
            {
                forwarded_api[j] = 0;
                HINSTANCE hTempDll = LoadLibraryExA(forwarded_api, 0, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
                if(hTempDll)
                {
                    uint local_addr = (uint)GetProcAddress(hTempDll, forwarded_api + j + 1);
                    if(local_addr)
                    {
                        uint remote_addr = ImporterGetRemoteAPIAddress(fdProcessInfo->hProcess, local_addr);
                        cbEnum(base, modname, cur_name, remote_addr);
                    }
                }
            }
        }
        else
        {
            cbEnum(base, modname, cur_name, curFunctionRva + base);
        }
    }
    return true;
}