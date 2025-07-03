# DbgCmdExec

Execute the command on the command processing thread.

```c++
bool DbgCmdExec(const char* cmd);
```

## Parameters

`cmd` The command string in UTF-8 encoding

## Return Value

`true` if the command is sent to the command processing thread for asynchronous execution, `false` otherwise.

## Example

```c++
DbgCmdExec("run");
```

## Related functions

- [DbgCmdExecDirect](./DbgCmdExecDirect.md)