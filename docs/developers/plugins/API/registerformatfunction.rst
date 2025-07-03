==============================
_plugin_registerformatfunction
==============================
This function registers a format function defined by a plugin, so that users can use it in :doc:`../../../introduction/Formatting`.

::

    bool _plugin_registerformatfunction(
        int pluginHandle,                  //plugin handle
        const char* type,                  //the name of format function
        CBPLUGINFORMATFUNCTION cbFunction, //callback function
        void* userdata                     //user data
    ); 

----------
Parameters
----------

:pluginHandle: Handle of the calling plugin. 
:type: The name of format function. This is the part before `;` in a string format function. Valid names must begin with `_` or letters, followed by `_`, `.`, letters or digits.
:cbFunction: Callback with the following typdef:

::

    typedef FORMATRESULT(*CBPLUGINFORMATFUNCTION)(char* dest, size_t destCount, int argc, char* argv[], duint value, void* userdata);

:userdata: A pointer value passed to the callback, may be used by plugin to pass additional information.

-------------
Return Values
-------------
Return `true` when the registration is successful, otherwise return `false`.
