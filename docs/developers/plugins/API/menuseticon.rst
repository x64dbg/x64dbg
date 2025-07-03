_plugin_menuseticon
===================
This function sets an icon to a menu.

::

    void _plugin_menuseticon(
        int hMenu, //handle of the menu
        const ICONDATA* icon //icon data
    ); 

Parameters
----------
:hMenu: Menu handle from a previously-added child menu or from the main plugin menu.
:icon: Icon data. See bridgemain.h for a definition.

Return Values
-------------
This function does not return a value.
