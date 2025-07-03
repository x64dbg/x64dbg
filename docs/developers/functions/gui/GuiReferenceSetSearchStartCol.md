# GuiReferenceSetSearchStartCol

Sets the search starting column in the current Reference View instance.

```c++
void GuiReferenceSetSearchStartCol(int col);
```

## Parameters

`col` An integer representing the 0 based column to use for searching.

## Return Value

This function does not return a value.

## Example

```c++
GuiReferenceSetSearchStartCol(1);
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
- [GuiReferenceSetSingleSelection](./GuiReferenceSetSingleSelection.md)