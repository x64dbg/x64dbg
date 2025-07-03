PLUG_CB_EXCEPTION
=================
Called on an unhandled (by the debugger) exception (in the debug loop), after setting the continue status, after locking the debugger to pause:

::

    struct PLUG_CB_EXCEPTION 
    {
        EXCEPTION_DEBUG_INFO* Exception;
    };
