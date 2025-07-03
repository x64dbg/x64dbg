# reffind/findref/ref

Find references to a certain value.

## arguments

`arg1` The value to look for.

`[arg2]` Address of/inside a memory page to look in. When not specified CIP will be used.

`[arg3]` The size of the data to search in.

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