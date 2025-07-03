=========================
_plugin_unregistercommand
=========================
This function removes a command set by a plugin. It is only possible to remove commands that you previously registered using `_plugin_registercommand`.

::

    bool _plugin_unregistercommand( 
        int pluginHandle, //plugin handle
        const char* command //command name
    ); 

----------
Parameters
----------

:pluginHandle: Handle of the calling plugin. 
:command: Command name. 

-------------
Return Values
-------------
This function returns true when the callback was removed without problems.

