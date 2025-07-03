# GuiSymbolLogAdd

Function description.

```c++
void GuiSymbolLogAdd(const char* message);
```

## Parameters

`message` String containing the message to add to the symbol log. Ensure that a carriage line and return feed are included with the string for it to properly display it. Encoding is UTF-8.

## Return Value

This function does not return a value.

## Example

```c++
GuiSymbolLogAdd(&szMsg);
```

## Related functions

- [GuiSymbolLogClear](./GuiSymbolLogClear.md)
- [GuiSymbolRefreshCurrent](./GuiSymbolRefreshCurrent.md)
- [GuiSymbolSetProgress](./GuiSymbolSetProgress.md)
- [GuiSymbolUpdateModuleList](./GuiSymbolUpdateModuleList.md)