================================
_plugin_unregisterformatfunction
================================
This function removes a string format function registered by a plugin. It is only possible to remove commands that you previously registered using `_plugin_registerformatfunction`.

::

    bool _plugin_unregisterformatfunction( 
        int pluginHandle, //plugin handle
        const char* type  //string format function name
    ); 

----------
Parameters
----------

:pluginHandle: Handle of the calling plugin. 
:type: String format function name.

-------------
Return Values
-------------
This function returns `true` when the string format function was removed without problems.

