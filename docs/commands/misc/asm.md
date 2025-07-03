# asm

Assemble an instruction.

## arguments

`arg1` Address to place the assembled instruction at.

`arg2` Instruction text. You can use [String Formatting](../../introduction/Formatting.md) here.

`[arg3]` When specified the remainder of the previous instruction will be filled with NOPs.

## result

$result will be set to the assembled instruction size. 0 on failure.
