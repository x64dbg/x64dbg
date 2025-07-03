Jxx/IFxx
========

There are various branches that can react on the flags set by the `cmp` (and maybe other) command(s):

* unconditional branch - `jmp`/`goto`
* branch if not equal - `jne`/`ifne(q)`/`jnz`/`ifnz`
* branch if equal - `je`/`ife(q)`/`jz`/`ifz`
* branch if smaller - `jb`/`ifb`/`jl`/`ifl`
* branch if bigger - `ja`/`ifa`/`jg`/`ifg`
* branch if smaller/equal - `jbe`/`ifbe(q)`/`jle`/`ifle(q)`
* branch if bigger/equal - `jae`/`ifae(q)`/`jge`/`ifge(q)`

arguments
---------
`arg1` The label to jump to.

result
------
This command does not set any result variables.
