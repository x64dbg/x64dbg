/**
 @file plugin_loader.cpp

 @brief Implements the plugin loader.
 */

#include "plugin_loader.h"
#include "console.h"
#include "debugger.h"
#include "threading.h"
#include "expressionfunctions.h"

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
\brief List of plugin exprfunctions.
*/
static std::vector<PLUG_EXPRFUNCTION> pluginExprfunctionList;

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
    PLUG_DATA pluginData;

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
    sprintf(searchName, "%s\\%s", StringUtils::Utf16ToUtf8(pluginDirectory.c_str()).c_str(), name);
#else
    sprintf(searchName, "%s\\%s", StringUtils::Utf16ToUtf8(pluginDirectory.c_str()).c_str(), name);
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
        FreeLibrary(pluginData.hPlugin);
        SetCurrentDirectoryW(currentDir);
        return false;
    }
    pluginData.plugstop = (PLUGSTOP)GetProcAddress(pluginData.hPlugin, "plugstop");
    pluginData.plugsetup = (PLUGSETUP)GetProcAddress(pluginData.hPlugin, "plugsetup");

    strncpy_s(pluginData.plugpath, searchName, MAX_PATH);
    strncpy_s(pluginData.plugname, name, MAX_PATH);
    //auto-register callbacks for certain export names
    auto cbPlugin = CBPLUGIN(GetProcAddress(pluginData.hPlugin, "CBALLEVENTS"));
    if(cbPlugin)
    {
        for(int i = CB_INITDEBUG; i < CB_LAST; i++)
            pluginregistercallback(curPluginHandle, CBTYPE(i), cbPlugin);
    }
    auto regExport = [&pluginData](const char* exportname, CBTYPE cbType)
    {
        auto cbPlugin = CBPLUGIN(GetProcAddress(pluginData.hPlugin, exportname));
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, cbType, cbPlugin);
    };
    regExport("CBINITDEBUG", CB_INITDEBUG);
    regExport("CBSTOPDEBUG", CB_STOPDEBUG);
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
    regExport("CBANALYZE", CB_ANALYZE);

    //init plugin
    if(!pluginData.pluginit(&pluginData.initStruct))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] pluginit failed for plugin: %s\n"), name);
        FreeLibrary(pluginData.hPlugin);
        SetCurrentDirectoryW(currentDir);
        return false;
    }
    else if(pluginData.initStruct.sdkVersion < PLUG_SDKVERSION)  //the plugin SDK is not compatible
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] %s is incompatible with this SDK version\n"), pluginData.initStruct.pluginName);
        FreeLibrary(pluginData.hPlugin);
        SetCurrentDirectoryW(currentDir);
        return false;
    }
    else
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] %s v%d Loaded!\n"), pluginData.initStruct.pluginName, pluginData.initStruct.pluginVersion);

    SectionLocker<LockPluginMenuList, false> menuLock; //exclusive lock

    //add plugin menu
    int hNewMenu = GuiMenuAdd(GUI_PLUGIN_MENU, pluginData.initStruct.pluginName);
    if(hNewMenu == -1)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] GuiMenuAdd(GUI_PLUGIN_MENU) failed for plugin: %s\n"), pluginData.initStruct.pluginName);
        pluginData.hMenu = -1;
    }
    else
    {
        PLUG_MENU newMenu;
        newMenu.hEntryMenu = hNewMenu;
        newMenu.hEntryPlugin = -1;
        newMenu.pluginHandle = pluginData.initStruct.pluginHandle;
        pluginMenuList.push_back(newMenu);
        pluginData.hMenu = newMenu.hEntryMenu;
    }

    //add disasm plugin menu
    hNewMenu = GuiMenuAdd(GUI_DISASM_MENU, pluginData.initStruct.pluginName);
    if(hNewMenu == -1)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] GuiMenuAdd(GUI_DISASM_MENU) failed for plugin: %s\n"), pluginData.initStruct.pluginName);
        pluginData.hMenu = -1;
    }
    else
    {
        PLUG_MENU newMenu;
        newMenu.hEntryMenu = hNewMenu;
        newMenu.hEntryPlugin = -1;
        newMenu.pluginHandle = pluginData.initStruct.pluginHandle;
        pluginMenuList.push_back(newMenu);
        pluginData.hMenuDisasm = newMenu.hEntryMenu;
    }

    //add dump plugin menu
    hNewMenu = GuiMenuAdd(GUI_DUMP_MENU, pluginData.initStruct.pluginName);
    if(hNewMenu == -1)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] GuiMenuAdd(GUI_DUMP_MENU) failed for plugin: %s\n"), pluginData.initStruct.pluginName);
        pluginData.hMenu = -1;
    }
    else
    {
        PLUG_MENU newMenu;
        newMenu.hEntryMenu = hNewMenu;
        newMenu.hEntryPlugin = -1;
        newMenu.pluginHandle = pluginData.initStruct.pluginHandle;
        pluginMenuList.push_back(newMenu);
        pluginData.hMenuDump = newMenu.hEntryMenu;
    }

    //add stack plugin menu
    hNewMenu = GuiMenuAdd(GUI_STACK_MENU, pluginData.initStruct.pluginName);
    if(hNewMenu == -1)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] GuiMenuAdd(GUI_STACK_MENU) failed for plugin: %s\n"), pluginData.initStruct.pluginName);
        pluginData.hMenu = -1;
    }
    else
    {
        PLUG_MENU newMenu;
        newMenu.hEntryMenu = hNewMenu;
        newMenu.hEntryPlugin = -1;
        newMenu.pluginHandle = pluginData.initStruct.pluginHandle;
        pluginMenuList.push_back(newMenu);
        pluginData.hMenuStack = newMenu.hEntryMenu;
    }
    menuLock.Unlock();

    //add the plugin to the list
    SectionLocker<LockPluginList, false> pluginLock; //exclusive lock
    pluginList.push_back(pluginData);
    pluginLock.Unlock();

    //setup plugin
    if(pluginData.plugsetup)
    {
        PLUG_SETUPSTRUCT setupStruct;
        setupStruct.hwndDlg = GuiGetWindowHandle();
        setupStruct.hMenu = pluginData.hMenu;
        setupStruct.hMenuDisasm = pluginData.hMenuDisasm;
        setupStruct.hMenuDump = pluginData.hMenuDump;
        setupStruct.hMenuStack = pluginData.hMenuStack;
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
    bool foundPlugin = false;
    PLUG_DATA currentPlugin;
    char name[MAX_PATH] = "";
    strncpy_s(name, pluginName, _TRUNCATE);

    if(!unloadall)
#ifdef _WIN64
        strncat_s(name, ".dp64", _TRUNCATE);
#else
        strncat_s(name, ".dp32", _TRUNCATE);
#endif

    {
        EXCLUSIVE_ACQUIRE(LockPluginList);
        for(auto it = pluginList.begin(); it != pluginList.end(); ++it)
        {
            if(_stricmp(it->plugname, name) == 0)
            {
                currentPlugin = *it;
                foundPlugin = true;
                break;
            }
        }
    }

    if(foundPlugin)
    {
        if(currentPlugin.plugstop)
            currentPlugin.plugstop();
        plugincmdunregisterall(currentPlugin.initStruct.pluginHandle);
        pluginexprfuncunregisterall(currentPlugin.initStruct.pluginHandle);

        //remove the callbacks
        {
            EXCLUSIVE_ACQUIRE(LockPluginCallbackList);
            for(auto & cbList : pluginCallbackList)
            {
                for(auto it = cbList.begin(); it != cbList.end();)
                {
                    if(it->pluginHandle == currentPlugin.initStruct.pluginHandle)
                        it = cbList.erase(it);
                    else
                        ++it;
                }
            }
        }
        {
            EXCLUSIVE_ACQUIRE(LockPluginList);
            pluginmenuclear(currentPlugin.hMenu);

            //remove from main pluginlist. We do this so unloadall doesn't try to unload an already released plugin
            auto pbegin = pluginList.begin();
            auto pend = pluginList.end();
            auto new_pend = std::remove_if(pbegin, pend, [&](PLUG_DATA & pData)
            {
                if(_stricmp(pData.plugname, currentPlugin.plugname) == 0)
                    return true;
                return false;
            });
            pluginList.erase(new_pend, pluginList.end());
        }

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
    //load new plugins
    wchar_t currentDir[deflen] = L"";
    pluginDirectory = StringUtils::Utf8ToUtf16(pluginDir);
    GetCurrentDirectoryW(deflen, currentDir);
    SetCurrentDirectoryW(pluginDirectory.c_str());
    char searchName[deflen] = "";
#ifdef _WIN64
    sprintf(searchName, "%s\\*.dp64", pluginDir);
#else
    sprintf(searchName, "%s\\*.dp32", pluginDir);
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
    SetCurrentDirectoryW(currentDir);
}

/**
\brief Unloads all plugins.
*/
void pluginunloadall()
{
    EXCLUSIVE_ACQUIRE(LockPluginList);
    for(const auto & plugin : pluginList)
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
    auto i = commandList.begin();
    while(i != commandList.end())
    {
        auto currentCommand = *i;
        if(currentCommand.pluginHandle == pluginHandle)
        {
            i = commandList.erase(i);
            dbgcmddel(currentCommand.command);
        }
        else
            ++i;
    }
}

/**
\brief Unregister all plugin expression functions.
\param pluginHandle Handle of the plugin to remove the commands from.
*/
void pluginexprfuncunregisterall(int pluginHandle)
{
    SHARED_ACQUIRE(LockPluginExprfunctionList);
    auto commandList = pluginExprfunctionList; //copy for thread-safety reasons
    SHARED_RELEASE();
    auto i = commandList.begin();
    while(i != commandList.end())
    {
        auto currentExprfunction = *i;
        if(currentExprfunction.pluginHandle == pluginHandle)
        {
            i = commandList.erase(i);
            ExpressionFunctions::Unregister(currentExprfunction.name);
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
    PLUG_COMMAND plugCmd;
    plugCmd.pluginHandle = pluginHandle;
    strcpy_s(plugCmd.command, command);
    if(!dbgcmdnew(command, (CBCOMMAND)cbCommand, debugonly))
        return false;
    EXCLUSIVE_ACQUIRE(LockPluginCommandList);
    pluginCommandList.push_back(plugCmd);
    EXCLUSIVE_RELEASE();
    dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] command \"%s\" registered!\n"), command);
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
    EXCLUSIVE_ACQUIRE(LockPluginCommandList);
    for(auto it = pluginCommandList.begin(); it != pluginCommandList.end(); ++it)
    {
        const auto & currentCommand = *it;
        if(currentCommand.pluginHandle == pluginHandle && !strcmp(currentCommand.command, command))
        {
            pluginCommandList.erase(it);
            EXCLUSIVE_RELEASE();
            if(!dbgcmddel(command))
                return false;
            dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] command \"%s\" unregistered!\n"), command);
            return true;
        }
    }
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
        if(pluginMenuList.at(i).hEntryMenu == hMenu && pluginMenuList.at(i).hEntryPlugin == -1)
        {
            nFound = i;
            break;
        }
    }
    if(nFound == -1) //not a valid menu handle
        return -1;
    int hMenuNew = GuiMenuAdd(pluginMenuList.at(nFound).hEntryMenu, title);
    PLUG_MENU newMenu;
    newMenu.pluginHandle = pluginMenuList.at(nFound).pluginHandle;
    newMenu.hEntryPlugin = -1;
    newMenu.hEntryMenu = hMenuNew;
    pluginMenuList.push_back(newMenu);
    return hMenuNew;
}

/**
\brief Add a plugin menu entry to a menu.
\param hMenu The menu to add the entry to.
\param hEntry The handle you like to have the entry. This should be a unique value in the scope of the plugin that registered the \p hMenu.
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
        if(currentMenu.hEntryMenu == hMenu && currentMenu.hEntryPlugin == -1)
        {
            pluginHandle = currentMenu.pluginHandle;
            break;
        }
    }
    if(pluginHandle == -1) //not found
        return false;
    //search if hEntry was previously used
    for(const auto & currentMenu : pluginMenuList)
        if(currentMenu.pluginHandle == pluginHandle && currentMenu.hEntryPlugin == hEntry)
            return false;
    int hNewEntry = GuiMenuAddEntry(hMenu, title);
    if(hNewEntry == -1)
        return false;
    PLUG_MENU newMenu;
    newMenu.hEntryMenu = hNewEntry;
    newMenu.hEntryPlugin = hEntry;
    newMenu.pluginHandle = pluginHandle;
    pluginMenuList.push_back(newMenu);
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
        if(currentMenu.hEntryMenu == hMenu && currentMenu.hEntryPlugin == -1)
        {
            GuiMenuAddSeparator(hMenu);
            return true;
        }
    }
    return false;
}

/**
\brief Clears a plugin menu.
\param hMenu The menu to clear.
\return true if it succeeds, false otherwise.
*/
bool pluginmenuclear(int hMenu)
{
    EXCLUSIVE_ACQUIRE(LockPluginMenuList);
    bool bFound = false;
    for(auto it = pluginMenuList.begin(); it != pluginMenuList.end(); ++it)
    {
        const auto & currentMenu = *it;
        if(currentMenu.hEntryMenu == hMenu && currentMenu.hEntryPlugin == -1)
        {
            it = pluginMenuList.erase(it);
            bFound = true;
        }
    }
    if(!bFound)
        return false;
    GuiMenuClear(hMenu);
    return true;
}

/**
\brief Call the registered CB_MENUENTRY callbacks for a menu entry.
\param hEntry The menu entry that triggered the event.
*/
void pluginmenucall(int hEntry)
{
    if(hEntry == -1)
        return;

    SectionLocker<LockPluginMenuList, true> menuLock; //shared lock
    auto i = pluginMenuList.begin();
    while(i != pluginMenuList.end())
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
        if(currentMenu.hEntryMenu == hMenu && currentMenu.hEntryPlugin == -1)
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
    for(const auto & currentMenu : pluginMenuList)
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
    for(const auto & currentMenu : pluginMenuList)
    {
        if(currentMenu.pluginHandle == pluginHandle && currentMenu.hEntryPlugin == hEntry)
        {
            GuiMenuSetEntryChecked(currentMenu.hEntryMenu, checked);
            break;
        }
    }
}

bool pluginexprfuncregister(int pluginHandle, const char* name, int argc, CBPLUGINEXPRFUNCTION cbFunction, void* userdata)
{
    PLUG_EXPRFUNCTION plugExprfunction;
    plugExprfunction.pluginHandle = pluginHandle;
    strcpy_s(plugExprfunction.name, name);
    if(!ExpressionFunctions::Register(name, argc, cbFunction, userdata))
        return false;
    EXCLUSIVE_ACQUIRE(LockPluginExprfunctionList);
    pluginExprfunctionList.push_back(plugExprfunction);
    EXCLUSIVE_RELEASE();
    dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] expression function \"%s\" registered!\n"), name);
    return true;
}

bool pluginexprfuncunregister(int pluginHandle, const char* name)
{
    EXCLUSIVE_ACQUIRE(LockPluginExprfunctionList);
    for(auto it = pluginExprfunctionList.begin(); it != pluginExprfunctionList.end(); ++it)
    {
        const auto & currentExprfunction = *it;
        if(currentExprfunction.pluginHandle == pluginHandle && !strcmp(currentExprfunction.name, name))
        {
            pluginExprfunctionList.erase(it);
            EXCLUSIVE_RELEASE();
            if(!ExpressionFunctions::Unregister(name))
                return false;
            dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] expression function \"%s\" unregistered!\n"), name);
            return true;
        }
    }
    return false;
}
