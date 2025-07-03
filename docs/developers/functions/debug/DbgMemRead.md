# DbgMemRead

Function description.

```c++
bool DbgMemRead(
  duint va,
  void* dest,
  duint size
  );
```

## Parameters

`va` Virtual address to source<br>
`dest` Pointer to pre allocated buffer of size `size`<br>
`size` Number of bytes that should be read

## Return Value

Returns true on success.

## Example

```c++
// read user selected data from disassembly window
SELECTIONDATA sel;
GuiSelectionGet(GUI_DISASSEMBLY, &sel);
uint16_t size = sel.end - sel.start + 1;
uint8_t* dest = new uint8_t[size];
bool success = DbgMemRead(sel.start, dest, size);
// on success, the selected data is stored in dest
```

## Related functions

- List of related functions
