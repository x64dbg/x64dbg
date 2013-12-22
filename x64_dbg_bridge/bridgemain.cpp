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
BRIDGE_IMPEXP const char* BridgeInit()
{
    ///Settings load
    if(!GetModuleFileNameA(0, szIniFile, 1024))
        return "Error getting module path!";
    int len=strlen(szIniFile);
    while(szIniFile[len]!='.' && szIniFile[len]!='\\')
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
    //_gui_sendmessage;
    _gui_sendmessage=(GUISENDMESSAGE)GetProcAddress(hInstGui, "_gui_sendmessage");
    if(!_gui_sendmessage)
        return "Export \"_gui_sendmessage\" could not be found!";

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
    //_dbg_dbgcmddirectexec
    _dbg_dbgcmddirectexec=(DBGDBGCMDEXECDIRECT)GetProcAddress(hInstDbg, "_dbg_dbgcmddirectexec");
    if(!_dbg_dbgcmddirectexec)
        return "Export \"_dbg_dbgcmddirectexec\" could not be found!";
    return 0;
}

BRIDGE_IMPEXP const char* BridgeStart()
{
    if(!_dbg_dbginit || !_gui_guiinit)
        return "\"_dbg_dbginit\" || \"_gui_guiinit\" was not loaded yet, call BridgeInit!";
    _gui_guiinit(0, 0); //remove arguments
    _dbg_dbgexitsignal(); //send exit signal to debugger
    return 0;
}

BRIDGE_IMPEXP void* BridgeAlloc(size_t size)
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

BRIDGE_IMPEXP void BridgeFree(void* ptr)
{
    delete[] (unsigned char*)ptr;
}

BRIDGE_IMPEXP bool BridgeSettingGet(const char* section, const char* key, char* value)
{
    if(!section || !key || !value)
        return false;
    if(!GetPrivateProfileStringA(section, key, "", value, MAX_SETTING_SIZE, szIniFile))
        return false;
    return true;
}

BRIDGE_IMPEXP bool BridgeSettingGetUint(const char* section, const char* key, duint* value)
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

BRIDGE_IMPEXP bool BridgeSettingSet(const char* section, const char* key, const char* value)
{
    if(!section || !key || !value)
        return false;
    if(!WritePrivateProfileStringA(section, key, value, szIniFile))
        return false;
    return true;
}

BRIDGE_IMPEXP bool BridgeSettingSetUint(const char* section, const char* key, duint value)
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
BRIDGE_IMPEXP void DbgMemRead(duint va, unsigned char* dest, duint size)
{
    if(!_dbg_memread(va, dest, size, 0))
        memset(dest, 0x90, size);
}

BRIDGE_IMPEXP duint DbgMemGetPageSize(duint base)
{
    duint size=0;
    _dbg_memfindbaseaddr(base, &size);
    return size;
}

BRIDGE_IMPEXP duint DbgMemFindBaseAddr(duint addr, duint* size)
{
    return _dbg_memfindbaseaddr(addr, size);
}

BRIDGE_IMPEXP bool DbgCmdExec(const char* cmd)
{
    return _dbg_dbgcmdexec(cmd);
}

BRIDGE_IMPEXP bool DbgMemMap(MEMMAP* memmap)
{
    return _dbg_memmap(memmap);
}

BRIDGE_IMPEXP bool DbgIsValidExpression(const char* expression)
{
    duint value=0;
    return _dbg_valfromstring(expression, &value);
}

BRIDGE_IMPEXP bool DbgIsDebugging()
{
    return _dbg_isdebugging();
}

BRIDGE_IMPEXP bool DbgIsJumpGoingToExecute(duint addr)
{
    return _dbg_isjumpgoingtoexecute(addr);
}

BRIDGE_IMPEXP bool DbgGetLabelAt(duint addr, SEGMENTREG segment, char* text) //(module.)+label
{
    if(!text || !addr)
        return false;
    ADDRINFO info;
    memset(&info, 0, sizeof(info));
    info.flags=flaglabel;
    if(!_dbg_addrinfoget(addr, segment, &info))
    {
        duint addr_=0;
        if(!DbgMemIsValidReadPtr(addr))
            return false;
        DbgMemRead(addr, (unsigned char*)&addr_, sizeof(duint));
        if(!_dbg_addrinfoget(addr_, SEG_DEFAULT, &info))
            return false;
    }
    strcpy(text, info.label);
    return true;
}

BRIDGE_IMPEXP bool DbgSetLabelAt(duint addr, const char* text)
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

BRIDGE_IMPEXP bool DbgGetCommentAt(duint addr, char* text) //comment (not live)
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

BRIDGE_IMPEXP bool DbgSetCommentAt(duint addr, const char* text)
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

BRIDGE_IMPEXP bool DbgGetModuleAt(duint addr, char* text)
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

BRIDGE_IMPEXP bool DbgGetBookmarkAt(duint addr)
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

BRIDGE_IMPEXP bool DbgSetBookmarkAt(duint addr, bool isbookmark)
{
    if(!addr)
        return false;
    ADDRINFO info;
    memset(&info, 0, sizeof(info));
    info.flags=flagbookmark;
    info.isbookmark=isbookmark;
    return _dbg_addrinfoset(addr, &info);
}

BRIDGE_IMPEXP const char* DbgInit()
{
    return _dbg_dbginit();
}

BRIDGE_IMPEXP BPXTYPE DbgGetBpxTypeAt(duint addr)
{
    return _dbg_bpgettypeat(addr);
}

BRIDGE_IMPEXP duint DbgValFromString(const char* string)
{
    duint value=0;
    _dbg_valfromstring(string, &value);
    return value;
}

BRIDGE_IMPEXP bool DbgGetRegDump(REGDUMP* regdump)
{
    return _dbg_getregdump(regdump);
}

BRIDGE_IMPEXP bool DbgValToString(const char* string, duint value)
{
    duint valueCopy=value;
    return _dbg_valtostring(string, &valueCopy);
}

BRIDGE_IMPEXP bool DbgMemIsValidReadPtr(duint addr)
{
    return _dbg_memisvalidreadptr(addr);
}

BRIDGE_IMPEXP int DbgGetBpList(BPXTYPE type, BPMAP* list)
{
    return _dbg_getbplist(type, list);
}

BRIDGE_IMPEXP bool DbgCmdExecDirect(const char* cmd)
{
    return _dbg_dbgcmddirectexec(cmd);
}

BRIDGE_IMPEXP FUNCTYPE DbgGetFunctionTypeAt(duint addr)
{
    //NOTE: test code for 'function.exe'
    if(addr==0x0040132A)
        return FUNC_BEGIN;
    else if(addr>0x0040132A && addr<0x004013BA)
        return FUNC_MIDDLE;
    else if(addr==0x004013BA)
        return FUNC_END;
    else if(addr==0x004013BB)
        return FUNC_SINGLE;
    return FUNC_NONE;
}

BRIDGE_IMPEXP LOOPTYPE DbgGetLoopTypeAt(duint addr, int depth)
{
    //NOTE: test code for 'function.exe'
    if(depth==0)
    {
        if(addr==0x00401348)
            return LOOP_BEGIN;
        else if(addr>0x00401348 && addr<0x004013B3)
            return LOOP_MIDDLE;
        else if(addr==0x004013B3)
            return LOOP_END;
    }
    else if(depth==1)
    {
        if(addr==0x00401351)
            return LOOP_BEGIN;
        else if(addr>0x00401351 && addr<0x004013A3)
            return LOOP_MIDDLE;
        else if(addr==0x004013A3)
            return LOOP_END;
    }
    else if(depth==2)
    {
        if(addr==0x0040135A)
            return LOOP_BEGIN;
        else if(addr>0x0040135A && addr<0x00401393)
            return LOOP_MIDDLE;
        else if(addr==0x00401393)
            return LOOP_END;
    }
    return LOOP_NONE;
}

//GUI
BRIDGE_IMPEXP void GuiDisasmAt(duint addr, duint cip)
{
    _gui_sendmessage(GUI_DISASSEMBLE_AT, (void*)addr, (void*)cip);
}

BRIDGE_IMPEXP void GuiSetDebugState(DBGSTATE state)
{
    _gui_sendmessage(GUI_SET_DEBUG_STATE, (void*)state, 0);
}

BRIDGE_IMPEXP void GuiAddLogMessage(const char* msg)
{
    _gui_sendmessage(GUI_ADD_MSG_TO_LOG, (void*)msg, 0);
}

BRIDGE_IMPEXP void GuiLogClear()
{
    _gui_sendmessage(GUI_CLEAR_LOG, 0, 0);
}

BRIDGE_IMPEXP void GuiUpdateAllViews()
{
    GuiUpdateRegisterView();
    GuiUpdateDisassemblyView();
    GuiUpdateBreakpointsView();
}

BRIDGE_IMPEXP void GuiUpdateRegisterView()
{
    _gui_sendmessage(GUI_UPDATE_REGISTER_VIEW, 0, 0);
}

BRIDGE_IMPEXP void GuiUpdateDisassemblyView()
{
    _gui_sendmessage(GUI_UPDATE_DISASSEMBLY_VIEW, 0, 0);
}

BRIDGE_IMPEXP void GuiUpdateBreakpointsView()
{
    _gui_sendmessage(GUI_UPDATE_BREAKPOINTS_VIEW, 0, 0);
}

BRIDGE_IMPEXP void GuiUpdateWindowTitle(const char* filename)
{
    _gui_sendmessage(GUI_UPDATE_WINDOW_TITLE, (void*)filename, 0);
}

BRIDGE_IMPEXP void GuiUpdateCPUTitle(const char* modname)
{
    _gui_sendmessage(GUI_UPDATE_CPU_TITLE, (void*)modname, 0);
}

BRIDGE_IMPEXP HWND GuiGetWindowHandle()
{
    return (HWND)_gui_sendmessage(GUI_GET_WINDOW_HANDLE, 0, 0);
}

//Main
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    hInst=hinstDLL;
    return TRUE;
}

