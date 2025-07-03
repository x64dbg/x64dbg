# GuiReferenceGetCellContent

Retrieves the data stored in a specified cell in the current Reference View instance, based on the supplied row and column parameters.

```c++
char* GuiReferenceGetCellContent(int row, int col)
```

## Parameters

`row` An integer representing the row for the cell for which the data is fetched.

`col` An integer representing the column for the cell for which the data is fetched.

## Return Value

The return value is a pointer to a char representing the data (typically a string) that was stored at the specified row/column of the current Reference View instance. NULL if there was no data or the row/column specified was incorrect.

## Example

```c++
Data = GuiReferenceGetCellContent(0,0);
```

## Related functions

- [GuiReferenceAddColumn](./GuiReferenceAddColumn.md)
- [GuiReferenceDeleteAllColumns](./GuiReferenceDeleteAllColumns.md)
- [GuiReferenceGetRowCount](./GuiReferenceGetRowCount.md)
- [GuiReferenceInitialize](./GuiReferenceInitialize.md)
- [GuiReferenceReloadData](./GuiReferenceReloadData.md)
- [GuiReferenceSetCellContent](./GuiReferenceSetCellContent.md)
- [GuiReferenceSetCurrentTaskProgress](./GuiReferenceSetCurrentTaskProgress.md)
- [GuiReferenceSetProgress](./GuiReferenceSetProgress.md)
- [GuiReferenceSetRowCount](./GuiReferenceSetRowCount.md)
- [GuiReferenceSetSearchStartCol](./GuiReferenceSetSearchStartCol.md)
- [GuiReferenceSetSingleSelection](./GuiReferenceSetSingleSelection.md)