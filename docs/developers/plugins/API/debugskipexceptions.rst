===========================
_plugin_debugskipexceptions
===========================
This function returns sets if the debugger should skip first-chance exceptions. This is useful when creating unpackers or other plugins that run the debuggee.

::

   void _plugin_debugskipexceptions(
      bool skip //skip flag
   );

----------
Parameters
----------

:skip: Flag if we need to skip first-chance exceptions or not.

-------------
Return Values
-------------
This function does not return a value. 
