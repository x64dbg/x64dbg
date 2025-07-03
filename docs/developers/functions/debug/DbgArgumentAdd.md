# DbgArgumentAdd

This function will add an argument to the specified address range.

```c++
bool DbgArgumentAdd(duint start, duint end);
```

## Parameters

`start` first address of the argument range.

`end` last address of the argument range.

## Return Value

The function return TRUE if argument is successfully setted or FALSE otherwise.

## Example

```c++
if(DbgArgumentAdd(0x00401000, 0x00401013))
  GuiAddLogMessage("Argument successfully setted\r\n");
else
  GuiAddLogMessage("Argument couldn't be set\r\n");
```

## Related functions

- DbgArgumentDel
- DbgArgumentGet
- DbgArgumentOverlaps
