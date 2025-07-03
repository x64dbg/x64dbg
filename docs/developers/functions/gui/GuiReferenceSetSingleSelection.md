# GuiReferenceSetSingleSelection

Sets the currently selected row in the Reference View instance.

```c++
void GuiReferenceSetSingleSelection(int index, bool scroll);
```

## Parameters

`index` integer representing the row index to set the current selection to.

`scroll` a boolean value indicating if the selected index should be scrolled into view if it is not currently.

## Return Value

This function does not return a value.

## Example

```c++
GuiReferenceSetSingleSelection(0,true);
```

## Related functions

- [GuiReferenceAddColumn](./GuiReferenceAddColumn.md)
- [GuiReferenceDeleteAllColumns](./GuiReferenceDeleteAllColumns.md)
- [GuiReferenceGetCellContent](./GuiReferenceGetCellContent.md)
- [GuiReferenceGetRowCount](./GuiReferenceGetRowCount.md)
- [GuiReferenceInitialize](./GuiReferenceInitialize.md)
- [GuiReferenceReloadData](./GuiReferenceReloadData.md)
- [GuiReferenceSetCellContent](./GuiReferenceSetCellContent.md)
- [GuiReferenceSetCurrentTaskProgress](./GuiReferenceSetCurrentTaskProgress.md)
- [GuiReferenceSetProgress](./GuiReferenceSetProgress.md)
- [GuiReferenceSetRowCount](./GuiReferenceSetRowCount.md)
- [GuiReferenceSetSearchStartCol](./GuiReferenceSetSearchStartCol.md)

