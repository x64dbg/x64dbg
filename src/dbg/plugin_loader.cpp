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
#include <vector>

/**
\brief List of plugins.
*/
static std::vector<PLUG_DATA> gPluginList;

/**
\brief Saved plugin directory
*/
static std::wstring gPluginDirectory;

/**
\brief The current plugin handle.
*/
static int gCurPluginHandle = 1;

/**
\brief List of plugin callbacks.
*/
static std::vector<PLUG_CALLBACK> gPluginCallbackList[CB_LAST];

/**
\brief List of plugin commands.
*/
static std::vector<PLUG_COMMAND> gPluginCommandList;

/**
\brief List of plugin menus.
*/
static std::vector<PLUG_MENU> gPluginMenuList;

/**
\brief List of plugin menu entries.
*/
static std::vector<PLUG_MENUENTRY> gPluginMenuEntryList;

/**
\brief List of plugin exprfunctions.
*/
static std::vector<PLUG_EXPRFUNCTION> gPluginExprfunctionList;

/**
\brief List of plugin formatfunctions.
*/
static std::vector<PLUG_FORMATFUNCTION> gPluginFormatfunctionList;

/**
\brief Global data for the plugin currently being loaded
*/
static PLUG_DATA gLoadingPlugin;

/**
\brief Extension for the plugin file
*/
static const wchar_t* gPluginExtension = ArchValue(L".dp32", L".dp64");

/**
/brief Normalizes the plugin name. Strips the extension and path.
\param pluginName Can be name or partial plugin path.
\return The normalized plugin name.
 */
static std::string pluginNormalizeName(std::string pluginName)
{
    auto pathPos = pluginName.find_last_of("\\/");
    if(pathPos != std::string::npos)
        pluginName = pluginName.substr(pathPos + 1);
    auto extPos = pluginName.rfind(StringUtils::Utf16ToUtf8(gPluginExtension));
    if(extPos != std::string::npos)
        pluginName = pluginName.substr(0, extPos);
    return pluginName;
}

struct ChangeDirectory
{
    ChangeDirectory(const ChangeDirectory &) = delete;
    ChangeDirectory(ChangeDirectory &&) = delete;

    explicit ChangeDirectory(const wchar_t* newDirectory)
    {
        GetCurrentDirectoryW(_countof(mPreviousDirectory), mPreviousDirectory);
        SetCurrentDirectoryW(newDirectory);
    }

    ~ChangeDirectory()
    {
        SetCurrentDirectoryW(mPreviousDirectory);
    }

private:
    wchar_t mPreviousDirectory[deflen];
};

static void* addDllDirectory(const wchar_t* newDirectory)
{
    typedef PVOID(WINAPI * pfnAddDllDirectory)(LPCWSTR);
    static auto pAddDllDirectory = (pfnAddDllDirectory)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "AddDllDirectory");
    return pAddDllDirectory ? pAddDllDirectory(newDirectory) : nullptr;
}

static bool removeDllDirectory(void* cookie)
{
    typedef BOOL(WINAPI * pfnRemoveDllDirectory)(PVOID Cookie);
    static auto pRemoveDllDirectory = (pfnRemoveDllDirectory)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "RemoveDllDirectory");
    return pRemoveDllDirectory ? !!pRemoveDllDirectory(cookie) : false;
}

struct DllDirectory
{
    DllDirectory(const DllDirectory &) = delete;
    DllDirectory(DllDirectory &&) = delete;

    explicit DllDirectory(const wchar_t* directory)
    {
        if(directory != nullptr)
        {
            mCookie = addDllDirectory(directory);
        }
    }

    ~DllDirectory()
    {
        if(mCookie != nullptr)
        {
            removeDllDirectory(mCookie);
        }
    }

private:
    void* mCookie = nullptr;
};

/**
\brief Loads a plugin from the plugin directory.
\param pluginName Name of the plugin.
\param loadall true on unload all.
\return true if it succeeds, false if it fails.
*/
bool pluginload(const char* pluginName, bool loadall)
{
    // No empty plugin names allowed
    if(pluginName == nullptr || *pluginName == '\0')
        return false;

    // Normalize the plugin name, for flexibility the pluginload command
    auto normalizedName = pluginNormalizeName(pluginName);
    auto pluginDirectory = gPluginDirectory;

    // Obtain the actual plugin path.
    auto pluginPath = gPluginDirectory + L"\\" + StringUtils::Utf8ToUtf16(normalizedName);
    auto pluginSubPath = pluginPath + L"\\" + StringUtils::Utf8ToUtf16(normalizedName) + gPluginExtension;
    auto subdirPlugin = !!PathFileExistsW(pluginSubPath.c_str());
    if(subdirPlugin)
    {
        // Plugin resides in a subdirectory
        pluginDirectory = pluginPath;
        pluginPath = pluginSubPath;
    }
    else
    {
        pluginPath += gPluginExtension;
    }

    // Check to see if this plugin is already loaded
    EXCLUSIVE_ACQUIRE(LockPluginList);
    for(auto it = gPluginList.begin(); it != gPluginList.end(); ++it)
    {
        if(_stricmp(it->plugname, normalizedName.c_str()) == 0)
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] %s already loaded\n"), it->plugname);
            return false;
        }
    }

    // Check if the file exists
    if(!PathFileExistsW(pluginPath.c_str()))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] Cannot find plugin: %s\n"), StringUtils::Utf16ToUtf8(pluginPath).c_str());
        return false;
    }

    // Set the working and DLL load directories
    ChangeDirectory cd(pluginDirectory.c_str());
    DllDirectory dd(loadall && !subdirPlugin ? nullptr : pluginDirectory.c_str());

    // Setup plugin data
    // NOTE: This is a global because during registration the plugin
    // isn't in the gPluginList yet
    auto pluginHandle = gCurPluginHandle++;
    gLoadingPlugin = {};
    gLoadingPlugin.initStruct.pluginHandle = pluginHandle;
    gLoadingPlugin.hPlugin = LoadLibraryW(pluginPath.c_str()); //load the plugin library
    if(!gLoadingPlugin.hPlugin)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] Failed to load plugin: %s\n"), normalizedName.c_str());
        return false;
    }
    gLoadingPlugin.pluginit = (PLUGINIT)GetProcAddress(gLoadingPlugin.hPlugin, "pluginit");
    if(!gLoadingPlugin.pluginit)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] Export \"pluginit\" not found in plugin: %s\n"), normalizedName.c_str());
        for(int i = CB_INITDEBUG; i < CB_LAST; i++)
            pluginunregistercallback(pluginHandle, CBTYPE(i));
        FreeLibrary(gLoadingPlugin.hPlugin);
        return false;
    }
    gLoadingPlugin.plugstop = (PLUGSTOP)GetProcAddress(gLoadingPlugin.hPlugin, "plugstop");
    gLoadingPlugin.plugsetup = (PLUGSETUP)GetProcAddress(gLoadingPlugin.hPlugin, "plugsetup");

    strncpy_s(gLoadingPlugin.plugpath, StringUtils::Utf16ToUtf8(pluginPath).c_str(), _TRUNCATE);
    strncpy_s(gLoadingPlugin.plugname, normalizedName.c_str(), _TRUNCATE);
    strncpy_s(gLoadingPlugin.initStruct.pluginName, normalizedName.c_str(), _TRUNCATE);

    // Init plugin
    if(!gLoadingPlugin.pluginit(&gLoadingPlugin.initStruct))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] pluginit failed for plugin: %s\n"), normalizedName.c_str());
        for(int i = CB_INITDEBUG; i < CB_LAST; i++)
            pluginunregistercallback(pluginHandle, CBTYPE(i));
        FreeLibrary(gLoadingPlugin.hPlugin);
        return false;
    }
    if(gLoadingPlugin.initStruct.sdkVersion < PLUG_SDKVERSION) //the plugin SDK is not compatible
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] %s is incompatible with this SDK version\n"), gLoadingPlugin.initStruct.pluginName);
        for(int i = CB_INITDEBUG; i < CB_LAST; i++)
            pluginunregistercallback(pluginHandle, CBTYPE(i));
        FreeLibrary(gLoadingPlugin.hPlugin);
        return false;
    }

    dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] %s v%d Loaded!\n"), gLoadingPlugin.initStruct.pluginName, gLoadingPlugin.initStruct.pluginVersion);

    // Auto-register callbacks for certain export names
    auto cbPlugin = CBPLUGIN(GetProcAddress(gLoadingPlugin.hPlugin, "CBALLEVENTS"));
    if(cbPlugin)
    {
        for(int i = CB_INITDEBUG; i < CB_LAST; i++)
            pluginregistercallback(pluginHandle, CBTYPE(i), cbPlugin);
    }
    auto regExport = [pluginHandle](const char* exportname, CBTYPE cbType)
    {
        auto cbPlugin = CBPLUGIN(GetProcAddress(gLoadingPlugin.hPlugin, exportname));
        if(cbPlugin)
            pluginregistercallback(pluginHandle, cbType, cbPlugin);
    };
    regExport("CBINITDEBUG", CB_INITDEBUG);
    regExport("CBSTOPDEBUG", CB_STOPDEBUG);
    regExport("CBSTOPPINGDEBUG", CB_STOPPINGDEBUG);
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

    // Add plugin menus
    {
        SectionLocker<LockPluginMenuList, false, false> menuLock; //exclusive lock

        auto addPluginMenu = [](GUIMENUTYPE type)
        {
            int hNewMenu = GuiMenuAdd(type, gLoadingPlugin.initStruct.pluginName);
            if(hNewMenu == -1)
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] GuiMenuAdd(%d) failed for plugin: %s\n"), type, gLoadingPlugin.initStruct.pluginName);
                return -1;
            }
            else
            {
                PLUG_MENU newMenu;
                newMenu.hEntryMenu = hNewMenu;
                newMenu.hParentMenu = type;
                newMenu.pluginHandle = gLoadingPlugin.initStruct.pluginHandle;
                gPluginMenuList.push_back(newMenu);
                return newMenu.hEntryMenu;
            }
        };

        gLoadingPlugin.hMenu = addPluginMenu(GUI_PLUGIN_MENU);
        gLoadingPlugin.hMenuDisasm = addPluginMenu(GUI_DISASM_MENU);
        gLoadingPlugin.hMenuDump = addPluginMenu(GUI_DUMP_MENU);
        gLoadingPlugin.hMenuStack = addPluginMenu(GUI_STACK_MENU);
        gLoadingPlugin.hMenuGraph = addPluginMenu(GUI_GRAPH_MENU);
        gLoadingPlugin.hMenuMemmap = addPluginMenu(GUI_MEMMAP_MENU);
        gLoadingPlugin.hMenuSymmod = addPluginMenu(GUI_SYMMOD_MENU);
    }

    // Add the plugin to the list
    {
        SectionLocker<LockPluginList, false> pluginLock; //exclusive lock
        gPluginList.push_back(gLoadingPlugin);
    }

    // Setup plugin
    if(gLoadingPlugin.plugsetup)
    {
        PLUG_SETUPSTRUCT setupStruct;
        setupStruct.hwndDlg = GuiGetWindowHandle();
        setupStruct.hMenu = gLoadingPlugin.hMenu;
        setupStruct.hMenuDisasm = gLoadingPlugin.hMenuDisasm;
        setupStruct.hMenuDump = gLoadingPlugin.hMenuDump;
        setupStruct.hMenuStack = gLoadingPlugin.hMenuStack;
        setupStruct.hMenuGraph = gLoadingPlugin.hMenuGraph;
        setupStruct.hMenuMemmap = gLoadingPlugin.hMenuMemmap;
        setupStruct.hMenuSymmod = gLoadingPlugin.hMenuSymmod;
        gLoadingPlugin.plugsetup(&setupStruct);
    }

    // Clear the loading plugin structure (since it's now in the plugin list)
    gLoadingPlugin = {};

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
    // Normalize the plugin name.
    std::string name = pluginNormalizeName(pluginName);

    auto found = gPluginList.end();
    {
        EXCLUSIVE_ACQUIRE(LockPluginList);
        found = std::find_if(gPluginList.begin(), gPluginList.end(), [&name](const PLUG_DATA & a)
        {
            return _stricmp(a.plugname, name.c_str()) == 0;
        });
    }

    // Will contain the actual plugin name if found.
    std::string actualName = name;

    if(found != gPluginList.end())
    {
        bool canFreeLibrary = true;
        auto currentPlugin = *found;
        if(currentPlugin.plugstop)
            canFreeLibrary = currentPlugin.plugstop();
        int pluginHandle = currentPlugin.initStruct.pluginHandle;
        plugincmdunregisterall(pluginHandle);
        pluginexprfuncunregisterall(pluginHandle);
        pluginformatfuncunregisterall(pluginHandle);

        // Copy the actual name.
        actualName = currentPlugin.plugname;

        //remove the callbacks
        {
            EXCLUSIVE_ACQUIRE(LockPluginCallbackList);
            for(auto & cbList : gPluginCallbackList)
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
                gPluginList.erase(found);
            }
        }

        if(canFreeLibrary)
            FreeLibrary(currentPlugin.hPlugin);
        dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] %s unloaded\n"), actualName.c_str());
        return true;
    }
    dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] %s not found\n"), actualName.c_str());
    return false;
}

// Returns a list of all available plugins in the specified directory without the extension.
static std::vector<std::wstring> enumerateAvailablePlugins(const std::wstring & pluginDir)
{
    std::vector<std::wstring> result;

    const std::wstring pluginExt = gPluginExtension;

    wchar_t searchQuery[deflen] = L"";
    swprintf_s(searchQuery, L"%s\\*", pluginDir.c_str());

    WIN32_FIND_DATAW foundData;
    HANDLE hSearch = FindFirstFileW(searchQuery, &foundData);
    if(hSearch == INVALID_HANDLE_VALUE)
    {
        return result;
    }

    const auto stripExtension = [](const std::wstring & fileName) -> std::wstring
    {
        size_t pos = fileName.find_last_of(L'.');
        if(pos == std::wstring::npos)
        {
            return fileName;
        }
        return fileName.substr(0, pos);
    };

    do
    {
        // Check if directory
        if(foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // Check if plugin exists with the same name as the directory
            wchar_t pluginName[deflen] = L"";
            swprintf_s(pluginName, L"%s\\%s%s", foundData.cFileName, foundData.cFileName, pluginExt.c_str());

            wchar_t pluginPath[deflen] = L"";
            swprintf_s(pluginPath, L"%s\\%s", pluginDir.c_str(), pluginName);

            if(PathFileExistsW(pluginPath))
            {
                result.push_back(foundData.cFileName);
            }
        }
        else
        {
            // Check if filename ends with the extension
            wchar_t* extPos = wcsstr(foundData.cFileName, pluginExt.c_str());
            if(extPos == nullptr)
            {
                continue;
            }

            // Ensure that the extension is at the end of the filename
            if(extPos[pluginExt.size()] != L'\0')
            {
                continue;
            }

            auto pluginName = stripExtension(foundData.cFileName);
            auto pluginFolderPath = pluginDir + L"\\" + pluginName + L"\\" + pluginName + gPluginExtension;
            if(PathFileExistsW(pluginFolderPath.c_str()))
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "[PLUGIN] Ignoring top-level plugin in favor of the directory 'plugins\\%s'\n"), StringUtils::Utf16ToUtf8(pluginName).c_str());
            }
            else
            {
                result.push_back(pluginName);
            }
        }
    }
    while(FindNextFileW(hSearch, &foundData));

    FindClose(hSearch);

    // Sort the list of plugins.
    std::sort(result.begin(), result.end());

    return result;
}

/**
\brief Loads plugins from a specified directory.
\param pluginDir The directory to load plugins from.
*/
void pluginloadall(const char* pluginDir)
{
    //reserve menu space
    gPluginMenuList.reserve(1024);
    gPluginMenuEntryList.reserve(1024);

    //load new plugins
    wchar_t currentDir[deflen] = L"";
    gPluginDirectory = StringUtils::Utf8ToUtf16(pluginDir);

    // Enumerate all plugins from plugins directory.
    const auto availablePlugins = enumerateAvailablePlugins(StringUtils::Utf8ToUtf16(pluginDir));

    // Add the plugins directory as valid dependency directory
    auto pluginDirectoryCookie = addDllDirectory(gPluginDirectory.c_str());

    for(const std::wstring & pluginName : availablePlugins)
    {
        dprintf("[pluginload] %S\n", pluginName.c_str());
        pluginload(StringUtils::Utf16ToUtf8(pluginName).c_str(), true);
    }

    // Remove the plugins directory after loading finished
    if(pluginDirectoryCookie != nullptr)
    {
        removeDllDirectory(pluginDirectoryCookie);
    }
}

/**
\brief Unloads all plugins.
*/
void pluginunloadall()
{
    // Get a copy of the list of thread-safety reasons.
    const auto pluginListCopy = []()
    {
        SHARED_ACQUIRE(LockPluginList);
        return gPluginList;
    }();

    // Unload all plugins.
    for(const auto & plugin : pluginListCopy)
        pluginunload(plugin.plugname, true);

    // Remove all plugins from the list.
    SHARED_ACQUIRE(LockPluginList);
    gPluginList.clear();
    SHARED_RELEASE();
}

/**
\brief Unregister all plugin commands.
\param pluginHandle Handle of the plugin to remove the commands from.
*/
void plugincmdunregisterall(int pluginHandle)
{
    SHARED_ACQUIRE(LockPluginCommandList);
    auto commandList = gPluginCommandList; //copy for thread-safety reasons
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
    auto exprFuncList = gPluginExprfunctionList; //copy for thread-safety reasons
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
    auto formatFuncList = gPluginFormatfunctionList; //copy for thread-safety reasons
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
    gPluginCallbackList[cbType].push_back(cbStruct);
}

/**
\brief Unregister all plugin callbacks of a certain type.
\param pluginHandle Handle of the plugin to unregister a callback from.
\param cbType The type of the callback to unregister.
*/
bool pluginunregistercallback(int pluginHandle, CBTYPE cbType)
{
    EXCLUSIVE_ACQUIRE(LockPluginCallbackList);
    auto & cbList = gPluginCallbackList[cbType];
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
    if(gPluginCallbackList[cbType].empty())
        return;
    SHARED_ACQUIRE(LockPluginCallbackList);
    auto cbList = gPluginCallbackList[cbType]; //copy for thread-safety reasons
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
    return gPluginCallbackList[cbType].empty();
}

static bool findPluginName(int pluginHandle, String & name)
{
    SHARED_ACQUIRE(LockPluginList);
    if(gLoadingPlugin.initStruct.pluginHandle == pluginHandle)
    {
        name = gLoadingPlugin.initStruct.pluginName;
        return true;
    }
    for(auto & plugin : gPluginList)
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
    gPluginCommandList.push_back(plugCmd);
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
    for(auto it = gPluginCommandList.begin(); it != gPluginCommandList.end(); ++it)
    {
        const auto & currentCommand = *it;
        if(currentCommand.pluginHandle == pluginHandle && !strcmp(currentCommand.command, command))
        {
            gPluginCommandList.erase(it);
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
    for(unsigned int i = 0; i < gPluginMenuList.size(); i++)
    {
        if(gPluginMenuList.at(i).hEntryMenu == hMenu)
        {
            nFound = i;
            break;
        }
    }
    if(nFound == -1) //not a valid menu handle
        return -1;
    int hMenuNew = GuiMenuAdd(hMenu, title);
    PLUG_MENU newMenu;
    newMenu.pluginHandle = gPluginMenuList.at(nFound).pluginHandle;
    newMenu.hEntryMenu = hMenuNew;
    newMenu.hParentMenu = hMenu;
    gPluginMenuList.push_back(newMenu);
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
    for(const auto & currentMenu : gPluginMenuList)
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
    for(const auto & currentMenu : gPluginMenuEntryList)
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
    gPluginMenuEntryList.push_back(newMenu);
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
    for(const auto & currentMenu : gPluginMenuList)
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
    for(auto i = gPluginMenuEntryList.size() - 1; i != -1; i--)
        if(hMenu == gPluginMenuEntryList.at(i).hParentMenu) //we found an entry that has the menu as parent
            gPluginMenuEntryList.erase(gPluginMenuEntryList.begin() + i);
    //delete the menus
    std::vector<int> menuClearQueue;
    for(auto i = gPluginMenuList.size() - 1; i != -1; i--)
    {
        if(hMenu == gPluginMenuList.at(i).hParentMenu) //we found a menu that has the menu as parent
        {
            menuClearQueue.push_back(gPluginMenuList.at(i).hEntryMenu);
            gPluginMenuList.erase(gPluginMenuList.begin() + i);
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
    for(auto it = gPluginMenuList.begin(); it != gPluginMenuList.end(); ++it)
    {
        const auto & currentMenu = *it;
        if(currentMenu.hEntryMenu == hMenu)
        {
            if(erase)
            {
                it = gPluginMenuList.erase(it);
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
    auto i = gPluginMenuEntryList.begin();
    while(i != gPluginMenuEntryList.end())
    {
        const auto currentMenu = *i;
        ++i;
        if(currentMenu.hEntryMenu == hEntry && currentMenu.hEntryPlugin != -1)
        {
            PLUG_CB_MENUENTRY menuEntryInfo;
            menuEntryInfo.hEntry = currentMenu.hEntryPlugin;
            SectionLocker<LockPluginCallbackList, true> callbackLock; //shared lock
            const auto & cbList = gPluginCallbackList[CB_MENUENTRY];
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
    for(const auto & currentMenu : gPluginMenuList)
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
    for(const auto & currentMenu : gPluginMenuEntryList)
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
    for(const auto & currentMenu : gPluginMenuEntryList)
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
    for(const auto & currentMenu : gPluginMenuList)
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
    for(const auto & currentMenu : gPluginMenuEntryList)
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
    for(const auto & currentMenu : gPluginMenuList)
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
    for(const auto & currentMenu : gPluginMenuEntryList)
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
    for(const auto & currentMenu : gPluginMenuEntryList)
    {
        if(currentMenu.pluginHandle == pluginHandle && currentMenu.hEntryPlugin == hEntry)
        {
            for(const auto & plugin : gPluginList)
            {
                if(plugin.initStruct.pluginHandle == pluginHandle)
                {
                    char name[MAX_PATH] = "";
                    strcpy_s(name, plugin.plugname);
                    auto dot = strrchr(name, '.');
                    if(dot != nullptr)
                        *dot = '\0';
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
    for(const auto & currentMenu : gPluginMenuList)
        if(currentMenu.hEntryMenu == hMenu && currentMenu.hParentMenu < 256)
            return false;
    return pluginmenuclear(hMenu, true);
}

bool pluginmenuentryremove(int pluginHandle, int hEntry)
{
    EXCLUSIVE_ACQUIRE(LockPluginMenuList);
    for(auto it = gPluginMenuEntryList.begin(); it != gPluginMenuEntryList.end(); ++it)
    {
        const auto & currentEntry = *it;
        if(currentEntry.pluginHandle == pluginHandle && currentEntry.hEntryPlugin == hEntry)
        {
            GuiMenuRemove(currentEntry.hEntryMenu);
            gPluginMenuEntryList.erase(it);
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
    gPluginExprfunctionList.push_back(plugExprfunction);
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
    gPluginExprfunctionList.push_back(plugExprfunction);
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
    for(auto it = gPluginExprfunctionList.begin(); it != gPluginExprfunctionList.end(); ++it)
    {
        const auto & currentExprfunction = *it;
        if(currentExprfunction.pluginHandle == pluginHandle && !strcmp(currentExprfunction.name, name))
        {
            gPluginExprfunctionList.erase(it);
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
    gPluginFormatfunctionList.push_back(plugFormatfunction);
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
    for(auto it = gPluginFormatfunctionList.begin(); it != gPluginFormatfunctionList.end(); ++it)
    {
        const auto & currentFormatfunction = *it;
        if(currentFormatfunction.pluginHandle == pluginHandle && !strcmp(currentFormatfunction.name, type))
        {
            gPluginFormatfunctionList.erase(it);
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
