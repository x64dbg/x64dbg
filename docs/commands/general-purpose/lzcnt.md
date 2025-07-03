# lzcnt

Count the number of leading zeros of a value. If the value is 0, then the result is 64 on 64-bit platform and 32 on 32-bit platform.

## arguments

`arg1` Value.

## result

`arg1` is set to the number of leading zeros. Additionaly, the internal variable `$_EZ_FLAG` is set to 1 if `arg1` is 0, and set to 0 otherwise. The internal variable `$_BS_FLAG` is set to 0.
