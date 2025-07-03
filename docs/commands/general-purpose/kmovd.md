# kmovd/kmovq

The `kmovd`/`kmovq` commands are used to access AVX-512 opmask registers `K0`-`K7`. These commands are only available when the computer and x64dbg version supports AVX-512.

The `kmovd` command copies a 32-bit value from an expression to an AVX-512 opmask register, or from an AVX-512 opmask register to a memory location, a general purpose register, or an opmask register.
If the destination is a 64-bit register, for example, an opmask register, then the 32-bit value is zero-extended to 64 bits.

The `kmovq` command copies a 64-bit value from an expression to an AVX-512 opmask register, or from an AVX-512 opmask register to a memory location, a general purpose register, or an opmask register.
If executed in 32-bit environment, then only memory locations in the form of `[address]` or opmask registers are supported as operands.
In 64-bit environment, all valid expressions can be used to calculate the value to be copied into the destination opmask register.

## arguments

`arg1` The destination operand.

`arg2` The source operand.

## result

This command does not set any result variables.
