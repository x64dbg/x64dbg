# DbgMemFindBaseAddr

Returns the baseaddress and size of a specific module

```c++
duint DbgMemFindBaseAddr(
  duint addr,
  duint* size
  );
```

## Parameters

`addr` Virtual address which is in a specific module. <br>
`size` Pointer, which will, on success, hold the module size.

## Return Value

On success, returns the virtual address of a specific module. <br>
On failure, it will return 0.

## Example

```c++
Example code.
```

## Related functions

- [DbgMemGetPageSize](./DbgMemGetPageSize.md)
