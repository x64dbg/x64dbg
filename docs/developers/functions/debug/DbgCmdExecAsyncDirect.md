# DbgCmdExecAsyncDirect

Execute the command on the calling thread.

```c++
bool DbgCmdExecAsyncDirect(const char* cmd)
```

## Parameters

`cmd` The command string in UTF-8 encoding

## Return Value

`true` if the command is executed successfully, `false` otherwise.

## Example

```c++
DbgCmdExecAsyncDirect("run");
```

## Related functions

- [DbgCmdExecAsync](./DbgCmdExecAsync.md)