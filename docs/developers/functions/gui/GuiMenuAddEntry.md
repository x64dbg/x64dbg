# GuiMenuAddEntry

This function adds a menu entry to a menu.

```c++
int GuiMenuAddEntry(int hMenu, const char* title)
```

## Parameters

`hMenu` Menu handle from a previously-added menu or from the main menu.

`title` A const char repesenting the text title of the menu item to be added.

## Return Value

Returns the menu handle (unique), or -1 on failure.

## Example

```c++
hNewMenuEntry = GuiMenuAddEntry(hMenu, &szMenuEntryText);
```

## Related functions

- [GuiMenuAdd](./GuiMenuAdd.md)
- [GuiMenuAddSeparator](./GuiMenuAddSeparator.md)
- [GuiMenuClear](./GuiMenuClear.md)
- [GuiMenuSetEntryIcon](./GuiMenuSetEntryIcon.md)
- [GuiMenuSetIcon](./GuiMenuSetIcon.md)

Note: Plugin developers should make use of the plugin functions provided:

- [\_plugin\_menuadd](../../plugins/API/menuadd.rst)
- [\_plugin\_menuaddentry](../../plugins/API/menuaddentry.rst)
- [\_plugin\_menuaddseparator](../../plugins/API/menuaddseparator.rst)
- [\_plugin\_menuclear](../../plugins/API/menuclear.rst)
- [\_plugin\_menuentryseticon](../../plugins/API/menuentryseticon.rst)
- [\_plugin\_menuseticon](../../plugins/API/menuseticon.rst)