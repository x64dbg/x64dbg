PLUG_CB_PAUSEDEBUG
==================
Called after the debugger has been locked to pause (in the debug loop), before any other callback that's before pausing the debugger:

::

    struct PLUG_CB_PAUSEDEBUG
    {
        void* reserved;
    };
