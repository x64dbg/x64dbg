#include "_global.h"
#include "_dbgfunctions.h"
#include "assemble.h"
#include "debugger.h"
#include "addrinfo.h"
#include "patches.h"
#include "memory.h"
#include "disasm_fast.h"
#include "stackinfo.h"
#include "symbolinfo.h"

static DBGFUNCTIONS _dbgfunctions;

const DBGFUNCTIONS* dbgfunctionsget()
{
    return &_dbgfunctions;
}

static bool _assembleatex(duint addr, const char* instruction, char* error, bool fillnop)
{
    return assembleat(addr, instruction, 0, error, fillnop);
}

static bool _sectionfromaddr(duint addr, char* section)
{
    HMODULE hMod = (HMODULE)modbasefromaddr(addr);
    if(!hMod)
        return false;
    wchar_t curModPath[MAX_PATH] = L"";
    if(!GetModuleFileNameExW(fdProcessInfo->hProcess, hMod, curModPath, MAX_PATH))
        return false;
    HANDLE FileHandle;
    DWORD LoadedSize;
    HANDLE FileMap;
    ULONG_PTR FileMapVA;
    if(StaticFileLoadW(curModPath, UE_ACCESS_READ, false, &FileHandle, &LoadedSize, &FileMap, &FileMapVA))
    {
        uint rva = addr - (uint)hMod;
        int sectionNumber = GetPE32SectionNumberFromVA(FileMapVA, GetPE32DataFromMappedFile(FileMapVA, 0, UE_IMAGEBASE) + rva);
        if(sectionNumber >= 0)
        {
            const char* name = (const char*)GetPE32DataFromMappedFile(FileMapVA, sectionNumber, UE_SECTIONNAME);
            if(section)
                strcpy(section, name);
            StaticFileUnloadW(curModPath, false, FileHandle, LoadedSize, FileMap, FileMapVA);
            return true;
        }
        StaticFileUnloadW(curModPath, false, FileHandle, LoadedSize, FileMap, FileMapVA);
    }
    return false;
}

static bool _patchget(duint addr)
{
    return patchget(addr, 0);
}

static bool _patchinrange(duint start, duint end)
{
    if(start > end)
    {
        duint a = start;
        start = end;
        end = a;
    }
    for(duint i = start; i < end + 1; i++)
        if(_patchget(i))
            return true;
    return false;
}

static bool _mempatch(duint va, const unsigned char* src, duint size)
{
    return mempatch(fdProcessInfo->hProcess, (void*)va, src, size, 0);
}

static void _patchrestorerange(duint start, duint end)
{
    if(start > end)
    {
        duint a = start;
        start = end;
        end = a;
    }
    for(duint i = start; i < end + 1; i++)
        patchdel(i, true);
    GuiUpdatePatches();
}

static bool _patchrestore(duint addr)
{
    return patchdel(addr, true);
}

static void _getcallstack(DBGCALLSTACK* callstack)
{
    stackgetcallstack(GetContextDataEx(hActiveThread, UE_CSP), (CALLSTACK*)callstack);
}

static bool _getjitauto(bool* jit_auto)
{
    return dbggetjitauto(jit_auto, notfound, NULL, NULL);
}

static bool _getcmdline(char* cmd_line, size_t* cbsize)
{
    if(!cmd_line && !cbsize)
        return false;
    char* cmdline;
    if(!dbggetcmdline(&cmdline, NULL))
        return false;
    if(!cmd_line && cbsize)
        *cbsize = strlen(cmdline) + sizeof(char);
    else if(cmd_line)
        strcpy(cmd_line, cmdline);
    efree(cmdline, "_getcmdline:cmdline");
    return true;
}

static bool _setcmdline(const char* cmd_line)
{
    return dbgsetcmdline(cmd_line, NULL);
}

static bool _getjit(char* jit, bool jit64)
{
    arch dummy;
    char jit_tmp[JIT_ENTRY_MAX_SIZE] = "";
    if(jit != NULL)
    {
        if(!dbggetjit(jit_tmp, jit64 ? x64 : x32, &dummy, NULL))
            return false;
        strcpy(jit, jit_tmp);
    }
    else // if jit input == NULL: it returns false if there are not an OLD JIT STORED.
    {
        char oldjit[MAX_SETTING_SIZE] = "";
        if(!BridgeSettingGet("JIT", "Old", (char*) & oldjit))
            return false;
    }

    return true;
}

bool _getprocesslist(DBGPROCESSINFO** entries, int* count)
{
    std::vector<PROCESSENTRY32> list;
    if(!dbglistprocesses(&list))
        return false;
    *count = (int)list.size();
    if(!*count)
        return false;
    *entries = (DBGPROCESSINFO*)BridgeAlloc(*count * sizeof(DBGPROCESSINFO));
    for(int i = 0; i < *count; i++)
    {
        (*entries)[*count - i - 1].dwProcessId = list.at(i).th32ProcessID;
        strcpy_s((*entries)[*count - i - 1].szExeFile, list.at(i).szExeFile);
    }
    return true;
}

static void _memupdatemap()
{
    memupdatemap(fdProcessInfo->hProcess);
}

void dbgfunctionsinit()
{
    _dbgfunctions.AssembleAtEx = _assembleatex;
    _dbgfunctions.SectionFromAddr = _sectionfromaddr;
    _dbgfunctions.ModNameFromAddr = modnamefromaddr;
    _dbgfunctions.ModBaseFromAddr = modbasefromaddr;
    _dbgfunctions.ModBaseFromName = modbasefromname;
    _dbgfunctions.ModSizeFromAddr = modsizefromaddr;
    _dbgfunctions.Assemble = assemble;
    _dbgfunctions.PatchGet = _patchget;
    _dbgfunctions.PatchInRange = _patchinrange;
    _dbgfunctions.MemPatch = _mempatch;
    _dbgfunctions.PatchRestoreRange = _patchrestorerange;
    _dbgfunctions.PatchEnum = (PATCHENUM)patchenum;
    _dbgfunctions.PatchRestore = _patchrestore;
    _dbgfunctions.PatchFile = (PATCHFILE)patchfile;
    _dbgfunctions.ModPathFromAddr = modpathfromaddr;
    _dbgfunctions.ModPathFromName = modpathfromname;
    _dbgfunctions.DisasmFast = disasmfast;
    _dbgfunctions.MemUpdateMap = _memupdatemap;
    _dbgfunctions.GetCallStack = _getcallstack;
    _dbgfunctions.SymbolDownloadAllSymbols = symdownloadallsymbols;
    _dbgfunctions.GetJit = _getjit;
    _dbgfunctions.GetJitAuto = _getjitauto;
    _dbgfunctions.GetDefJit = dbggetdefjit;
    _dbgfunctions.GetProcessList = _getprocesslist;
    _dbgfunctions.GetPageRights = dbggetpagerights;
    _dbgfunctions.SetPageRights = dbgsetpagerights;
    _dbgfunctions.PageRightsToString = dbgpagerightstostring;
    _dbgfunctions.IsProcessElevated = IsProcessElevated;
    _dbgfunctions.GetCmdline = _getcmdline;
    _dbgfunctions.SetCmdline = _setcmdline;
    _dbgfunctions.FileOffsetToVa = valfileoffsettova;
    _dbgfunctions.VaToFileOffset = valvatofileoffset;
}
