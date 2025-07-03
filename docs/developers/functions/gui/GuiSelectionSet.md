# GuiSelectionSet

Sets the currently selected line (or lines) of the specified GUI view based on the start and end addresses stored in a SELECTIONDATA variable.

```c++
bool GuiSelectionSet(int hWindow, const SELECTIONDATA* selection)
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
sel.end += 4; //expand selection
GuiSelectionSet(GUI_DISASSEMBLY, &sel)
```

## Related functions

- [GuiSelectionGet](./GuiSelectionGet.md)