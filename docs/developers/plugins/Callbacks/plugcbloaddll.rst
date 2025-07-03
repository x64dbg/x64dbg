PLUG_CB_LOADDLL
===============
Called on DLL loading (in the debug loop), after the DLL has been added to the internal library list, after setting the DLL entry breakpoint:

::

    struct PLUG_CB_LOADDLL 
    {
        LOAD_DLL_DEBUG_INFO* LoadDll;
        IMAGEHLP_MODULE64* modInfo;
        const char* modname;
    };
