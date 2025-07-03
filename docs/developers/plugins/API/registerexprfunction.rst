============================
_plugin_registerexprfunction
============================
This function registers an expression function defined by a plugin, so that users can use it in expressions. All arguments are integer type.

::

    bool _plugin_registerexprfunction(
        int pluginHandle,                //plugin handle
        const char* name,                //name of expresison function
        int argc,                        //number of arguments
        CBPLUGINEXPRFUNCTION cbFunction, //callback function
        void* userdata                   //user data
    ); 

----------
Parameters
----------

:pluginHandle: Handle of the calling plugin. 
:name: Name of expresison function.
:argc: Number of arguments of expression function.
:cbFunction: Callback with the following typdef:

::

    typedef duint(*CBPLUGINEXPRFUNCTION)(int argc, const duint* argv, void* userdata);

:userdata: A pointer value passed to the callback, may be used by plugin to pass additional information.

-------------
Return Values
-------------
Return true when the registration is successful, otherwise return false.
