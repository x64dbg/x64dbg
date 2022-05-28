/**
 @file _plugins.cpp

 @brief Implements the plugins class.
 */

#include "_plugins.h"
#include "plugin_loader.h"
#include "console.h"
#include "debugger.h"
#include "threading.h"
#include "murmurhash.h"

///debugger plugin exports (wrappers)
PLUG_IMPEXP void _plugin_registercallback(int pluginHandle, CBTYPE cbType, CBPLUGIN cbPlugin)
{
    pluginregistercallback(pluginHandle, cbType, cbPlugin);
}

PLUG_IMPEXP bool _plugin_unregistercallback(int pluginHandle, CBTYPE cbType)
{
    return pluginunregistercallback(pluginHandle, cbType);
}

PLUG_IMPEXP bool _plugin_registercommand(int pluginHandle, const char* command, CBPLUGINCOMMAND cbCommand, bool debugonly)
{
    return plugincmdregister(pluginHandle, command, cbCommand, debugonly);
}

PLUG_IMPEXP bool _plugin_unregistercommand(int pluginHandle, const char* command)
{
    return plugincmdunregister(pluginHandle, command);
}

PLUG_IMPEXP void _plugin_logprintf(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    dprintf_args_untranslated(format, args);
    va_end(args);
}

PLUG_IMPEXP void _plugin_logprintf_html(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    dprintf_args_untranslated_html(format, args);
    va_end(args);
}

PLUG_IMPEXP void _plugin_logputs(const char* text)
{
    dputs_untranslated(text);
}

PLUG_IMPEXP void _plugin_logprint(const char* text)
{
    dprintf_untranslated("%s", text);
}

PLUG_IMPEXP void _plugin_debugpause()
{
    DebugUpdateGuiSetStateAsync(GetContextDataEx(hActiveThread, UE_CIP), true);
    lock(WAITID_RUN);
    dbgsetforeground();
    dbgsetskipexceptions(false);
    // Plugin callback
    PLUG_CB_PAUSEDEBUG pauseInfo = { nullptr };
    plugincbcall(CB_PAUSEDEBUG, &pauseInfo);
    wait(WAITID_RUN);
}

PLUG_IMPEXP void _plugin_debugskipexceptions(bool skip)
{
    dbgsetskipexceptions(skip);
}

PLUG_IMPEXP int _plugin_menuadd(int hMenu, const char* title)
{
    return pluginmenuadd(hMenu, title);
}

PLUG_IMPEXP bool _plugin_menuaddentry(int hMenu, int hEntry, const char* title)
{
    return pluginmenuaddentry(hMenu, hEntry, title);
}

PLUG_IMPEXP bool _plugin_menuaddseparator(int hMenu)
{
    return pluginmenuaddseparator(hMenu);
}

PLUG_IMPEXP bool _plugin_menuclear(int hMenu)
{
    return pluginmenuclear(hMenu, false);
}

PLUG_IMPEXP void _plugin_menuseticon(int hMenu, const ICONDATA* icon)
{
    pluginmenuseticon(hMenu, icon);
}

PLUG_IMPEXP void _plugin_menuentryseticon(int pluginHandle, int hEntry, const ICONDATA* icon)
{
    pluginmenuentryseticon(pluginHandle, hEntry, icon);
}

PLUG_IMPEXP void _plugin_menuentrysetchecked(int pluginHandle, int hEntry, bool checked)
{
    pluginmenuentrysetchecked(pluginHandle, hEntry, checked);
}

PLUG_IMPEXP void _plugin_menusetvisible(int pluginHandle, int hMenu, bool visible)
{
    pluginmenusetvisible(pluginHandle, hMenu, visible);
}

PLUG_IMPEXP void _plugin_menuentrysetvisible(int pluginHandle, int hEntry, bool visible)
{
    pluginmenuentrysetvisible(pluginHandle, hEntry, visible);
}

PLUG_IMPEXP void _plugin_menusetname(int pluginHandle, int hMenu, const char* name)
{
    pluginmenusetname(pluginHandle, hMenu, name);
}

PLUG_IMPEXP void _plugin_menuentrysetname(int pluginHandle, int hEntry, const char* name)
{
    pluginmenuentrysetname(pluginHandle, hEntry, name);
}

PLUG_IMPEXP void _plugin_menuentrysethotkey(int pluginHandle, int hEntry, const char* hotkey)
{
    pluginmenuentrysethotkey(pluginHandle, hEntry, hotkey);
}

PLUG_IMPEXP bool _plugin_menuremove(int hMenu)
{
    return pluginmenuremove(hMenu);
}

PLUG_IMPEXP bool _plugin_menuentryremove(int pluginHandle, int hEntry)
{
    return pluginmenuentryremove(pluginHandle, hEntry);
}

PLUG_IMPEXP void _plugin_startscript(CBPLUGINSCRIPT cbScript)
{
    dbgstartscriptthread(cbScript);
}

PLUG_IMPEXP bool _plugin_waituntilpaused()
{
    while(DbgIsDebugging() && dbgisrunning()) //wait until the debugger paused
    {
        Sleep(1);
        GuiProcessEvents(); //workaround for scripts being executed on the GUI thread
    }
    return DbgIsDebugging();
}

bool _plugin_registerexprfunction(int pluginHandle, const char* name, int argc, CBPLUGINEXPRFUNCTION cbFunction, void* userdata)
{
    return pluginexprfuncregister(pluginHandle, name, argc, cbFunction, userdata);
}

bool _plugin_registerexprfunctionex(int pluginHandle, const char* name, const ValueType & returnType, const ValueType* argTypes, size_t argCount, CBPLUGINEXPRFUNCTIONEX cbFunction, void* userdata)
{
    return pluginexprfuncregisterex(pluginHandle, name, returnType, argTypes, argCount, cbFunction, userdata);
}

bool _plugin_unregisterexprfunction(int pluginHandle, const char* name)
{
    return pluginexprfuncunregister(pluginHandle, name);
}

PLUG_IMPEXP bool _plugin_unload(const char* pluginName)
{
    return pluginunload(pluginName);
}

PLUG_IMPEXP bool _plugin_load(const char* pluginName)
{
    return pluginload(pluginName);
}

duint _plugin_hash(const void* data, duint size)
{
    return murmurhash(data, int(size));
}

PLUG_IMPEXP bool _plugin_registerformatfunction(int pluginHandle, const char* type, CBPLUGINFORMATFUNCTION cbFunction, void* userdata)
{
    return pluginformatfuncregister(pluginHandle, type, cbFunction, userdata);
}

PLUG_IMPEXP bool _plugin_unregisterformatfunction(int pluginHandle, const char* type)
{
    return pluginformatfuncunregister(pluginHandle, type);
}
