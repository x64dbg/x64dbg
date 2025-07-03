# findall

Find all occurrences of a pattern in a memory page.

## arguments

`arg1` The address to start searching from. Notice that the searching will stop when the end of the memory page this address resides in has been reached. This means you cannot search the complete process memory without enumerating the memory pages first. You can use [findallmem](./findallmem.md) to search for a pattern in the whole memory.

`arg2` The byte pattern to search for. This byte pattern can contain wildcards (?) for example: `EB0?90??8D`. You can use [String Formatting](../../introduction/Formatting.md) here.

`[arg3]` The size of the data to search in. Default is the size of the memory region.

## result

`$result` is set to the number of occurrences.

## examples

Search for all occurrences a pattern in the memory page CIP is residing:

```
findall mem.base(cip), "0FA2 E8 ???????? C3"
```

Search for all occurences of the value of cax in the stack memory page:

```
findall mem.base(csp), "{bswap@cax}"
```

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