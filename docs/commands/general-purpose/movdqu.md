# movdqu/movups/movupd

Read/write an XMM register. The source and destination operands can be either an XMM register, or a memory location. The value of destination operand will be set to source operand. When using a memory location, only the syntax `[addr]` is supported, where `addr` is an expression of the memory address. Unlike other commands, the size of operands are 16 bytes. MM/YMM/ZMM registers are not supported by this command, to access YMM/ZMM registers, use [`vmovdqu`](vmovdqu.md) command.

## arguments

`arg1` The destination operand.

`arg2` The source operand.

## result

This command does not set any result variables.
