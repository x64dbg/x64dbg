# GuiSetDebuggeeNotes

Sets the notes of the target being debugged (the debuggee), based on the text variable passed to the function. The text variable is a pointer to a string containing the information to set as the debuggee's notes.

```c++
void GuiSetDebuggeeNotes(char** text)
```

## Parameters

`text` A variable that contains a pointer to a string that contains the text to set as the debuggee notes.

## Return Value

This function does not return a value.

## Example

```c++
const char* text = json_string_value(json_object_get(root, "notes"));
GuiSetDebuggeeNotes(text);
```

## Related functions

- [GuiGetDebuggeeNotes](./GuiGetDebuggeeNotes.md)
- [GuiGetGlobalNotes](./GuiGetGlobalNotes.md)
- [GuiSetGlobalNotes](./GuiSetGlobalNotes.md)