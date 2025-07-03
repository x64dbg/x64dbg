PLUG_CB_UNLOADDLL
=================
Called on DLL unloading (in the debug loop), before removing the DLL from the internal library list, before breaking on DLL unloading:

::

    struct PLUG_CB_UNLOADDLL 
    {
        UNLOAD_DLL_DEBUG_INFO* UnloadDll;
    };
