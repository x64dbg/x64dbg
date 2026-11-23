# Breakpoint lifecycle API

Use `DbgFunctions()` from `_dbgfunctions.h` to create, configure and remove user
breakpoints without formatting debugger commands. Commands use the same underlying
breakpoint lifecycle operations.

## Create and remove

```cpp
BP_CREATEINFO info{};
info.size = sizeof(info);
info.type = bp_normal;
info.address = address;
// Create disabled, configure it, then enable it.

BP_REF ref{};
auto api = DbgFunctions();
if(!api->BpCreate(&info, &ref))
    return false;

if(!ref.SetField(bpf_breakcondition, std::string("eax == 1")) ||
        !ref.SetField(bpf_enabled, 1))
{
    api->BpRemove(&ref);
    return false;
}

// Later: this removes the engine registration as well as the database entry.
return api->BpRemove(&ref);
```

`BpCreate` does not overwrite existing breakpoints. On failure, a supplied output
reference is set to `bp_none`. `BpRemove` fails for a missing breakpoint or a
rejected engine removal. A failed removal retains the entry and its registration
state so the operation can be retried.

Initialize `BP_CREATEINFO` to zero and set `size` to `sizeof(BP_CREATEINFO)`.
`enabled`, `singleshoot` and `name` specify the initial configuration. The debugger
copies the input strings; their storage need not outlive the call.

| Type | Location | `typeEx` | Other fields |
| --- | --- | --- | --- |
| `bp_normal` | `address` is a VA, or an RVA when `module` is supplied | `0` (default software encoding) | None |
| `bp_hardware` | VA or module + RVA | `hw_access`, `hw_write`, `hw_execute` | `hwSize`: `hw_byte`, `hw_word`, `hw_dword`, or x64-only `hw_qword`. Execute breakpoints require `hw_byte`; addresses must be aligned. |
| `bp_memory` | VA or module + RVA | `mem_access`, `mem_read`, `mem_write`, `mem_execute` | `memsize`: range length. Zero derives the containing region's size and requires its base address. |
| `bp_dll` | `module` names the DLL; `address` is ignored | `dll_load`, `dll_unload`, `dll_all` | DLL paths are reduced to their file name. |
| `bp_exception` | `address` is the 32-bit exception code | `ex_firstchance`, `ex_secondchance`, `ex_all` | `module` is ignored. |

Address breakpoints may be created for an unloaded module using its name and an
RVA. They are installed when that module loads. Module names are case-insensitive
and paths are reduced to file names. Absolute-address creation requires a mapped
address. Enabled creation fails if an immediately required engine installation
fails; it does not leave a database-only phantom entry.

## Configure through fields

`BpRefVa`, `BpRefRva`, `BpRefDll` and `BpRefException` construct references, not
breakpoints. Use `BpRefExists` to test existence.

- Setting `bpf_enabled` installs/removes the registration. Repeating the same
  request is supported. An enabled but inactive entry can be retried by setting
  `bpf_enabled` to true again.
- Setting `bpf_singleshoot` also updates an installed software, memory or DLL
  registration. Hardware and exception single-shot handling happens in the
  debugger's event callback.
- `bpf_active` remains read-only. It describes the owned logical registration,
  not whether an INT3 byte is physically armed during an engine's stepping cycle.
- Names, conditions, logging and other metadata continue to use the corresponding
  number/text fields. Text getter callbacks run without the breakpoint lock and
  may themselves mutate breakpoints.

`enabled=true, active=false` is valid for an unloaded module or an unsuccessful
restoration. A successful enable request for an unloaded module returns true but
remains inactive until installation is possible. If changing single-shot mode
fails after removal, the API attempts to restore the old configuration. If that
also fails, it returns false and reports the retained enabled entry as inactive;
it does not claim that the engine still has it installed.

## Scope and compatibility

An active debug session is required for creation and mutation. Calls are
synchronous and do not execute debugger commands. The lifecycle operations
serialize database/registration changes with the existing breakpoint lock;
registration calls do not invoke plugin callbacks. Bulk debugger commands remain
per-entry operations, not all-or-nothing transactions.

The descriptor and the appended `DBGFUNCTIONS.BpCreate`/`BpRemove` entries require
a debugger build providing this API. Existing `BP_REF` layouts, `BP_FIELD` values
and function-table offsets are unchanged for existing plugins. References identify
a breakpoint location, not a generation: deleting and recreating at the same
location makes existing references refer to the replacement.

The API owns user-breakpoint registrations. Internal stepping/tracing helper
breakpoints and plugins calling an engine directly are outside that ownership
contract. Engine failures are reported using the engine's result; this API does
not make partial changes inside an engine implementation transactional.
