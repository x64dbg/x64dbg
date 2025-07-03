# findallmem/findmemall

Find all occurrences of a pattern in the entire memory map.

## arguments

`arg1` The address to start searching from.

`arg2` The byte pattern to search for. This byte pattern can contain wildcards (?) for example: `EB0?90??8D`. You can use [String Formatting](../../introduction/Formatting.md) here.

`[arg3]` The size of the data to search in. Default is the entire memory map (`-1`).

`[arg4]` Set to `user`/`system`/`module` to only include (user/system) modules in the search.

## result

`$result` is set to the number of occurrences.

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