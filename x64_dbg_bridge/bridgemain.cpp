#include "_global.h"
#include "bridgemain.h"
#include <stdio.h>
#include <new>

static HINSTANCE hInst;
static char szIniFile[1024]="";

#ifdef _WIN64
#define dbg_lib "x64_dbg.dll"
#define gui_lib "x64_gui.dll"
#else
#define dbg_lib "x32_dbg.dll"
#define gui_lib "x32_gui.dll"
#endif // _WIN64

//Bridge
DLL_IMPEXP const char* BridgeInit()
{
    ///Settings load
    GetModuleFileNameA(0, szIniFile, 1024);
    int len=strlen(szIniFile);
    while(szIniFile[len]!='.' and szIniFile[len]!='\\')
        len--;
    if(szIniFile[len]=='\\')
        strcat(szIniFile, ".ini");
    else
        strcpy(&szIniFile[len], ".ini");
    ///GUI Load
    hInstGui=LoadLibraryA(gui_lib); //Sigma
    if(!hInstGui)
        return "Error loading GUI library ("gui_lib")!";
    //_gui_guiinit
    _gui_guiinit=(GUIGUIINIT)GetProcAddress(hInstGui, "_gui_guiinit");
    if(!_gui_guiinit)
        return "Export \"_gui_guiinit\" could not be found!";
    //_gui_disassembleat
    _gui_disassembleat=(GUIDISASSEMBLEAT)GetProcAddress(hInstGui, "_gui_disassembleat");
    if(!_gui_disassembleat)
        return "Export \"_gui_disassembleat\" could not be found!";
    //_gui_setdebugstate
    _gui_setdebugstate=(GUISETDEBUGSTATE)GetProcAddress(hInstGui, "_gui_setdebugstate");
    if(!_gui_setdebugstate)
        return "Export \"_gui_setdebugstate\" could not be found!";
    //_gui_addlogmessage
    _gui_addlogmessage=(GUIADDLOGMESSAGE)GetProcAddress(hInstGui, "_gui_addlogmessage");
    if(!_gui_addlogmessage)
        return "Export \"_gui_addlogmessage\" could not be found!";
    //_gui_logclear
    _gui_logclear=(GUILOGCLEAR)GetProcAddress(hInstGui, "_gui_logclear");
    if(!_gui_logclear)
        return "Export \"_gui_logclear\" could not be found!";
    //_gui_updateregisterview
    _gui_updateregisterview=(GUIUPDATEREGISTERVIEW)GetProcAddress(hInstGui, "_gui_updateregisterview");
    if(!_gui_updateregisterview)
        return "Export \"_gui_updateregisterview\" could not be found!";
    //_gui_updatedisassemblyview
    _gui_updatedisassemblyview=(GUIUPDATEDISASSEMBLYVIEW)GetProcAddress(hInstGui, "_gui_updatedisassemblyview");
    if(!_gui_updatedisassemblyview)
        return "Export \"_gui_updatedisassemblyview\" could not be found!";

    ///DBG Load
    hInstDbg=LoadLibraryA(dbg_lib); //Mr. eXoDia
    if(!hInstDbg)
        return "Error loading debugger library ("dbg_lib")!";
    //_dbg_dbginit
    _dbg_dbginit=(DBGDBGINIT)GetProcAddress(hInstDbg, "_dbg_dbginit");
    if(!_dbg_dbginit)
        return "Export \"_dbg_dbginit\" could not be found!";
    //_dbg_memfindbaseaddr
    _dbg_memfindbaseaddr=(DBGMEMFINDBASEADDR)GetProcAddress(hInstDbg, "_dbg_memfindbaseaddr");
    if(!_dbg_memfindbaseaddr)
        return "Export \"_dbg_memfindbaseaddr\" could not be found!";
    //_dbg_memfindbaseaddr
    _dbg_memread=(DBGMEMREAD)GetProcAddress(hInstDbg, "_dbg_memread");
    if(!_dbg_memread)
        return "Export \"_dbg_memread\" could not be found!";
    //_dbg_dbgcmdexec
    _dbg_dbgcmdexec=(DBGDBGCMDEXEC)GetProcAddress(hInstDbg, "_dbg_dbgcmdexec");
    if(!_dbg_dbgcmdexec)
        return "Export \"_dbg_dbgcmdexec\" could not be found!";
    //_dbg_memmap
    _dbg_memmap=(DBGMEMMAP)GetProcAddress(hInstDbg, "_dbg_memmap");
    if(!_dbg_memmap)
        return "Export \"_dbg_memmap\" could not be found!";
    //_dbg_dbgexitsignal
    _dbg_dbgexitsignal=(DBGDBGEXITSIGNAL)GetProcAddress(hInstDbg, "_dbg_dbgexitsignal");
    if(!_dbg_dbgexitsignal)
        return "Export \"_dbg_dbgexitsignal\" could not be found!";
    //_dbg_valfromstring
    _dbg_valfromstring=(DBGVALFROMSTRING)GetProcAddress(hInstDbg, "_dbg_valfromstring");
    if(!_dbg_valfromstring)
        return "Export \"_dbg_valfromstring\" could not be found!";
    //_dbg_isdebugging
    _dbg_isdebugging=(DBGISDEBUGGING)GetProcAddress(hInstDbg, "_dbg_isdebugging");
    if(!_dbg_isdebugging)
        return "Export \"_dbg_isdebugging\" could not be found!";
    //_dbg_isjumpgoingtoexecute
    _dbg_isjumpgoingtoexecute=(DBGISJUMPGOINGTOEXECUTE)GetProcAddress(hInstDbg, "_dbg_isjumpgoingtoexecute");
    if(!_dbg_isjumpgoingtoexecute)
        return "Export \"_dbg_isjumpgoingtoexecute\" could not be found!";
    //_dbg_addrinfoget
    _dbg_addrinfoget=(DBGADDRINFOGET)GetProcAddress(hInstDbg, "_dbg_addrinfoget");
    if(!_dbg_addrinfoget)
        return "Export \"_dbg_addrinfoget\" could not be found!";
    //_dbg_addrinfoset
    _dbg_addrinfoset=(DBGADDRINFOSET)GetProcAddress(hInstDbg, "_dbg_addrinfoset");
    if(!_dbg_addrinfoset)
        return "Export \"_dbg_addrinfoset\" could not be found!";
    //_dbg_bpgettypeat
    _dbg_bpgettypeat=(DBGBPGETTYPEAT)GetProcAddress(hInstDbg, "_dbg_bpgettypeat");
    if(!_dbg_bpgettypeat)
        return "Export \"_dbg_bpgettypeat\" could not be found!";
    //_dbg_getregdump
    _dbg_getregdump=(DBGGETREGDUMP)GetProcAddress(hInstDbg, "_dbg_getregdump");
    if(!_dbg_getregdump)
        return "Export \"_dbg_getregdump\" could not be found!";
    //_dbg_valtostring
    _dbg_valtostring=(DBGVALTOSTRING)GetProcAddress(hInstDbg, "_dbg_valtostring");
    if(!_dbg_valtostring)
        return "Export \"_dbg_valtostring\" could not be found!";
    //_dbg_memisvalidreadptr
    _dbg_memisvalidreadptr=(DBGMEMISVALIDREADPTR)GetProcAddress(hInstDbg, "_dbg_memisvalidreadptr");
    if(!_dbg_memisvalidreadptr)
        return "Export \"_dbg_memisvalidreadptr\" could not be found!";
    //_dbg_getbplist
    _dbg_getbplist=(DBGGETBPLIST)GetProcAddress(hInstDbg, "_dbg_getbplist");
    if(!_dbg_getbplist)
        return "Export \"_dbg_getbplist\" could not be found!";
    return 0;
}

DLL_IMPEXP const char* BridgeStart()
{
    if(!_dbg_dbginit || !_gui_guiinit)
        return "\"_dbg_dbginit\" || \"_gui_guiinit\" was not loaded yet, call BridgeInit!";
    const char* errormsg=_dbg_dbginit();
    if(errormsg)
        return errormsg;
    _gui_guiinit(0, 0); //remove arguments
    _dbg_dbgexitsignal(); //send exit signal to debugger
    return 0;
}

DLL_IMPEXP void* BridgeAlloc(size_t size)
{
    unsigned char* a= new (std::nothrow)unsigned char[size];
    if(!a)
    {
        MessageBoxA(0, "Could not allocate memory", "Error", MB_ICONERROR);
        ExitProcess(1);
    }
    memset(a, 0, size);
    return a;
}

DLL_IMPEXP void BridgeFree(void* ptr)
{
    delete[] (unsigned char*)ptr;
}

DLL_IMPEXP bool BridgeSettingGet(const char* section, const char* key, char* value)
{
    if(!section || !key || !value)
        return false;
    if(!GetPrivateProfileStringA(section, key, "", value, MAX_SETTING_SIZE, szIniFile))
        return false;
    return true;
}

DLL_IMPEXP bool BridgeSettingGetUint(const char* section, const char* key, duint* value)
{
    if(!section || !key || !value)
        return false;
    char newvalue[MAX_SETTING_SIZE]="";
    if(!BridgeSettingGet(section, key, newvalue))
        return false;
#ifdef _WIN64
    int ret=sscanf(newvalue, "%llX", value);
#else
    int ret=sscanf(newvalue, "%X", value);
#endif //_WIN64
    if(ret)
        return true;
    return false;
}

DLL_IMPEXP bool BridgeSettingSet(const char* section, const char* key, const char* value)
{
    if(!section || !key || !value)
        return false;
    if(!WritePrivateProfileStringA(section, key, value, szIniFile))
        return false;
    return true;
}

DLL_IMPEXP bool BridgeSettingSetUint(const char* section, const char* key, duint value)
{
    if(!section || !key)
        return false;
    char newvalue[MAX_SETTING_SIZE]="";
#ifdef _WIN64
    sprintf(newvalue, "%llX", value);
#else
    sprintf(newvalue, "%X", value);
#endif //_WIN64
    return BridgeSettingSet(section, key, newvalue);
}

//Debugger
DLL_IMPEXP void DbgMemRead(duint va, unsigned char* dest, duint size)
{
    if(!_dbg_memread(va, dest, size, 0))
        memset(dest, 0x90, size);
}

DLL_IMPEXP duint DbgMemGetPageSize(duint base)
{
    duint size=0;
    _dbg_memfindbaseaddr(base, &size);
    return size;
}

DLL_IMPEXP duint DbgMemFindBaseAddr(duint addr, duint* size)
{
    return _dbg_memfindbaseaddr(addr, size);
}

DLL_IMPEXP bool DbgCmdExec(const char* cmd)
{
    return _dbg_dbgcmdexec(cmd);
}

DLL_IMPEXP bool DbgMemMap(MEMMAP* memmap)
{
    return _dbg_memmap(memmap);
}

DLL_IMPEXP bool DbgIsValidExpression(const char* expression)
{
    duint value=0;
    return _dbg_valfromstring(expression, &value);
}

DLL_IMPEXP bool DbgIsDebugging()
{
    return _dbg_isdebugging();
}

DLL_IMPEXP bool DbgIsJumpGoingToExecute(duint addr)
{
    return _dbg_isjumpgoingtoexecute(addr);
}

DLL_IMPEXP bool DbgGetLabelAt(duint addr, SEGMENTREG segment, char* text) //(module.)+label
{
    if(!text || !addr)
        return false;
    ADDRINFO info;
    memset(&info, 0, sizeof(info));
    info.flags=flaglabel;
    if(!_dbg_addrinfoget(addr, segment, &info))
        return false;
    strcpy(text, info.label);
    return true;
}

DLL_IMPEXP bool DbgSetLabelAt(duint addr, const char* text)
{
    if(!text || strlen(text)>=MAX_LABEL_SIZE || !addr)
        return false;
    ADDRINFO info;
    memset(&info, 0, sizeof(info));
    info.flags=flaglabel;
    strcpy(info.label, text);
    if(!_dbg_addrinfoset(addr, &info))
        return false;
    return true;
}

DLL_IMPEXP bool DbgGetCommentAt(duint addr, char* text) //comment (not live)
{
    if(!text || !addr)
        return false;
    ADDRINFO info;
    memset(&info, 0, sizeof(info));
    info.flags=flagcomment;
    if(!_dbg_addrinfoget(addr, SEG_DEFAULT, &info))
        return false;
    strcpy(text, info.comment);
    return true;
}

DLL_IMPEXP bool DbgSetCommentAt(duint addr, const char* text)
{
    if(!text || strlen(text)>=MAX_COMMENT_SIZE || !addr)
        return false;
    ADDRINFO info;
    memset(&info, 0, sizeof(info));
    info.flags=flagcomment;
    strcpy(info.comment, text);
    if(!_dbg_addrinfoset(addr, &info))
        return false;
    return true;
}

DLL_IMPEXP bool DbgGetModuleAt(duint addr, char* text)
{
    if(!text || !addr)
        return false;
    ADDRINFO info;
    memset(&info, 0, sizeof(info));
    info.flags=flagmodule;
    if(!_dbg_addrinfoget(addr, SEG_DEFAULT, &info))
        return false;
    strcpy(text, info.module);
    return true;
}

DLL_IMPEXP bool DbgGetBookmarkAt(duint addr)
{
    if(!addr)
        return false;
    ADDRINFO info;
    memset(&info, 0, sizeof(info));
    info.flags=flagbookmark;
    if(!_dbg_addrinfoget(addr, SEG_DEFAULT, &info))
        return false;
    return info.isbookmark;
}

DLL_IMPEXP bool DbgSetBookmarkAt(duint addr, bool isbookmark)
{
    if(!addr)
        return false;
    ADDRINFO info;
    memset(&info, 0, sizeof(info));
    info.flags=flagbookmark;
    info.isbookmark=isbookmark;
    return _dbg_addrinfoset(addr, &info);
}

DLL_IMPEXP BPXTYPE DbgGetBpxTypeAt(duint addr)
{
    return _dbg_bpgettypeat(addr);
}

DLL_IMPEXP duint DbgValFromString(const char* string)
{
    duint value=0;
    _dbg_valfromstring(string, &value);
    return value;
}

DLL_IMPEXP bool DbgGetRegDump(REGDUMP* regdump)
{
    return _dbg_getregdump(regdump);
}

DLL_IMPEXP bool DbgValToString(const char* string, duint value)
{
    duint valueCopy=value;
    return _dbg_valtostring(string, &valueCopy);
}

DLL_IMPEXP bool DbgMemIsValidReadPtr(duint addr)
{
    return _dbg_memisvalidreadptr(addr);
}

DLL_IMPEXP int DbgGetBpList(BPXTYPE type, BPMAP* list)
{
    return _dbg_getbplist(type, list);
}

//GUI
DLL_IMPEXP void GuiDisasmAt(duint addr, duint cip)
{
    _gui_disassembleat(addr, cip);
}

DLL_IMPEXP void GuiSetDebugState(DBGSTATE state)
{
    _gui_setdebugstate(state);
}

DLL_IMPEXP void GuiAddLogMessage(const char* msg)
{
    _gui_addlogmessage(msg);
}

DLL_IMPEXP void GuiLogClear()
{
    _gui_logclear();
}

DLL_IMPEXP void GuiUpdateAllViews()
{
    GuiUpdateRegisterView();
    GuiUpdateDisassemblyView();
}

DLL_IMPEXP void GuiUpdateRegisterView()
{
    _gui_updateregisterview();
}

DLL_IMPEXP void GuiUpdateDisassemblyView()
{
    _gui_updatedisassemblyview();
}

//Main
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    hInst=hinstDLL;
    return TRUE;
}

