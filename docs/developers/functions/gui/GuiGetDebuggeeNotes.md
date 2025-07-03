# GuiGetDebuggeeNotes

Returns into a variable a pointer to a string containing notes and information that a user has made relating to the target being debugged (the debuggee). The function GuiGetGlobalNotes can be used to get the global notes stored by a user.

```c++
void GuiGetDebuggeeNotes(char** text)
```

## Parameters

`text` A variable that will contain a pointer to a buffer on return. The pointer returned points to a string that will contain the notes for the debuggee.

## Return Value

This function does not return a value. The string containing the notes is returned via the pointer supplied via the `text` parameter.

## Example

```c++
char* text = nullptr;
GuiGetDebuggeeNotes(&text);
if(text)
{
    // do something with text
    BridgeFree(text);
}
```

## Related functions

- [GuiSetDebuggeeNotes](./GuiSetDebuggeeNotes.md)
- [GuiGetGlobalNotes](./GuiGetGlobalNotes.md)
- [GuiSetGlobalNotes](./GuiSetGlobalNotes.md)
