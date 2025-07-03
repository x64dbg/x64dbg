# GuiReferenceSetCurrentTaskProgress

Sets the percentage bar and some status text in the current Reference View instance. This can indicate to the user that an operation is taking place, e.g. searching/sorting.

```c++
void GuiReferenceSetCurrentTaskProgress(int progress, const char* taskTitle);
```

## Parameters

`progress` An integer representing the value of the percentage bar.

`taskTitle` A const char representing a text string to indicate status or progress to the user.

## Return Value

This function does not return a value.

## Example

```c++
GuiReferenceSetCurrentTaskProgress(0,"Starting Search, Please Wait...");
// do something
GuiReferenceSetCurrentTaskProgress(50,"Searching, Please Wait...");
// do something else
GuiReferenceSetCurrentTaskProgress(100,"Finished Searching.");
// finished
```

## Related functions

- [GuiReferenceAddColumn](./GuiReferenceAddColumn.md)
- [GuiReferenceDeleteAllColumns](./GuiReferenceDeleteAllColumns.md)
- [GuiReferenceGetCellContent](./GuiReferenceGetCellContent.md)
- [GuiReferenceGetRowCount](./GuiReferenceGetRowCount.md)
- [GuiReferenceInitialize](./GuiReferenceInitialize.md)
- [GuiReferenceReloadData](./GuiReferenceReloadData.md)
- [GuiReferenceSetCellContent](./GuiReferenceSetCellContent.md)
- [GuiReferenceSetProgress](./GuiReferenceSetProgress.md)
- [GuiReferenceSetRowCount](./GuiReferenceSetRowCount.md)
- [GuiReferenceSetSearchStartCol](./GuiReferenceSetSearchStartCol.md)
- [GuiReferenceSetSingleSelection](./GuiReferenceSetSingleSelection.md)