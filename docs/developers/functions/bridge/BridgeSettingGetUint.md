# BridgeSettingGetUint

Reads an integer from the settings.

```c++
bool BridgeSettingGetUint(
    const char* section, // ini section name to write to
    const char* key, // ini key in the section to write
    duint* value // an integer variable to hold the value read
    );
```

## Parameters

`section` Section name to read.

`key` Key in the section to read.

`value` Destination value.

## Return Value

This function returns true if successful or false otherwise.

## Example

```c++
Example code.
```

## Related functions

- List of related functions