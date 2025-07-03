==================
_plugin_debugpause
==================
This function returns debugger control to the user. You would use this function when you write an unpacker that needs support from x64dbg (for example in development). Calling this function will set the debug state to 'paused' and it will not return until the user runs the debuggee using the `run` command .

::

    void _plugin_debugpause();

----------
Parameters 
----------
This function has no parameters.

-------------
Return Values 
-------------
This function does not return a value. 
