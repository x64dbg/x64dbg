# modcallfind

Find all inter-modular calls.

## arguments

`[arg1]` Address of/inside a memory page to find inter-modular calls in. When not specified EIP/RIP will be used.

`[arg2]` The size of the data to search in.

## result

The `$result` variable is set to the number of inter-modular calls found.

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