===================
_plugin_startscript
===================
Creates a new thread to run the callback function asynchronously.

::

    void _plugin_startscript(
        CBPLUGINSCRIPT cbScript //callback
    );

----------
Parameters 
----------
:cbScript: Callback with the following typedef:

::

    typedef void (*CBPLUGINSCRIPT)();

-------------
Return Values 
-------------
This function does not return a value. 
