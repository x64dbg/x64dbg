PLUG_CB_WINEVENT
================
Called before TranslateMessage and DispatchMessage Windows functions (PreTranslateMessage). 

Avoid calling user32 functions without precautions here, there will be a recursive call if you fail to take countermeasures:

::

    struct PLUG_CB_WINEVENT
    {
        MSG* message;
        long* result;
        bool retval; //only set this to true if you want Qt to ignore the event.
    };
