# GuiMenuSetEntryIcon

Sets an icon for a specified menu entry.

```c++
void GuiMenuSetEntryIcon(int hEntry, const ICONDATA* icon)
```

## Parameters

`hEntry` Parameter description.

`icon` 

## Return Value

This function does not return a value.

## Example

```c++
ICONDATA rocket;
rocket.data = icon_rocket;
rocket.size = sizeof(icon_rocket);
hNewMenuEntry = GuiMenuAddEntry(hMenu, &szMenuEntryText);
GuiMenuSetEntryIcon(hNewMenuEntry,&rocket);
```

## Related functions

- [GuiMenuAdd](./GuiMenuAdd.md)
- [GuiMenuAddEntry](./GuiMenuAddEntry.md)
- [GuiMenuAddSeparator](./GuiMenuAddSeparator.md)
- [GuiMenuClear](./GuiMenuClear.md)
- [GuiMenuSetIcon](./GuiMenuSetIcon.md)

Note: Plugin developers should make use of the plugin functions provided:

- [\_plugin\_menuadd](../../plugins/API/menuadd.rst)
- [\_plugin\_menuaddentry](../../plugins/API/menuaddentry.rst)
- [\_plugin\_menuaddseparator](../../plugins/API/menuaddseparator.rst)
- [\_plugin\_menuclear](../../plugins/API/menuclear.rst)
- [\_plugin\_menuentryseticon](../../plugins/API/menuentryseticon.rst)
- [\_plugin\_menuseticon](../../plugins/API/menuseticon.rst)