PLUG_INITSTRUCT
===============
This structure is used by the only **needed** export in the plugin interface:

::

    struct PLUG_INITSTRUCT
    {
        //data provided by the debugger to the plugin.
        [IN] int pluginHandle; //handle of the plugin
    
        //data provided by the plugin to the debugger (required).
        [OUT] int sdkVersion; //plugin SDK version, use the PLUG_SDKVERSION define for this
        [OUT] int pluginVersion; //plugin version, useful for crash reports
        [OUT] char pluginName[256]; //plugin name, also useful for crash reports
    };

