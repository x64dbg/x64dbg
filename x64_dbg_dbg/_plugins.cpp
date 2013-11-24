#include "_plugins.h"
#include "plugin_loader.h"
#include "console.h"
#include "debugger.h"
#include "threading.h"

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
    char msg[deflen]="";
    vsprintf(msg, format, args);
    GuiAddLogMessage(msg);
}

PLUG_IMPEXP void _plugin_logputs(const char* text)
{
    dputs(text);
}

PLUG_IMPEXP void _plugin_debugpause()
{
    DebugUpdateGui(GetContextData(UE_CIP));
    GuiSetDebugState(paused);
    lock(WAITID_RUN);
    wait(WAITID_RUN);
}
