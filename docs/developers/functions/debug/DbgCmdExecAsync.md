# DbgCmdExecAsync

Execute the command on the command processing thread.

```c++
bool DbgCmdExecAsync(const char* cmd);
```

## Parameters

`cmd` The command string in UTF-8 encoding

## Return Value

`true` if the command is sent to the command processing thread for asynchronous execution, `false` otherwise.

## Example

```c++
DbgCmdExecAsync("run");
```

## Related functions

- [DbgCmdExecAsyncDirect](./DbgCmdExecAsyncDirect.md)