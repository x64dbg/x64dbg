PLUG_CB_OUTPUTDEBUGSTRING
=========================
Called on a DebugString event (in the debug loop), before dumping the string to the log, before breaking on a debug string:

::

    struct PLUG_CB_OUTPUTDEBUGSTRING 
    {
        OUTPUT_DEBUG_STRING_INFO* DebugString;
    };
