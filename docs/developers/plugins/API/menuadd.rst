===============
_plugin_menuadd
===============
This function adds a new child menu to a menu.

::

    int _plugin_menuadd(
        int hMenu, //menu handle to add the new child menu to
        const char* title //child menu title
    );

----------
Parameters
----------

:hMenu: Menu handle from a previously-added child menu or from the main plugin menu.
:title: Menu title.

-------------
Return Values
-------------
Returns the child menu handle (unique), -1 on failure.
