# exinfo

Print the `EXCEPTION_DEBUG_INFO` structure from the last exception.

Sample output:

```
EXCEPTION_DEBUG_INFO:
           dwFirstChance: 1
           ExceptionCode: 80000001 (EXCEPTION_GUARD_PAGE)
          ExceptionFlags: 00000000
        ExceptionAddress: 00007FFE16FB1B91 ntdll.00007FFE16FB1B91
        NumberParameters: 2
ExceptionInformation[00]: 0000000000000008
ExceptionInformation[01]: 00007FFE16FB1B91 ntdll.00007FFE16FB1B91
```

arguments
---------
This command has no arguments

results
-------
This command does not set any result variables.
