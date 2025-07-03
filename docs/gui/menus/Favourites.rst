Favourites
==========

This menu is customizable. When you click "Manage Favourite Tools" menu entry, a dialog will appear. You can add your custom tools to the menu, and also assign hotkeys to them. By default the path of the tool or script will appear in the menu, but if you set description of it, the description will appear in the menu instead.

- If you add ``%PID%`` in the command line of a tool, it will be replaced with the (decimal) PID of the debuggee (or 0 if not debugging).
- If you add ``%DEBUGGEE%`` it will add the (unquoted) full path of the debuggee.
- If you add ``%MODULE%`` it will add the (unquoted) full path of the module currently in the disassembly.
- If you add ``%-????-%`` it will perform :doc:`../../introduction/Formatting` on whatever you put in place of ``????``. Example: ``%-{cip}-%`` will be replaced with the hex value of ``cip``.

Currently, three types of entries may be inserted into this menu: Tool, Script and Command.

**See also:**

You can also add entries to this menu via the following commands:

:doc:`../../commands/gui/AddFavouriteCommand`

:doc:`../../commands/gui/AddFavouriteTool`

:doc:`../../commands/gui/AddFavouriteToolShortcut`