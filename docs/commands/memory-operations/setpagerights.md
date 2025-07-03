# setpagerights/setpagerights/setrightspage

Change the rights of a memory page.

## arguments

`arg1` Memory Address of page (it fix the address if this arg is not the top address of a page).

`arg2` New Rights, this can be one of the following values: "Execute", "ExecuteRead", "ExecuteReadWrite", "ExecuteWriteCopy", "NoAccess", "ReadOnly", "ReadWrite", "WriteCopy". You can add a G at first for add PAGE GUARD. example: "GReadOnly". Read the MSDN for more info.

## result

This command does not set any result variables.
