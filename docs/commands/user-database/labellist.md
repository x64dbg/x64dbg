# labellist

List user-defined labels in reference view.

## arguments

This command has no arguments.

## result

`$result` will be set to the number of user-defined labels.

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