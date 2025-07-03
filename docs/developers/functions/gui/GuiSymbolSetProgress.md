# GuiSymbolSetProgress

Sets the progress bar in the symbol view based on the integer value supplied. This can be used to convey to the user an operation and how close it is to completion, for example with searches.

```c++
void GuiSymbolSetProgress(int percent);
```

## Parameters

`percent` an integer representing the percentage to set for the progress bar.

## Return Value

This function does not return a value.

## Example

```c++
GuiSymbolSetProgress(50);
```

## Related functions

- [GuiSymbolLogAdd](./GuiSymbolLogAdd.md)
- [GuiSymbolLogClear](./GuiSymbolLogClear.md)
- [GuiSymbolRefreshCurrent](./GuiSymbolRefreshCurrent.md)
- [GuiSymbolUpdateModuleList](./GuiSymbolUpdateModuleList.md)