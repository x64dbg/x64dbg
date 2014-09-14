/**
 @file _plugins.cpp

 @brief Implements the plugins class.
 */

#include "_plugins.h"
#include "plugin_loader.h"
#include "console.h"
#include "debugger.h"
#include "threading.h"

/**
 @brief The msg[ 66000].
 */

static char msg[66000];

/**
 @fn PLUG_IMPEXP void _plugin_registercallback(int pluginHandle, CBTYPE cbType, CBPLUGIN cbPlugin)

 @brief debugger plugin exports (wrappers)

 @param pluginHandle Handle of the plugin.
 @param cbType       The type.
 @param cbPlugin     The plugin.
 */

PLUG_IMPEXP void _plugin_registercallback(int pluginHandle, CBTYPE cbType, CBPLUGIN cbPlugin)
{
    pluginregistercallback(pluginHandle, cbType, cbPlugin);
}

/**
 @fn PLUG_IMPEXP bool _plugin_unregistercallback(int pluginHandle, CBTYPE cbType)

 @brief Unregistercallback, called when the plugin unregister.

 @param pluginHandle Handle of the plugin.
 @param cbType       The type.

 @return true if it succeeds, false if it fails.
 */

PLUG_IMPEXP bool _plugin_unregistercallback(int pluginHandle, CBTYPE cbType)
{
    return pluginunregistercallback(pluginHandle, cbType);
}

/**
 @fn PLUG_IMPEXP bool _plugin_registercommand(int pluginHandle, const char* command, CBPLUGINCOMMAND cbCommand, bool debugonly)

 @brief Plugin registercommand.

 @param pluginHandle Handle of the plugin.
 @param command      The command.
 @param cbCommand    The command.
 @param debugonly    true to debugonly.

 @return true if it succeeds, false if it fails.
 */

PLUG_IMPEXP bool _plugin_registercommand(int pluginHandle, const char* command, CBPLUGINCOMMAND cbCommand, bool debugonly)
{
    return plugincmdregister(pluginHandle, command, cbCommand, debugonly);
}

/**
 @fn PLUG_IMPEXP bool _plugin_unregistercommand(int pluginHandle, const char* command)

 @brief Plugin unregistercommand.

 @param pluginHandle Handle of the plugin.
 @param command      The command.

 @return true if it succeeds, false if it fails.
 */

PLUG_IMPEXP bool _plugin_unregistercommand(int pluginHandle, const char* command)
{
    return plugincmdunregister(pluginHandle, command);
}

/**
 @fn PLUG_IMPEXP void _plugin_logprintf(const char* format, ...)

 @brief Plugin logprintf.

 @param format Describes the format to use.
 */

PLUG_IMPEXP void _plugin_logprintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vsprintf(msg, format, args);
    GuiAddLogMessage(msg);
}

/**
 @fn PLUG_IMPEXP void _plugin_logputs(const char* text)

 @brief Plugin logputs.

 @param text The text.
 */

PLUG_IMPEXP void _plugin_logputs(const char* text)
{
    dputs(text);
}

/**
 @fn PLUG_IMPEXP void _plugin_debugpause()

 @brief Plugin debugpause.
 */

PLUG_IMPEXP void _plugin_debugpause()
{
    GuiSetDebugState(paused);
    DebugUpdateGui(GetContextDataEx(hActiveThread, UE_CIP), true);
    lock(WAITID_RUN);
    SetForegroundWindow(GuiGetWindowHandle());
    dbgsetskipexceptions(false);
    wait(WAITID_RUN);
}

/**
 @fn PLUG_IMPEXP void _plugin_debugskipexceptions(bool skip)

 @brief Plugin debugskipexceptions.

 @param skip true to skip.
 */

PLUG_IMPEXP void _plugin_debugskipexceptions(bool skip)
{
    dbgsetskipexceptions(skip);
}

/**
 @fn PLUG_IMPEXP int _plugin_menuadd(int hMenu, const char* title)

 @brief Plugin menuadd.

 @param hMenu The menu.
 @param title The title.

 @return An int.
 */

PLUG_IMPEXP int _plugin_menuadd(int hMenu, const char* title)
{
    return pluginmenuadd(hMenu, title);
}

/**
 @fn PLUG_IMPEXP bool _plugin_menuaddentry(int hMenu, int hEntry, const char* title)

 @brief Plugin menuaddentry.

 @param hMenu  The menu.
 @param hEntry The entry.
 @param title  The title.

 @return true if it succeeds, false if it fails.
 */

PLUG_IMPEXP bool _plugin_menuaddentry(int hMenu, int hEntry, const char* title)
{
    return pluginmenuaddentry(hMenu, hEntry, title);
}

/**
 @fn PLUG_IMPEXP bool _plugin_menuaddseparator(int hMenu)

 @brief Plugin menuaddseparator.

 @param hMenu The menu.

 @return true if it succeeds, false if it fails.
 */

PLUG_IMPEXP bool _plugin_menuaddseparator(int hMenu)
{
    return pluginmenuaddseparator(hMenu);
}

/**
 @fn PLUG_IMPEXP bool _plugin_menuclear(int hMenu)

 @brief Plugin menuclear.

 @param hMenu The menu.

 @return true if it succeeds, false if it fails.
 */

PLUG_IMPEXP bool _plugin_menuclear(int hMenu)
{
    return pluginmenuclear(hMenu);
}
