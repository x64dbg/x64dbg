#include "_global.h"
#include "bridgemain.h"
#include <stdio.h>
#include "simpleini.h"

static HINSTANCE hInst;
static wchar_t szIniFile[MAX_PATH] = L"";

#ifdef _WIN64
#define dbg_lib "x64_dbg.dll"
#define gui_lib "x64_gui.dll"
#else
#define dbg_lib "x32_dbg.dll"
#define gui_lib "x32_gui.dll"
#endif // _WIN64

#define LOADLIBRARY(name) \
    szLib=name; \
    hInst=LoadLibraryA(name); \
    if(!hInst) \
        return "Error loading library \""name"\"!"

#define LOADEXPORT(name) \
    *((FARPROC*)&name)=GetProcAddress(hInst, #name); \
    if(!name) \
    { \
        sprintf(szError, "Export %s:%s could not be found!", szLib, #name); \
        return szError; \
    }

//Bridge
BRIDGE_IMPEXP const char* BridgeInit()
{
    ///Settings load
    if(!GetModuleFileNameW(0, szIniFile, MAX_PATH))
        return "Error getting module path!";
    int len = (int)wcslen(szIniFile);
    while(szIniFile[len] != L'.' && szIniFile[len] != L'\\' && len)
        len--;
    if(szIniFile[len] == L'\\')
        wcscat_s(szIniFile, L".ini");
    else
        wcscpy_s(&szIniFile[len], _countof(szIniFile) - len, L".ini");

    HINSTANCE hInst;
    const char* szLib;
    static char szError[256] = "";

    //GUI Load
    LOADLIBRARY(gui_lib);
    LOADEXPORT(_gui_guiinit);
    LOADEXPORT(_gui_sendmessage);

    //DBG Load
    LOADLIBRARY(dbg_lib);
    LOADEXPORT(_dbg_dbginit);
    LOADEXPORT(_dbg_memfindbaseaddr);
    LOADEXPORT(_dbg_memread);
    LOADEXPORT(_dbg_memwrite);
    LOADEXPORT(_dbg_dbgcmdexec);
    LOADEXPORT(_dbg_memmap);
    LOADEXPORT(_dbg_dbgexitsignal);
    LOADEXPORT(_dbg_valfromstring);
    LOADEXPORT(_dbg_isdebugging);
    LOADEXPORT(_dbg_isjumpgoingtoexecute);
    LOADEXPORT(_dbg_addrinfoget);
    LOADEXPORT(_dbg_addrinfoset);
    LOADEXPORT(_dbg_bpgettypeat);
    LOADEXPORT(_dbg_getregdump);
    LOADEXPORT(_dbg_valtostring);
    LOADEXPORT(_dbg_memisvalidreadptr);
    LOADEXPORT(_dbg_getbplist);
    LOADEXPORT(_dbg_dbgcmddirectexec);
    LOADEXPORT(_dbg_getbranchdestination);
    LOADEXPORT(_dbg_sendmessage);
    return 0;
}

BRIDGE_IMPEXP const char* BridgeStart()
{
    if(!_dbg_dbginit || !_gui_guiinit)
        return "\"_dbg_dbginit\" || \"_gui_guiinit\" was not loaded yet, call BridgeInit!";
    _gui_guiinit(0, 0); //remove arguments
    return 0;
}

BRIDGE_IMPEXP void* BridgeAlloc(size_t size)
{
    unsigned char* a = (unsigned char*)GlobalAlloc(GMEM_FIXED, size);
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
    GlobalFree(ptr);
}

BRIDGE_IMPEXP bool BridgeSettingGet(const char* section, const char* key, char* value)
{
    if(!section || !key || !value)
        return false;
    CSimpleIniA inifile(true, false, false);
    if(inifile.LoadFile(szIniFile) < 0)
        return false;
    const char* szValue = inifile.GetValue(section, key);
    if(!szValue)
        return false;
    strcpy_s(value, MAX_SETTING_SIZE, szValue);
    return true;
}

BRIDGE_IMPEXP bool BridgeSettingGetUint(const char* section, const char* key, duint* value)
{
    if(!section || !key || !value)
        return false;
    char newvalue[MAX_SETTING_SIZE] = "";
    if(!BridgeSettingGet(section, key, newvalue))
        return false;
#ifdef _WIN64
    int ret = sscanf(newvalue, "%llX", value);
#else
    int ret = sscanf(newvalue, "%X", value);
#endif //_WIN64
    if(ret)
        return true;
    return false;
}

BRIDGE_IMPEXP bool BridgeSettingSet(const char* section, const char* key, const char* value)
{
    if(!section)
        return false;
    CSimpleIniA inifile(true, false, false);
    inifile.LoadFile(szIniFile);
    if(!key || !value) //delete value/key when 0
        inifile.Delete(section, key, true);
    else
        inifile.SetValue(section, key, value);
    return inifile.SaveFile(szIniFile, false) >= 0;
}

BRIDGE_IMPEXP bool BridgeSettingSetUint(const char* section, const char* key, duint value)
{
    if(!section || !key)
        return false;
    char newvalue[MAX_SETTING_SIZE] = "";
#ifdef _WIN64
    sprintf(newvalue, "%llX", value);
#else
    sprintf(newvalue, "%X", value);
#endif //_WIN64
    return BridgeSettingSet(section, key, newvalue);
}

BRIDGE_IMPEXP int BridgeGetDbgVersion()
{
    return DBG_VERSION;
}

//Debugger
BRIDGE_IMPEXP bool DbgMemRead(duint va, unsigned char* dest, duint size)
{
    if(IsBadWritePtr(dest, size))
    {
        GuiAddLogMessage("DbgMemRead with invalid boundaries!\n");
        return false;
    }
    bool ret = _dbg_memread(va, dest, size, 0);
    if(!ret)
        memset(dest, 0x90, size);
    return ret;
}

BRIDGE_IMPEXP bool DbgMemWrite(duint va, const unsigned char* src, duint size)
{
    if(IsBadReadPtr(src, size))
    {
        GuiAddLogMessage("DbgMemWrite with invalid boundaries!\n");
        return false;
    }
    return _dbg_memwrite(va, src, size, 0);
}

BRIDGE_IMPEXP duint DbgMemGetPageSize(duint base)
{
    duint size = 0;
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
    duint value = 0;
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
    info.flags = flaglabel;
    if(!_dbg_addrinfoget(addr, segment, &info))
    {
        duint addr_ = 0;
        if(!DbgMemIsValidReadPtr(addr))
            return false;
        DbgMemRead(addr, (unsigned char*)&addr_, sizeof(duint));
        ADDRINFO ptrinfo = info;
        if(!_dbg_addrinfoget(addr_, SEG_DEFAULT, &ptrinfo))
            return false;
        sprintf_s(info.label, "&%s", ptrinfo.label);
    }
    strcpy(text, info.label);
    return true;
}

BRIDGE_IMPEXP bool DbgSetLabelAt(duint addr, const char* text)
{
    if(!text || strlen(text) >= MAX_LABEL_SIZE || !addr)
        return false;
    ADDRINFO info;
    memset(&info, 0, sizeof(info));
    info.flags = flaglabel;
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
    info.flags = flagcomment;
    if(!_dbg_addrinfoget(addr, SEG_DEFAULT, &info))
        return false;
    strcpy(text, info.comment);
    return true;
}

BRIDGE_IMPEXP bool DbgSetCommentAt(duint addr, const char* text)
{
    if(!text || strlen(text) >= MAX_COMMENT_SIZE || !addr)
        return false;
    ADDRINFO info;
    memset(&info, 0, sizeof(info));
    info.flags = flagcomment;
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
    info.flags = flagmodule;
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
    info.flags = flagbookmark;
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
    info.flags = flagbookmark;
    info.isbookmark = isbookmark;
    return _dbg_addrinfoset(addr, &info);
}

BRIDGE_IMPEXP const char* DbgInit()
{
    return _dbg_dbginit();
}

BRIDGE_IMPEXP void DbgExit()
{
    _dbg_dbgexitsignal(); //send exit signal to debugger
}

BRIDGE_IMPEXP BPXTYPE DbgGetBpxTypeAt(duint addr)
{
    return _dbg_bpgettypeat(addr);
}

BRIDGE_IMPEXP duint DbgValFromString(const char* string)
{
    duint value = 0;
    _dbg_valfromstring(string, &value);
    return value;
}

BRIDGE_IMPEXP bool DbgGetRegDump(REGDUMP* regdump)
{
    return _dbg_getregdump(regdump);
}

BRIDGE_IMPEXP bool DbgValToString(const char* string, duint value)
{
    duint valueCopy = value;
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
    ADDRINFO info;
    memset(&info, 0, sizeof(info));
    info.flags = flagfunction;
    if(!_dbg_addrinfoget(addr, SEG_DEFAULT, &info))
        return FUNC_NONE;
    duint start = info.function.start;
    duint end = info.function.end;
    if(start == end)
        return FUNC_SINGLE;
    else if(addr == start)
        return FUNC_BEGIN;
    else if(addr == end)
        return FUNC_END;
    return FUNC_MIDDLE;
}

BRIDGE_IMPEXP LOOPTYPE DbgGetLoopTypeAt(duint addr, int depth)
{
    ADDRINFO info;
    memset(&info, 0, sizeof(info));
    info.flags = flagloop;
    info.loop.depth = depth;
    if(!_dbg_addrinfoget(addr, SEG_DEFAULT, &info))
        return LOOP_NONE;
    duint start = info.loop.start;
    duint end = info.loop.end;
    if(addr == start)
        return LOOP_BEGIN;
    else if(addr == end)
        return LOOP_END;
    return LOOP_MIDDLE;
}

BRIDGE_IMPEXP duint DbgGetBranchDestination(duint addr)
{
    return _dbg_getbranchdestination(addr);
}

BRIDGE_IMPEXP void DbgScriptLoad(const char* filename)
{
    _dbg_sendmessage(DBG_SCRIPT_LOAD, (void*)filename, 0);
}

BRIDGE_IMPEXP void DbgScriptUnload()
{
    _dbg_sendmessage(DBG_SCRIPT_UNLOAD, 0, 0);
}

BRIDGE_IMPEXP void DbgScriptRun(int destline)
{
    _dbg_sendmessage(DBG_SCRIPT_RUN, (void*)(duint)destline, 0);
}

BRIDGE_IMPEXP void DbgScriptStep()
{
    _dbg_sendmessage(DBG_SCRIPT_STEP, 0, 0);
}

BRIDGE_IMPEXP bool DbgScriptBpToggle(int line)
{
    if(_dbg_sendmessage(DBG_SCRIPT_BPTOGGLE, (void*)(duint)line, 0))
        return true;
    return false;
}

BRIDGE_IMPEXP bool DbgScriptBpGet(int line)
{
    if(_dbg_sendmessage(DBG_SCRIPT_BPGET, (void*)(duint)line, 0))
        return true;
    return false;
}

BRIDGE_IMPEXP bool DbgScriptCmdExec(const char* command)
{
    if(_dbg_sendmessage(DBG_SCRIPT_CMDEXEC, (void*)command, 0))
        return true;
    return false;
}

BRIDGE_IMPEXP void DbgScriptAbort()
{
    _dbg_sendmessage(DBG_SCRIPT_ABORT, 0, 0);
}

BRIDGE_IMPEXP SCRIPTLINETYPE DbgScriptGetLineType(int line)
{
    return (SCRIPTLINETYPE)_dbg_sendmessage(DBG_SCRIPT_GETLINETYPE, (void*)(duint)line, 0);
}

BRIDGE_IMPEXP void DbgScriptSetIp(int line)
{
    _dbg_sendmessage(DBG_SCRIPT_SETIP, (void*)(duint)line, 0);
}

BRIDGE_IMPEXP bool DbgScriptGetBranchInfo(int line, SCRIPTBRANCH* info)
{
    return !!_dbg_sendmessage(DBG_SCRIPT_GETBRANCHINFO, (void*)(duint)line, info);
}

BRIDGE_IMPEXP void DbgSymbolEnum(duint base, CBSYMBOLENUM cbSymbolEnum, void* user)
{
    SYMBOLCBINFO cbInfo;
    cbInfo.base = base;
    cbInfo.cbSymbolEnum = cbSymbolEnum;
    cbInfo.user = user;
    _dbg_sendmessage(DBG_SYMBOL_ENUM, &cbInfo, 0);
}

BRIDGE_IMPEXP bool DbgAssembleAt(duint addr, const char* instruction)
{
    if(_dbg_sendmessage(DBG_ASSEMBLE_AT, (void*)addr, (void*)instruction))
        return true;
    return false;
}

BRIDGE_IMPEXP duint DbgModBaseFromName(const char* name)
{
    return _dbg_sendmessage(DBG_MODBASE_FROM_NAME, (void*)name, 0);
}

BRIDGE_IMPEXP void DbgDisasmAt(duint addr, DISASM_INSTR* instr)
{
    _dbg_sendmessage(DBG_DISASM_AT, (void*)addr, instr);
}

BRIDGE_IMPEXP bool DbgStackCommentGet(duint addr, STACK_COMMENT* comment)
{
    return !!_dbg_sendmessage(DBG_STACK_COMMENT_GET, (void*)addr, comment);
}

BRIDGE_IMPEXP void DbgGetThreadList(THREADLIST* list)
{
    _dbg_sendmessage(DBG_GET_THREAD_LIST, list, 0);
}

BRIDGE_IMPEXP void DbgSettingsUpdated()
{
    _dbg_sendmessage(DBG_SETTINGS_UPDATED, 0, 0);
}

BRIDGE_IMPEXP void DbgDisasmFastAt(duint addr, BASIC_INSTRUCTION_INFO* basicinfo)
{
    _dbg_sendmessage(DBG_DISASM_FAST_AT, (void*)addr, basicinfo);
}

BRIDGE_IMPEXP void DbgMenuEntryClicked(int hEntry)
{
    _dbg_sendmessage(DBG_MENU_ENTRY_CLICKED, (void*)(duint)hEntry, 0);
}


BRIDGE_IMPEXP bool DbgFunctionGet(duint addr, duint* start, duint* end)
{
    FUNCTION_LOOP_INFO info;
    info.addr = addr;
    if(!_dbg_sendmessage(DBG_FUNCTION_GET, &info, 0))
        return false;
    *start = info.start;
    *end = info.end;
    return true;
}

BRIDGE_IMPEXP bool DbgFunctionOverlaps(duint start, duint end)
{
    FUNCTION_LOOP_INFO info;
    info.start = start;
    info.end = end;
    if(!_dbg_sendmessage(DBG_FUNCTION_OVERLAPS, &info, 0))
        return false;
    return true;
}

BRIDGE_IMPEXP bool DbgFunctionAdd(duint start, duint end)
{
    FUNCTION_LOOP_INFO info;
    info.start = start;
    info.end = end;
    info.manual = false;
    if(!_dbg_sendmessage(DBG_FUNCTION_ADD, &info, 0))
        return false;
    return true;
}

BRIDGE_IMPEXP bool DbgFunctionDel(duint addr)
{
    FUNCTION_LOOP_INFO info;
    info.addr = addr;
    if(!_dbg_sendmessage(DBG_FUNCTION_DEL, &info, 0))
        return false;
    return true;
}

BRIDGE_IMPEXP bool DbgLoopGet(int depth, duint addr, duint* start, duint* end)
{
    FUNCTION_LOOP_INFO info;
    info.addr = addr;
    info.depth = depth;
    if(!_dbg_sendmessage(DBG_LOOP_GET, &info, 0))
        return false;
    *start = info.start;
    *end = info.end;
    return true;
}

BRIDGE_IMPEXP bool DbgLoopOverlaps(int depth, duint start, duint end)
{
    FUNCTION_LOOP_INFO info;
    info.start = start;
    info.end = end;
    info.depth = depth;
    if(!_dbg_sendmessage(DBG_LOOP_OVERLAPS, &info, 0))
        return false;
    return true;
}

BRIDGE_IMPEXP bool DbgLoopAdd(duint start, duint end)
{
    FUNCTION_LOOP_INFO info;
    info.start = start;
    info.end = end;
    info.manual = false;
    if(!_dbg_sendmessage(DBG_LOOP_ADD, &info, 0))
        return false;
    return true;
}

BRIDGE_IMPEXP bool DbgLoopDel(int depth, duint addr)
{
    FUNCTION_LOOP_INFO info;
    info.addr = addr;
    info.depth = depth;
    if(!_dbg_sendmessage(DBG_LOOP_DEL, &info, 0))
        return false;
    return true;
}

BRIDGE_IMPEXP bool DbgIsRunLocked()
{
    if(_dbg_sendmessage(DBG_IS_RUN_LOCKED, 0, 0))
        return true;
    return false;
}

BRIDGE_IMPEXP bool DbgIsBpDisabled(duint addr)
{
    if(_dbg_sendmessage(DBG_IS_BP_DISABLED, (void*)addr, 0))
        return true;
    return false;
}

BRIDGE_IMPEXP bool DbgSetAutoCommentAt(duint addr, const char* text)
{
    if(_dbg_sendmessage(DBG_SET_AUTO_COMMENT_AT, (void*)addr, (void*)text))
        return true;
    return false;
}

BRIDGE_IMPEXP void DbgClearAutoCommentRange(duint start, duint end)
{
    _dbg_sendmessage(DBG_DELETE_AUTO_COMMENT_RANGE, (void*)start, (void*)end);
}

BRIDGE_IMPEXP bool DbgSetAutoLabelAt(duint addr, const char* text)
{
    if(_dbg_sendmessage(DBG_SET_AUTO_LABEL_AT, (void*)addr, (void*)text))
        return true;
    return false;
}

BRIDGE_IMPEXP void DbgClearAutoLabelRange(duint start, duint end)
{
    _dbg_sendmessage(DBG_DELETE_AUTO_LABEL_RANGE, (void*)start, (void*)end);
}

BRIDGE_IMPEXP bool DbgSetAutoBookmarkAt(duint addr)
{
    if(_dbg_sendmessage(DBG_SET_AUTO_BOOKMARK_AT, (void*)addr, 0))
        return true;
    return false;
}

BRIDGE_IMPEXP void DbgClearAutoBookmarkRange(duint start, duint end)
{
    _dbg_sendmessage(DBG_DELETE_AUTO_BOOKMARK_RANGE, (void*)start, (void*)end);
}

BRIDGE_IMPEXP bool DbgSetAutoFunctionAt(duint start, duint end)
{
    if(_dbg_sendmessage(DBG_SET_AUTO_FUNCTION_AT, (void*)start, (void*)end))
        return true;
    return false;
}

BRIDGE_IMPEXP void DbgClearAutoFunctionRange(duint start, duint end)
{
    _dbg_sendmessage(DBG_DELETE_AUTO_FUNCTION_RANGE, (void*)start, (void*)end);
}

BRIDGE_IMPEXP bool DbgGetStringAt(duint addr, char* text)
{
    if(_dbg_sendmessage(DBG_GET_STRING_AT, (void*)addr, text))
        return true;
    return false;
}

BRIDGE_IMPEXP const DBGFUNCTIONS* DbgFunctions()
{
    return (const DBGFUNCTIONS*)_dbg_sendmessage(DBG_GET_FUNCTIONS, 0, 0);
}

BRIDGE_IMPEXP bool DbgWinEvent(MSG* message, long* result)
{
    if(_dbg_sendmessage(DBG_WIN_EVENT, message, result))
        return true;
    return false;
}

BRIDGE_IMPEXP bool DbgWinEventGlobal(MSG* message)
{
    if(_dbg_sendmessage(DBG_WIN_EVENT_GLOBAL, message, 0))
        return true;
    return false;
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
    GuiUpdateDumpView();
    GuiUpdateThreadView();
    GuiUpdateSideBar();
    GuiRepaintTableView();
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

BRIDGE_IMPEXP HWND GuiGetWindowHandle()
{
    return (HWND)_gui_sendmessage(GUI_GET_WINDOW_HANDLE, 0, 0);
}

BRIDGE_IMPEXP void GuiDumpAt(duint va)
{
    _gui_sendmessage(GUI_DUMP_AT, (void*)va, 0);
}

BRIDGE_IMPEXP void GuiScriptAdd(int count, const char** lines)
{
    _gui_sendmessage(GUI_SCRIPT_ADD, (void*)(duint)count, (void*)lines);
}

BRIDGE_IMPEXP void GuiScriptClear()
{
    _gui_sendmessage(GUI_SCRIPT_CLEAR, 0, 0);
}

BRIDGE_IMPEXP void GuiScriptSetIp(int line)
{
    _gui_sendmessage(GUI_SCRIPT_SETIP, (void*)(duint)line, 0);
}

BRIDGE_IMPEXP void GuiScriptError(int line, const char* message)
{
    _gui_sendmessage(GUI_SCRIPT_ERROR, (void*)(duint)line, (void*)message);
}

BRIDGE_IMPEXP void GuiScriptSetTitle(const char* title)
{
    _gui_sendmessage(GUI_SCRIPT_SETTITLE, (void*)title, 0);
}

BRIDGE_IMPEXP void GuiScriptSetInfoLine(int line, const char* info)
{
    _gui_sendmessage(GUI_SCRIPT_SETINFOLINE, (void*)(duint)line, (void*)info);
}

BRIDGE_IMPEXP void GuiScriptMessage(const char* message)
{
    _gui_sendmessage(GUI_SCRIPT_MESSAGE, (void*)message, 0);
}

BRIDGE_IMPEXP int GuiScriptMsgyn(const char* message)
{
    return (int)(duint)_gui_sendmessage(GUI_SCRIPT_MSGYN, (void*)message, 0);
}

BRIDGE_IMPEXP void GuiScriptEnableHighlighting(bool enable)
{
    _gui_sendmessage(GUI_SCRIPT_ENABLEHIGHLIGHTING, (void*)(duint)enable, 0);
}

BRIDGE_IMPEXP void GuiSymbolLogAdd(const char* message)
{
    _gui_sendmessage(GUI_SYMBOL_LOG_ADD, (void*)message, 0);
}

BRIDGE_IMPEXP void GuiSymbolLogClear()
{
    _gui_sendmessage(GUI_SYMBOL_LOG_CLEAR, 0, 0);
}

BRIDGE_IMPEXP void GuiSymbolSetProgress(int percent)
{
    _gui_sendmessage(GUI_SYMBOL_SET_PROGRESS, (void*)(duint)percent, 0);
}

BRIDGE_IMPEXP void GuiSymbolUpdateModuleList(int count, SYMBOLMODULEINFO* modules)
{
    _gui_sendmessage(GUI_SYMBOL_UPDATE_MODULE_LIST, (void*)(duint)count, (void*)modules);
}

BRIDGE_IMPEXP void GuiReferenceAddColumn(int width, const char* title)
{
    _gui_sendmessage(GUI_REF_ADDCOLUMN, (void*)(duint)width, (void*)title);
}

BRIDGE_IMPEXP void GuiSymbolRefreshCurrent()
{
    _gui_sendmessage(GUI_SYMBOL_REFRESH_CURRENT, 0, 0);
}

BRIDGE_IMPEXP void GuiReferenceSetRowCount(int count)
{
    _gui_sendmessage(GUI_REF_SETROWCOUNT, (void*)(duint)count, 0);
}

BRIDGE_IMPEXP int GuiReferenceGetRowCount()
{
    return (int)(duint)_gui_sendmessage(GUI_REF_GETROWCOUNT, 0, 0);
}

BRIDGE_IMPEXP void GuiReferenceDeleteAllColumns()
{
    _gui_sendmessage(GUI_REF_DELETEALLCOLUMNS, 0, 0);
}

BRIDGE_IMPEXP void GuiReferenceInitialize(const char* name)
{
    _gui_sendmessage(GUI_REF_INITIALIZE, (void*)name, 0);
}

BRIDGE_IMPEXP void GuiReferenceSetCellContent(int row, int col, const char* str)
{
    CELLINFO info;
    info.row = row;
    info.col = col;
    info.str = str;
    _gui_sendmessage(GUI_REF_SETCELLCONTENT, &info, 0);
}

BRIDGE_IMPEXP const char* GuiReferenceGetCellContent(int row, int col)
{
    return (const char*)_gui_sendmessage(GUI_REF_GETCELLCONTENT, (void*)(duint)row, (void*)(duint)col);
}

BRIDGE_IMPEXP void GuiReferenceReloadData()
{
    _gui_sendmessage(GUI_REF_RELOADDATA, 0, 0);
}

BRIDGE_IMPEXP void GuiReferenceSetSingleSelection(int index, bool scroll)
{
    _gui_sendmessage(GUI_REF_SETSINGLESELECTION, (void*)(duint)index, (void*)(duint)scroll);
}

BRIDGE_IMPEXP void GuiReferenceSetProgress(int progress)
{
    _gui_sendmessage(GUI_REF_SETPROGRESS, (void*)(duint)progress, 0);
}

BRIDGE_IMPEXP void GuiReferenceSetSearchStartCol(int col)
{
    _gui_sendmessage(GUI_REF_SETSEARCHSTARTCOL, (void*)(duint)col, 0);
}

BRIDGE_IMPEXP void GuiStackDumpAt(duint addr, duint csp)
{
    _gui_sendmessage(GUI_STACK_DUMP_AT, (void*)addr, (void*)csp);
}

BRIDGE_IMPEXP void GuiUpdateDumpView()
{
    _gui_sendmessage(GUI_UPDATE_DUMP_VIEW, 0, 0);
}

BRIDGE_IMPEXP void GuiUpdateMemoryView()
{
    _gui_sendmessage(GUI_UPDATE_MEMORY_VIEW, 0, 0);
}

BRIDGE_IMPEXP void GuiUpdateThreadView()
{
    _gui_sendmessage(GUI_UPDATE_THREAD_VIEW, 0, 0);
}

BRIDGE_IMPEXP void GuiAddRecentFile(const char* file)
{
    _gui_sendmessage(GUI_ADD_RECENT_FILE, (void*)file, 0);
}

BRIDGE_IMPEXP void GuiSetLastException(unsigned int exception)
{
    _gui_sendmessage(GUI_SET_LAST_EXCEPTION, (void*)(duint)exception, 0);
}

BRIDGE_IMPEXP bool GuiGetDisassembly(duint addr, char* text)
{
    return !!_gui_sendmessage(GUI_GET_DISASSEMBLY, (void*)addr, text);
}

BRIDGE_IMPEXP int GuiMenuAdd(int hMenu, const char* title)
{
    return (int)(duint)_gui_sendmessage(GUI_MENU_ADD, (void*)(duint)hMenu, (void*)title);
}

BRIDGE_IMPEXP int GuiMenuAddEntry(int hMenu, const char* title)
{
    return (int)(duint)_gui_sendmessage(GUI_MENU_ADD_ENTRY, (void*)(duint)hMenu, (void*)title);
}

BRIDGE_IMPEXP void GuiMenuAddSeparator(int hMenu)
{
    _gui_sendmessage(GUI_MENU_ADD_SEPARATOR, (void*)(duint)hMenu, 0);
}

BRIDGE_IMPEXP void GuiMenuClear(int hMenu)
{
    _gui_sendmessage(GUI_MENU_CLEAR, (void*)(duint)hMenu, 0);
}

BRIDGE_IMPEXP bool GuiSelectionGet(int hWindow, SELECTIONDATA* selection)
{
    return !!_gui_sendmessage(GUI_SELECTION_GET, (void*)(duint)hWindow, selection);
}

BRIDGE_IMPEXP bool GuiSelectionSet(int hWindow, const SELECTIONDATA* selection)
{
    return !!_gui_sendmessage(GUI_SELECTION_SET, (void*)(duint)hWindow, (void*)selection);
}

BRIDGE_IMPEXP bool GuiGetLineWindow(const char* title, char* text)
{
    return !!_gui_sendmessage(GUI_GETLINE_WINDOW, (void*)title, text);
}

BRIDGE_IMPEXP void GuiAutoCompleteAddCmd(const char* cmd)
{
    _gui_sendmessage(GUI_AUTOCOMPLETE_ADDCMD, (void*)cmd, 0);
}

BRIDGE_IMPEXP void GuiAutoCompleteDelCmd(const char* cmd)
{
    _gui_sendmessage(GUI_AUTOCOMPLETE_DELCMD, (void*)cmd, 0);
}

BRIDGE_IMPEXP void GuiAutoCompleteClearAll()
{
    _gui_sendmessage(GUI_AUTOCOMPLETE_CLEARALL, 0, 0);
}

BRIDGE_IMPEXP void GuiAddStatusBarMessage(const char* msg)
{
    _gui_sendmessage(GUI_ADD_MSG_TO_STATUSBAR, (void*)msg, 0);
}

BRIDGE_IMPEXP void GuiUpdateSideBar()
{
    _gui_sendmessage(GUI_UPDATE_SIDEBAR, 0, 0);
}

BRIDGE_IMPEXP void GuiRepaintTableView()
{
    _gui_sendmessage(GUI_REPAINT_TABLE_VIEW, 0, 0);
}

BRIDGE_IMPEXP void GuiUpdatePatches()
{
    _gui_sendmessage(GUI_UPDATE_PATCHES, 0, 0);
}

BRIDGE_IMPEXP void GuiUpdateCallStack()
{
    _gui_sendmessage(GUI_UPDATE_CALLSTACK, 0, 0);
}

//Main
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    hInst = hinstDLL;
    return TRUE;
}

