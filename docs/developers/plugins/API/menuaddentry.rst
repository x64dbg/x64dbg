====================
_plugin_menuaddentry
====================
This function adds a menu entry to a menu.

::

    bool _plugin_menuaddentry(
        int hMenu, //menu handle to add the new child menu to
        int hEntry, //plugin-wide identifier for the menu entry
        const char* title //menu entry title
    );

Parameters 
----------

:hMenu: Menu handle from a previously-added child menu or from the main plugin menu.
:hEntry: A plugin-wide identifier for the menu entry. This is the value you will get in the `PLUG_CB_MENUENTRY` callback structure.
:title: Menu entry title.

Return Values 
-------------
Returns `true` when the entry was added without problems, `false` otherwise.
