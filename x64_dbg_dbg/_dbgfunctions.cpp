#include "_global.h"
#include "_dbgfunctions.h"
#include "assemble.h"
#include "debugger.h"
#include "addrinfo.h"

static DBGFUNCTIONS _dbgfunctions;

const DBGFUNCTIONS* dbgfunctionsget()
{
    return &_dbgfunctions;
}

static bool sectionfromaddr(duint addr, char* section)
{
    HMODULE hMod=(HMODULE)modbasefromaddr(addr);
    if(!hMod)
        return false;
    char curModPath[MAX_PATH]="";
    if(!GetModuleFileNameExA(fdProcessInfo->hProcess, hMod, curModPath, MAX_PATH))
        return false;
    HANDLE FileHandle;
    DWORD LoadedSize;
    HANDLE FileMap;
    ULONG_PTR FileMapVA;
    if(StaticFileLoad(curModPath, UE_ACCESS_READ, false, &FileHandle, &LoadedSize, &FileMap, &FileMapVA))
    {
        uint rva=addr-(uint)hMod;
        int sectionNumber=GetPE32SectionNumberFromVA(FileMapVA, GetPE32DataFromMappedFile(FileMapVA, 0, UE_IMAGEBASE)+rva);
        if(sectionNumber>=0)
        {
            const char* name=(const char*)GetPE32DataFromMappedFile(FileMapVA, sectionNumber, UE_SECTIONNAME);
            if(section)
                strcpy(section, name);
            StaticFileUnload(curModPath, false, FileHandle, LoadedSize, FileMap, FileMapVA);
            return true;
        }
        StaticFileUnload(curModPath, false, FileHandle, LoadedSize, FileMap, FileMapVA);
    }
    return false;
}

void dbgfunctionsinit()
{
    _dbgfunctions.AssembleAtEx=assembleat;
    _dbgfunctions.SectionFromAddr=sectionfromaddr;
    _dbgfunctions.ModNameFromAddr=modnamefromaddr;
    _dbgfunctions.ModBaseFromAddr=modbasefromaddr;
    _dbgfunctions.ModBaseFromName=modbasefromname;
    _dbgfunctions.ModSizeFromAddr=modsizefromaddr;
    _dbgfunctions.Assemble=assemble;
}