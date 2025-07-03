PLUG_CB_SYSTEMBREAKPOINT
========================
Called at the system breakpoint (in the debug loop), after setting the initial dump location, before breaking the debugger on the system breakpoint:

::

    struct PLUG_CB_SYSTEMBREAKPOINT 
    {
        void* reserved;
    };
