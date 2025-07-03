PLUG_CB_STEPPED
===============
Called after the debugger stepped (in the debug loop), after locking the debugger to pause:

::

    struct PLUG_CB_STEPPED
    {
        void* reserved;
    };
