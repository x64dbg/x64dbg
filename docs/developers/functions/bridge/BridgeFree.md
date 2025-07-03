# BridgeFree

Function description.

```c++
void BridgeFree(
    void* ptr // pointer to memory block to free
    );
```

## Parameters

`ptr` Pointer to memory block to free

## Return Value

This function does not return a value.

## Example

```c++
auto ptr = (char*)BridgeAlloc(128);
//do something with ptr
BridgeFree(ptr);
```

## Related functions

- [BridgeAlloc](./BridgeAlloc.md)