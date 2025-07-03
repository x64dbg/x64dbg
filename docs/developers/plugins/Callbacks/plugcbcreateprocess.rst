PLUG_CB_CREATEPROCESS
=====================
Called after process creation (in the debug loop), after the initialization of the symbol handler, the database file and setting breakpoints on TLS callbacks / the entry breakpoint:

::

    struct PLUG_CB_CREATEPROCESS 
    {
        CREATE_PROCESS_DEBUG_INFO* CreateProcessInfo;
        IMAGEHLP_MODULE64* modInfo;
        const char* DebugFileName;
        PROCESS_INFORMATION* fdProcessInfo;
    };
