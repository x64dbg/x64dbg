_plugin_menuclear
=================
This function removes all entries and child menus from a menu. It will not remove the menu itself.

::

    bool  _plugin_menuclear( 
        int hMenu //menu handle of the menu to clear  
    );

Parameters
----------
:hMenu: Menu handle from a previously-added child menu or from the main plugin menu.

Return Values
-------------
Returns `true` on success.
