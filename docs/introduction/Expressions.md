# Expressions

The debugger allows usage of basic expressions. Apart from calculations, it allows variable assignment using a C-like syntax. You can play around with expressions by typing them in the command bar, or using the calculator (<span class="title-ref">Help -\> Calculator</span> menu).

## Values

All values can be used as constants in expressions, see [values](./Values.md) for more information and examples.

**Warning: All numbers in expressions are interpreted as hex by default!** For decimal use `.123`.

## Operators

You can use the following operators in your expression. They are processed in the following order:

1.  *parentheses/brackets*: `(1+2)`, `[1+6]` have priority over other operations.
2.  *unary minus/binary not/logical not*: `-1` (negative 1), `~1` (binary not of 1), `!0` (logical not of 0).
3.  *multiplication/division*: `2*3` (regular multiplication), `` 2`3 `` (gets high part of the multiplication), `6/3` (regular division), `5%3` (modulo/remainder of the division).
4.  *addition/subtraction*: `1+3` (addition), `5-2` (subtraction).
5.  *left/right shift/rotate*: `1<<2` (shift left, shl for unsigned, sal for signed), `10>>1` (shift right, shl for unsigned, sal for signed), `1<<<2` (rotate left), `1>>>2` (rotate right).
6.  *smaller (equal)/bigger (equal)*: `4<10`, `3>6`, `1<=2`, `6>=7` (resolves to 1 if true, 0 if false).
7.  *equal/not equal*: `1==1`, `2!=6` (resolves to 1 if true, 0 if false).
8.  *binary and*: `12&2` (regular binary and).
9.  *binary xor*: `2^1` (regular binary xor).
10. *binary or*: `2|8` (regular binary or).
11. *logical and*: `0&&3` (resolves to 1 if true, 0 if false).
12. *logical or*: `0||3` (resolves to 1 if true, 0 if false).
13. *logical implication*: `0->1` (resolved to 1 if true, 0 if false).

## Quick-Assigning

Changing memory, a variable, register or flag can be easily done using a C-like syntax:

-   `a?=b` where `?` can be any non-logical operator. `a` can be any register, flag, variable or memory location. `b` can be anything that is recognized as an expression.
-   `a++/a--` where `a` can be any register, flag, variable or memory location.

## Functions

You can use functions in expressions. See [expression functions](./Expression-functions.md) for the documentation of these functions.