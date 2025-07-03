# AddWatch

Add a watch item.

## arguments

`arg1` The expression to watch.

`[arg2]` The data type of the watch item. `uint` displays hexadecimal value, `int` displays signed decimal value, `ascii` displays the ASCII string pointed by the value. `unicode` displays the Unicode string pointed by the value. `uint` is the default type.

## results

This command sets `$result` value to the id of the watch item.