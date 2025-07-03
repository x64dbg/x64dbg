# DbgValFromString

Evaluate the expression.

```c++
duint DbgValFromString(const char* string);
```

## Parameters

`string` The [expression](../../../introduction/Expressions.md) to evaluate in UTF-8 encoding.

## Return Value

The value of the expression.

## Example

```c++
eip = DbgValFromString("cip");
```

## Related functions

- [DbgValToString](./DbgValToString.md)