PLUG_CB_CREATETHREAD
====================
Called after thread creation (in the debug loop), after adding the thread to the internal thread list, before breaking the debugger on thread creation and after setting breakpoints on the thread entry:

::

    struct PLUG_CB_CREATETHREAD 
    {
        CREATE_THREAD_DEBUG_INFO* CreateThread;
        DWORD dwThreadId;
    };
