# GuiSetGlobalNotes

Sets the global notes, based on the text variable passed to the function. The text variable is a pointer to a string containing the information to set as the global notes.

```c++
void GuiSetGlobalNotes(char** text)
```

## Parameters

`text` A variable that contains a pointer to a string that contains the text to set as the global notes.

## Return Value

This function does not return a value.

## Example

```c++
notesFile = String(szProgramDir) + "\\notes.txt";
String text;
if(!FileExists(notesFile.c_str()) || FileHelper::ReadAllText(notesFile, text))
    GuiSetGlobalNotes(text.c_str());
```

## Related functions

- [GuiGetGlobalNotes](./GuiGetGlobalNotes.md)
- [GuiGetDebuggeeNotes](./GuiGetDebuggeeNotes.md)
- [GuiSetDebuggeeNotes](./GuiSetDebuggeeNotes.md)

