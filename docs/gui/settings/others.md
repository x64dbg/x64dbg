# Other settings

These settings do not appear in settings dialog, nor can they be changed in x64dbg GUI elsewhere, but can be modified by editing the INI configuration file.

## Engine
### AnimateInterval
If set to a value of milliseconds, animation will proceed every specified milliseconds.

Update: This setting has been added into settings dialog, and previous lower limit of 20ms has been removed.

### MaxSkipExceptionCount
If set (default is 10000), during a run that ignores first-chance exceptions(example, [erun](../../commands/debug-control/erun)), it will only ignore that specified number of first-chance exceptions. After that the debuggee will pause when one more first-chance exception happens. If set to 0 first-chance exceptions will always be ignored during such runs.

## Gui
### NonprintReplaceCharacter
If set to a Unicode value, dump view will use this character to represent nonprintable characters, instead of the default "."

### NullReplaceCharacter
If set to a Unicode value, dump view will use this character to represent null characters, instead of the default "."

## Misc
### AnimateIgnoreError
Set to 1 to ignore errors while animating, so animation will continue when an error in the animated command occurs.

### NoSeasons
Set to 1 to disable easter eggs and Christmas icons.
