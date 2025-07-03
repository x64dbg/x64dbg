# DbgArgumentDel

This function deletes a previous setted argument at the specified address.

```c++
bool DbgArgumentDel(duint addr);
```

## Parameters

`addr` Address of the argument to delete.

## Return Value

The function return TRUE if argument is successfully deleted or FALSE otherwise.

## Example

```c++
if(DbgArgumentDel(0x00401013))
  GuiAddLogMessage("Argument successfully deleted\r\n");
else
  GuiAddLogMessage("Argument couldn't be deleted\r\n");
```

## Related functions

- DbgArgumentAdd
- DbgArgumentGet
- DbgArgumentOverlaps
