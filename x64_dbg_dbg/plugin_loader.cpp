#include "plugin_loader.h"
#include "console.h"
#include "command.h"
#include "x64_dbg.h"

static std::vector<PLUG_DATA> pluginList;
static int curPluginHandle=0;
static std::vector<PLUG_CALLBACK> pluginCallbackList;
static std::vector<PLUG_COMMAND> pluginCommandList;
static std::vector<PLUG_MENU> pluginMenuList;

///internal plugin functions
void pluginload(const char* pluginDir)
{
    //load new plugins
    char currentDir[deflen]="";
    GetCurrentDirectoryA(deflen, currentDir);
    SetCurrentDirectoryA(pluginDir);
    char searchName[deflen]="";
#ifdef _WIN64
    sprintf(searchName, "%s\\*.dp64", pluginDir);
#else
    sprintf(searchName, "%s\\*.dp32", pluginDir);
#endif // _WIN64
    WIN32_FIND_DATA foundData;
    HANDLE hSearch=FindFirstFileA(searchName, &foundData);
    if(hSearch==INVALID_HANDLE_VALUE)
    {
        SetCurrentDirectoryA(currentDir);
        return;
    }
    PLUG_DATA pluginData;
    do
    {
        //set plugin data
        pluginData.initStruct.pluginHandle=curPluginHandle;
        char szPluginPath[MAX_PATH]="";
        sprintf(szPluginPath, "%s\\%s", pluginDir, foundData.cFileName);
        pluginData.hPlugin=LoadLibraryA(szPluginPath); //load the plugin library
        if(!pluginData.hPlugin)
        {
            dprintf("[PLUGIN] Failed to load plugin: %s\n", foundData.cFileName);
            continue;
        }
        pluginData.pluginit=(PLUGINIT)GetProcAddress(pluginData.hPlugin, "pluginit");
        if(!pluginData.pluginit)
        {
            dprintf("[PLUGIN] Export \"pluginit\" not found in plugin: %s\n", foundData.cFileName);
            FreeLibrary(pluginData.hPlugin);
            continue;
        }
        pluginData.plugstop=(PLUGSTOP)GetProcAddress(pluginData.hPlugin, "plugstop");
        pluginData.plugsetup=(PLUGSETUP)GetProcAddress(pluginData.hPlugin, "plugsetup");
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
        int hNewMenu=GuiMenuAdd(GUI_PLUGIN_MENU, pluginData.initStruct.pluginName);
        if(hNewMenu==-1)
        {
            dprintf("[PLUGIN] GuiMenuAdd failed for plugin: %s\n", pluginData.initStruct.pluginName);
            pluginData.hMenu=-1;
        }
        else
        {
            PLUG_MENU newMenu;
            newMenu.hEntryMenu=hNewMenu;
            newMenu.hEntryPlugin=-1;
            newMenu.pluginHandle=pluginData.initStruct.pluginHandle;
            pluginMenuList.push_back(newMenu);
            pluginData.hMenu=hNewMenu;
        }
        pluginList.push_back(pluginData);
        //setup plugin
        if(pluginData.plugsetup)
        {
            PLUG_SETUPSTRUCT setupStruct;
            setupStruct.hwndDlg=GuiGetWindowHandle();
            setupStruct.hMenu=hNewMenu;
            pluginData.plugsetup(&setupStruct);
        }
        curPluginHandle++;
    }
    while(FindNextFileA(hSearch, &foundData));
    SetCurrentDirectoryA(currentDir);
}

static void plugincmdunregisterall(int pluginHandle)
{
    int listsize=pluginCommandList.size();
    for(int i=listsize-1; i>=0; i--)
    {
        if(pluginCommandList.at(i).pluginHandle==pluginHandle)
        {
            cmddel(dbggetcommandlist(), pluginCommandList.at(i).command);
            pluginCommandList.erase(pluginCommandList.begin()+i);
        }
    }
}

void pluginunload()
{
    int pluginCount=pluginList.size();
    for(int i=0; i<pluginCount; i++)
    {
        PLUGSTOP stop=pluginList.at(i).plugstop;
        if(stop)
            stop();
        plugincmdunregisterall(pluginList.at(i).initStruct.pluginHandle);
        FreeLibrary(pluginList.at(i).hPlugin);
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
    cbStruct.pluginHandle=pluginHandle;
    cbStruct.cbType=cbType;
    cbStruct.cbPlugin=cbPlugin;
    pluginCallbackList.push_back(cbStruct);
}

bool pluginunregistercallback(int pluginHandle, CBTYPE cbType)
{
    int pluginCallbackCount=pluginCallbackList.size();
    for(int i=0; i<pluginCallbackCount; i++)
    {
        if(pluginCallbackList.at(i).pluginHandle==pluginHandle and pluginCallbackList.at(i).cbType==cbType)
        {
            pluginCallbackList.erase(pluginCallbackList.begin()+i);
            return true;
        }
    }
    return false;
}

void plugincbcall(CBTYPE cbType, void* callbackInfo)
{
    int pluginCallbackCount=pluginCallbackList.size();
    for(int i=0; i<pluginCallbackCount; i++)
    {
        if(pluginCallbackList.at(i).cbType==cbType)
        {
            //TODO: handle exceptions
            pluginCallbackList.at(i).cbPlugin(cbType, callbackInfo);
        }
    }
}

bool plugincmdregister(int pluginHandle, const char* command, CBPLUGINCOMMAND cbCommand, bool debugonly)
{
    if(!command or strlen(command)>=deflen or strstr(command, "\1"))
        return false;
    PLUG_COMMAND plugCmd;
    plugCmd.pluginHandle=pluginHandle;
    strcpy(plugCmd.command, command);
    if(!cmdnew(dbggetcommandlist(), command, (CBCOMMAND)cbCommand, debugonly))
        return false;
    pluginCommandList.push_back(plugCmd);
    dprintf("[PLUGIN] command \"%s\" registered!\n", command);
    return true;
}

bool plugincmdunregister(int pluginHandle, const char* command)
{
    if(!command or strlen(command)>=deflen or strstr(command, "\1"))
        return false;
    int listsize=pluginCommandList.size();
    for(int i=0; i<listsize; i++)
    {
        if(pluginCommandList.at(i).pluginHandle==pluginHandle and !strcmp(pluginCommandList.at(i).command, command))
        {
            if(!cmddel(dbggetcommandlist(), command))
                return false;
            pluginCommandList.erase(pluginCommandList.begin()+i);
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
    int nFound=-1;
    for(unsigned int i=0; i<pluginMenuList.size(); i++)
    {
        if(pluginMenuList.at(i).hEntryMenu==hMenu and pluginMenuList.at(i).hEntryPlugin==-1)
        {
            nFound=i;
            break;
        }
    }
    if(nFound==-1) //not a valid menu handle
        return -1;
    int hMenuNew=GuiMenuAdd(pluginMenuList.at(nFound).hEntryMenu, title);
    PLUG_MENU newMenu;
    newMenu.pluginHandle=pluginMenuList.at(nFound).pluginHandle;
    newMenu.hEntryPlugin=-1;
    newMenu.hEntryMenu=hMenuNew;
    pluginMenuList.push_back(newMenu);
    return hMenuNew;
}

bool pluginmenuaddentry(int hMenu, int hEntry, const char* title)
{
    if(!title or !strlen(title) or hEntry==-1)
        return false;
    int pluginHandle=-1;
    //find plugin handle
    for(unsigned int i=0; i<pluginMenuList.size(); i++)
    {
        if(pluginMenuList.at(i).hEntryMenu==hMenu and pluginMenuList.at(i).hEntryPlugin==-1)
        {
            pluginHandle=pluginMenuList.at(i).pluginHandle;
            break;
        }
    }
    if(pluginHandle==-1) //not found
        return false;
    //search if hEntry was previously used
    for(unsigned int i=0; i<pluginMenuList.size(); i++)
        if(pluginMenuList.at(i).pluginHandle==pluginHandle && pluginMenuList.at(i).hEntryPlugin==hEntry)
            return false;
    int hNewEntry=GuiMenuAddEntry(hMenu, title);
    if(hNewEntry==-1)
        return false;
    PLUG_MENU newMenu;
    newMenu.hEntryMenu=hNewEntry;
    newMenu.hEntryPlugin=hEntry;
    newMenu.pluginHandle=pluginHandle;
    pluginMenuList.push_back(newMenu);
    return true;
}

bool pluginmenuaddseparator(int hMenu)
{
    bool bFound=false;
    for(unsigned int i=0; i<pluginMenuList.size(); i++)
    {
        if(pluginMenuList.at(i).hEntryMenu==hMenu and pluginMenuList.at(i).hEntryPlugin==-1)
        {
            bFound=true;
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
    bool bFound=false;
    for(unsigned int i=0; i<pluginMenuList.size(); i++)
    {
        if(pluginMenuList.at(i).hEntryMenu==hMenu and pluginMenuList.at(i).hEntryPlugin==-1)
        {
            bFound=true;
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
    dprintf("pluginmenucall(%d)\n", hEntry);
    if(hEntry==-1)
        return;
    for(unsigned int i=0; i<pluginMenuList.size(); i++)
    {
        if(pluginMenuList.at(i).hEntryMenu==hEntry && pluginMenuList.at(i).hEntryPlugin!=-1)
        {
            PLUG_CB_MENUENTRY menuEntryInfo;
            menuEntryInfo.hEntry=pluginMenuList.at(i).hEntryPlugin;
            int pluginCallbackCount=pluginCallbackList.size();
            int pluginHandle=pluginMenuList.at(i).pluginHandle;
            for(int j=0; j<pluginCallbackCount; j++)
            {
                if(pluginCallbackList.at(j).pluginHandle==pluginHandle and pluginCallbackList.at(j).cbType==CB_MENUENTRY)
                {
                    //TODO: handle exceptions
                    pluginCallbackList.at(j).cbPlugin(CB_MENUENTRY, &menuEntryInfo);
                    return;
                }
            }
        }
    }
}