#ifndef _PLUGIN_LOADER_H
#define _PLUGIN_LOADER_H

#include "_global.h"
#include "_plugins.h"

//typedefs
typedef bool (*PLUGINIT)(PLUG_INITSTRUCT* initStruct);
typedef bool (*PLUGSTOP)();
typedef void (*PLUGSETUP)(PLUG_SETUPSTRUCT* setupStruct);

//structures
struct PLUG_MENU
{
    int pluginHandle; //plugin handle
    int hEntryMenu; //GUI entry/menu handle (unique)
    int hEntryPlugin; //plugin entry handle (unique per plugin)
};

struct PLUG_DATA
{
    HINSTANCE hPlugin;
    PLUGINIT pluginit;
    PLUGSTOP plugstop;
    PLUGSETUP plugsetup;
    int hMenu;
    PLUG_INITSTRUCT initStruct;
};

struct PLUG_CALLBACK
{
    int pluginHandle;
    CBTYPE cbType;
    CBPLUGIN cbPlugin;
};

struct PLUG_COMMAND
{
    int pluginHandle;
    char command[deflen];
};

//plugin management functions
void pluginload(const char* pluginDir);
void pluginunload();
void pluginregistercallback(int pluginHandle, CBTYPE cbType, CBPLUGIN cbPlugin);
bool pluginunregistercallback(int pluginHandle, CBTYPE cbType);
void plugincbcall(CBTYPE cbType, void* callbackInfo);
bool plugincmdregister(int pluginHandle, const char* command, CBPLUGINCOMMAND cbCommand, bool debugonly);
bool plugincmdunregister(int pluginHandle, const char* command);
int pluginmenuadd(int hMenu, const char* title);
bool pluginmenuaddentry(int hMenu, int hEntry, const char* title);
bool pluginmenuaddseparator(int hMenu);
bool pluginmenuclear(int hMenu);
void pluginmenucall(int hEntry);
bool pluginwinevent(MSG* message, long* result);
bool pluginwineventglobal(MSG* message);

#endif // _PLUGIN_LOADER_H
