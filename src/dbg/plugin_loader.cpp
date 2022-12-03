/**
 @file plugin_loader.cpp

 @brief Implements the plugin loader.
 */

#include "plugin_loader.h"
#include "console.h"
#include "debugger.h"
#include "threading.h"
#include "expressionfunctions.h"
#include "formatfunctions.h"
#include <algorithm>
#include <shlwapi.h>

/**
\brief List of plugins.
*/
static std::vector<PLUG_DATA> pluginList;

/**
\brief Saved plugin directory
*/
static std::wstring pluginDirectory;

/**
\brief The current plugin handle.
*/
static int curPluginHandle = 0;

/**
\brief List of plugin callbacks.
*/
static std::vector<PLUG_CALLBACK> pluginCallbackList[CB_LAST];

/**
\brief List of plugin commands.
*/
static std::vector<PLUG_COMMAND> pluginCommandList;

/**
\brief List of plugin menus.
*/
static std::vector<PLUG_MENU> pluginMenuList;

/**
\brief List of plugin menu entries.
*/
static std::vector<PLUG_MENUENTRY> pluginMenuEntryList;

/**
\brief List of plugin exprfunctions.
*/
static std::vector<PLUG_EXPRFUNCTION> pluginExprfunctionList;

/**
\brief List of plugin formatfunctions.
*/
static std::vector<PLUG_FORMATFUNCTION> pluginFormatfunctionList;

static PLUG_DATA pluginData;

/**
\brief Loads a plugin from the plugin directory.
\param pluginName Name of the plugin.
\param loadall true on unload all.
\return true if it succeeds, false if it fails.
*/
bool pluginload(const char* pluginName, bool loadall)
{
    //no empty plugin names allowed
    if(!pluginName)
        return false;

    char name[MAX_PATH] = "";
    strncpy_s(name, pluginName, _TRUNCATE);

    if(!loadall)
#ifdef _WIN64
        strncat_s(name, ".dp64", _TRUNCATE);
#else
        strncat_s(name, ".dp32", _TRUNCATE);
#endif

    wchar_t currentDir[deflen] = L"";
    if(!loadall)
    {
        GetCurrentDirectoryW(deflen, currentDir);
        SetCurrentDirectoryW(pluginDirectory.c_str());
    }
    char searchName[deflen] = "";
#ifdef _WIN64
    sprintf_s(searchName, "%s\\%s", StringUtils::Utf16ToUtf8(pluginDirectory.c_str()).c_str(), name);
#else
    sprintf_s(searchName, "%s\\%s", StringUtils::Utf16ToUtf8(pluginDirectory.c_str()).c_str(), name);
#endif // _WIN64

    //Check to see if this plugin is already loaded
    if(!loadall)
    {
        EXCLUSIVE_ACQUIRE(LockPluginList);
        for(auto it = pluginList.begin(); it != pluginList.end(); ++it)
        {
            if(_stricmp(it->plugname, name) == 0 && it->isLoaded)
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] %s already loaded\n"), name);
                SetCurrentDirectoryW(currentDir);
                return false;
            }
        }
    }

    //check if the file exists
    if(!loadall && !PathFileExistsW(StringUtils::Utf8ToUtf16(searchName).c_str()))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] Cannot find plugin: %s\n"), name);
        return false;
    }

    //setup plugin data
    memset(&pluginData, 0, sizeof(pluginData));
    pluginData.initStruct.pluginHandle = curPluginHandle;
    pluginData.hPlugin = LoadLibraryW(StringUtils::Utf8ToUtf16(searchName).c_str()); //load the plugin library
    if(!pluginData.hPlugin)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] Failed to load plugin: %s\n"), name);
        SetCurrentDirectoryW(currentDir);
        return false;
    }
    pluginData.pluginit = (PLUGINIT)GetProcAddress(pluginData.hPlugin, "pluginit");
    if(!pluginData.pluginit)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] Export \"pluginit\" not found in plugin: %s\n"), name);
        for(int i = CB_INITDEBUG; i < CB_LAST; i++)
            pluginunregistercallback(curPluginHandle, CBTYPE(i));
        FreeLibrary(pluginData.hPlugin);
        SetCurrentDirectoryW(currentDir);
        return false;
    }
    pluginData.plugstop = (PLUGSTOP)GetProcAddress(pluginData.hPlugin, "plugstop");
    pluginData.plugsetup = (PLUGSETUP)GetProcAddress(pluginData.hPlugin, "plugsetup");

    strncpy_s(pluginData.plugpath, searchName, MAX_PATH);
    strncpy_s(pluginData.plugname, name, MAX_PATH);

    //init plugin
    if(!pluginData.pluginit(&pluginData.initStruct))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] pluginit failed for plugin: %s\n"), name);
        for(int i = CB_INITDEBUG; i < CB_LAST; i++)
            pluginunregistercallback(curPluginHandle, CBTYPE(i));
        FreeLibrary(pluginData.hPlugin);
        SetCurrentDirectoryW(currentDir);
        return false;
    }
    if(pluginData.initStruct.sdkVersion < PLUG_SDKVERSION) //the plugin SDK is not compatible
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] %s is incompatible with this SDK version\n"), pluginData.initStruct.pluginName);
        for(int i = CB_INITDEBUG; i < CB_LAST; i++)
            pluginunregistercallback(curPluginHandle, CBTYPE(i));
        FreeLibrary(pluginData.hPlugin);
        SetCurrentDirectoryW(currentDir);
        return false;
    }

    dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] %s v%d Loaded!\n"), pluginData.initStruct.pluginName, pluginData.initStruct.pluginVersion);

    //auto-register callbacks for certain export names
    auto cbPlugin = CBPLUGIN(GetProcAddress(pluginData.hPlugin, "CBALLEVENTS"));
    if(cbPlugin)
    {
        for(int i = CB_INITDEBUG; i < CB_LAST; i++)
            pluginregistercallback(curPluginHandle, CBTYPE(i), cbPlugin);
    }
    auto regExport = [](const char* exportname, CBTYPE cbType)
    {
        auto cbPlugin = CBPLUGIN(GetProcAddress(pluginData.hPlugin, exportname));
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, cbType, cbPlugin);
    };
    regExport("CBINITDEBUG", CB_INITDEBUG);
    regExport("CBSTOPDEBUG", CB_STOPDEBUG);
    regExport("CB_STOPPINGDEBUG", CB_STOPPINGDEBUG);
    regExport("CBCREATEPROCESS", CB_CREATEPROCESS);
    regExport("CBEXITPROCESS", CB_EXITPROCESS);
    regExport("CBCREATETHREAD", CB_CREATETHREAD);
    regExport("CBEXITTHREAD", CB_EXITTHREAD);
    regExport("CBSYSTEMBREAKPOINT", CB_SYSTEMBREAKPOINT);
    regExport("CBLOADDLL", CB_LOADDLL);
    regExport("CBUNLOADDLL", CB_UNLOADDLL);
    regExport("CBOUTPUTDEBUGSTRING", CB_OUTPUTDEBUGSTRING);
    regExport("CBEXCEPTION", CB_EXCEPTION);
    regExport("CBBREAKPOINT", CB_BREAKPOINT);
    regExport("CBPAUSEDEBUG", CB_PAUSEDEBUG);
    regExport("CBRESUMEDEBUG", CB_RESUMEDEBUG);
    regExport("CBSTEPPED", CB_STEPPED);
    regExport("CBATTACH", CB_ATTACH);
    regExport("CBDETACH", CB_DETACH);
    regExport("CBDEBUGEVENT", CB_DEBUGEVENT);
    regExport("CBMENUENTRY", CB_MENUENTRY);
    regExport("CBWINEVENT", CB_WINEVENT);
    regExport("CBWINEVENTGLOBAL", CB_WINEVENTGLOBAL);
    regExport("CBLOADDB", CB_LOADDB);
    regExport("CBSAVEDB", CB_SAVEDB);
    regExport("CBFILTERSYMBOL", CB_FILTERSYMBOL);
    regExport("CBTRACEEXECUTE", CB_TRACEEXECUTE);
    regExport("CBSELCHANGED", CB_SELCHANGED);
    regExport("CBANALYZE", CB_ANALYZE);
    regExport("CBADDRINFO", CB_ADDRINFO);
    regExport("CBVALFROMSTRING", CB_VALFROMSTRING);
    regExport("CBVALTOSTRING", CB_VALTOSTRING);
    regExport("CBMENUPREPARE", CB_MENUPREPARE);

    //add plugin menus
    {
        SectionLocker<LockPluginMenuList, false, false> menuLock; //exclusive lock

        auto addPluginMenu = [](GUIMENUTYPE type)
        {
            int hNewMenu = GuiMenuAdd(type, pluginData.initStruct.pluginName);
            if(hNewMenu == -1)
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] GuiMenuAdd(%d) failed for plugin: %s\n"), type, pluginData.initStruct.pluginName);
                return -1;
            }
            else
            {
                PLUG_MENU newMenu;
                newMenu.hEntryMenu = hNewMenu;
                newMenu.hParentMenu = type;
                newMenu.pluginHandle = pluginData.initStruct.pluginHandle;
                pluginMenuList.push_back(newMenu);
                return newMenu.hEntryMenu;
            }
        };

        pluginData.hMenu = addPluginMenu(GUI_PLUGIN_MENU);
        pluginData.hMenuDisasm = addPluginMenu(GUI_DISASM_MENU);
        pluginData.hMenuDump = addPluginMenu(GUI_DUMP_MENU);
        pluginData.hMenuStack = addPluginMenu(GUI_STACK_MENU);
        pluginData.hMenuGraph = addPluginMenu(GUI_GRAPH_MENU);
        pluginData.hMenuMemmap = addPluginMenu(GUI_MEMMAP_MENU);
        pluginData.hMenuSymmod = addPluginMenu(GUI_SYMMOD_MENU);
    }

    //add the plugin to the list
    {
        SectionLocker<LockPluginList, false> pluginLock; //exclusive lock
        pluginList.push_back(pluginData);
    }

    //setup plugin
    if(pluginData.plugsetup)
    {
        PLUG_SETUPSTRUCT setupStruct;
        setupStruct.hwndDlg = GuiGetWindowHandle();
        setupStruct.hMenu = pluginData.hMenu;
        setupStruct.hMenuDisasm = pluginData.hMenuDisasm;
        setupStruct.hMenuDump = pluginData.hMenuDump;
        setupStruct.hMenuStack = pluginData.hMenuStack;
        setupStruct.hMenuGraph = pluginData.hMenuGraph;
        setupStruct.hMenuMemmap = pluginData.hMenuMemmap;
        setupStruct.hMenuSymmod = pluginData.hMenuSymmod;
        pluginData.plugsetup(&setupStruct);
    }
    pluginData.isLoaded = true;
    curPluginHandle++;

    if(!loadall)
        SetCurrentDirectoryW(currentDir);
    return true;
}

/**
\brief Unloads a plugin from the plugin directory.
\param pluginName Name of the plugin.
\param unloadall true on unload all.
\return true if it succeeds, false if it fails.
*/
bool pluginunload(const char* pluginName, bool unloadall)
{
    char name[MAX_PATH] = "";
    strncpy_s(name, pluginName, _TRUNCATE);

    if(!unloadall)
#ifdef _WIN64
        strncat_s(name, ".dp64", _TRUNCATE);
#else
        strncat_s(name, ".dp32", _TRUNCATE);
#endif

    auto found = pluginList.end();
    {
        EXCLUSIVE_ACQUIRE(LockPluginList);
        found = std::find_if(pluginList.begin(), pluginList.end(), [&name](const PLUG_DATA & a)
        {
            return _stricmp(a.plugname, name) == 0;
        });
    }

    if(found != pluginList.end())
    {
        bool canFreeLibrary = true;
        auto currentPlugin = *found;
        if(currentPlugin.plugstop)
            canFreeLibrary = currentPlugin.plugstop();
        int pluginHandle = currentPlugin.initStruct.pluginHandle;
        plugincmdunregisterall(pluginHandle);
        pluginexprfuncunregisterall(pluginHandle);
        pluginformatfuncunregisterall(pluginHandle);

        //remove the callbacks
        {
            EXCLUSIVE_ACQUIRE(LockPluginCallbackList);
            for(auto & cbList : pluginCallbackList)
            {
                for(auto it = cbList.begin(); it != cbList.end();)
                {
                    if(it->pluginHandle == pluginHandle)
                        it = cbList.erase(it);
                    else
                        ++it;
                }
            }
        }
        {
            EXCLUSIVE_ACQUIRE(LockPluginList);
            pluginmenuclear(currentPlugin.hMenu, true);
            pluginmenuclear(currentPlugin.hMenuDisasm, true);
            pluginmenuclear(currentPlugin.hMenuDump, true);
            pluginmenuclear(currentPlugin.hMenuStack, true);
            pluginmenuclear(currentPlugin.hMenuGraph, true);
            pluginmenuclear(currentPlugin.hMenuMemmap, true);
            pluginmenuclear(currentPlugin.hMenuSymmod, true);

            if(!unloadall)
            {
                //remove from main pluginlist. We do this so unloadall doesn't try to unload an already released plugin
                pluginList.erase(found);
            }
        }

        if(canFreeLibrary)
            FreeLibrary(currentPlugin.hPlugin);
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] %s unloaded\n"), name);
        return true;
    }
    dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] %s not found\n"), name);
    return false;
}

/**
\brief Loads plugins from a specified directory.
\param pluginDir The directory to load plugins from.
*/
void pluginloadall(const char* pluginDir)
{
    //reserve menu space
    pluginMenuList.reserve(1024);
    pluginMenuEntryList.reserve(1024);
    //load new plugins
    wchar_t currentDir[deflen] = L"";
    pluginDirectory = StringUtils::Utf8ToUtf16(pluginDir);
    GetCurrentDirectoryW(deflen, currentDir);
    SetCurrentDirectoryW(pluginDirectory.c_str());
    char searchName[deflen] = "";
#ifdef _WIN64
    sprintf_s(searchName, "%s\\*.dp64", pluginDir);
#else
    sprintf_s(searchName, "%s\\*.dp32", pluginDir);
#endif // _WIN64
    WIN32_FIND_DATAW foundData;
    HANDLE hSearch = FindFirstFileW(StringUtils::Utf8ToUtf16(searchName).c_str(), &foundData);
    if(hSearch == INVALID_HANDLE_VALUE)
    {
        SetCurrentDirectoryW(currentDir);
        return;
    }
    do
    {
        pluginload(StringUtils::Utf16ToUtf8(foundData.cFileName).c_str(), true);
    }
    while(FindNextFileW(hSearch, &foundData));
    FindClose(hSearch);
    SetCurrentDirectoryW(currentDir);
}

/**
\brief Unloads all plugins.
*/
void pluginunloadall()
{
    SHARED_ACQUIRE(LockPluginList);
    auto pluginListCopy = pluginList;
    SHARED_RELEASE();
    for(const auto & plugin : pluginListCopy)
        pluginunload(plugin.plugname, true);
}

/**
\brief Unregister all plugin commands.
\param pluginHandle Handle of the plugin to remove the commands from.
*/
void plugincmdunregisterall(int pluginHandle)
{
    SHARED_ACQUIRE(LockPluginCommandList);
    auto commandList = pluginCommandList; //copy for thread-safety reasons
    SHARED_RELEASE();
    for(auto itr = commandList.begin(); itr != commandList.end();)
    {
        auto currentCommand = *itr;
        if(currentCommand.pluginHandle == pluginHandle)
        {
            itr = commandList.erase(itr);
            dbgcmddel(currentCommand.command);
        }
        else
        {
            ++itr;
        }
    }
}

/**
\brief Unregister all plugin expression functions.
\param pluginHandle Handle of the plugin to remove the expression functions from.
*/
void pluginexprfuncunregisterall(int pluginHandle)
{
    SHARED_ACQUIRE(LockPluginExprfunctionList);
    auto exprFuncList = pluginExprfunctionList; //copy for thread-safety reasons
    SHARED_RELEASE();
    auto i = exprFuncList.begin();
    while(i != exprFuncList.end())
    {
        auto currentExprFunc = *i;
        if(currentExprFunc.pluginHandle == pluginHandle)
        {
            i = exprFuncList.erase(i);
            ExpressionFunctions::Unregister(currentExprFunc.name);
        }
        else
            ++i;
    }
}

/**
\brief Unregister all plugin format functions.
\param pluginHandle Handle of the plugin to remove the format functions from.
*/
void pluginformatfuncunregisterall(int pluginHandle)
{
    SHARED_ACQUIRE(LockPluginFormatfunctionList);
    auto formatFuncList = pluginFormatfunctionList; //copy for thread-safety reasons
    SHARED_RELEASE();
    auto i = formatFuncList.begin();
    while(i != formatFuncList.end())
    {
        auto currentFormatFunc = *i;
        if(currentFormatFunc.pluginHandle == pluginHandle)
        {
            i = formatFuncList.erase(i);
            FormatFunctions::Unregister(currentFormatFunc.name);
        }
        else
            ++i;
    }
}

/**
\brief Register a plugin callback.
\param pluginHandle Handle of the plugin to register a callback for.
\param cbType The type of the callback to register.
\param cbPlugin The actual callback function.
*/
void pluginregistercallback(int pluginHandle, CBTYPE cbType, CBPLUGIN cbPlugin)
{
    pluginunregistercallback(pluginHandle, cbType); //remove previous callback
    PLUG_CALLBACK cbStruct;
    cbStruct.pluginHandle = pluginHandle;
    cbStruct.cbType = cbType;
    cbStruct.cbPlugin = cbPlugin;
    EXCLUSIVE_ACQUIRE(LockPluginCallbackList);
    pluginCallbackList[cbType].push_back(cbStruct);
}

/**
\brief Unregister all plugin callbacks of a certain type.
\param pluginHandle Handle of the plugin to unregister a callback from.
\param cbType The type of the callback to unregister.
*/
bool pluginunregistercallback(int pluginHandle, CBTYPE cbType)
{
    EXCLUSIVE_ACQUIRE(LockPluginCallbackList);
    auto & cbList = pluginCallbackList[cbType];
    for(auto it = cbList.begin(); it != cbList.end();)
    {
        if(it->pluginHandle == pluginHandle)
        {
            cbList.erase(it);
            return true;
        }
        else
            ++it;
    }
    return false;
}

/**
\brief Call all registered callbacks of a certain type.
\param cbType The type of callbacks to call.
\param [in,out] callbackInfo Information describing the callback. See plugin documentation for more information on this.
*/
void plugincbcall(CBTYPE cbType, void* callbackInfo)
{
    if(pluginCallbackList[cbType].empty())
        return;
    SHARED_ACQUIRE(LockPluginCallbackList);
    auto cbList = pluginCallbackList[cbType]; //copy for thread-safety reasons
    SHARED_RELEASE();
    for(const auto & currentCallback : cbList)
        currentCallback.cbPlugin(cbType, callbackInfo);
}

/**
\brief Checks if any callbacks are registered
\param cbType The type of the callback.
\return true if no callbacks are registered.
*/
bool plugincbempty(CBTYPE cbType)
{
    return pluginCallbackList[cbType].empty();
}

static bool findPluginName(int pluginHandle, String & name)
{
    SHARED_ACQUIRE(LockPluginList);
    if(pluginData.initStruct.pluginHandle == pluginHandle)
    {
        name = pluginData.initStruct.pluginName;
        return true;
    }
    for(auto & plugin : pluginList)
    {
        if(plugin.initStruct.pluginHandle == pluginHandle)
        {
            name = plugin.initStruct.pluginName;
            return true;
        }
    }
    dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] Invalid plugin handle %d...\n"), pluginHandle);
    return false;
}

/**
\brief Register a plugin command.
\param pluginHandle Handle of the plugin to register a command for.
\param command The command text to register. This text cannot contain the '\1' character. This text is not case sensitive.
\param cbCommand The command callback.
\param debugonly true if the command can only be called during debugging.
\return true if it the registration succeeded, false otherwise.
*/
bool plugincmdregister(int pluginHandle, const char* command, CBPLUGINCOMMAND cbCommand, bool debugonly)
{
    if(!command || strlen(command) >= deflen || strstr(command, "\1"))
        return false;
    String plugName;
    if(!findPluginName(pluginHandle, plugName))
        return false;
    PLUG_COMMAND plugCmd;
    plugCmd.pluginHandle = pluginHandle;
    strcpy_s(plugCmd.command, command);
    if(!dbgcmdnew(command, (CBCOMMAND)cbCommand, debugonly))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN, %s] Command \"%s\" failed to register...\n"), plugName.c_str(), command);
        return false;
    }
    EXCLUSIVE_ACQUIRE(LockPluginCommandList);
    pluginCommandList.push_back(plugCmd);
    EXCLUSIVE_RELEASE();
    dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN, %s] Command \"%s\" registered!\n"), plugName.c_str(), command);
    return true;
}

/**
\brief Unregister a plugin command.
\param pluginHandle Handle of the plugin to unregister the command from.
\param command The command text to unregister. This text is not case sensitive.
\return true if the command was found and removed, false otherwise.
*/
bool plugincmdunregister(int pluginHandle, const char* command)
{
    if(!command || strlen(command) >= deflen || strstr(command, "\1"))
        return false;
    String plugName;
    if(!findPluginName(pluginHandle, plugName))
        return false;
    EXCLUSIVE_ACQUIRE(LockPluginCommandList);
    for(auto it = pluginCommandList.begin(); it != pluginCommandList.end(); ++it)
    {
        const auto & currentCommand = *it;
        if(currentCommand.pluginHandle == pluginHandle && !strcmp(currentCommand.command, command))
        {
            pluginCommandList.erase(it);
            EXCLUSIVE_RELEASE();
            if(!dbgcmddel(command))
                goto beach;
            dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN, %s] Command \"%s\" unregistered!\n"), plugName.c_str(), command);
            return true;
        }
    }
beach:
    dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN, %s] Command \"%s\" failed to unregister...\n"), plugName.c_str(), command);
    return false;
}

/**
\brief Add a new plugin (sub)menu.
\param hMenu The menu handle to add the (sub)menu to.
\param title The title of the (sub)menu.
\return The handle of the new (sub)menu.
*/
int pluginmenuadd(int hMenu, const char* title)
{
    if(!title || !strlen(title))
        return -1;
    EXCLUSIVE_ACQUIRE(LockPluginMenuList);
    int nFound = -1;
    for(unsigned int i = 0; i < pluginMenuList.size(); i++)
    {
        if(pluginMenuList.at(i).hEntryMenu == hMenu)
        {
            nFound = i;
            break;
        }
    }
    if(nFound == -1) //not a valid menu handle
        return -1;
    int hMenuNew = GuiMenuAdd(hMenu, title);
    PLUG_MENU newMenu;
    newMenu.pluginHandle = pluginMenuList.at(nFound).pluginHandle;
    newMenu.hEntryMenu = hMenuNew;
    newMenu.hParentMenu = hMenu;
    pluginMenuList.push_back(newMenu);
    return hMenuNew;
}

/**
\brief Add a plugin menu entry to a menu.
\param hMenu The menu to add the entry to.
\param hEntry The handle you like to have the entry. This should be a unique value in the scope of the plugin that registered the \p hMenu. Cannot be -1.
\param title The menu entry title.
\return true if the \p hEntry was unique and the entry was successfully added, false otherwise.
*/
bool pluginmenuaddentry(int hMenu, int hEntry, const char* title)
{
    if(!title || !strlen(title) || hEntry == -1)
        return false;
    EXCLUSIVE_ACQUIRE(LockPluginMenuList);
    int pluginHandle = -1;
    //find plugin handle
    for(const auto & currentMenu : pluginMenuList)
    {
        if(currentMenu.hEntryMenu == hMenu)
        {
            pluginHandle = currentMenu.pluginHandle;
            break;
        }
    }
    if(pluginHandle == -1) //not found
        return false;
    //search if hEntry was previously used
    for(const auto & currentMenu : pluginMenuEntryList)
        if(currentMenu.pluginHandle == pluginHandle && currentMenu.hEntryPlugin == hEntry)
            return false;
    int hNewEntry = GuiMenuAddEntry(hMenu, title);
    if(hNewEntry == -1)
        return false;
    PLUG_MENUENTRY newMenu;
    newMenu.hEntryMenu = hNewEntry;
    newMenu.hParentMenu = hMenu;
    newMenu.hEntryPlugin = hEntry;
    newMenu.pluginHandle = pluginHandle;
    pluginMenuEntryList.push_back(newMenu);
    return true;
}

/**
\brief Add a menu separator to a menu.
\param hMenu The menu to add the separator to.
\return true if it succeeds, false otherwise.
*/
bool pluginmenuaddseparator(int hMenu)
{
    SHARED_ACQUIRE(LockPluginMenuList);
    for(const auto & currentMenu : pluginMenuList)
    {
        if(currentMenu.hEntryMenu == hMenu)
        {
            GuiMenuAddSeparator(hMenu);
            return true;
        }
    }
    return false;
}

/// <summary>
/// Helper function that recursively clears the menus and their items.
/// </summary>
/// <param name="hMenu">Handle of the menu to clear.</param>
static void pluginmenuclear_helper(int hMenu)
{
    //delete menu entries
    for(auto i = pluginMenuEntryList.size() - 1; i != -1; i--)
        if(hMenu == pluginMenuEntryList.at(i).hParentMenu) //we found an entry that has the menu as parent
            pluginMenuEntryList.erase(pluginMenuEntryList.begin() + i);
    //delete the menus
    std::vector<int> menuClearQueue;
    for(auto i = pluginMenuList.size() - 1; i != -1; i--)
    {
        if(hMenu == pluginMenuList.at(i).hParentMenu) //we found a menu that has the menu as parent
        {
            menuClearQueue.push_back(pluginMenuList.at(i).hEntryMenu);
            pluginMenuList.erase(pluginMenuList.begin() + i);
        }
    }
    //recursively clear the menus
    for(auto & hMenu : menuClearQueue)
        pluginmenuclear_helper(hMenu);
}

/**
\brief Clears a plugin menu.
\param hMenu The menu to clear.
\return true if it succeeds, false otherwise.
*/
bool pluginmenuclear(int hMenu, bool erase)
{
    EXCLUSIVE_ACQUIRE(LockPluginMenuList);
    pluginmenuclear_helper(hMenu);
    for(auto it = pluginMenuList.begin(); it != pluginMenuList.end(); ++it)
    {
        const auto & currentMenu = *it;
        if(currentMenu.hEntryMenu == hMenu)
        {
            if(erase)
            {
                it = pluginMenuList.erase(it);
                GuiMenuRemove(hMenu);
            }
            else
                GuiMenuClear(hMenu);
            return true;
        }
    }
    return false;
}

/**
\brief Call the registered CB_MENUENTRY callbacks for a menu entry.
\param hEntry The menu entry that triggered the event.
*/
void pluginmenucall(int hEntry)
{
    if(hEntry == -1)
        return;

    SectionLocker<LockPluginMenuList, true, false> menuLock; //shared lock
    auto i = pluginMenuEntryList.begin();
    while(i != pluginMenuEntryList.end())
    {
        const auto currentMenu = *i;
        ++i;
        if(currentMenu.hEntryMenu == hEntry && currentMenu.hEntryPlugin != -1)
        {
            PLUG_CB_MENUENTRY menuEntryInfo;
            menuEntryInfo.hEntry = currentMenu.hEntryPlugin;
            SectionLocker<LockPluginCallbackList, true> callbackLock; //shared lock
            const auto & cbList = pluginCallbackList[CB_MENUENTRY];
            for(auto j = cbList.begin(); j != cbList.end();)
            {
                auto currentCallback = *j++;
                if(currentCallback.pluginHandle == currentMenu.pluginHandle)
                {
                    menuLock.Unlock();
                    callbackLock.Unlock();
                    currentCallback.cbPlugin(currentCallback.cbType, &menuEntryInfo);
                    return;
                }
            }
        }
    }
}

/**
\brief Calls the registered CB_WINEVENT callbacks.
\param [in,out] message the message that triggered the event. Cannot be null.
\param [out] result The result value. Cannot be null.
\return The value the plugin told it to return. See plugin documentation for more information.
*/
bool pluginwinevent(MSG* message, long* result)
{
    PLUG_CB_WINEVENT winevent;
    winevent.message = message;
    winevent.result = result;
    winevent.retval = false; //false=handle event, true=ignore event
    plugincbcall(CB_WINEVENT, &winevent);
    return winevent.retval;
}

/**
\brief Calls the registered CB_WINEVENTGLOBAL callbacks.
\param [in,out] message the message that triggered the event. Cannot be null.
\return The value the plugin told it to return. See plugin documentation for more information.
*/
bool pluginwineventglobal(MSG* message)
{
    PLUG_CB_WINEVENTGLOBAL winevent;
    winevent.message = message;
    winevent.retval = false; //false=handle event, true=ignore event
    plugincbcall(CB_WINEVENTGLOBAL, &winevent);
    return winevent.retval;
}

/**
\brief Sets an icon for a menu.
\param hMenu The menu handle.
\param icon The icon (can be all kinds of formats).
*/
void pluginmenuseticon(int hMenu, const ICONDATA* icon)
{
    SHARED_ACQUIRE(LockPluginMenuList);
    for(const auto & currentMenu : pluginMenuList)
    {
        if(currentMenu.hEntryMenu == hMenu)
        {
            GuiMenuSetIcon(hMenu, icon);
            break;
        }
    }
}

/**
\brief Sets an icon for a menu entry.
\param pluginHandle Plugin handle.
\param hEntry The menu entry handle (unique per plugin).
\param icon The icon (can be all kinds of formats).
*/
void pluginmenuentryseticon(int pluginHandle, int hEntry, const ICONDATA* icon)
{
    if(hEntry == -1)
        return;
    SHARED_ACQUIRE(LockPluginMenuList);
    for(const auto & currentMenu : pluginMenuEntryList)
    {
        if(currentMenu.pluginHandle == pluginHandle && currentMenu.hEntryPlugin == hEntry)
        {
            GuiMenuSetEntryIcon(currentMenu.hEntryMenu, icon);
            break;
        }
    }
}

void pluginmenuentrysetchecked(int pluginHandle, int hEntry, bool checked)
{
    if(hEntry == -1)
        return;
    SHARED_ACQUIRE(LockPluginMenuList);
    for(const auto & currentMenu : pluginMenuEntryList)
    {
        if(currentMenu.pluginHandle == pluginHandle && currentMenu.hEntryPlugin == hEntry)
        {
            GuiMenuSetEntryChecked(currentMenu.hEntryMenu, checked);
            break;
        }
    }
}

void pluginmenusetvisible(int pluginHandle, int hMenu, bool visible)
{
    SHARED_ACQUIRE(LockPluginMenuList);
    for(const auto & currentMenu : pluginMenuList)
    {
        if(currentMenu.hEntryMenu == hMenu)
        {
            GuiMenuSetVisible(hMenu, visible);
            break;
        }
    }
}

void pluginmenuentrysetvisible(int pluginHandle, int hEntry, bool visible)
{
    if(hEntry == -1)
        return;
    SHARED_ACQUIRE(LockPluginMenuList);
    for(const auto & currentMenu : pluginMenuEntryList)
    {
        if(currentMenu.pluginHandle == pluginHandle && currentMenu.hEntryPlugin == hEntry)
        {
            GuiMenuSetEntryVisible(currentMenu.hEntryMenu, visible);
            break;
        }
    }
}

void pluginmenusetname(int pluginHandle, int hMenu, const char* name)
{
    if(!name)
        return;
    SHARED_ACQUIRE(LockPluginMenuList);
    for(const auto & currentMenu : pluginMenuList)
    {
        if(currentMenu.hEntryMenu == hMenu)
        {
            GuiMenuSetName(hMenu, name);
            break;
        }
    }
}

void pluginmenuentrysetname(int pluginHandle, int hEntry, const char* name)
{
    if(hEntry == -1 || !name)
        return;
    SHARED_ACQUIRE(LockPluginMenuList);
    for(const auto & currentMenu : pluginMenuEntryList)
    {
        if(currentMenu.pluginHandle == pluginHandle && currentMenu.hEntryPlugin == hEntry)
        {
            GuiMenuSetEntryName(currentMenu.hEntryMenu, name);
            break;
        }
    }
}

void pluginmenuentrysethotkey(int pluginHandle, int hEntry, const char* hotkey)
{
    if(hEntry == -1 || !hotkey)
        return;
    SHARED_ACQUIRE(LockPluginMenuList);
    for(const auto & currentMenu : pluginMenuEntryList)
    {
        if(currentMenu.pluginHandle == pluginHandle && currentMenu.hEntryPlugin == hEntry)
        {
            for(const auto & plugin : pluginList)
            {
                if(plugin.initStruct.pluginHandle == pluginHandle)
                {
                    char name[MAX_PATH] = "";
                    strcpy_s(name, plugin.plugname);
                    *strrchr(name, '.') = '\0';
                    auto hack = StringUtils::sprintf("%s\1%s_%d", hotkey, name, hEntry);
                    GuiMenuSetEntryHotkey(currentMenu.hEntryMenu, hack.c_str());
                    break;
                }
            }
            break;
        }
    }
}

bool pluginmenuremove(int hMenu)
{
    EXCLUSIVE_ACQUIRE(LockPluginMenuList);
    for(const auto & currentMenu : pluginMenuList)
        if(currentMenu.hEntryMenu == hMenu && currentMenu.hParentMenu < 256)
            return false;
    return pluginmenuclear(hMenu, true);
}

bool pluginmenuentryremove(int pluginHandle, int hEntry)
{
    EXCLUSIVE_ACQUIRE(LockPluginMenuList);
    for(auto it = pluginMenuEntryList.begin(); it != pluginMenuEntryList.end(); ++it)
    {
        const auto & currentEntry = *it;
        if(currentEntry.pluginHandle == pluginHandle && currentEntry.hEntryPlugin == hEntry)
        {
            GuiMenuRemove(currentEntry.hEntryMenu);
            pluginMenuEntryList.erase(it);
            return true;
        }
    }
    return false;
}

struct ExprFuncWrapper
{
    void* user;
    int argc;
    CBPLUGINEXPRFUNCTION cbFunc;
    std::vector<duint> cbArgv;

    static bool callback(ExpressionValue* result, int argc, const ExpressionValue* argv, void* userdata)
    {
        auto cbUser = reinterpret_cast<ExprFuncWrapper*>(userdata);

        cbUser->cbArgv.clear();
        for(auto i = 0; i < argc; i++)
            cbUser->cbArgv.push_back(argv[i].number);

        result->type = ValueTypeNumber;
        result->number = cbUser->cbFunc(argc, cbUser->cbArgv.data(), cbUser->user);

        return true;
    }
};

bool pluginexprfuncregister(int pluginHandle, const char* name, int argc, CBPLUGINEXPRFUNCTION cbFunction, void* userdata)
{
    String plugName;
    if(!findPluginName(pluginHandle, plugName))
        return false;
    PLUG_EXPRFUNCTION plugExprfunction;
    plugExprfunction.pluginHandle = pluginHandle;
    strcpy_s(plugExprfunction.name, name);

    ExprFuncWrapper* wrapper = new ExprFuncWrapper;
    wrapper->argc = argc;
    wrapper->cbFunc = cbFunction;
    wrapper->user = userdata;

    std::vector<ValueType> args(argc);

    for(auto & arg : args)
        arg = ValueTypeNumber;

    if(!ExpressionFunctions::Register(name, ValueTypeNumber, args, wrapper->callback, wrapper))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN, %s] Expression function \"%s\" failed to register...\n"), plugName.c_str(), name);
        return false;
    }
    EXCLUSIVE_ACQUIRE(LockPluginExprfunctionList);
    pluginExprfunctionList.push_back(plugExprfunction);
    EXCLUSIVE_RELEASE();
    dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN, %s] Expression function \"%s\" registered!\n"), plugName.c_str(), name);
    return true;
}

bool pluginexprfuncregisterex(int pluginHandle, const char* name, const ValueType & returnType, const ValueType* argTypes, size_t argCount, CBPLUGINEXPRFUNCTIONEX cbFunction, void* userdata)
{
    String plugName;
    if(!findPluginName(pluginHandle, plugName))
        return false;
    PLUG_EXPRFUNCTION plugExprfunction;
    plugExprfunction.pluginHandle = pluginHandle;
    strcpy_s(plugExprfunction.name, name);

    std::vector<ValueType> argTypesVec(argCount);

    for(size_t i = 0; i < argCount; i++)
        argTypesVec[i] = argTypes[i];

    if(!ExpressionFunctions::Register(name, returnType, argTypesVec, cbFunction, userdata))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN, %s] Expression function \"%s\" failed to register...\n"), plugName.c_str(), name);
        return false;
    }
    EXCLUSIVE_ACQUIRE(LockPluginExprfunctionList);
    pluginExprfunctionList.push_back(plugExprfunction);
    EXCLUSIVE_RELEASE();
    dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN, %s] Expression function \"%s\" registered!\n"), plugName.c_str(), name);
    return true;
}


bool pluginexprfuncunregister(int pluginHandle, const char* name)
{
    String plugName;
    if(!findPluginName(pluginHandle, plugName))
        return false;
    EXCLUSIVE_ACQUIRE(LockPluginExprfunctionList);
    for(auto it = pluginExprfunctionList.begin(); it != pluginExprfunctionList.end(); ++it)
    {
        const auto & currentExprfunction = *it;
        if(currentExprfunction.pluginHandle == pluginHandle && !strcmp(currentExprfunction.name, name))
        {
            pluginExprfunctionList.erase(it);
            EXCLUSIVE_RELEASE();
            if(!ExpressionFunctions::Unregister(name))
                goto beach;
            dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN, %s] Expression function \"%s\" unregistered!\n"), plugName.c_str(), name);
            return true;
        }
    }
beach:
    dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN, %s] Expression function \"%s\" failed to unregister...\n"), plugName.c_str(), name);
    return false;
}

bool pluginformatfuncregister(int pluginHandle, const char* type, CBPLUGINFORMATFUNCTION cbFunction, void* userdata)
{
    String plugName;
    if(!findPluginName(pluginHandle, plugName))
        return false;
    PLUG_FORMATFUNCTION plugFormatfunction;
    plugFormatfunction.pluginHandle = pluginHandle;
    strcpy_s(plugFormatfunction.name, type);
    if(!FormatFunctions::Register(type, cbFunction, userdata))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN, %s] Format function \"%s\" failed to register...\n"), plugName.c_str(), type);
        return false;
    }
    EXCLUSIVE_ACQUIRE(LockPluginFormatfunctionList);
    pluginFormatfunctionList.push_back(plugFormatfunction);
    EXCLUSIVE_RELEASE();
    dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN, %s] Format function \"%s\" registered!\n"), plugName.c_str(), type);
    return true;
}

bool pluginformatfuncunregister(int pluginHandle, const char* type)
{
    String plugName;
    if(!findPluginName(pluginHandle, plugName))
        return false;
    EXCLUSIVE_ACQUIRE(LockPluginFormatfunctionList);
    for(auto it = pluginFormatfunctionList.begin(); it != pluginFormatfunctionList.end(); ++it)
    {
        const auto & currentFormatfunction = *it;
        if(currentFormatfunction.pluginHandle == pluginHandle && !strcmp(currentFormatfunction.name, type))
        {
            pluginFormatfunctionList.erase(it);
            EXCLUSIVE_RELEASE();
            if(!FormatFunctions::Unregister(type))
                goto beach;
            dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN, %s] Format function \"%s\" unregistered!\n"), plugName.c_str(), type);
            return true;
        }
    }
beach:
    dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN, %s] Format function \"%s\" failed to unregister...\n"), plugName.c_str(), type);
    return false;
}
