# DbgSetCommentAt

Set a comment at the given address.

```c++
bool DbgSetCommentAt(duint addr, const char* text)
```

## Parameters

`addr` The address to comment.

`text` The comment in UTF-8 encoding.

## Return Value

`true` if the function is successful, `false` otherwise.

## Example

```c++
DbgSetCommentAt(DbgValFromString("dis.sel()"), "This is the currently selected instruction");
```

## Related functions

- [DbgSetAutoCommentAt](./DbgSetAutoCommentAt.md)
- [DbgSetAutoLabelAt](./DbgSetAutoLabelAt.md)
- [DbgSetAutoBookmarkAt](./DbgSetAutoBookmarkAt.md)
- [DbgSetLabelAt](./DbgSetLabelAt.md)
- [DbgSetBookmarkAt](./DbgSetBookmarkAt.md)