# findguid/guidfind

Find references to GUID. The referenced GUID must be registered in the system, otherwise it will not be found.

## arguments

`[arg1]` The base of the memory range. If not specified, `RIP` or `EIP` will be used.

`[arg2]` The size of the memory range.

`[arg3]` The region to search. `0` is current region (specified with arg1 and arg2). `1` is current module (the module specified with arg1). `2` is all modules.

## results

Set `$result` to `1` if any GUID is found, `0` otherwise.

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