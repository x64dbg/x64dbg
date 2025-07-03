Callback Structures
===================
This section describes the various plugin SDK callback structures.

These structures are used inside event callbacks (registered using _plugin_registercallback). 

Notice that the pointer 'void* callbackInfo' is never NULL, but the members of the various structures can be NULL. 

Also remember that you cannot use any of the provided pointers out of the callback function scope. 

In general AVOID time-consuming operations inside callbacks, do these in separate threads.

You can register a callback with :doc:`../API/registercallback` function. The type definition for the callbacks is:

::

    void CBPLUGIN(
    CBTYPE bType //event type (useful when you use the same function for multiple events
    void* callbackInfo //pointer to a structure of information (see below)
    ); 

**Contents:**

.. toctree::
   :maxdepth: 1
   
   plugcbinitdebug
   plugcbstopdebug
   plugcbcreateprocess
   plugcbexitprocess
   plugcbcreatethread
   plugcbexitthread
   plugcbsystembreakpoint
   plugcbloaddll
   plugcbunloaddll
   plugcboutputdebugstring
   plugcbexception
   plugcbbreakpoint
   plugcbpausedebug
   plugcbresumedebug
   plugcbstepped
   plugcbattach
   plugcbdetach
   plugcbdebugevent
   plugcbmenuentry
   plugcbwinevent
   plugcbwineventglobal
   plugcbloadsavedb
   plugcbfiltersymbol
   plugcbtraceexecute
