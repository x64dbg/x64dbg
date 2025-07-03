# GuiGetGlobalNotes

Returns into a variable a pointer to a string containing global notes and information that a user has made. The function GuiGetDebuggeeNotes can be used to get the local notes stored by a user for the target being debugged (the debuggee).

```c++
void GuiGetGlobalNotes(char** text)
```

## Parameters

`text` A variable that will contain a pointer to a buffer on return. The pointer returned points to a string that will contain the global notes.

## Return Value

This function does not return a value. The string containing the notes is returned via the pointer supplied via the `text` parameter.

## Example

```c++
char* text = nullptr;
GuiGetGlobalNotes(&text);
if(text)
{
    // do something with text
    BridgeFree(text);
}
```

## Related functions

- [GuiSetGlobalNotes](./GuiSetGlobalNotes.md)
- [GuiGetDebuggeeNotes](./GuiGetDebuggeeNotes.md)
- [GuiSetDebuggeeNotes](./GuiSetDebuggeeNotes.md)
