_plugin_menuentryseticon
========================
This function sets an icon to a menu entry.

::

    void _plugin_menuentryseticon(
        int pluginHandle, //plugin handle
        int hEntry, //handle of the menu entry
        const ICONDATA* icon //icon data
    ); 

Parameters
----------
:pluginHandle: Handle of the calling plugin.
:hEntry: Menu handle from a previously-added child menu or from the main plugin menu.
:icon: Icon data. See `bridgemain.h` for a definition.

Return Values
-------------
This function does not return a value.
