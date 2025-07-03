# SetWatchdog

Set the watchdog mode of a watch item.

## arguments

`arg1` The id of the watch item.

`[arg2]` The watchdog mode. Possible values:

* disabled : Watchdog is disabled.
* changed : Watchdog is triggered when the value is changed.
* unchanged : Watchdog is triggered when the value is not changed.
* istrue : Watchdog is triggered when the value is not 0.
* isfalse : Watchdog is triggered when the value is 0.

When this argument is not specified, the mode will be set to "changed" if the current watchdog mode is "disabled", otherwise watchdog will be disabled.

## results

This command does not set any result variables.
