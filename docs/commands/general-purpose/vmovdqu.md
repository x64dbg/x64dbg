# vmovdqu/vmovups/vmovupd

Read/write a YMM/ZMM register. The source and destination operands can be either an YMM/ZMM register, or a memory location. The value of destination operand will be set to source operand. When using a memory location, only the syntax `[addr]` is supported, where `addr` is an expression of the memory address. Unlike other commands, the size of operands are 32 bytes for YMM registers and 64 bytes for ZMM registers. To access ZMM registers, the computer and x64dbg version must support AVX-512. To access XMM registers, use [`movdqu`](movdqu.md) command.

## arguments

`arg1` The destination operand.

`arg2` The source operand.

## result

This command does not set any result variables.
