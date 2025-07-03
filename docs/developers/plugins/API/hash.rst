============
_plugin_hash
============
This function allows you to hash some data. It is used by x64dbg in various places.

::

    duint _plugin_hash(
        const void* data, //data to hash
        duint size //size (in bytes) of the data to hash
    );

----------
Parameters 
----------
:data: Data to hash
:size: Size (in bytes) of the data to hash.

-------------
Return Values 
-------------
Returns the hash.
