#include "plugin_loader.h"
#include "console.h"
#include "command.h"
#include "x64_dbg.h"

static std::vector<PLUG_DATA> pluginList;
static int curPluginHandle=0;
static std::vector<PLUG_CALLBACK> pluginCallbackList;
static std::vector<PLUG_COMMAND> pluginCommandList;

///internal plugin functions
void pluginload(const char* pluginDir)
{
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
        memset(&pluginData, 0, sizeof(PLUG_DATA));
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
        //TODO: handle exceptions
        if(!pluginData.pluginit(&pluginData.initStruct))
        {
            dprintf("[PLUGIN] pluginit failed for plugin: %s\n", foundData.cFileName);
            FreeLibrary(pluginData.hPlugin);
            continue;
        }
        else
            dprintf("[PLUGIN] %s v%d Loaded!\n", pluginData.initStruct.pluginName, pluginData.initStruct.pluginVersion);
        pluginList.push_back(pluginData);
        curPluginHandle++;
    }
    while(FindNextFileA(hSearch, &foundData));
    SetCurrentDirectoryA(currentDir);
}

void pluginunload()
{
    int pluginCount=pluginList.size();
    for(int i=0; i<pluginCount; i++)
    {
        PLUGSTOP stop=pluginList.at(i).plugstop;
        if(stop)
            stop();
        FreeLibrary(pluginList.at(i).hPlugin);
    }
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
            return true;
        }
    }
    return false;
}
