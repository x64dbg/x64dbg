# config

Get or set the configuration of x64dbg. It can also be used to load and store configuration specific to the script in the configuration file of x64dbg.

## arguments

`arg1` Section name of the INI file.

`arg2` Key name of the INI file.

`[arg3]` Optional new value of the configuration. If this argument is set to a number, it will be stored in the configuration file and ``$result`` is not updated. If this argument is not set, the current configuration will be read into ``$result``.

## results

This command sets `$result` to the current configuration number if `arg3` is not set.