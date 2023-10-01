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
    int hParentMenu; //parent GUI menu handle
};

struct PLUG_MENUENTRY : PLUG_MENU
{
    int hEntryPlugin; //plugin entry handle (unique per plugin)
};

struct PLUG_DATA
{
    char plugpath[MAX_PATH];
    char plugname[MAX_PATH];
    HINSTANCE hPlugin;
    PLUGINIT pluginit;
    PLUGSTOP plugstop;
    PLUGSETUP plugsetup;
    int hMenu;
    int hMenuDisasm;
    int hMenuDump;
    int hMenuStack;
    int hMenuGraph;
    int hMenuMemmap;
    int hMenuSymmod;
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

struct PLUG_EXPRFUNCTION
{
    int pluginHandle;
    char name[deflen];
};

struct PLUG_FORMATFUNCTION
{
    int pluginHandle;
    char name[deflen];
};

//plugin management functions
bool pluginload(const char* pluginname);
bool pluginunload(const char* pluginname, bool unloadall = false);
void pluginloadall(const char* pluginDir);
void pluginunloadall();
void plugincmdunregisterall(int pluginHandle);
void pluginexprfuncunregisterall(int pluginHandle);
void pluginformatfuncunregisterall(int pluginHandle);
void pluginregistercallback(int pluginHandle, CBTYPE cbType, CBPLUGIN cbPlugin);
bool pluginunregistercallback(int pluginHandle, CBTYPE cbType);
void plugincbcall(CBTYPE cbType, void* callbackInfo);
bool plugincbempty(CBTYPE cbType);
bool plugincmdregister(int pluginHandle, const char* command, CBPLUGINCOMMAND cbCommand, bool debugonly);
bool plugincmdunregister(int pluginHandle, const char* command);
int pluginmenuadd(int hMenu, const char* title);
bool pluginmenuaddentry(int hMenu, int hEntry, const char* title);
bool pluginmenuaddseparator(int hMenu);
bool pluginmenuclear(int hMenu, bool erase);
void pluginmenucall(int hEntry);
bool pluginwinevent(MSG* message, long* result);
bool pluginwineventglobal(MSG* message);
void pluginmenuseticon(int hMenu, const ICONDATA* icon);
void pluginmenuentryseticon(int pluginHandle, int hEntry, const ICONDATA* icon);
void pluginmenuentrysetchecked(int pluginHandle, int hEntry, bool checked);
void pluginmenusetvisible(int pluginHandle, int hMenu, bool visible);
void pluginmenuentrysetvisible(int pluginHandle, int hEntry, bool visible);
void pluginmenusetname(int pluginHandle, int hMenu, const char* name);
void pluginmenuentrysetname(int pluginHandle, int hEntry, const char* name);
void pluginmenuentrysethotkey(int pluginHandle, int hEntry, const char* hotkey);
bool pluginmenuremove(int hMenu);
bool pluginmenuentryremove(int pluginHandle, int hEntry);
bool pluginexprfuncregister(int pluginHandle, const char* name, int argc, CBPLUGINEXPRFUNCTION cbFunction, void* userdata);
bool pluginexprfuncregisterex(int pluginHandle, const char* name, const ValueType & returnType, const ValueType* argTypes, size_t argCount, CBPLUGINEXPRFUNCTIONEX cbFunction, void* userdata);
bool pluginexprfuncunregister(int pluginHandle, const char* name);
bool pluginformatfuncregister(int pluginHandle, const char* type, CBPLUGINFORMATFUNCTION cbFunction, void* userdata);
bool pluginformatfuncunregister(int pluginHandle, const char* type);

#endif // _PLUGIN_LOADER_H
