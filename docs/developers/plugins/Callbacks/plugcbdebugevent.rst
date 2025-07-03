PLUG_CB_DEBUGEVENT
==================
Called on any debug event, even the ones that are handled internally. 

Avoid doing stuff that takes time here, this will slow the debugger down a lot!:

::

    struct PLUG_CB_DEBUGEVENT
    {
        DEBUG_EVENT* DebugEvent;
    };
