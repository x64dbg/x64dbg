PLUG_CB_EXITTHREAD
==================
Called after thread termination (in the debug loop), before the thread is removed from the internal thread list, before breaking on thread termination:

::

    struct PLUG_CB_EXITTHREAD 
    {
        EXIT_THREAD_DEBUG_INFO* ExitThread;
        DWORD dwThreadId;
    };
