==============================
_plugin_unregisterexprfunction
==============================
This function removes an expression function registered by a plugin. It is only possible to remove commands that you previously registered using `_plugin_registerexprfunction` or `_plugin_registerexprfunctionex`.

::

    bool _plugin_unregisterexprfunction( 
        int pluginHandle, //plugin handle
        const char* name //expression function name
    ); 

----------
Parameters
----------

:pluginHandle: Handle of the calling plugin. 
:name: Expression function name.

-------------
Return Values
-------------
This function returns true when the callback was removed without problems.

