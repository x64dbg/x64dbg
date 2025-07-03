# Notes

The following coding guide are helpful to make your plugin more complaint.

## Character encoding

x64dbg uses UTF-8 encoding everywhere it accepts a string. If you are passing a string to x64dbg, ensure that it is converted to UTF-8 encoding. This will help to reduce encoding errors.