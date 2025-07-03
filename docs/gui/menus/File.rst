====
File
====

The file menu contains the following entries:

----
Open
----

The **Open** action lets you open an executable to debug it. The file can be an EXE file or a DLL file.

The command for this action is :doc:`../../commands/debug-control/InitDebug`.

------------
Recent Files
------------

The **Recent Files** submenu contains several entries that you previously debugged. It does not include any file that cannot be debugged by the program.

The entries for this submenu can be found in the ``Recent Files`` section of the config INI file. You can edit that file to remove entries.

------
Attach
------

Attach lets you attach to a running process. It will show a dialog listing the running processes, and allow you to choose one to attach. Currently you can only attach to an executable that is of the same architecture as the program. (eg, you cannot attach to a 64-bit process with x32dbg)

If you are debugging an executable, attaching to another process will terminate the previous debuggee.

The command for this action is :doc:`../../commands/debug-control/AttachDebugger`.

------
Detach
------

This action will detach the debugger from the debuggee, allowing the debuggee to run without being controlled by the debugger. You cannot execute this action when you are not debugging.

The command for this action is :doc:`../../commands/debug-control/DetachDebugger`.

---------------
Import database
---------------

This allows you to import a database.

The relevant command for this action is :doc:`../../commands/user-database/dbload`.

---------------
Export database
---------------

This allows you to export an uncompressed database.

The relevant command for this action is :doc:`../../commands/user-database/dbsave`.

----------
Patch file
----------

Opens the patch dialog. You can view your patches and apply the patch to a file in the dialog.

----------------
Restart as Admin
----------------

It will restart x64dbg and the current debuggee with administrator privilege.

----
Exit
----

Terminate the debugger. If any process is being debugged by this program, they are going to be terminated as well.