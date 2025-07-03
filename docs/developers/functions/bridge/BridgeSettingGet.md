# BridgeSettingGet

Reads a string file from the settings.

```c++
bool BridgeSettingGet(
    const char* section, // ini section name to read
    const char* key, // ini key in the section to read
    char* value // string to hold the value read
    );
```

## Parameters

`section` Section name to read.

`key` Key in the section to read.

`value` Destination buffer. Should be of `MAX_SETTING_SIZE`.

## Return Value

This function returns true if successful or false otherwise.

## Example

```c++
Example code.
```

## Related functions

- List of related functions