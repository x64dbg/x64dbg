/**
 * \file bridgemain.cpp
 *
 * \brief Defines functions to initialize and start the Bridge and
 *        to interface with the GUI and the DBG.
 */
#include "_global.h"
#include "bridgemain.h"
#include <stdio.h>
#include "simpleini.h"

/**
 * \brief Global variable that stores a handle to the Bridge's DLL.
 */
static HINSTANCE hInst;

/**
 * \brief Path to the Bridge's INI file. It set when BridgeInit is called.
 */
static wchar_t szIniFile[MAX_PATH] = L"";

#ifdef _WIN64
#define dbg_lib "x64_dbg.dll"
#define gui_lib "x64_gui.dll"
#else
#define dbg_lib "x32_dbg.dll"
#define gui_lib "x32_gui.dll"
#endif // _WIN64

/**
 * \brief Loads a DLL and stores a handle to it into a local variable `hInst`.
 *
 * \param name The name of the library to load.
 */
#define LOADLIBRARY(name) \
    szLib=name; \
    hInst=LoadLibraryA(name); \
    if(!hInst) \
        return "Error loading library \""name"\"!"

/**
 * \brief Loads an exported function and stores it into a global variable of the same
 *        name. The exported function will be loaded from the library whose handle resides in
 *        in a local variable `hInst` that was modified on the last LOADLIBRARY(name) macro expansion.
 *
 * \param name The name of the exported function to load.
 */
#define LOADEXPORT(name) \
    *((FARPROC*)&name)=GetProcAddress(hInst, #name); \
    if(!name) \
    { \
        sprintf(szError, "Export %s:%s could not be found!", szLib, #name); \
        return szError; \
    }

/**
 * \brief Initializes the Bridge.
 *
 * Sets the path to the INI file by deriving its name from the name of the Bridge's
 * module and placing it into the same directory as the Bridge. Loads the functions
 * exported by the GUI and the DBG.
 *
 * \return A null pointer on success, a pointer to an error message on failure.
 */
BRIDGE_IMPEXP const char* BridgeInit()
{
    //Settings load
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

/**
 * \brief Starts the bridge by initializing the GUI.
 *
 * \return A null pointer on success, a pointer to an error message on failure.
 */
BRIDGE_IMPEXP const char* BridgeStart()
{
    if(!_dbg_dbginit || !_gui_guiinit)
        return "\"_dbg_dbginit\" || \"_gui_guiinit\" was not loaded yet, call BridgeInit!";
    _gui_guiinit(0, 0); //remove arguments
    return 0;
}

/**
 * \brief Allocates a memory block inside the Bridge.
 *
 * \param [in] size The number of bytes to allocate.
 *
 * \return A pointer to the allocated memory block on success. On failure, the process exits.
 */
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

/**
 * \brief Frees a memory block that was previously allocated with BridgeAlloc().
 *
 * \param [in] ptr A pointer to a memory block.
 */
BRIDGE_IMPEXP void BridgeFree(void* ptr)
{
    GlobalFree(ptr);
}

/**
 * \brief Reads a string value from the Bridge's INI file.
 *
 * \param [in] section Name of the section to look under.
 * \param [in] key     Name of the key to look for.
 * \param [out] value  A pointer to the value of the specified key. Defaults to an **empty
 *                     string** if the key wasn't found.
 *
 * \return True on success, false on failure.
 */
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

/**
 * \brief Reads a `uint` value from the Bridge's INI file.
 *
 * \param [in] section Name of the section to look under.
 * \param [in] key     Name of the key to look for.
 * \param [out] value  A pointer to the value of the specified key. The value is left unchanged
 *                     if the key wasn't found.
 *
 * \return True on success, false on failure.
 */
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

/**
 * \brief Writes a string value to the Bridge's INI file.
 *
 * \param [in] section Name of the section to put the key under. The section will be created if
 *                     it doesn't exist.
 * \param [in] key     Name of the key.
 * \param [in] value   The value.
 *
 * \return True on success, false on failure.
 */
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

/**
 * \brief Writes a `uint` value to the Bridge's INI file.
 *
 * \param [in] section Name of the section to put the key under. The sections will be created if
 *                     it doesn't exist.
 * \param [in] key     Name of the key.
 * \param [in] value   The value.
 *
 * \return True on success, false on failure.
 */
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

/**
 * \brief Get the DBG's version number.
 *
 * \return The version number.
 */
BRIDGE_IMPEXP int BridgeGetDbgVersion()
{
    return DBG_VERSION;
}

/**
 * \brief Reads memory of the debugged process.
 *
 * \param [in] va    The virtual address of the debugged process to read from.
 * \param [out] dest The buffer to write the read bytes to. The buffer has to be big enough to
 *                   hold the read bytes, i.e. at least `size` bytes long.
 * \param [in] size  The number of bytes to read from the specified virtual address.
 *
 * \return True on success, false on failure.
 */
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

/**
 * \brief Writes to memory of the debugged process.
 *
 * \param [in] va   The virtual address of the debugged process to write to.
 * \param [in] src  The byte buffer to write from.
 * \param [in] size The number of bytes to write from `src`. The buffer should contain at least
 *                  `size` bytes.
 *
 * \return True on success, false on failure.
 */
BRIDGE_IMPEXP bool DbgMemWrite(duint va, const unsigned char* src, duint size)
{
    if(IsBadReadPtr(src, size))
    {
        GuiAddLogMessage("DbgMemWrite with invalid boundaries!\n");
        return false;
    }
    return _dbg_memwrite(va, src, size, 0);
}

// FIXME, not exactly base if it still does a find?
/**
 * \brief Get the size of a memory page in the debugged process.
 *
 * \param [in] base The base address of the memory page.
 *
 * \return The size of the memory page.
 */
BRIDGE_IMPEXP duint DbgMemGetPageSize(duint base)
{
    duint size = 0;
    _dbg_memfindbaseaddr(base, &size);
    return size;
}

/**
 * \brief Finds the base address of a memory page that contains an address.
 *
 * \param [in] addr  The address contained within the memory page.
 * \param [out] size If non-null, the size of the memory page.
 *
 * \return The base address.
 */
BRIDGE_IMPEXP duint DbgMemFindBaseAddr(duint addr, duint* size)
{
    return _dbg_memfindbaseaddr(addr, size);
}

/**
 * \brief Executes a debugger command.
 *
 * \param [in] cmd The debugger command to execute.
 *
 * \return True on success, false on failure.
 */
BRIDGE_IMPEXP bool DbgCmdExec(const char* cmd)
{
    return _dbg_dbgcmdexec(cmd);
}

// FIXME
/**
 * \brief Debug memory map.
 *
 * \param [in,out] memmap If non-null, the memmap.
 *
 * \return true if it succeeds, false if it fails.
 */
BRIDGE_IMPEXP bool DbgMemMap(MEMMAP* memmap)
{
    return _dbg_memmap(memmap);
}

/**
 * \brief Checks if an expression is a valid debugger expression.
 *
 * \param [in] expression The expression to validate.
 *
 * \return True if the expression is valid, false if it isn't.
 */
BRIDGE_IMPEXP bool DbgIsValidExpression(const char* expression)
{
    duint value = 0;
    return _dbg_valfromstring(expression, &value);
}

/**
 * \brief Determines if the debugger is currently debugging.
 *
 * \return True if the debugger is debugging, false if it isn't.
 */
BRIDGE_IMPEXP bool DbgIsDebugging()
{
    return _dbg_isdebugging();
}

/**
 * \brief Determines if a JMP instruction will be executed.
 *
 * \param [in] addr The address of the JMP instruction.
 *
 * \return True if the JMP instruction will be executed, false if not.
 */
BRIDGE_IMPEXP bool DbgIsJumpGoingToExecute(duint addr)
{
    return _dbg_isjumpgoingtoexecute(addr);
}

// FIXME required size of arg _text_?
/**
 * \brief Get a label from the debugger.
 *
 * \param [in] addr    The address to look for the label at.
 * \param [in] segment The segment.
 * \param [out] text   Non-null pointer to a buffer to write the label to.
 *
 * \return True on success, false on failure.
 */
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

/**
 * \brief Sets a label in the debugger.
 *
 * \param [in] addr The address to set the label at.
 * \param [in] text The text to use for the label.
 *
 * \return True if the label was set, false if it wasn't.
 */
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

// FIXME required size of arg _text_?
/**
 * \brief Get a comment from the debugger.
 *
 * \param [in] addr  The address to get the comment from.
 * \param [out] text The buffer to write the comment to.
 *
 * \return True if the label was retrieved, false if it wasn't.
 */
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

/**
 * \brief Sets a comment in the debugger.
 *
 * \param [in] addr The address to set the comment at.
 * \param [in] text The text for the comment.
 *
 * \return true if it the comment was set, false if it wasn't.
 */
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

// FIXME required size of arg _text_?
/**
 * \brief Get the name of the debugged process's module which contains an address.
 *
 * \param [in] addr  The address within the module.
 * \param [out] text The buffer to write the module's name to.
 *
 * \return true if the module's name was retrieved, false if the module wasn't found.
 */
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

/**
 * \brief Checks if a bookmark is set in the debugger at an address.
 *
 * \param [in] addr The address to check.
 *
 * \return True if a bookmark is set, false if it isn't.
 */
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

/**
 * \brief Sets or unsets a bookmark in the debugger at an address.
 *
 * \param [in] addr       The address to set/unset the bookmark at.
 * \param [in] isbookmark True to set the bookmark, false to unset it.
 *
 * \return True if the bookmark was set/unset, false if it wasn't.
 */
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

// FIXME return on success?
/**
 * \brief Initializes the debugger.
 *
 * \return A ???? on success, a null pointer on failure.
 */
BRIDGE_IMPEXP const char* DbgInit()
{
    return _dbg_dbginit();
}

/**
 * \brief Exits from the debugger.
 */
BRIDGE_IMPEXP void DbgExit()
{
    _dbg_dbgexitsignal(); //send exit signal to debugger
}

/**
 * \brief Get the type of a breakpoint at an address.
 *
 * \param [in] addr The address of the breakpoint to get the type of.
 *
 * \return The type of the breakpoint.
 */
BRIDGE_IMPEXP BPXTYPE DbgGetBpxTypeAt(duint addr)
{
    return _dbg_bpgettypeat(addr);
}

/**
 * \brief Get a value from the debugger queried by a string.
 *
 * \param [in] string The query string that the debugger will try to get the value for.
 *
 * \return The value returned by the debugger.
 */
BRIDGE_IMPEXP duint DbgValFromString(const char* string)
{
    duint value = 0;
    _dbg_valfromstring(string, &value);
    return value;
}

/**
 * \brief Get a dump of registers' and flags' values.
 *
 * \param [out] regdump Pointer to a REGDUMP structure that will be filled out with the values.
 *
 * \return True on success, false on failure.
 */
BRIDGE_IMPEXP bool DbgGetRegDump(REGDUMP* regdump)
{
    return _dbg_getregdump(regdump);
}

// FIXME all
/**
 * \brief Debug value to string.
 *
 * \param string The string.
 * \param value  The value.
 *
 * \return true if it succeeds, false if it fails.
 */
BRIDGE_IMPEXP bool DbgValToString(const char* string, duint value)
{
    duint valueCopy = value;
    return _dbg_valtostring(string, &valueCopy);
}

/**
 * \brief Checks if an address inside the debugged process is valid for reading.
 *
 * \param [in] addr The address to check.
 *
 * \return True if it's valid for reading, false if it isn't.
 */
BRIDGE_IMPEXP bool DbgMemIsValidReadPtr(duint addr)
{
    return _dbg_memisvalidreadptr(addr);
}

// FIXME return
/**
 * \brief Get a list of breakpoints of a certain type from the debugger.
 *
 * \param [in] type  The type of the breakpoints to get a list of.
 * \param [out] list A pointer to a BPMAP structure that will be filled out.
 *
 * \return The number of breakpoints in the list.
 */
BRIDGE_IMPEXP int DbgGetBpList(BPXTYPE type, BPMAP* list)
{
    return _dbg_getbplist(type, list);
}

// FIXME all
/**
 * \brief Debug command execute direct.
 *
 * \param [in] cmd The command.
 *
 * \return True on success, false on failure.
 */
BRIDGE_IMPEXP bool DbgCmdExecDirect(const char* cmd)
{
    return _dbg_dbgcmddirectexec(cmd);
}

/**
 * \brief Get the type of a function at an address.
 *
 * \param addr An address within a function.
 *
 * \return The type of the function.
 */
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

// FIXME depth
/**
 * \brief Get the type of a loop at an address.
 *
 * \param [in] addr  The address to check for the loop at.
 * \param [in] depth The depth.
 *
 * \return The type of the loop.
 */
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

/**
 * \brief Get the destination address of a branch instruction.
 *
 * \param [in] addr The address of the branch instructions.
 *
 * \return The destination address.
 */
BRIDGE_IMPEXP duint DbgGetBranchDestination(duint addr)
{
    return _dbg_getbranchdestination(addr);
}

/**
 * \brief Loads a script.
 *
 * \param [in] filename
 */
BRIDGE_IMPEXP void DbgScriptLoad(const char* filename)
{
    _dbg_sendmessage(DBG_SCRIPT_LOAD, (void*)filename, 0);
}

// FIXME every?
/**
 * \brief Unloads every script.
 */
BRIDGE_IMPEXP void DbgScriptUnload()
{
    _dbg_sendmessage(DBG_SCRIPT_UNLOAD, 0, 0);
}

// FIXME "the script?"; destline
/**
 * \brief Runs the script.
 *
 * \param destline [in] The destline.
 */
BRIDGE_IMPEXP void DbgScriptRun(int destline)
{
    _dbg_sendmessage(DBG_SCRIPT_RUN, (void*)(duint)destline, 0);
}

/**
 * \brief Steps over one instruction in the script.
 */
BRIDGE_IMPEXP void DbgScriptStep()
{
    _dbg_sendmessage(DBG_SCRIPT_STEP, 0, 0);
}

/**
 * \brief Toggles a breakpoint on a specified line of the script.
 *
 * \param [in] line The line to toggle the breakpoint at.
 *
 * \return True on success, false on failure.
 */
BRIDGE_IMPEXP bool DbgScriptBpToggle(int line)
{
    if(_dbg_sendmessage(DBG_SCRIPT_BPTOGGLE, (void*)(duint)line, 0))
        return true;
    return false;
}

/**
 * \brief Debug script bp get.
 *
 * \param line The line.
 *
 * \return true if it succeeds, false if it fails.
 */
BRIDGE_IMPEXP bool DbgScriptBpGet(int line)
{
    if(_dbg_sendmessage(DBG_SCRIPT_BPGET, (void*)(duint)line, 0))
        return true;
    return false;
}

/**
 * \brief Executes a script command.
 *
 * \param [in] command The command to execute.
 *
 * \return True on success, false on failure.
 */
BRIDGE_IMPEXP bool DbgScriptCmdExec(const char* command)
{
    if(_dbg_sendmessage(DBG_SCRIPT_CMDEXEC, (void*)command, 0))
        return true;
    return false;
}

/**
 * \brief Aborts the current script.
 */
BRIDGE_IMPEXP void DbgScriptAbort()
{
    _dbg_sendmessage(DBG_SCRIPT_ABORT, 0, 0);
}

/**
 * \brief Get the type of a line in the current script.
 *
 * \param [in] line Ordinal number of the line.
 *
 * \return The type of the line.
 */
BRIDGE_IMPEXP SCRIPTLINETYPE DbgScriptGetLineType(int line)
{
    return (SCRIPTLINETYPE)_dbg_sendmessage(DBG_SCRIPT_GETLINETYPE, (void*)(duint)line, 0);
}

/**
 * \brief Sets the current script's instruction pointer, i.e. a
 *        line that should be executed next.
 *
 * \param [in] line The line.
 */
BRIDGE_IMPEXP void DbgScriptSetIp(int line)
{
    _dbg_sendmessage(DBG_SCRIPT_SETIP, (void*)(duint)line, 0);
}

// FIXME non-null?
/**
 * \brief Get information about a branch in the current script.
 *
 * \param [in] line  The line at which the branch is located at.
 * \param [out] info If non-null, information about the branch.
 *
 * \return True on success, false on failure.
 */
BRIDGE_IMPEXP bool DbgScriptGetBranchInfo(int line, SCRIPTBRANCH* info)
{
    return !!_dbg_sendmessage(DBG_SCRIPT_GETBRANCHINFO, (void*)(duint)line, info);
}

// FIXME all
/**
 * \brief Debug symbol enum.
 *
 * \param base          The base.
 * \param cbSymbolEnum  The symbol enum.
 * \param [in,out] user If non-null, the user.
 */
BRIDGE_IMPEXP void DbgSymbolEnum(duint base, CBSYMBOLENUM cbSymbolEnum, void* user)
{
    SYMBOLCBINFO cbInfo;
    cbInfo.base = base;
    cbInfo.cbSymbolEnum = cbSymbolEnum;
    cbInfo.user = user;
    _dbg_sendmessage(DBG_SYMBOL_ENUM, &cbInfo, 0);
}

/**
 * \brief Assembles an instruction at an address.
 *
 * \param [in] addr        The address to assemble the instruction at.
 * \param [in] instruction The instruction to assemble.
 *
 * \return True on success, false on failure.
 */
BRIDGE_IMPEXP bool DbgAssembleAt(duint addr, const char* instruction)
{
    if(_dbg_sendmessage(DBG_ASSEMBLE_AT, (void*)addr, (void*)instruction))
        return true;
    return false;
}

/**
 * \brief Get a module's base address from its name.
 *
 * \param name The name of the module.
 *
 * \return The base address of the module.
 */
BRIDGE_IMPEXP duint DbgModBaseFromName(const char* name)
{
    return _dbg_sendmessage(DBG_MODBASE_FROM_NAME, (void*)name, 0);
}

/**
 * \brief Disassemble at an address.
 *
 * \param [in] addr   The address to disassemble at.
 * \param [out] instr A pointer to a DISASM_INSTR structure that will be filled out.
 */
BRIDGE_IMPEXP void DbgDisasmAt(duint addr, DISASM_INSTR* instr)
{
    _dbg_sendmessage(DBG_DISASM_AT, (void*)addr, instr);
}

/**
 * \brief Get a stack comment at an address.
 *
 * \param [in] addr     The address to get the stack comment from.
 * \param [out] comment A pointer to a STACK_COMMENT structure that will be filled out.
 *
 * \return true if it succeeds, false if it fails.
 */
BRIDGE_IMPEXP bool DbgStackCommentGet(duint addr, STACK_COMMENT* comment)
{
    return !!_dbg_sendmessage(DBG_STACK_COMMENT_GET, (void*)addr, comment);
}

/**
 * \brief Get a list of threads in the debugged process.
 *
 * \param [out] list A pointer to a THREADLIST structure that will be filled out.
 */
BRIDGE_IMPEXP void DbgGetThreadList(THREADLIST* list)
{
    _dbg_sendmessage(DBG_GET_THREAD_LIST, list, 0);
}

/**
 * \brief Notifies the debugger that the settings have been updated.
 */
BRIDGE_IMPEXP void DbgSettingsUpdated()
{
    _dbg_sendmessage(DBG_SETTINGS_UPDATED, 0, 0);
}

/**
 * \brief Disassemble at an address but return only the most basic information.
 *
 * \param [in] addr       The address to disassemble at.
 * \param [out] basicinfo A pointer to a BASIC_INSTRUCTION_INFO structure that will be filled out.
 */
BRIDGE_IMPEXP void DbgDisasmFastAt(duint addr, BASIC_INSTRUCTION_INFO* basicinfo)
{
    _dbg_sendmessage(DBG_DISASM_FAST_AT, (void*)addr, basicinfo);
}

/**
 * \brief Notifies the debugger that a menu entry has been clicked.
 *
 * \param [in] hEntry ID of the menu entry that has been clicked.
 */
BRIDGE_IMPEXP void DbgMenuEntryClicked(int hEntry)
{
    _dbg_sendmessage(DBG_MENU_ENTRY_CLICKED, (void*)(duint)hEntry, 0);
}

// FIXME not sure
/**
 * \brief Get the starting and ending address of a function given an address within the function.
 *
 * \param [in] addr   An address within a function.
 * \param [out] start The starting address of the function.
 * \param [out] end   The ending address of the function.
 *
 * \return True on success, false on failure.
 */
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

// FIXME brief, return
/**
 * \brief Check if a function overlaps.
 *
 * \param [in] start The starting address of the function.
 * \param [in] end   The ending address of the function.
 *
 * \return True of it overlaps, false if it doesn't. ????
 */
BRIDGE_IMPEXP bool DbgFunctionOverlaps(duint start, duint end)
{
    FUNCTION_LOOP_INFO info;
    info.start = start;
    info.end = end;
    if(!_dbg_sendmessage(DBG_FUNCTION_OVERLAPS, &info, 0))
        return false;
    return true;
}

// FIXME brief, return
/**
 * \brief Add a function. ????
 *
 * \param [in] start The starting address of the function.
 * \param [in] end   The ending address of the function.
 *
 * \return True on success, false on failure.
 */
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

// FIXME brief, return
/**
 * \brief Delete a function. ????
 *
 * \param [in] addr An address within a function.
 *
 * \return true if it succeeds, false if it fails.
 */
BRIDGE_IMPEXP bool DbgFunctionDel(duint addr)
{
    FUNCTION_LOOP_INFO info;
    info.addr = addr;
    if(!_dbg_sendmessage(DBG_FUNCTION_DEL, &info, 0))
        return false;
    return true;
}

// FIXME depth
/**
 * \brief Get information about a loop.
 *
 * \param [in] depth  ????
 * \param [in] addr   An address within the loop.
 * \param [out] start The starting address of the function.
 * \param [out] end   The ending address of the function.
 *
 * \return True on success, false on failure.
 */
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

// FIXME brief, depth, return
/**
 * \brief Check if a loop overlaps.
 *
 * \param [in] depth ????
 * \param [in] start The starting address of the loop.
 * \param [in] end   The ending address of the loop.
 *
 * \return True if the loop overlaps, false if it doesn't.
 */
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

// FIXME brief, return
/**
 * \brief Add a loop.
 *
 * \param [in] start The starting address of the loop.
 * \param [in] end   The ending address of the loop.
 *
 * \return True on success, false on failure.
 */
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

// FIXME brief, brief
/**
 * \brief Delete a loop. ????
 *
 * \param [in] depth ????
 * \param [in] addr  An address within the loop?
 *
 * \return True on success, false on failure.
 */
BRIDGE_IMPEXP bool DbgLoopDel(int depth, duint addr)
{
    FUNCTION_LOOP_INFO info;
    info.addr = addr;
    info.depth = depth;
    if(!_dbg_sendmessage(DBG_LOOP_DEL, &info, 0))
        return false;
    return true;
}

// FIXME all
/**
 * \brief Determine if running is locked. ????
 *
 * \return True if ???? is locked, false if it isn't.
 */
BRIDGE_IMPEXP bool DbgIsRunLocked()
{
    if(_dbg_sendmessage(DBG_IS_RUN_LOCKED, 0, 0))
        return true;
    return false;
}

/**
 * \brief Check if a breakpoint at an address is disabled.
 *
 * \param [in] addr The address to check for a disabled breakpoint at.
 *
 * \return True if the breakpoint is disabled, false if it isn't.
 */
BRIDGE_IMPEXP bool DbgIsBpDisabled(duint addr)
{
    if(_dbg_sendmessage(DBG_IS_BP_DISABLED, (void*)addr, 0))
        return true;
    return false;
}

/**
 * \brief Set an auto-comment at an address.
 *
 * \param [in] addr The address to set the comment at.
 * \param [in] text The comment's text.
 *
 * \return True on success, false on failure.
 */
BRIDGE_IMPEXP bool DbgSetAutoCommentAt(duint addr, const char* text)
{
    if(_dbg_sendmessage(DBG_SET_AUTO_COMMENT_AT, (void*)addr, (void*)text))
        return true;
    return false;
}

// FIXME brief
/**
 * \brief Clear all the auto-comments inside an address range.
 *
 * \param [in] start The starting address of the range.
 * \param [in] end   The ending address of the range.
 */
BRIDGE_IMPEXP void DbgClearAutoCommentRange(duint start, duint end)
{
    _dbg_sendmessage(DBG_DELETE_AUTO_COMMENT_RANGE, (void*)start, (void*)end);
}

/**
 * \brief Set an auto-label at an address.
 *
 * \param [in] addr The address to set the auto-label at.
 * \param [in] text The text of the auto-label.
 *
 * \return True on success, false on failure.
 */
BRIDGE_IMPEXP bool DbgSetAutoLabelAt(duint addr, const char* text)
{
    if(_dbg_sendmessage(DBG_SET_AUTO_LABEL_AT, (void*)addr, (void*)text))
        return true;
    return false;
}

// FIXME brief
/**
 * \brief Clear all the auto-labels inside an address range.
 *
 * \param [in] start The starting address of the range.
 * \param [in] end   The ending address of the range.
 */
BRIDGE_IMPEXP void DbgClearAutoLabelRange(duint start, duint end)
{
    _dbg_sendmessage(DBG_DELETE_AUTO_LABEL_RANGE, (void*)start, (void*)end);
}

/**
 * \brief Set an auto-bookmark at an address.
 *
 * \param [in] addr The address to set the auto-bookmark at.
 *
 * \return True on success, false on failure.
 */
BRIDGE_IMPEXP bool DbgSetAutoBookmarkAt(duint addr)
{
    if(_dbg_sendmessage(DBG_SET_AUTO_BOOKMARK_AT, (void*)addr, 0))
        return true;
    return false;
}

// FIXME brief
/**
 * \brief Clear all the auto-bookmarks inside an address range.
 *
 * \param [in] start The starting address of the range.
 * \param [in] end   The ending address of the range.
 */
BRIDGE_IMPEXP void DbgClearAutoBookmarkRange(duint start, duint end)
{
    _dbg_sendmessage(DBG_DELETE_AUTO_BOOKMARK_RANGE, (void*)start, (void*)end);
}

/**
 * \brief Set an auto-function at an address.
 *
 * \param [in] start The starting address of the function.
 * \param [in] end   The ending address of the function.
 *
 * \return True on success, false on failure.
 */
BRIDGE_IMPEXP bool DbgSetAutoFunctionAt(duint start, duint end)
{
    if(_dbg_sendmessage(DBG_SET_AUTO_FUNCTION_AT, (void*)start, (void*)end))
        return true;
    return false;
}

// FIXME brief
/**
 * \brief Clear all the auto-functions inside an address range.
 *
 * \param [in] start The starting address of the range.
 * \param [in] end   The ending address of the range.
 */
BRIDGE_IMPEXP void DbgClearAutoFunctionRange(duint start, duint end)
{
    _dbg_sendmessage(DBG_DELETE_AUTO_FUNCTION_RANGE, (void*)start, (void*)end);
}

// FIXME size of the buffer?
/**
 * \brief Get a string from a specified address.
 *
 * \param [in] addr  The address to get the string from.
 * \param [out] text A pointer to a buffer to copy the string to.
 *
 * \return True on success, false on failure.
 */
BRIDGE_IMPEXP bool DbgGetStringAt(duint addr, char* text)
{
    if(_dbg_sendmessage(DBG_GET_STRING_AT, (void*)addr, text))
        return true;
    return false;
}

/**
 @fn BRIDGE_IMPEXP const DBGFUNCTIONS* DbgFunctions()

 @brief Debug functions.

 @return null if it fails, else a DBGFUNCTIONS*.
 */

BRIDGE_IMPEXP const DBGFUNCTIONS* DbgFunctions()
{
    return (const DBGFUNCTIONS*)_dbg_sendmessage(DBG_GET_FUNCTIONS, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP bool DbgWinEvent(MSG* message, long* result)

 @brief Debug window event.

 @param [in,out] message If non-null, the message.
 @param [out] result     If non-null, the result.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgWinEvent(MSG* message, long* result)
{
    if(_dbg_sendmessage(DBG_WIN_EVENT, message, result))
        return true;
    return false;
}

/**
 @fn BRIDGE_IMPEXP bool DbgWinEventGlobal(MSG* message)

 @brief Debug window event global.

 @param [in,out] message If non-null, the message.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgWinEventGlobal(MSG* message)
{
    if(_dbg_sendmessage(DBG_WIN_EVENT_GLOBAL, message, 0))
        return true;
    return false;
}

/**
 @fn BRIDGE_IMPEXP void GuiDisasmAt(duint addr, duint cip)

 @brief GUI.

 @param addr The address.
 @param cip  The cip.
 */

BRIDGE_IMPEXP void GuiDisasmAt(duint addr, duint cip)
{
    _gui_sendmessage(GUI_DISASSEMBLE_AT, (void*)addr, (void*)cip);
}

/**
 @fn BRIDGE_IMPEXP void GuiSetDebugState(DBGSTATE state)

 @brief Graphical user interface set debug state.

 @param state The state.
 */

BRIDGE_IMPEXP void GuiSetDebugState(DBGSTATE state)
{
    _gui_sendmessage(GUI_SET_DEBUG_STATE, (void*)state, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiAddLogMessage(const char* msg)

 @brief Graphical user interface add log message.

 @param msg The message.
 */

BRIDGE_IMPEXP void GuiAddLogMessage(const char* msg)
{
    _gui_sendmessage(GUI_ADD_MSG_TO_LOG, (void*)msg, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiLogClear()

 @brief Graphical user interface log clear.
 */

BRIDGE_IMPEXP void GuiLogClear()
{
    _gui_sendmessage(GUI_CLEAR_LOG, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiUpdateAllViews()

 @brief Graphical user interface update all views.
 */

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

/**
 @fn BRIDGE_IMPEXP void GuiUpdateRegisterView()

 @brief Graphical user interface update register view.
 */

BRIDGE_IMPEXP void GuiUpdateRegisterView()
{
    _gui_sendmessage(GUI_UPDATE_REGISTER_VIEW, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiUpdateDisassemblyView()

 @brief Graphical user interface update disassembly view.
 */

BRIDGE_IMPEXP void GuiUpdateDisassemblyView()
{
    _gui_sendmessage(GUI_UPDATE_DISASSEMBLY_VIEW, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiUpdateBreakpointsView()

 @brief Graphical user interface update breakpoints view.
 */

BRIDGE_IMPEXP void GuiUpdateBreakpointsView()
{
    _gui_sendmessage(GUI_UPDATE_BREAKPOINTS_VIEW, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiUpdateWindowTitle(const char* filename)

 @brief Graphical user interface update window title.

 @param filename Filename of the file.
 */

BRIDGE_IMPEXP void GuiUpdateWindowTitle(const char* filename)
{
    _gui_sendmessage(GUI_UPDATE_WINDOW_TITLE, (void*)filename, 0);
}

/**
 @fn BRIDGE_IMPEXP HWND GuiGetWindowHandle()

 @brief Graphical user interface get window handle.

 @return The handle of the window.
 */

BRIDGE_IMPEXP HWND GuiGetWindowHandle()
{
    return (HWND)_gui_sendmessage(GUI_GET_WINDOW_HANDLE, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiDumpAt(duint va)

 @brief Graphical user interface dump at.

 @param va The variable arguments.
 */

BRIDGE_IMPEXP void GuiDumpAt(duint va)
{
    _gui_sendmessage(GUI_DUMP_AT, (void*)va, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiScriptAdd(int count, const char** lines)

 @brief Graphical user interface script add.

 @param count Number of.
 @param lines The lines.
 */

BRIDGE_IMPEXP void GuiScriptAdd(int count, const char** lines)
{
    _gui_sendmessage(GUI_SCRIPT_ADD, (void*)(duint)count, (void*)lines);
}

/**
 @fn BRIDGE_IMPEXP void GuiScriptClear()

 @brief Graphical user interface script clear.
 */

BRIDGE_IMPEXP void GuiScriptClear()
{
    _gui_sendmessage(GUI_SCRIPT_CLEAR, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiScriptSetIp(int line)

 @brief Graphical user interface script set IP.

 @param line The line.
 */

BRIDGE_IMPEXP void GuiScriptSetIp(int line)
{
    _gui_sendmessage(GUI_SCRIPT_SETIP, (void*)(duint)line, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiScriptError(int line, const char* message)

 @brief Graphical user interface script error.

 @param line    The line.
 @param message The message.
 */

BRIDGE_IMPEXP void GuiScriptError(int line, const char* message)
{
    _gui_sendmessage(GUI_SCRIPT_ERROR, (void*)(duint)line, (void*)message);
}

/**
 @fn BRIDGE_IMPEXP void GuiScriptSetTitle(const char* title)

 @brief Graphical user interface script set title.

 @param title The title.
 */

BRIDGE_IMPEXP void GuiScriptSetTitle(const char* title)
{
    _gui_sendmessage(GUI_SCRIPT_SETTITLE, (void*)title, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiScriptSetInfoLine(int line, const char* info)

 @brief Graphical user interface script set information line.

 @param line The line.
 @param info The information.
 */

BRIDGE_IMPEXP void GuiScriptSetInfoLine(int line, const char* info)
{
    _gui_sendmessage(GUI_SCRIPT_SETINFOLINE, (void*)(duint)line, (void*)info);
}

/**
 @fn BRIDGE_IMPEXP void GuiScriptMessage(const char* message)

 @brief Graphical user interface script message.

 @param message The message.
 */

BRIDGE_IMPEXP void GuiScriptMessage(const char* message)
{
    _gui_sendmessage(GUI_SCRIPT_MESSAGE, (void*)message, 0);
}

/**
 @fn BRIDGE_IMPEXP int GuiScriptMsgyn(const char* message)

 @brief Graphical user interface script msgyn.

 @param message The message.

 @return An int.
 */

BRIDGE_IMPEXP int GuiScriptMsgyn(const char* message)
{
    return (int)(duint)_gui_sendmessage(GUI_SCRIPT_MSGYN, (void*)message, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiScriptEnableHighlighting(bool enable)

 @brief Graphical user interface script enable highlighting.

 @param enable true to enable, false to disable.
 */

BRIDGE_IMPEXP void GuiScriptEnableHighlighting(bool enable)
{
    _gui_sendmessage(GUI_SCRIPT_ENABLEHIGHLIGHTING, (void*)(duint)enable, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiSymbolLogAdd(const char* message)

 @brief Graphical user interface symbol log add.

 @param message The message.
 */

BRIDGE_IMPEXP void GuiSymbolLogAdd(const char* message)
{
    _gui_sendmessage(GUI_SYMBOL_LOG_ADD, (void*)message, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiSymbolLogClear()

 @brief Graphical user interface symbol log clear.
 */

BRIDGE_IMPEXP void GuiSymbolLogClear()
{
    _gui_sendmessage(GUI_SYMBOL_LOG_CLEAR, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiSymbolSetProgress(int percent)

 @brief Graphical user interface symbol set progress.

 @param percent The percent.
 */

BRIDGE_IMPEXP void GuiSymbolSetProgress(int percent)
{
    _gui_sendmessage(GUI_SYMBOL_SET_PROGRESS, (void*)(duint)percent, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiSymbolUpdateModuleList(int count, SYMBOLMODULEINFO* modules)

 @brief Graphical user interface symbol update module list.

 @param count            Number of.
 @param [in,out] modules If non-null, the modules.
 */

BRIDGE_IMPEXP void GuiSymbolUpdateModuleList(int count, SYMBOLMODULEINFO* modules)
{
    _gui_sendmessage(GUI_SYMBOL_UPDATE_MODULE_LIST, (void*)(duint)count, (void*)modules);
}

/**
 @fn BRIDGE_IMPEXP void GuiReferenceAddColumn(int width, const char* title)

 @brief Graphical user interface reference add column.

 @param width The width.
 @param title The title.
 */

BRIDGE_IMPEXP void GuiReferenceAddColumn(int width, const char* title)
{
    _gui_sendmessage(GUI_REF_ADDCOLUMN, (void*)(duint)width, (void*)title);
}

/**
 @fn BRIDGE_IMPEXP void GuiSymbolRefreshCurrent()

 @brief Graphical user interface symbol refresh current.
 */

BRIDGE_IMPEXP void GuiSymbolRefreshCurrent()
{
    _gui_sendmessage(GUI_SYMBOL_REFRESH_CURRENT, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiReferenceSetRowCount(int count)

 @brief Graphical user interface reference set row count.

 @param count Number of.
 */

BRIDGE_IMPEXP void GuiReferenceSetRowCount(int count)
{
    _gui_sendmessage(GUI_REF_SETROWCOUNT, (void*)(duint)count, 0);
}

/**
 @fn BRIDGE_IMPEXP int GuiReferenceGetRowCount()

 @brief Graphical user interface reference get row count.

 @return An int.
 */

BRIDGE_IMPEXP int GuiReferenceGetRowCount()
{
    return (int)(duint)_gui_sendmessage(GUI_REF_GETROWCOUNT, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiReferenceDeleteAllColumns()

 @brief Graphical user interface reference delete all columns.
 */

BRIDGE_IMPEXP void GuiReferenceDeleteAllColumns()
{
    _gui_sendmessage(GUI_REF_DELETEALLCOLUMNS, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiReferenceSetCellContent(int row, int col, const char* str)

 @brief Graphical user interface reference set cell content.

 @param row The row.
 @param col The col.
 @param str The.
 */

BRIDGE_IMPEXP void GuiReferenceSetCellContent(int row, int col, const char* str)
{
    CELLINFO info;
    info.row = row;
    info.col = col;
    info.str = str;
    _gui_sendmessage(GUI_REF_SETCELLCONTENT, &info, 0);
}

/**
 @fn BRIDGE_IMPEXP const char* GuiReferenceGetCellContent(int row, int col)

 @brief Graphical user interface reference get cell content.

 @param row The row.
 @param col The col.

 @return null if it fails, else a char*.
 */

BRIDGE_IMPEXP const char* GuiReferenceGetCellContent(int row, int col)
{
    return (const char*)_gui_sendmessage(GUI_REF_GETCELLCONTENT, (void*)(duint)row, (void*)(duint)col);
}

/**
 @fn BRIDGE_IMPEXP void GuiReferenceReloadData()

 @brief Graphical user interface reference reload data.
 */

BRIDGE_IMPEXP void GuiReferenceReloadData()
{
    _gui_sendmessage(GUI_REF_RELOADDATA, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiReferenceSetSingleSelection(int index, bool scroll)

 @brief Graphical user interface reference set single selection.

 @param index  Zero-based index of the.
 @param scroll true to scroll.
 */

BRIDGE_IMPEXP void GuiReferenceSetSingleSelection(int index, bool scroll)
{
    _gui_sendmessage(GUI_REF_SETSINGLESELECTION, (void*)(duint)index, (void*)(duint)scroll);
}

/**
 @fn BRIDGE_IMPEXP void GuiReferenceSetProgress(int progress)

 @brief Graphical user interface reference set progress.

 @param progress The progress.
 */

BRIDGE_IMPEXP void GuiReferenceSetProgress(int progress)
{
    _gui_sendmessage(GUI_REF_SETPROGRESS, (void*)(duint)progress, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiReferenceSetSearchStartCol(int col)

 @brief Graphical user interface reference set search start col.

 @param col The col.
 */

BRIDGE_IMPEXP void GuiReferenceSetSearchStartCol(int col)
{
    _gui_sendmessage(GUI_REF_SETSEARCHSTARTCOL, (void*)(duint)col, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiStackDumpAt(duint addr, duint csp)

 @brief Graphical user interface stack dump at.

 @param addr The address.
 @param csp  The csp.
 */

BRIDGE_IMPEXP void GuiStackDumpAt(duint addr, duint csp)
{
    _gui_sendmessage(GUI_STACK_DUMP_AT, (void*)addr, (void*)csp);
}

/**
 @fn BRIDGE_IMPEXP void GuiUpdateDumpView()

 @brief Graphical user interface update dump view.
 */

BRIDGE_IMPEXP void GuiUpdateDumpView()
{
    _gui_sendmessage(GUI_UPDATE_DUMP_VIEW, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiUpdateMemoryView()

 @brief Graphical user interface update memory view.
 */

BRIDGE_IMPEXP void GuiUpdateMemoryView()
{
    _gui_sendmessage(GUI_UPDATE_MEMORY_VIEW, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiUpdateThreadView()

 @brief Graphical user interface update thread view.
 */

BRIDGE_IMPEXP void GuiUpdateThreadView()
{
    _gui_sendmessage(GUI_UPDATE_THREAD_VIEW, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiAddRecentFile(const char* file)

 @brief Graphical user interface add recent file.

 @param file The file.
 */

BRIDGE_IMPEXP void GuiAddRecentFile(const char* file)
{
    _gui_sendmessage(GUI_ADD_RECENT_FILE, (void*)file, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiSetLastException(unsigned int exception)

 @brief Graphical user interface set last exception.

 @param exception The exception.
 */

BRIDGE_IMPEXP void GuiSetLastException(unsigned int exception)
{
    _gui_sendmessage(GUI_SET_LAST_EXCEPTION, (void*)(duint)exception, 0);
}

/**
 @fn BRIDGE_IMPEXP bool GuiGetDisassembly(duint addr, char* text)

 @brief Graphical user interface get disassembly.

 @param addr          The address.
 @param [in,out] text If non-null, the text.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool GuiGetDisassembly(duint addr, char* text)
{
    return !!_gui_sendmessage(GUI_GET_DISASSEMBLY, (void*)addr, text);
}

/**
 @fn BRIDGE_IMPEXP int GuiMenuAdd(int hMenu, const char* title)

 @brief Graphical user interface menu add.

 @param hMenu The menu.
 @param title The title.

 @return An int.
 */

BRIDGE_IMPEXP int GuiMenuAdd(int hMenu, const char* title)
{
    return (int)(duint)_gui_sendmessage(GUI_MENU_ADD, (void*)(duint)hMenu, (void*)title);
}

/**
 @fn BRIDGE_IMPEXP int GuiMenuAddEntry(int hMenu, const char* title)

 @brief Graphical user interface menu add entry.

 @param hMenu The menu.
 @param title The title.

 @return An int.
 */

BRIDGE_IMPEXP int GuiMenuAddEntry(int hMenu, const char* title)
{
    return (int)(duint)_gui_sendmessage(GUI_MENU_ADD_ENTRY, (void*)(duint)hMenu, (void*)title);
}

/**
 @fn BRIDGE_IMPEXP void GuiMenuAddSeparator(int hMenu)

 @brief Graphical user interface menu add separator.

 @param hMenu The menu.
 */

BRIDGE_IMPEXP void GuiMenuAddSeparator(int hMenu)
{
    _gui_sendmessage(GUI_MENU_ADD_SEPARATOR, (void*)(duint)hMenu, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiMenuClear(int hMenu)

 @brief Graphical user interface menu clear.

 @param hMenu The menu.
 */

BRIDGE_IMPEXP void GuiMenuClear(int hMenu)
{
    _gui_sendmessage(GUI_MENU_CLEAR, (void*)(duint)hMenu, 0);
}

/**
 @fn BRIDGE_IMPEXP bool GuiSelectionGet(int hWindow, SELECTIONDATA* selection)

 @brief Graphical user interface selection get.

 @param hWindow            The window.
 @param [in,out] selection If non-null, the selection.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool GuiSelectionGet(int hWindow, SELECTIONDATA* selection)
{
    return !!_gui_sendmessage(GUI_SELECTION_GET, (void*)(duint)hWindow, selection);
}

/**
 @fn BRIDGE_IMPEXP bool GuiSelectionSet(int hWindow, const SELECTIONDATA* selection)

 @brief Graphical user interface selection set.

 @param hWindow   The window.
 @param selection The selection.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool GuiSelectionSet(int hWindow, const SELECTIONDATA* selection)
{
    return !!_gui_sendmessage(GUI_SELECTION_SET, (void*)(duint)hWindow, (void*)selection);
}

/**
 @fn BRIDGE_IMPEXP bool GuiGetLineWindow(const char* title, char* text)

 @brief Graphical user interface get line window.

 @param title         The title.
 @param [in,out] text If non-null, the text.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool GuiGetLineWindow(const char* title, char* text)
{
    return !!_gui_sendmessage(GUI_GETLINE_WINDOW, (void*)title, text);
}

/**
 @fn BRIDGE_IMPEXP void GuiAutoCompleteAddCmd(const char* cmd)

 @brief Graphical user interface automatic complete add command.

 @param cmd The command.
 */

BRIDGE_IMPEXP void GuiAutoCompleteAddCmd(const char* cmd)
{
    _gui_sendmessage(GUI_AUTOCOMPLETE_ADDCMD, (void*)cmd, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiAutoCompleteDelCmd(const char* cmd)

 @brief Graphical user interface automatic complete delete command.

 @param cmd The command.
 */

BRIDGE_IMPEXP void GuiAutoCompleteDelCmd(const char* cmd)
{
    _gui_sendmessage(GUI_AUTOCOMPLETE_DELCMD, (void*)cmd, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiAutoCompleteClearAll()

 @brief Graphical user interface automatic complete clear all.
 */

BRIDGE_IMPEXP void GuiAutoCompleteClearAll()
{
    _gui_sendmessage(GUI_AUTOCOMPLETE_CLEARALL, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiAddStatusBarMessage(const char* msg)

 @brief Graphical user interface add status bar message.

 @param msg The message.
 */

BRIDGE_IMPEXP void GuiAddStatusBarMessage(const char* msg)
{
    _gui_sendmessage(GUI_ADD_MSG_TO_STATUSBAR, (void*)msg, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiUpdateSideBar()

 @brief Graphical user interface update side bar.
 */

BRIDGE_IMPEXP void GuiUpdateSideBar()
{
    _gui_sendmessage(GUI_UPDATE_SIDEBAR, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiRepaintTableView()

 @brief Graphical user interface repaint table view.
 */

BRIDGE_IMPEXP void GuiRepaintTableView()
{
    _gui_sendmessage(GUI_REPAINT_TABLE_VIEW, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiUpdatePatches()

 @brief Graphical user interface update patches.
 */

BRIDGE_IMPEXP void GuiUpdatePatches()
{
    _gui_sendmessage(GUI_UPDATE_PATCHES, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void GuiUpdateCallStack()

 @brief Graphical user interface update call stack.
 */

BRIDGE_IMPEXP void GuiUpdateCallStack()
{
    _gui_sendmessage(GUI_UPDATE_CALLSTACK, 0, 0);
}

/**
 @fn BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)

 @brief Main.

 @param hinstDLL    The hinst DLL.
 @param fdwReason   The fdw reason.
 @param lpvReserved The lpv reserved.

 @return A WINAPI.
 */

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    hInst = hinstDLL;
    return TRUE;
}

