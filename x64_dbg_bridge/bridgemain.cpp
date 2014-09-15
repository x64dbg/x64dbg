/**********************************************************************************************//**
 * \file bridgemain.cpp
 *
 * \brief Defines the Bridge's \ref BridgeInit and BridgeStart functions.
 *        Defines functions to interface with both the GUI and the DBG.
 **************************************************************************************************/
#include "_global.h"
#include "bridgemain.h"
#include <stdio.h>
#include <new>

/**********************************************************************************************//**
 * \brief Global variable that stores a handle to the Bridge's DLL.
 **************************************************************************************************/
static HINSTANCE hInst;

/**********************************************************************************************//**
 * \brief Path to the INI file.
 **************************************************************************************************/
static char szIniFile[1024] = "";

#ifdef _WIN64
#define dbg_lib "x64_dbg.dll"
#define gui_lib "x64_gui.dll"
#else
#define dbg_lib "x32_dbg.dll"
#define gui_lib "x32_gui.dll"
#endif // _WIN64

/**********************************************************************************************//**
 * \brief Loads the library __name__ and stores a handle to it into a local variable _hInst_.
 *
 * \param name The name of the library to load.
 **************************************************************************************************/
#define LOADLIBRARY(name) \
    szLib=name; \
    hInst=LoadLibraryA(name); \
    if(!hInst) \
        return "Error loading library \""name"\"!"

/**********************************************************************************************//**
 * \brief Loads the exported function __name__ and stores it into a global variable of the same name.
 *        The exported function will be loaded from the library whose handle resides in hInst
 *        that was set on the last LOADLIBRARY call.
 *        \see _global.h
 *
 * \param name The name of the exported function to load.
 **************************************************************************************************/
#define LOADEXPORT(name) \
    *((FARPROC*)&name)=GetProcAddress(hInst, #name); \
    if(!name) \
    { \
        sprintf(szError, "Export %s:%s could not be found!", szLib, #name); \
        return szError; \
    }

/**********************************************************************************************//**
 * \brief Initializes the bridge.
 *
 * \details Derives the path to the INI file from the Bridge's module name and stores it into \a
 *          szIniFile. Loads both the GUI's and the DBG's exported functions.
 *
 * \return Null if it succeeds, a pointer to an error message if it fails.
 **************************************************************************************************/
BRIDGE_IMPEXP const char* BridgeInit()
{
    ///Settings load
    if(!GetModuleFileNameA(0, szIniFile, 1024))
        return "Error getting module path!";
    int len = (int)strlen(szIniFile);
    while(szIniFile[len] != '.' && szIniFile[len] != '\\' && len)
        len--;
    if(szIniFile[len] == '\\')
        strcat(szIniFile, ".ini");
    else
        strcpy(&szIniFile[len], ".ini");

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

/**********************************************************************************************//**
 * \brief Starts the bridge by initializing the GUI.
 *        \see BridgeInit()
 *
 * \return Null if it succeeds, a pointer to an error message if BridgeInit wasn't called before
 *         calling BridgeStart.
 **************************************************************************************************/
BRIDGE_IMPEXP const char* BridgeStart()
{
    if(!_dbg_dbginit || !_gui_guiinit)
        return "\"_dbg_dbginit\" || \"_gui_guiinit\" was not loaded yet, call BridgeInit!";
    _gui_guiinit(0, 0); //remove arguments
    return 0;
}

/**********************************************************************************************//**
 * \brief Allocates a memory block inside the Bridge. If it fails to allocate the memory block
 *        it exists the process.
 *
 * \param[in] size The number of bytes to allocate.
 *
 * \return A _void*_ to the allocated memory.
 **************************************************************************************************/
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

/**********************************************************************************************//**
 * \brief Frees a memory block that was previously allocated with BridgeAlloc
 *
 * \param [in] ptr A pointer to a memory block.
 **************************************************************************************************/
BRIDGE_IMPEXP void BridgeFree(void* ptr)
{
    GlobalFree(ptr);
}

/**********************************************************************************************//**
 * \brief Reads a string value from the Bridge's INI file.
 *
 * \param [in] section The section to look under.
 * \param [in] key     The key to look for.
 * \param [out] value  A pointer to the value of the specified _key_. Defaults to an empty
 *                     string if the key wasn't found.
 *
 * \return true if it succeeds, false if it fails.
 **************************************************************************************************/
BRIDGE_IMPEXP bool BridgeSettingGet(const char* section, const char* key, char* value)
{
    if(!section || !key || !value)
        return false;
    if(!GetPrivateProfileStringA(section, key, "", value, MAX_SETTING_SIZE, szIniFile))
        return false;
    return true;
}

/**********************************************************************************************//**
 * \brief Reads a uint value from the Bridge's INI file.
 *
 * \param [in] section The section to look under.
 * \param [in] key     The key to look for.
 * \param [out] value  A pointer to the value of the specified _key_. Leaves the value
 *                     unchanged if the key wasn't found.
 *
 * \return true if it succeeds, false if it fails.
 **************************************************************************************************/
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

/**********************************************************************************************//**
 * \brief Writes a string value to the Bridge's INI file.
 *
 * \param [in] section The section to put the key under. The section will be created if it doesn't
 *                     exist.
 * \param [in] key     The name of the key.
 * \param [in] value   The value.
 *
 * \return true if it succeeds, false if it fails.
 **************************************************************************************************/
BRIDGE_IMPEXP bool BridgeSettingSet(const char* section, const char* key, const char* value)
{
    if(!section)
        return false;
    if(!WritePrivateProfileStringA(section, key, value, szIniFile))
        return false;
    return true;
}

/**********************************************************************************************//**
 * \brief Writes a uint value to the Bridge's INI file.
 *
 * \param [in] section The section to put the key under. The sections will be created if it doesn't
 *                     exist.
 * \param [in] key     The name of the key.
 * \param [in] value   The value.
 *
 * \return true if it succeeds, false if it fails.
 **************************************************************************************************/
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

/**********************************************************************************************//**
 * \brief Gets the DBG's version number.
 *
 * \return An int specifying the DBG's version.
 **************************************************************************************************/
BRIDGE_IMPEXP int BridgeGetDbgVersion()
{
    return DBG_VERSION;
}

/**********************************************************************************************//**
 * \brief Reads memory of the debugged process.
 *
 * \param [in] va    The virtual address of the debugged process to read from.
 * \param [out] dest The buffer to write the read bytes to. The buffer has to be big enough to
 *                   hold the read bytes, i.e. at least of size _size_.
 * \param [in] size  The number of bytes to read from the specified virtual address.
 *
 * \return true if it succeeds, false if it fails.
 **************************************************************************************************/
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

/**********************************************************************************************//**
 * \brief Writes to memory of the debugged process.
 *
 * \param [in] va   The virtual address of the debugged process to write to.
 * \param [in] src  The byte buffer to write from.
 * \param [in] size The number of bytes to write from _src. The buffer should contain at least
 *                  _size_ bytes.
 *
 * \return true if it succeeds, false if it fails.
 **************************************************************************************************/
BRIDGE_IMPEXP bool DbgMemWrite(duint va, const unsigned char* src, duint size)
{
    if(IsBadReadPtr(src, size))
    {
        GuiAddLogMessage("DbgMemWrite with invalid boundaries!\n");
        return false;
    }
    return _dbg_memwrite(va, src, size, 0);
}

// FIXME, not exactly base if it still does a find
/**********************************************************************************************//**
 * \brief Gets the size of a memory page in the debugged process.
 *
 * \param [in] base The base address of the memory page.
 *
 * \return The size of the memory page.
 **************************************************************************************************/
BRIDGE_IMPEXP duint DbgMemGetPageSize(duint base)
{
    duint size = 0;
    _dbg_memfindbaseaddr(base, &size);
    return size;
}

/**********************************************************************************************//**
 * \brief Finds the base address of the memory page that contains an address.
 *
 * \param [in] addr  The address contained within the memory page.
 * \param [out] size If non-null, the size of the memory page.
 *
 * \return The base address.
 **************************************************************************************************/
BRIDGE_IMPEXP duint DbgMemFindBaseAddr(duint addr, duint* size)
{
    return _dbg_memfindbaseaddr(addr, size);
}

/**********************************************************************************************//**
 * \brief Executes a debugger command.
 *
 * \param [in] cmd The debugger command to execute.
 *
 * \return true if it succeeds, false if it fails.
 **************************************************************************************************/
BRIDGE_IMPEXP bool DbgCmdExec(const char* cmd)
{
    return _dbg_dbgcmdexec(cmd);
}

// FIXME
/**********************************************************************************************//**
 * \brief Debug memory map.
 *
 * \param [in,out] memmap If non-null, the memmap.
 *
 * \return true if it succeeds, false if it fails.
 **************************************************************************************************/
BRIDGE_IMPEXP bool DbgMemMap(MEMMAP* memmap)
{
    return _dbg_memmap(memmap);
}

/**********************************************************************************************//**
 * \brief Checks if an expression is a valid debugger expression.
 *
 * \param [in] expression The expression to validate.
 *
 * \return true if the expression is valid, false if it's not.
 **************************************************************************************************/
BRIDGE_IMPEXP bool DbgIsValidExpression(const char* expression)
{
    duint value = 0;
    return _dbg_valfromstring(expression, &value);
}

/**********************************************************************************************//**
 * \brief Determines if the debugger is currently debugging.
 *
 * \return true if the debugger is debugging, false if it isn't.
 **************************************************************************************************/
BRIDGE_IMPEXP bool DbgIsDebugging()
{
    return _dbg_isdebugging();
}

/**********************************************************************************************//**
 * \brief Determines if a JMP instruction will be executed.
 *
 * \param [in] addr The address of the JMP instruction.
 *
 * \return true if the JMP instruction will be executed, false if not.
 **************************************************************************************************/
BRIDGE_IMPEXP bool DbgIsJumpGoingToExecute(duint addr)
{
    return _dbg_isjumpgoingtoexecute(addr);
}

// FIXME required size of arg _text_?
/**********************************************************************************************//**
 * \brief Gets a label from the debugger.
 *
 * \param [in] addr    The address to look for the label at.
 * \param [in] segment The segment.
 * \param [out] text   The buffer to write the label to.
 *
 * \return true if it succeeds, false if it fails or if _text_ was null.
 **************************************************************************************************/
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

/**********************************************************************************************//**
 * \brief Sets a label in the debugger
 *
 * \param [in] addr The address to set the label at.
 * \param [in] text The text to use for the label.
 *
 * \return true if the label was set, false if either _addr_ or _text_ are null or if the _text_'s
 *         size is bigger than MAX_LABEL_SIZE.
 **************************************************************************************************/
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
/**********************************************************************************************//**
 * \brief Gets a comment from the debugger.
 *
 * \param [in] addr  The address to get the comment from.
 * \param [out] text The buffer to write the comment to.
 *
 * \return true if the label was retrieved, false if either _text_ or _addr_ are null or if
 *         the label couldn't be retrieved.
 **************************************************************************************************/
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

/**********************************************************************************************//**
 * \brief Sets a comment in the debugger.
 *
 * \param [in] addr The address to set the comment at.
 * \param [in] text The text for the comment.
 *
 * \return true if it the comment was set, false if either _addr_ or _text_ are null or if _text_'s
 *         size is bigger than MAX_COMMENT_SIZE.
 **************************************************************************************************/
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
/**********************************************************************************************//**
 * \brief Gets the name of the debugged process's module which contains an address.
 *
 * \param [in] addr  The address within the module.
 * \param [out] text The buffer to write the module's name to.
 *
 * \return true if the module's name was retrieved, false if either _addr_ or _text_ are null or
 *         if the module wasn't found.
 **************************************************************************************************/
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

/**********************************************************************************************//**
 * \brief Checks if a bookmark is set in the debugger at an address.
 *
 * \param [in] addr The address to check.
 *
 * \return true if a bookmark is set, false if it isn't.
 **************************************************************************************************/
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

/**********************************************************************************************//**
 * \brief Sets or unsets a bookmark in the debugger at an address.
 *
 * \param [in] addr       The address to set/unset the bookmark at.
 * \param [in] isbookmark true to set the bookmark, false to unset it.
 *
 * \return true if the bookmark was set/unset, false if it wasn't.
 **************************************************************************************************/
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

/**********************************************************************************************//**
 * \brief Initializes the debugger
 *
 * \return null if it fails, else a char*.
 **************************************************************************************************/
BRIDGE_IMPEXP const char* DbgInit()
{
    return _dbg_dbginit();
}

/**********************************************************************************************//**
 * \brief Exits from the debugger.
 **************************************************************************************************/
BRIDGE_IMPEXP void DbgExit()
{
    _dbg_dbgexitsignal(); //send exit signal to debugger
}

/**********************************************************************************************//**
 * \brief Get the type of a breakpoint at an address.
 *
 * \param [in] addr The address of the breakpoint to get the type of.
 *
 * \return The type of the breakpoint.
 **************************************************************************************************/
BRIDGE_IMPEXP BPXTYPE DbgGetBpxTypeAt(duint addr)
{
    return _dbg_bpgettypeat(addr);
}

/**********************************************************************************************//**
 * \brief Gets a value from the debugger queried by a string.
 *
 * \param [in] string The query string that the debugger will try to get the value for.
 *             
 * \return The value returned by the debugger.
 **************************************************************************************************/
BRIDGE_IMPEXP duint DbgValFromString(const char* string)
{
    duint value = 0;
    _dbg_valfromstring(string, &value);
    return value;
}

// FIXME all
/**********************************************************************************************//**
 * \brief Get a dump of registers' and flags' values.
 *
 * \param [out] regdump Pointer to a REGDUMP structure that will be filled out with the values.
 *
 * \return true if it succeeds, false if it fails.
 **************************************************************************************************/
BRIDGE_IMPEXP bool DbgGetRegDump(REGDUMP* regdump)
{
    return _dbg_getregdump(regdump);
}

// FIXME all
/**********************************************************************************************//**
 * \brief 
 *
 * \param string The string.
 * \param value  The value.
 *
 * \return true if it succeeds, false if it fails.
 **************************************************************************************************/
BRIDGE_IMPEXP bool DbgValToString(const char* string, duint value)
{
    duint valueCopy = value;
    return _dbg_valtostring(string, &valueCopy);
}

/**********************************************************************************************//**
 * \brief Checks if an address inside the debugged process is valid for reading.
 *
 * \param [in] addr The address to check.
 *
 * \return true if it's valid for reading, false if it isn't.
 **************************************************************************************************/
BRIDGE_IMPEXP bool DbgMemIsValidReadPtr(duint addr)
{
    return _dbg_memisvalidreadptr(addr);
}

// FIXME return
/**********************************************************************************************//**
 * \brief Gets a list of breakpoints of a certain type from the debugger.
 *
 * \param [in] type  The type of the breakpoints to get a list of.
 * \param [out] list A pointer to a BPMAP structure that will be filled out.
 *
 * \return The number of breakpoints in the list.
 **************************************************************************************************/
BRIDGE_IMPEXP int DbgGetBpList(BPXTYPE type, BPMAP* list)
{
    return _dbg_getbplist(type, list);
}

// FIXME all
/**********************************************************************************************//**
 * \brief Debug command execute direct.
 *
 * \param cmd The command.
 *
 * \return true if it succeeds, false if it fails.
 **************************************************************************************************/
BRIDGE_IMPEXP bool DbgCmdExecDirect(const char* cmd)
{
    return _dbg_dbgcmddirectexec(cmd);
}

/**********************************************************************************************//**
 * \brief Gets the type of the function at an address from the debugger.
 *
 * \param addr The address within a function.
 *
 * \return The type of the function.
 **************************************************************************************************/
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

/**
 @fn BRIDGE_IMPEXP LOOPTYPE DbgGetLoopTypeAt(duint addr, int depth)

 @brief Debug get loop type at.

 @param addr  The address.
 @param depth The depth.

 @return A LOOPTYPE.
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
 @fn BRIDGE_IMPEXP duint DbgGetBranchDestination(duint addr)

 @brief Debug get branch destination.

 @param addr The address.

 @return A duint.
 */

BRIDGE_IMPEXP duint DbgGetBranchDestination(duint addr)
{
    return _dbg_getbranchdestination(addr);
}

/**
 @fn BRIDGE_IMPEXP void DbgScriptLoad(const char* filename)

 @brief Debug script load.

 @param filename Filename of the file.
 */

BRIDGE_IMPEXP void DbgScriptLoad(const char* filename)
{
    _dbg_sendmessage(DBG_SCRIPT_LOAD, (void*)filename, 0);
}

/**
 @fn BRIDGE_IMPEXP void DbgScriptUnload()

 @brief Debug script unload.
 */

BRIDGE_IMPEXP void DbgScriptUnload()
{
    _dbg_sendmessage(DBG_SCRIPT_UNLOAD, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void DbgScriptRun(int destline)

 @brief Debug script run.

 @param destline The destline.
 */

BRIDGE_IMPEXP void DbgScriptRun(int destline)
{
    _dbg_sendmessage(DBG_SCRIPT_RUN, (void*)(duint)destline, 0);
}

/**
 @fn BRIDGE_IMPEXP void DbgScriptStep()

 @brief Debug script step.
 */

BRIDGE_IMPEXP void DbgScriptStep()
{
    _dbg_sendmessage(DBG_SCRIPT_STEP, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP bool DbgScriptBpToggle(int line)

 @brief Debug script bp toggle.

 @param line The line.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgScriptBpToggle(int line)
{
    if(_dbg_sendmessage(DBG_SCRIPT_BPTOGGLE, (void*)(duint)line, 0))
        return true;
    return false;
}

/**
 @fn BRIDGE_IMPEXP bool DbgScriptBpGet(int line)

 @brief Debug script bp get.

 @param line The line.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgScriptBpGet(int line)
{
    if(_dbg_sendmessage(DBG_SCRIPT_BPGET, (void*)(duint)line, 0))
        return true;
    return false;
}

/**
 @fn BRIDGE_IMPEXP bool DbgScriptCmdExec(const char* command)

 @brief Debug script command execute.

 @param command The command.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgScriptCmdExec(const char* command)
{
    if(_dbg_sendmessage(DBG_SCRIPT_CMDEXEC, (void*)command, 0))
        return true;
    return false;
}

/**
 @fn BRIDGE_IMPEXP void DbgScriptAbort()

 @brief Debug script abort.
 */

BRIDGE_IMPEXP void DbgScriptAbort()
{
    _dbg_sendmessage(DBG_SCRIPT_ABORT, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP SCRIPTLINETYPE DbgScriptGetLineType(int line)

 @brief Debug script get line type.

 @param line The line.

 @return A SCRIPTLINETYPE.
 */

BRIDGE_IMPEXP SCRIPTLINETYPE DbgScriptGetLineType(int line)
{
    return (SCRIPTLINETYPE)_dbg_sendmessage(DBG_SCRIPT_GETLINETYPE, (void*)(duint)line, 0);
}

/**
 @fn BRIDGE_IMPEXP void DbgScriptSetIp(int line)

 @brief Debug script set IP.

 @param line The line.
 */

BRIDGE_IMPEXP void DbgScriptSetIp(int line)
{
    _dbg_sendmessage(DBG_SCRIPT_SETIP, (void*)(duint)line, 0);
}

/**
 @fn BRIDGE_IMPEXP bool DbgScriptGetBranchInfo(int line, SCRIPTBRANCH* info)

 @brief Debug script get branch information.

 @param line          The line.
 @param [in,out] info If non-null, the information.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgScriptGetBranchInfo(int line, SCRIPTBRANCH* info)
{
    return !!_dbg_sendmessage(DBG_SCRIPT_GETBRANCHINFO, (void*)(duint)line, info);
}

/**
 @fn BRIDGE_IMPEXP void DbgSymbolEnum(duint base, CBSYMBOLENUM cbSymbolEnum, void* user)

 @brief Debug symbol enum.

 @param base          The base.
 @param cbSymbolEnum  The symbol enum.
 @param [in,out] user If non-null, the user.
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
 @fn BRIDGE_IMPEXP bool DbgAssembleAt(duint addr, const char* instruction)

 @brief Debug assemble at.

 @param addr        The address.
 @param instruction The instruction.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgAssembleAt(duint addr, const char* instruction)
{
    if(_dbg_sendmessage(DBG_ASSEMBLE_AT, (void*)addr, (void*)instruction))
        return true;
    return false;
}

/**
 @fn BRIDGE_IMPEXP duint DbgModBaseFromName(const char* name)

 @brief Debug modifier base from name.

 @param name The name.

 @return A duint.
 */

BRIDGE_IMPEXP duint DbgModBaseFromName(const char* name)
{
    return _dbg_sendmessage(DBG_MODBASE_FROM_NAME, (void*)name, 0);
}

/**
 @fn BRIDGE_IMPEXP void DbgDisasmAt(duint addr, DISASM_INSTR* instr)

 @brief Debug disasm at.

 @param addr           The address.
 @param [in,out] instr If non-null, the instr.
 */

BRIDGE_IMPEXP void DbgDisasmAt(duint addr, DISASM_INSTR* instr)
{
    _dbg_sendmessage(DBG_DISASM_AT, (void*)addr, instr);
}

/**
 @fn BRIDGE_IMPEXP bool DbgStackCommentGet(duint addr, STACK_COMMENT* comment)

 @brief Debug stack comment get.

 @param addr             The address.
 @param [in,out] comment If non-null, the comment.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgStackCommentGet(duint addr, STACK_COMMENT* comment)
{
    return !!_dbg_sendmessage(DBG_STACK_COMMENT_GET, (void*)addr, comment);
}

/**
 @fn BRIDGE_IMPEXP void DbgGetThreadList(THREADLIST* list)

 @brief Debug get thread list.

 @param [in,out] list If non-null, the list.
 */

BRIDGE_IMPEXP void DbgGetThreadList(THREADLIST* list)
{
    _dbg_sendmessage(DBG_GET_THREAD_LIST, list, 0);
}

/**
 @fn BRIDGE_IMPEXP void DbgSettingsUpdated()

 @brief Debug settings updated.
 */

BRIDGE_IMPEXP void DbgSettingsUpdated()
{
    _dbg_sendmessage(DBG_SETTINGS_UPDATED, 0, 0);
}

/**
 @fn BRIDGE_IMPEXP void DbgDisasmFastAt(duint addr, BASIC_INSTRUCTION_INFO* basicinfo)

 @brief Debug disasm fast at.

 @param addr               The address.
 @param [in,out] basicinfo If non-null, the basicinfo.
 */

BRIDGE_IMPEXP void DbgDisasmFastAt(duint addr, BASIC_INSTRUCTION_INFO* basicinfo)
{
    _dbg_sendmessage(DBG_DISASM_FAST_AT, (void*)addr, basicinfo);
}

/**
 @fn BRIDGE_IMPEXP void DbgMenuEntryClicked(int hEntry)

 @brief Debug menu entry clicked.

 @param hEntry The entry.
 */

BRIDGE_IMPEXP void DbgMenuEntryClicked(int hEntry)
{
    _dbg_sendmessage(DBG_MENU_ENTRY_CLICKED, (void*)(duint)hEntry, 0);
}

/**
 @fn BRIDGE_IMPEXP bool DbgFunctionGet(duint addr, duint* start, duint* end)

 @brief Debug function get.

 @param addr           The address.
 @param [in,out] start If non-null, the start.
 @param [in,out] end   If non-null, the end.

 @return true if it succeeds, false if it fails.
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

/**
 @fn BRIDGE_IMPEXP bool DbgFunctionOverlaps(duint start, duint end)

 @brief Debug function overlaps.

 @param start The start.
 @param end   The end.

 @return true if it succeeds, false if it fails.
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

/**
 @fn BRIDGE_IMPEXP bool DbgFunctionAdd(duint start, duint end)

 @brief Debug function add.

 @param start The start.
 @param end   The end.

 @return true if it succeeds, false if it fails.
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

/**
 @fn BRIDGE_IMPEXP bool DbgFunctionDel(duint addr)

 @brief Debug function delete.

 @param addr The address.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgFunctionDel(duint addr)
{
    FUNCTION_LOOP_INFO info;
    info.addr = addr;
    if(!_dbg_sendmessage(DBG_FUNCTION_DEL, &info, 0))
        return false;
    return true;
}

/**
 @fn BRIDGE_IMPEXP bool DbgLoopGet(int depth, duint addr, duint* start, duint* end)

 @brief Debug loop get.

 @param depth          The depth.
 @param addr           The address.
 @param [in,out] start If non-null, the start.
 @param [in,out] end   If non-null, the end.

 @return true if it succeeds, false if it fails.
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

/**
 @fn BRIDGE_IMPEXP bool DbgLoopOverlaps(int depth, duint start, duint end)

 @brief Debug loop overlaps.

 @param depth The depth.
 @param start The start.
 @param end   The end.

 @return true if it succeeds, false if it fails.
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

/**
 @fn BRIDGE_IMPEXP bool DbgLoopAdd(duint start, duint end)

 @brief Debug loop add.

 @param start The start.
 @param end   The end.

 @return true if it succeeds, false if it fails.
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

/**
 @fn BRIDGE_IMPEXP bool DbgLoopDel(int depth, duint addr)

 @brief Debug loop delete.

 @param depth The depth.
 @param addr  The address.

 @return true if it succeeds, false if it fails.
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

/**
 @fn BRIDGE_IMPEXP bool DbgIsRunLocked()

 @brief Determines if we can debug is run locked.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgIsRunLocked()
{
    if(_dbg_sendmessage(DBG_IS_RUN_LOCKED, 0, 0))
        return true;
    return false;
}

/**
 @fn BRIDGE_IMPEXP bool DbgIsBpDisabled(duint addr)

 @brief Debug is bp disabled.

 @param addr The address.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgIsBpDisabled(duint addr)
{
    if(_dbg_sendmessage(DBG_IS_BP_DISABLED, (void*)addr, 0))
        return true;
    return false;
}

/**
 @fn BRIDGE_IMPEXP bool DbgSetAutoCommentAt(duint addr, const char* text)

 @brief Debug set automatic comment at.

 @param addr The address.
 @param text The text.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgSetAutoCommentAt(duint addr, const char* text)
{
    if(_dbg_sendmessage(DBG_SET_AUTO_COMMENT_AT, (void*)addr, (void*)text))
        return true;
    return false;
}

/**
 @fn BRIDGE_IMPEXP void DbgClearAutoCommentRange(duint start, duint end)

 @brief Debug clear automatic comment range.

 @param start The start.
 @param end   The end.
 */

BRIDGE_IMPEXP void DbgClearAutoCommentRange(duint start, duint end)
{
    _dbg_sendmessage(DBG_DELETE_AUTO_COMMENT_RANGE, (void*)start, (void*)end);
}

/**
 @fn BRIDGE_IMPEXP bool DbgSetAutoLabelAt(duint addr, const char* text)

 @brief Debug set automatic label at.

 @param addr The address.
 @param text The text.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgSetAutoLabelAt(duint addr, const char* text)
{
    if(_dbg_sendmessage(DBG_SET_AUTO_LABEL_AT, (void*)addr, (void*)text))
        return true;
    return false;
}

/**
 @fn BRIDGE_IMPEXP void DbgClearAutoLabelRange(duint start, duint end)

 @brief Debug clear automatic label range.

 @param start The start.
 @param end   The end.
 */

BRIDGE_IMPEXP void DbgClearAutoLabelRange(duint start, duint end)
{
    _dbg_sendmessage(DBG_DELETE_AUTO_LABEL_RANGE, (void*)start, (void*)end);
}

/**
 @fn BRIDGE_IMPEXP bool DbgSetAutoBookmarkAt(duint addr)

 @brief Debug set automatic bookmark at.

 @param addr The address.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgSetAutoBookmarkAt(duint addr)
{
    if(_dbg_sendmessage(DBG_SET_AUTO_BOOKMARK_AT, (void*)addr, 0))
        return true;
    return false;
}

/**
 @fn BRIDGE_IMPEXP void DbgClearAutoBookmarkRange(duint start, duint end)

 @brief Debug clear automatic bookmark range.

 @param start The start.
 @param end   The end.
 */

BRIDGE_IMPEXP void DbgClearAutoBookmarkRange(duint start, duint end)
{
    _dbg_sendmessage(DBG_DELETE_AUTO_BOOKMARK_RANGE, (void*)start, (void*)end);
}

/**
 @fn BRIDGE_IMPEXP bool DbgSetAutoFunctionAt(duint start, duint end)

 @brief Debug set automatic function at.

 @param start The start.
 @param end   The end.

 @return true if it succeeds, false if it fails.
 */

BRIDGE_IMPEXP bool DbgSetAutoFunctionAt(duint start, duint end)
{
    if(_dbg_sendmessage(DBG_SET_AUTO_FUNCTION_AT, (void*)start, (void*)end))
        return true;
    return false;
}

/**
 @fn BRIDGE_IMPEXP void DbgClearAutoFunctionRange(duint start, duint end)

 @brief Debug clear automatic function range.

 @param start The start.
 @param end   The end.
 */

BRIDGE_IMPEXP void DbgClearAutoFunctionRange(duint start, duint end)
{
    _dbg_sendmessage(DBG_DELETE_AUTO_FUNCTION_RANGE, (void*)start, (void*)end);
}

/**
 @fn BRIDGE_IMPEXP bool DbgGetStringAt(duint addr, char* text)

 @brief Debug get string at.

 @param addr          The address.
 @param [in,out] text If non-null, the text.

 @return true if it succeeds, false if it fails.
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

