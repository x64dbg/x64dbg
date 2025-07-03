# GuiAddStatusBarMessage

Shows text in the statusbar, which can be used to inform the user.

```c++
void GuiAddStatusBarMessage(const char* msg);
```

## Parameters

`msg` String containing the message to add to the status bar.

## Return Value

This function does not return a value.

## Example

```c++
GuiAddStatusBarMessage("This text will be displayed in the statusbar.");
```

## Related functions

- [GuiAddLogMessage](./GuiAddLogMessage.md)