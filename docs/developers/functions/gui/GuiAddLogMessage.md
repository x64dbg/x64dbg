# GuiAddLogMessage

Adds a message to the log. The message will be shown in the log window and on the status bar at the bottom of x64dbg.

```c++
void GuiAddLogMessage(
    const char* msg // string containg message to add to log
    );
```

## Parameters

`msg` String containing the message to add to the log. Ensure that a carriage line and return feed are included with the string for it to properly display it. Encoding is UTF-8.

## Return Value

This function does not return a value.

## Example

```c++
GuiAddLogMessage("This text will be displayed in the log view.\n");
```

```nasm
.data
szMsg db "This text will be displayed in the log view",13,10,0 ; CRLF
    
.code
Invoke GuiAddLogMessage, Addr szMsg
```

## Related functions

- [GuiLogClear](./GuiLogClear.md)
- [GuiAddStatusBarMessage](./GuiAddStatusBarMessage.md)