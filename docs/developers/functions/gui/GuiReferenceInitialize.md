# GuiReferenceInitialize

Initializes and creates a new instance of a Reference View. A new tab will appear under Reference Views entitled as per the name parameter. Columns can be added to this instance via the GuiReferenceAddColumn function. Rows can be inserted by using the GuiReferenceSetRowCount function and data for each cell (row and column) can be added via the GuiReferenceSetCellContent function. See the appropriate functions for more details.

```c++
void GuiReferenceInitialize(const char* name)
```

## Parameters

`name` A const char representing the text string to name the Reference View instance.

## Return Value

This function does not return a value.

## Example

```c++
GuiReferenceInitialize("Code Caves");
```

## Related functions

- [GuiReferenceAddColumn](./GuiReferenceAddColumn.md)
- [GuiReferenceDeleteAllColumns](./GuiReferenceDeleteAllColumns.md)
- [GuiReferenceGetCellContent](./GuiReferenceGetCellContent.md)
- [GuiReferenceGetRowCount](./GuiReferenceGetRowCount.md)
- [GuiReferenceReloadData](./GuiReferenceReloadData.md)
- [GuiReferenceSetCellContent](./GuiReferenceSetCellContent.md)
- [GuiReferenceSetCurrentTaskProgress](./GuiReferenceSetCurrentTaskProgress.md)
- [GuiReferenceSetProgress](./GuiReferenceSetProgress.md)
- [GuiReferenceSetRowCount](./GuiReferenceSetRowCount.md)
- [GuiReferenceSetSearchStartCol](./GuiReferenceSetSearchStartCol.md)
- [GuiReferenceSetSingleSelection](./GuiReferenceSetSingleSelection.md)