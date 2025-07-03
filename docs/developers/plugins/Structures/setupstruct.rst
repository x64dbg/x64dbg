================
PLUG_SETUPSTRUCT
================
This structure is used by the function that allows the creation of plugin menu entries:

::

    struct PLUG_SETUPSTRUCT 
    {
        //data provided by the debugger to the plugin. 
        [IN] HWND hwndDlg; //GUI window handle
        [IN] int hMenu; //plugin menu handle
        [IN] int hMenuDisasm; //plugin disasm menu handle
        [IN] int hMenuDump; //plugin dump menu handle
        [IN] int hMenuStack; //plugin stack menu handle
    };
