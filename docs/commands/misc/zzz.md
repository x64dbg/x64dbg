# zzz/doSleep

Halt execution for some time (equivalent of calling kernel32.Sleep).

## arguments

`[arg1]` Time (in milliseconds) to sleep. If not specified this is set to 100ms (0.1 second). Keep in mind that input is in hex per default so `Sleep 100` will actually sleep 256 milliseconds (use `Sleep .100` instead).

## result

This command does not set any result variables.
