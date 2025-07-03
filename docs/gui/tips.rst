Tips
====
This section contains some useful tips about the user interface of this program.

Modules view
------------

The modules view is inside the symbols view.

Relative Addressing
-------------------
If you double-click the address column, then relative addressing will be used. The address column will show the relative address relative to the double-clicked address.

Tables
------

You can reorder and hide any column by right-clicking, middle-clicking or double-clicking on the header. Alternatively, you can drag one column header to another one to exchange their order.

Highlight mode
--------------

Don't know how to hightlight a register? Press Ctrl+H (or click "Highlight mode" menu on the disassembly view). When the red border is shown, click on the register(or command, immediate or any token), then that token will be hightlighted with an underline.

Middle mouse button
-------------------

In disassembly view, pressing middle mouse button will copy the selected address to the clipboard.

Select the entire function or block
-----------------------------------

You can select the entire function by double-clicking on the checkbox next to the disassembly. This checkbox can also be used to
fold the block into a single line.

Note: when you select only 1 instruction, or if the function is not analyzed, the checkbox might not appear. In this case, please select the instruction you want to fold first.

Code page
---------

You can use the codepage dialog(in the context menu of the dump view) to select a code page. UTF-16LE is the codepage that matches windows unicode encoding. You can use UTF-16LE code page to view strings in a unicode application.

Change Window Title
-------------------

You can rename the windows of x64dbg by renaming "x64dbg.exe" or "x32dbg.exe" to another name, if the debuggee doesn't support running in a system with a window or process named as such.
You should also rename the "x64dbg.ini" or "x32dbg.ini" to keep it the same name as the debugger.

Search for strings
------------------

You can use the following methods to search for string:

 -  Search for / Pattern: you will be asked to provide a string to search, and x64dbg will search for it and display the results in the references view.
 -  Search for / Strings references: x64dbg will search all pointers that look like an ANSI or Unicode string and display the results in the references view. Older versions of x64dbg supports only Latin strings, while latest x64dbg version supports non-English languges through a generic algorithm that may or may not work well in your language. **If you need to search for strings in other languages better, please install appropriate plugins.**
 -  Search for / Constant: search for a constant that is the first DWORD/QWORD of the string.
