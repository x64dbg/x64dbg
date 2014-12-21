#include "plugin_loader.h"
#include "console.h"
#include "debugger.h"
#include "memory.h"
#include "x64_dbg.h"

static std::vector<PLUG_DATA> pluginList;
static int curPluginHandle = 0;
static std::vector<PLUG_CALLBACK> pluginCallbackList;
static std::vector<PLUG_COMMAND> pluginCommandList;
static std::vector<PLUG_MENU> pluginMenuList;

///internal plugin functions
void pluginload(const char* pluginDir)
{
    //load new plugins
    wchar_t currentDir[deflen] = L"";
    GetCurrentDirectoryW(deflen, currentDir);
    SetCurrentDirectoryW(StringUtils::Utf8ToUtf16(pluginDir).c_str());
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
    PLUG_DATA pluginData;
    do
    {
        //set plugin data
        pluginData.initStruct.pluginHandle = curPluginHandle;
        char szPluginPath[MAX_PATH] = "";
        sprintf_s(szPluginPath, "%s\\%s", pluginDir, StringUtils::Utf16ToUtf8(foundData.cFileName).c_str());
        pluginData.hPlugin = LoadLibraryW(StringUtils::Utf8ToUtf16(szPluginPath).c_str()); //load the plugin library
        if(!pluginData.hPlugin)
        {
            dprintf("[PLUGIN] Failed to load plugin: %s\n", foundData.cFileName);
            continue;
        }
        pluginData.pluginit = (PLUGINIT)GetProcAddress(pluginData.hPlugin, "pluginit");
        if(!pluginData.pluginit)
        {
            dprintf("[PLUGIN] Export \"pluginit\" not found in plugin: %s\n", foundData.cFileName);
            FreeLibrary(pluginData.hPlugin);
            continue;
        }
        pluginData.plugstop = (PLUGSTOP)GetProcAddress(pluginData.hPlugin, "plugstop");
        pluginData.plugsetup = (PLUGSETUP)GetProcAddress(pluginData.hPlugin, "plugsetup");
        //auto-register callbacks for certain export names
        CBPLUGIN cbPlugin;
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBALLEVENTS");
        if(cbPlugin)
        {
            pluginregistercallback(curPluginHandle, CB_INITDEBUG, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_STOPDEBUG, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_CREATEPROCESS, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_EXITPROCESS, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_CREATETHREAD, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_EXITTHREAD, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_SYSTEMBREAKPOINT, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_LOADDLL, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_UNLOADDLL, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_OUTPUTDEBUGSTRING, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_EXCEPTION, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_BREAKPOINT, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_PAUSEDEBUG, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_RESUMEDEBUG, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_STEPPED, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_ATTACH, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_DETACH, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_DEBUGEVENT, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_MENUENTRY, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_WINEVENT, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_WINEVENTGLOBAL, cbPlugin);
        }
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBINITDEBUG");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_INITDEBUG, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBSTOPDEBUG");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_STOPDEBUG, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBCREATEPROCESS");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_CREATEPROCESS, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBEXITPROCESS");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_EXITPROCESS, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBCREATETHREAD");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_CREATETHREAD, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBEXITTHREAD");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_EXITTHREAD, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBSYSTEMBREAKPOINT");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_SYSTEMBREAKPOINT, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBLOADDLL");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_LOADDLL, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBUNLOADDLL");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_UNLOADDLL, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBOUTPUTDEBUGSTRING");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_OUTPUTDEBUGSTRING, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBEXCEPTION");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_EXCEPTION, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBBREAKPOINT");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_BREAKPOINT, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBPAUSEDEBUG");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_PAUSEDEBUG, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBRESUMEDEBUG");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_RESUMEDEBUG, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBSTEPPED");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_STEPPED, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBATTACH");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_ATTACH, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBDETACH");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_DETACH, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBDEBUGEVENT");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_DEBUGEVENT, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBMENUENTRY");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_MENUENTRY, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBWINEVENT");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_WINEVENT, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBWINEVENTGLOBAL");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_WINEVENTGLOBAL, cbPlugin);
        //init plugin
        //TODO: handle exceptions
        if(!pluginData.pluginit(&pluginData.initStruct))
        {
            dprintf("[PLUGIN] pluginit failed for plugin: %s\n", foundData.cFileName);
            FreeLibrary(pluginData.hPlugin);
            continue;
        }
        else if(pluginData.initStruct.sdkVersion < PLUG_SDKVERSION) //the plugin SDK is not compatible
        {
            dprintf("[PLUGIN] %s is incompatible with this SDK version\n", pluginData.initStruct.pluginName);
            FreeLibrary(pluginData.hPlugin);
            continue;
        }
        else
            dprintf("[PLUGIN] %s v%d Loaded!\n", pluginData.initStruct.pluginName, pluginData.initStruct.pluginVersion);
        //add plugin menu
        int hNewMenu = GuiMenuAdd(GUI_PLUGIN_MENU, pluginData.initStruct.pluginName);
        if(hNewMenu == -1)
        {
            dprintf("[PLUGIN] GuiMenuAdd failed for plugin: %s\n", pluginData.initStruct.pluginName);
            pluginData.hMenu = -1;
        }
        else
        {
            PLUG_MENU newMenu;
            newMenu.hEntryMenu = hNewMenu;
            newMenu.hEntryPlugin = -1;
            newMenu.pluginHandle = pluginData.initStruct.pluginHandle;
            pluginMenuList.push_back(newMenu);
            pluginData.hMenu = hNewMenu;
        }
        pluginList.push_back(pluginData);
        //setup plugin
        if(pluginData.plugsetup)
        {
            PLUG_SETUPSTRUCT setupStruct;
            setupStruct.hwndDlg = GuiGetWindowHandle();
            setupStruct.hMenu = hNewMenu;
            pluginData.plugsetup(&setupStruct);
        }
        curPluginHandle++;
    }
    while(FindNextFileW(hSearch, &foundData));
    SetCurrentDirectoryW(currentDir);
}

static void plugincmdunregisterall(int pluginHandle)
{
    int listsize = (int)pluginCommandList.size();
    for(int i = listsize - 1; i >= 0; i--)
    {
        if(pluginCommandList.at(i).pluginHandle == pluginHandle)
        {
            dbgcmddel(pluginCommandList.at(i).command);
            pluginCommandList.erase(pluginCommandList.begin() + i);
        }
    }
}

void pluginunload()
{
    int pluginCount = (int)pluginList.size();
    for(int i = pluginCount - 1; i > -1; i--)
    {
        PLUGSTOP stop = pluginList.at(i).plugstop;
        if(stop)
            stop();
        plugincmdunregisterall(pluginList.at(i).initStruct.pluginHandle);
        FreeLibrary(pluginList.at(i).hPlugin);
        pluginList.erase(pluginList.begin() + i);
    }
    pluginCallbackList.clear(); //remove all callbacks
    pluginMenuList.clear(); //clear menu list
    GuiMenuClear(GUI_PLUGIN_MENU); //clear the plugin menu
}

///debugging plugin exports
void pluginregistercallback(int pluginHandle, CBTYPE cbType, CBPLUGIN cbPlugin)
{
    pluginunregistercallback(pluginHandle, cbType); //remove previous callback
    PLUG_CALLBACK cbStruct;
    cbStruct.pluginHandle = pluginHandle;
    cbStruct.cbType = cbType;
    cbStruct.cbPlugin = cbPlugin;
    pluginCallbackList.push_back(cbStruct);
}

bool pluginunregistercallback(int pluginHandle, CBTYPE cbType)
{
    int pluginCallbackCount = (int)pluginCallbackList.size();
    for(int i = 0; i < pluginCallbackCount; i++)
    {
        if(pluginCallbackList.at(i).pluginHandle == pluginHandle and pluginCallbackList.at(i).cbType == cbType)
        {
            pluginCallbackList.erase(pluginCallbackList.begin() + i);
            return true;
        }
    }
    return false;
}

void plugincbcall(CBTYPE cbType, void* callbackInfo)
{
    int pluginCallbackCount = (int)pluginCallbackList.size();
    for(int i = 0; i < pluginCallbackCount; i++)
    {
        if(pluginCallbackList.at(i).cbType == cbType)
        {
            CBPLUGIN cbPlugin = pluginCallbackList.at(i).cbPlugin;
            if(memisvalidreadptr(GetCurrentProcess(), (uint)cbPlugin))
                cbPlugin(cbType, callbackInfo);
        }
    }
}

bool plugincmdregister(int pluginHandle, const char* command, CBPLUGINCOMMAND cbCommand, bool debugonly)
{
    if(!command or strlen(command) >= deflen or strstr(command, "\1"))
        return false;
    PLUG_COMMAND plugCmd;
    plugCmd.pluginHandle = pluginHandle;
    strcpy(plugCmd.command, command);
    if(!dbgcmdnew(command, (CBCOMMAND)cbCommand, debugonly))
        return false;
    pluginCommandList.push_back(plugCmd);
    dprintf("[PLUGIN] command \"%s\" registered!\n", command);
    return true;
}

bool plugincmdunregister(int pluginHandle, const char* command)
{
    if(!command or strlen(command) >= deflen or strstr(command, "\1"))
        return false;
    int listsize = (int)pluginCommandList.size();
    for(int i = 0; i < listsize; i++)
    {
        if(pluginCommandList.at(i).pluginHandle == pluginHandle and !strcmp(pluginCommandList.at(i).command, command))
        {
            if(!dbgcmddel(command))
                return false;
            pluginCommandList.erase(pluginCommandList.begin() + i);
            dprintf("[PLUGIN] command \"%s\" unregistered!\n", command);
            return true;
        }
    }
    return false;
}

int pluginmenuadd(int hMenu, const char* title)
{
    if(!title or !strlen(title))
        return -1;
    int nFound = -1;
    for(unsigned int i = 0; i < pluginMenuList.size(); i++)
    {
        if(pluginMenuList.at(i).hEntryMenu == hMenu and pluginMenuList.at(i).hEntryPlugin == -1)
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

bool pluginmenuaddentry(int hMenu, int hEntry, const char* title)
{
    if(!title or !strlen(title) or hEntry == -1)
        return false;
    int pluginHandle = -1;
    //find plugin handle
    for(unsigned int i = 0; i < pluginMenuList.size(); i++)
    {
        if(pluginMenuList.at(i).hEntryMenu == hMenu and pluginMenuList.at(i).hEntryPlugin == -1)
        {
            pluginHandle = pluginMenuList.at(i).pluginHandle;
            break;
        }
    }
    if(pluginHandle == -1) //not found
        return false;
    //search if hEntry was previously used
    for(unsigned int i = 0; i < pluginMenuList.size(); i++)
        if(pluginMenuList.at(i).pluginHandle == pluginHandle && pluginMenuList.at(i).hEntryPlugin == hEntry)
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

bool pluginmenuaddseparator(int hMenu)
{
    bool bFound = false;
    for(unsigned int i = 0; i < pluginMenuList.size(); i++)
    {
        if(pluginMenuList.at(i).hEntryMenu == hMenu and pluginMenuList.at(i).hEntryPlugin == -1)
        {
            bFound = true;
            break;
        }
    }
    if(!bFound)
        return false;
    GuiMenuAddSeparator(hMenu);
    return true;
}

bool pluginmenuclear(int hMenu)
{
    bool bFound = false;
    for(unsigned int i = 0; i < pluginMenuList.size(); i++)
    {
        if(pluginMenuList.at(i).hEntryMenu == hMenu and pluginMenuList.at(i).hEntryPlugin == -1)
        {
            bFound = true;
            break;
        }
    }
    if(!bFound)
        return false;
    GuiMenuClear(hMenu);
    return false;
}

void pluginmenucall(int hEntry)
{
    if(hEntry == -1)
        return;
    for(unsigned int i = 0; i < pluginMenuList.size(); i++)
    {
        if(pluginMenuList.at(i).hEntryMenu == hEntry && pluginMenuList.at(i).hEntryPlugin != -1)
        {
            PLUG_CB_MENUENTRY menuEntryInfo;
            menuEntryInfo.hEntry = pluginMenuList.at(i).hEntryPlugin;
            int pluginCallbackCount = (int)pluginCallbackList.size();
            int pluginHandle = pluginMenuList.at(i).pluginHandle;
            for(int j = 0; j < pluginCallbackCount; j++)
            {
                if(pluginCallbackList.at(j).pluginHandle == pluginHandle and pluginCallbackList.at(j).cbType == CB_MENUENTRY)
                {
                    //TODO: handle exceptions
                    pluginCallbackList.at(j).cbPlugin(CB_MENUENTRY, &menuEntryInfo);
                    return;
                }
            }
        }
    }
}

bool pluginwinevent(MSG* message, long* result)
{
    PLUG_CB_WINEVENT winevent;
    winevent.message = message;
    winevent.result = result;
    winevent.retval = false;
    plugincbcall(CB_WINEVENT, &winevent);
    return winevent.retval;
}

bool pluginwineventglobal(MSG* message)
{
    PLUG_CB_WINEVENTGLOBAL winevent;
    winevent.message = message;
    winevent.retval = false;
    plugincbcall(CB_WINEVENTGLOBAL, &winevent);
    return winevent.retval;
}