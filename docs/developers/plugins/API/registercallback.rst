========================
_plugin_registercallback
========================
This function registers an event callback for a plugin. Every plugin can have it's own callbacks for every event. It is not possible to have multiple callbacks on the same event.

::

    void _plugin_registercallback(
        int pluginHandle, //plugin handle
        CBTYPE cbType, //event type
        CBPLUGIN cbPlugin //callback function
    ); 

----------
Parameters
----------

:pluginHandle: Handle of the calling plugin. 
:cbType: The event type. This can be any of the following values:

    :CB_INITDEBUG:  , //callbackInfo\: PLUG_CB_INITDEBUG*
    :CB_STOPDEBUG:  , //callbackInfo\: PLUG_CB_STOPDEBUG*
    :CB_CREATEPROCESS:  , //callbackInfo\: PLUG_CB_CREATEPROCESS*
    :CB_EXITPROCESS:  , //callbackInfo\: PLUG_CB_EXITPROCESS*
    :CB_CREATETHREAD:  , //callbackInfo\: PLUG_CB_CREATETHREAD*
    :CB_EXITTHREAD:  , //callbackInfo\: PLUG_CB_EXITTHREAD*
    :CB_SYSTEMBREAKPOINT:  , //callbackInfo\: PLUG_CB_SYSTEMBREAKPOINT*
    :CB_LOADDLL:  , //callbackInfo\: PLUG_CB_LOADDLL*
    :CB_UNLOADDLL:  , //callbackInfo\: PLUG_CB_UNLOADDLL*
    :CB_OUTPUTDEBUGSTRING:  , //callbackInfo\: PLUG_CB_OUTPUTDEBUGSTRING*
    :CB_EXCEPTION:  , //callbackInfo\: PLUG_CB_EXCEPTION*
    :CB_BREAKPOINT:  , //callbackInfo\: PLUG_CB_BREAKPOINT*
    :CB_PAUSEDEBUG:  , //callbackInfo\: PLUG_CB_PAUSEDEBUG*
    :CB_RESUMEDEBUG:  , //callbackInfo\: PLUG_CB_RESUMEDEBUG*
    :CB_STEPPED:  , //callbackInfo\: PLUG_CB_STEPPED* 
    :CB_ATTACH:  , //callbackInfo\: PLUG_CB_ATTACHED*
    :CB_DETACH:  , //callbackInfo\: PLUG_CB_DETACHED*
    :CB_DEBUGEVENT:  , //callbackInfo\: PLUG_CB_DEBUGEVENT*
    :CB_MENUENTRY:  , //callbackInfo\: PLUG_CB_MENUENTRY*
    :CB_WINEVENT:  , //callbackInfo\: PLUG_CB_WINEVENT* 
    :CB_WINEVENTGLOBAL:  , //callbackInfo\: PLUG_CB_WINEVENTGLOBAL*
    :CB_LOADDB:  , //callbackInfo\: PLUG_CB_LOADSAVEDB*
    :CB_SAVEDB:  , //callbackInfo\: PLUG_CB_LOADSAVEDB*
    :CB_FILTERSYMBOL:  , //callbackInfo\: PLUG_CB_FILTERSYMBOL*
    :CB_TRACEEXECUTE:  , //callbackInfo\: PLUG_CB_TRACEEXECUTE*

:cbPlugin: Callback with the following typdef:

::

    void CBPLUGIN(
    CBTYPE bType //event type (useful when you use the same function for multiple events
    void* callbackInfo //pointer to a structure of information (see above)
    ); 

-------------
Return Values
-------------
This function does not return a value. 
