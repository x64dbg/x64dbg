# BridgeSettingSet

Writes a string value to the settings.

```c++
bool BridgeSettingSet(
    const char* section, // ini section name to write to
    const char* key, // ini key in the section to write
    char* value // string value to write
    );
```

## Parameters

`section` Section name to write to.

`key` Key in the section to write.

`value` New setting value.

## Return Value

This function returns true if successful or false otherwise.

## Example

```c++
Example code.
```

## Related functions

- List of related functions