# GuiReferenceSetProgress

Sets the progress bar in the Reference View instance to a specific percentage value. This can be used to indicate to the user an operation is occurring, e.g. searching/sorting etc.

```c++
void GuiReferenceSetProgress(int progress);
```

## Parameters

`progress` An integer representing the percentage value to set the progress bar to.

## Return Value

This function does not return a value.

## Example

```c++
GuiReferenceSetProgress(0);
// do something
GuiReferenceSetProgress(50);
// do something else
GuiReferenceSetProgress(100);
// tell user operation has ended

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
- [GuiReferenceSetRowCount](./GuiReferenceSetRowCount.md)
- [GuiReferenceSetSearchStartCol](./GuiReferenceSetSearchStartCol.md)
- [GuiReferenceSetSingleSelection](./GuiReferenceSetSingleSelection.md)