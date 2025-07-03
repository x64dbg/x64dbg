==========================
_plugin_unregistercallback
==========================
This plugin unregisters a previously set callback. It is only possible to remove callbacks that were previously set using _plugin_registercallback.

::

    bool _plugin_unregistercallback( 
        int pluginHandle, //plugin handle
        CBTYPE cbType //callback type to remove
    ); 

Parameters 
----------

:pluginHandle: Handle of the calling plugin. 
:cbType: The event type. This can be any of the following values:

*    CB_INITDEBUG, 
*    CB_STOPDEBUG,
*    CB_CREATEPROCESS, 
*    CB_EXITPROCESS,
*    CB_CREATETHREAD,
*    CB_EXITTHREAD,
*    CB_SYSTEMBREAKPOINT,
*    CB_LOADDLL,
*    CB_UNLOADDLL, 
*    CB_OUTPUTDEBUGSTRING,
*    CB_EXCEPTION, 
*    CB_BREAKPOINT,
*    CB_PAUSEDEBUG, 
*    CB_RESUMEDEBUG,
*    CB_STEPPED,
*    CB_ATTACH,
*    CB_DETACH,
*    CB_DEBUGEVENT,
*    CB_MENUENTRY,
*    CB_WINEVENT,
*    CB_WINEVENTGLOBAL

Return Values
-------------
This function returns true when the callback was removed without problems.
