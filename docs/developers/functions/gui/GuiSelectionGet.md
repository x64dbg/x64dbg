# GuiSelectionGet

Gets the currently selected line (or lines) of the specified GUI view and returns the information as start and end addresses into a SELECTIONDATA variable.

```c++
bool GuiSelectionGet(int hWindow, SELECTIONDATA* selection)
```

## Parameters

`hWindow` an integer representing one of the following supported GUI views: GUI_DISASSEMBLY, GUI_DUMP, GUI_STACK.

`selection` a SELECTIONDATA structure variable that stores the start and end address of the current selection.

## Return Value

Return TRUE if successful or FALSE otherwise.

## Example

```c++
SELECTIONDATA sel;
GuiSelectionGet(GUI_DISASSEMBLY, &sel)
sprintf(msg, "%p - %p", sel.start, sel.end);
```

## Related functions

- [GuiSelectionSet](./GuiSelectionSet.md)