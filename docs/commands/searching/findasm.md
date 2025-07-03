# findasm/asmfind

Find assembled instruction.

## arguments

`arg1` Instruction to look for (make sure to use quoted "mov eax, ebx" to ensure you actually search for that instruction). You can use [String Formatting](../../introduction/Formatting.md) here.

`[arg2]` Address of/inside a memory page to look in. When not specified CIP will be used.

`[arg3]` The size of the data to search in. Default is the size of the memory region.

## result

The `$result` variable is set to the number of references found.

## remarks

The contents of the reference view can be iterated in a script with the `ref.addr` [expression function](../../introduction/Expression-functions.md):

```
i = 0
loop:
  addr = ref.addr(i)
  log "reference {d:i} = {p:addr}"
  i++
  cmp i, ref.count()
  jne loop
```