
=======================
_plugin_registercommand
=======================
This function registers a command for usage inside scripts or the command bar.

::

    bool _plugin_registercommand( 
        int pluginHandle, //plugin handle
        const char* command, //command name
        CBPLUGINCOMMAND cbCommand, //function that is called when the command is executed
        bool debugonly //restrict the command to debug-only
    );

----------
Parameters
----------

:pluginHandle: Handle of the calling plugin.

:command: Command name.

:cbCommand: Callback with the following typedef:

::

    bool CBPLUGINCOMMAND(
    int argc //argument count (number of arguments + 1)
    char* argv[] //array of arguments (argv[0] is the full command, arguments start at argv[1])
    ); 

:debugonly: When set, the command will never be executed when there is no target is being debugged. 

-------------
Return Values
-------------
This function returns true when the command was successfully registered, make sure to check this, other plugins may have already registered the same command.
