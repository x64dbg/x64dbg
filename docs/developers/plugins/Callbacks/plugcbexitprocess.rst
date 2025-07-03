PLUG_CB_EXITPROCESS
===================
Called after the process exits (in the debug loop), before the symbol handler is cleaned up:

::

    struct PLUG_CB_EXITPROCESS 
    {
        EXIT_PROCESS_DEBUG_INFO* ExitProcess;
    };
