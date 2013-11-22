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
