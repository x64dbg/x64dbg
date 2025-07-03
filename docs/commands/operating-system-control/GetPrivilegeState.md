# GetPrivilegeState

Query whether the privilege is enabled on the debuggee.

## arguments

`arg1` The name of the privilege. Example: `SeDebugPrivilege`.

## results

This command sets `$result` to `1` if the privilege is disabled on the debuggee, `2` or `3` if the privilege is enabled on the debuggee, `0` if the privilege is not found in the privilege collection of the token of the debuggee or something is wrong with the API.
