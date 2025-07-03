# DbgSetAutoCommentAt

Set an auto comment at the given address.

```c++
bool DbgSetAutoCommentAt(duint addr, const char* text)
```

## Parameters

`addr` The address to comment.

`text` The auto comment in UTF-8 encoding.

## Return Value

`true` if the function is successful, `false` otherwise.

## Example

```c++
DbgSetAutoCommentAt(DbgValFromString("dis.sel()"), "This is the currently selected instruction");
```

## Related functions

- [DbgSetCommentAt](./DbgSetCommentAt.md)
- [DbgSetAutoLabelAt](./DbgSetAutoLabelAt.md)
- [DbgSetAutoBookmarkAt](./DbgSetAutoBookmarkAt.md)
- [DbgSetLabelAt](./DbgSetLabelAt.md)
- [DbgSetBookmarkAt](./DbgSetBookmarkAt.md)