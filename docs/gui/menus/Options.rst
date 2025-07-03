=======
Options
=======

The options menu contains the following entries:

-----------
Preferences
-----------

Show the :doc:`../settings/index` dialog. You can modify various settings in the dialog.

----------
Appearance
----------

Show the **Appearance** dialog. You can customize the color scheme or font in the dialog.

---------
Shortcuts
---------

Show the **Shortcuts** dialog. You can customize the shortcut keys for most of the operations.

---------------
Customize Menus
---------------

Show the "Customize menus" dialog. You can click on the nodes to expand the corresponding menu and check or uncheck the menu items. Checked item will appear in "More commands" section of the menu, to shorten the menu displayed. You can check all menu entries that you don't use.

-------
Topmost
-------

Keep the main window above other windows(or stop staying topmost).

----------------
Reload style.css
----------------

Reload ``style.css`` file. If this file is present, new color scheme specified in this file will be applied.

-------------------------
Set Initialization Script
-------------------------

Set a initialization script globally or for the debuggee. If a global initialization script is specified, it will be executed when the program is at the system breakpoint or the attach breakpoint for every debuggee. If a per-debuggee initialization script is specified, it will be executed after the global initialization script finishes. You can clear the script setting by clearing the file path and click "OK" in the browse dialog.

---------------
Import settings
---------------

Import settings from another configuration file. The corresponding entries in the configuration file will override the current configuration, but the missing entries will stay unmodified.

---------
Languages
---------

Allow the user to choose a language for the program. "*American English - United States*" is the native language for the program.