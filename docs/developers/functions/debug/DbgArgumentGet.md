# DbgArgumentGet

This function gets the boundaries of the given argument location as start and end addresses.

```c++
bool DbgArgumentGet(duint addr, duint* start, duint* end);
```

## Parameters

`addr` Address of the argument to fetch.

`start` Pointer to a duint variable that will hold the start address of the argument.

`end` Pointer to a duint variable that will hold the end address of the argument.

## Return Value

The function return TRUE if the start and end addresses are found or FALSE otherwise. If TRUE, the variables `start` and `end` will hold the fetched values.

## Example

```c++
duint start;
duint end;
std::string message;

if(DbgArgumentGet(0x00401000, &start, &end))
{
  sprintf_s(message.c_str(), MAX_PATH, "Argument range: %08X-%08X\r\n", start, end);
  GuiAddLogMessage(message);
}
else
{
  GuiAddLogMessage("Argument start and end addresses couldn't be get\r\n");
}
```

## Related functions

- DbgArgumentAdd
- DbgArgumentDel
- DbgArgumentOverlaps
