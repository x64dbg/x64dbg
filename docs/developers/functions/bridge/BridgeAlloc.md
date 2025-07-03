# BridgeAlloc

Function description.

```c++
void* BridgeAlloc(
    size_t size // memory size to allocate
    );
```

## Parameters

`size` Memory size (in bytes) to allocate.

## Return Value

Returns a pointer to the memory block allocated. If an error occurs allocating memory, then x64dbg is closed down.

## Example

```c++
auto ptr = (char*)BridgeAlloc(128);
//do something with ptr
BridgeFree(ptr);
```

## Related functions

- [BridgeFree](./BridgeFree.md)