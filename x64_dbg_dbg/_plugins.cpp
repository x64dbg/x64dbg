#include "_plugins.h"
#include "plugin_loader.h"
#include "console.h"
#include "debugger.h"
#include "threading.h"

static char msg[66000];

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
    vsprintf(msg, format, args);
    GuiAddLogMessage(msg);
}

PLUG_IMPEXP void _plugin_logputs(const char* text)
{
    dputs(text);
}

PLUG_IMPEXP void _plugin_debugpause()
{
    GuiSetDebugState(paused);
    DebugUpdateGui(GetContextDataEx(hActiveThread, UE_CIP), true);
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    dbgsetskipexceptions(false);
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
    return pluginmenuclear(hMenu);
}
