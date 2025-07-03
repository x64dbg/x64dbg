# reffindrange/findrefrange/refrange

Find references to a certain range of values.

## arguments

`arg1` Start of the range (will be included in the results when found).

`[arg2]` End of range (will be included in the results when found). When not specified the first argument will be used.

`[arg3]` Address of/inside a memory page to look in. When not specified CIP will be used.

`[arg4]` The size of the data to search in.

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
