# DbgIsDebugging

Determines if the debugger is currently debugging an opened file or attached process.

```c++
bool DbgIsDebugging();
```

## Parameters

This function has no parameters.

## Return Value

This function returns true if x64dbg is currently debugging, or false otherwise.

## Example

```c++
if(!DbgIsDebugging())
{
    GuiAddLogMessage("You need to be debugging to use this option!\n");
    return false;
}
```

```nasm
.data
szMsg db "You need to be debugging to use this option!",13,10,0 ; CRLF
    
.code
Invoke DbgIsDebugging
.IF eax == FALSE
    Invoke GuiAddLogMessage, Addr szMsg
.ENDIF
```

## Related functions

- [DbgIsRunning](./DbgIsRunning.md)