# GuiSymbolUpdateModuleList

Refreshes the symbol view modules list.

```c++
void GuiSymbolUpdateModuleList(int count, SYMBOLMODULEINFO* modules)
```

## Parameters

`count` An integer representing the number of symbol module's to update.

`modules` A SYMBOLMODULEINFO variable that will hold the symbol module information.

## Return Value

This function does not return a value.

## Example

```c++
// Build the vector of modules
std::vector<SYMBOLMODULEINFO> modList;

if(!SymGetModuleList(&modList))
{
    GuiSymbolUpdateModuleList(0, nullptr);
    return;
}

// Create a new array to be sent to the GUI thread
size_t moduleCount = modList.size();
SYMBOLMODULEINFO* data = (SYMBOLMODULEINFO*)BridgeAlloc(moduleCount * sizeof(SYMBOLMODULEINFO));

// Direct copy from std::vector data
memcpy(data, modList.data(), moduleCount * sizeof(SYMBOLMODULEINFO));

// Send the module data to the GUI for updating
GuiSymbolUpdateModuleList((int)moduleCount, data);
```

## Related functions

- [GuiSymbolLogAdd](./GuiSymbolLogAdd.md)
- [GuiSymbolLogClear](./GuiSymbolLogClear.md)
- [GuiSymbolRefreshCurrent](./GuiSymbolRefreshCurrent.md)
- [GuiSymbolSetProgress](./GuiSymbolSetProgress.md)
