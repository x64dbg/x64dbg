# GuiMenuClear

This function removes all entries and child menus from a menu. It will not remove the menu itself.

```c++
void GuiMenuClear(int hMenu)
```

## Parameters

`hMenu` Menu handle from a previously-added menu or from the main menu.

## Return Value

This function does not return a value.

## Example

```c++
hNewMenu = GuiMenuAdd(hMenu, &szMenuTitle);
GuiMenuClear(hMenuNew);
```

## Related functions

- [GuiMenuAdd](./GuiMenuAdd.md)
- [GuiMenuAddEntry](./GuiMenuAddEntry.md)
- [GuiMenuAddSeparator](./GuiMenuAddSeparator.md)
- [GuiMenuSetEntryIcon](./GuiMenuSetEntryIcon.md)
- [GuiMenuSetIcon](./GuiMenuSetIcon.md)

Note: Plugin developers should make use of the plugin functions provided:

- [\_plugin\_menuadd](../../plugins/API/menuadd.rst)
- [\_plugin\_menuaddentry](../../plugins/API/menuaddentry.rst)
- [\_plugin\_menuaddseparator](../../plugins/API/menuaddseparator.rst)
- [\_plugin\_menuclear](../../plugins/API/menuclear.rst)
- [\_plugin\_menuentryseticon](../../plugins/API/menuentryseticon.rst)
- [\_plugin\_menuseticon](../../plugins/API/menuseticon.rst)