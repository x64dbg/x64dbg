Commands
========

Syntax
------

Commands have the following format:

``command arg1, arg2, argN``

The first space separates the command name from its arguments. Commas, not spaces, separate arguments. Spaces outside quoted arguments are ignored, so ``log hello world`` passes ``helloworld`` as one argument. A semicolon separates multiple commands unless it appears inside a quoted argument.

For example:

.. code-block:: text

   savedata "C:\Program Files\dump.bin", 401000, 1000

Quoted arguments
----------------

Wrap an argument in double quotes when it contains commas, semicolons, or significant spaces. The surrounding quotes group the argument and are not passed to the command itself. Backslashes before ordinary characters are preserved verbatim, so Windows path separators and UNC paths do not need to be doubled.

When a command argument contains an expression with commas or string literals, quote the complete command argument and escape the expression's quotes:

.. code-block:: text

   SetBreakpointCondition 401000, "streq(utf8(rax), \"hello\")"

The outer quotes keep the condition together as one command argument. Each ``\"`` becomes a literal quote in the condition passed to the expression parser.

Escaping special characters
---------------------------

Outside a quoted argument, a backslash can escape a space, comma, or double quote. For example, ``one\ two`` is passed as ``one two`` and ``one\,two`` is passed as ``one,two``.

Inside a quoted argument:

- ``\"`` produces a literal double quote.
- ``\{`` produces a literal opening brace without entering string-formatting mode.
- A backslash before any other character is preserved, for example ``C:\data\file.db``.

Runs of backslashes immediately before a double quote follow the Windows quoting rule:

- ``2N`` backslashes produce ``N`` literal backslashes, and the quote ends the quoted section.
- ``2N+1`` backslashes produce ``N`` literal backslashes followed by a literal quote.

Consequently, a quoted argument ending in ``C:\directory\`` is written as ``"C:\directory\\"``. A literal backslash followed by a literal quote is written as ``\\\"`` inside the quoted argument.

String formatting
-----------------

An unescaped ``{`` inside a quoted argument starts a :doc:`string-formatting expression <../introduction/Formatting>`. Quotes inside the formatting expression remain part of that expression until its matching ``}``, which permits commands such as:

.. code-block:: text

   log "is jmp: {streq(dis.mnemonic(dis.sel()), "jmp")}"

Use ``\{`` when the opening brace should be literal instead. Runs of backslashes before ``{`` use the same even/odd rule as backslashes before a quote: an even run enters formatting mode, while an odd run makes the brace literal.

Notes
-----

- All integer constants are represented in hexadecimal. For example, after ``mov $i, 100``, ``$i`` is 0x100 (256 decimal). This also means a variable cannot begin with a letter from A through F.
- Throughout this documentation, ``[arg1]`` means that an argument is optional, while ``arg1`` means it is required. In expressions, ``[`` and ``]`` perform a memory dereference; omit them when the pointer value itself is wanted.
- Expressions do not support string comparison through numeric operators such as ``[eax] == "abcd"``. Use the appropriate :doc:`string expression function <../introduction/Expression-functions>` instead.

**Contents:**

.. toctree::
   :maxdepth: 1
   
   general-purpose/index
   debug-control/index
   breakpoint-control/index
   conditional-breakpoint-control/index
   tracing/index
   thread-control/index
   memory-operations/index
   operating-system-control/index
   watch-control/index
   variables/index
   searching/index
   user-database/index
   analysis/index
   types/index
   plugins/index
   script/index
   gui/index
   misc/index
