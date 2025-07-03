=================
_plugin_logprintf
=================

This function prints a message to the log window.

::

    void _plugin_logprintf(
        const char* format, //format string
        ... //additional arguments
    ); 

----------
Parameters
----------

:format: Format string that has the same specifications as printf. 
:...: Additional arguments (when needed by the format string). 

-------------
Return Values
-------------
This function does not return a value. 
