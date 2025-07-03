PLUG_CB_BREAKPOINT
==================
Called on a normal/memory/hardware breakpoint (in the debug loop), after locking the debugger to pause:

::

    struct PLUG_CB_BREAKPOINT
    {
        BRIDGEBP* breakpoint;
    };
