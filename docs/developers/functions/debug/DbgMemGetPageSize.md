# DbgMemGetPageSize

Function description.

```c++
Function definition.
```

## Parameters

`param1` Parameter description.

## Return Value

Return value description.

## Example

Get page base and size of selected instruction.
```c++
    SELECTIONDATA sel; // Define Address the slected line in the Disassembly window ( begin , End )
    GuiSelectionGet(GUI_DISASSEMBLY, &sel); // Get the value of sel(begin addr , End addr )
    duint pagesize = DbgMemGetPageSize(sel.start);  // get the page size of the section from the selected memory addr
    //Or use the following statement to get page base and size in one call.
    duint sctionbase = DbgMemFindBaseAddr(sel.start, &pagesize); // get the base of this section ( begin addr of the section )
```

## Related functions

- List of related functions
