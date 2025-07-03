# GuiReferenceSetCellContent

Sets a specified cell's data content in the Reference View based on the supplied row and column parameters.

```c++
void GuiReferenceSetCellContent(int row, int col, const char* str);
```

## Parameters

`row` integer representing the row to set data for.

`col` integer representing the column to set data for.

`str` const char* representing the string data to set at the row, col specified.


## Return Value

This function does not return a value.

## Example

```c++
const char szRefStart = "Start";
const char szRefFinish = "Finish";
const char szRefType = "Type";
GuiReferenceInitialize("Some Information"); // Add Reference View Header Title
GuiReferenceAddColumn(2 * sizeof(DWORD),&szRefStart);  // Add column Name
GuiReferenceAddColumn(2 * sizeof(DWORD),&szRefFinish); // Add column Name
GuiReferenceAddColumn(8,&szRefType); // Add column Name
GuiReferenceSetRowCount(2); // add 2 rows
int iRow = 0;
GuiReferenceSetCellContent(iRow,0,&szCodeCaveStartAddress); // add start address
GuiReferenceSetCellContent(iRow,1,&szCodeCaveFinishAddress); // add finish address
GuiReferenceSetCellContent(iRow,2,&szNop); // add type
iRow = iRow + 1; // Increment rows
// get variables to convert to strings (szCodeCaveStartAddress, szCodeCaveFinishAddress etc)
// add to next row's columns
```

## Notes

The Reference View must be initialized beforehand, and any columns required added before adding any rows and setting data for them.

Ensure you increment the row counter after you have set all data for all columns in a particular row, otherwise you will just overwrite any data you have set previously.

GuiReferenceSetRowCount needs to be called before setting cell contents - to update the reference view with a total count of rows, for example to add 5 rows: GuiReferenceSetRowCount(5), if you then decide to add another row later on then you would specify GuiReferenceSetRowCount(6)

Ideally you will use some variable that is incremented to automatically keep track of total rows added.


## Related functions

- [GuiReferenceAddColumn](./GuiReferenceAddColumn.md)
- [GuiReferenceDeleteAllColumns](./GuiReferenceDeleteAllColumns.md)
- [GuiReferenceGetCellContent](./GuiReferenceGetCellContent.md)
- [GuiReferenceGetRowCount](./GuiReferenceGetRowCount.md)
- [GuiReferenceInitialize](./GuiReferenceInitialize.md)
- [GuiReferenceReloadData](./GuiReferenceReloadData.md)
- [GuiReferenceSetCurrentTaskProgress](./GuiReferenceSetCurrentTaskProgress.md)
- [GuiReferenceSetProgress](./GuiReferenceSetProgress.md)
- [GuiReferenceSetRowCount](./GuiReferenceSetRowCount.md)
- [GuiReferenceSetSearchStartCol](./GuiReferenceSetSearchStartCol.md)
- [GuiReferenceSetSingleSelection](./GuiReferenceSetSingleSelection.md)