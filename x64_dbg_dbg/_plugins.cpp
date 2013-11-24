#include "_plugins.h"
#include "plugin_loader.h"

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
