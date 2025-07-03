# DbgCmdExecDirect

Execute the command on the calling thread.

```c++
bool DbgCmdExecDirect(const char* cmd)
```

## Parameters

`cmd` The command string in UTF-8 encoding

## Return Value

`true` if the command is executed successfully, `false` otherwise.

## Example

```c++
DbgCmdExecDirect("run");
```

## Related functions

- [DbgCmdExec](./DbgCmdExec.md)