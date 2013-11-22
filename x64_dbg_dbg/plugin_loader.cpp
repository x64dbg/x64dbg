#include "plugin_loader.h"
#include "console.h"

static std::vector<PLUG_DATA> pluginList;
static int curPluginHandle=0;
static std::vector<PLUG_CALLBACK> pluginCallbackList;

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
    char errorMsg[deflen]="";
    do
    {
        memset(&pluginData, 0, sizeof(PLUG_DATA));
        pluginData.initStruct.pluginHandle=curPluginHandle;
        pluginData.hPlugin=LoadLibraryA(foundData.cFileName); //load the plugin library
        if(!pluginData.hPlugin)
        {
            sprintf(errorMsg, "Failed to load plugin: %s", foundData.cFileName);
            MessageBoxA(0, errorMsg, "Plugin Error!", MB_ICONERROR|MB_SYSTEMMODAL);
            continue;
        }
        pluginData.pluginit=(PLUGINIT)GetProcAddress(pluginData.hPlugin, "pluginit");
        if(!pluginData.pluginit)
        {
            sprintf(errorMsg, "Export \"pluginit\" not found in plugin: %s", foundData.cFileName);
            MessageBoxA(0, errorMsg, "Plugin Error!", MB_ICONERROR|MB_SYSTEMMODAL);
            FreeLibrary(pluginData.hPlugin);
            continue;
        }
        pluginData.plugstop=(PLUGSTOP)GetProcAddress(pluginData.hPlugin, "plugstop");
        if(!pluginData.plugstop)
        {
            sprintf(errorMsg, "Export \"plugstop\" not found in plugin: %s", foundData.cFileName);
            MessageBoxA(0, errorMsg, "Plugin Error!", MB_ICONERROR|MB_SYSTEMMODAL);
            FreeLibrary(pluginData.hPlugin);
            continue;
        }
        //TODO: handle exceptions
        if(!pluginData.pluginit(&pluginData.initStruct))
        {
            sprintf(errorMsg, "pluginit failed for plugin: %s", foundData.cFileName);
            MessageBoxA(0, errorMsg, "Plugin Error!", MB_ICONERROR|MB_SYSTEMMODAL);
            FreeLibrary(pluginData.hPlugin);
            continue;
        }
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
        pluginList.at(i).plugstop();
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
