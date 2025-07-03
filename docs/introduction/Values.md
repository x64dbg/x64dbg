# Values

A lot of [commands](../commands/index.rst) accept [expressions](./Expressions.md) and values as arguments. Below is a list of all the value formats supported. You can play around with these by typing them in the command bar, or using the calculator (`Help -> Calculator` menu).

## Numbers

**All numbers are interpreted as hex by default!** If you want to be sure, you can `x` or `0x` as a prefix. Decimal numbers can be used by prefixing the number with a dot: `.123=7B`.

## Variables

Variables optionally start with a `$` and can only store one DWORD (QWORD on x64). This means that `myvar` and `$myvar` are equivalent. See the [variables](./Variables.md) section for more information.

## Registers

All registers of all sizes, except floating-point registers (eg: RAX, EAX, AL) can be used as variables.

Floating-point registers like XMM0, YMM0, ZMM0, K0 or ST(0) may not be used as variables, but they may be logged via the [String Formatting](./Formatting.md) floating-point type. Commands [`movdqu`](../commands/general-purpose/movdqu.md), [`vmovdqu`](../commands/general-purpose/vmovdqu.md), [`kmovd`](../commands/general-purpose/kmovd.md) can also be used to access floating-point registers.

### Remarks

- The variable names for most registers are the same as the names for them, except for the following registers: 
 - **x87 Control Word Flag**: The flags for this register is named like this: `_x87CW_UM`
- In addition to the registers in the architecture, x64dbg provides the following registers: `CAX` , `CBX` , `CCX` , `CDX` , `CSP` , `CBP` , `CSI` , `CDI` , `CIP`. These registers are mapped to 32-bit registers on 32-bit platform, and to 64-bit registers on 64-bit platform. For example, `CIP` is `EIP` on 32-bit platform, and is `RIP` on 64-bit platform. This feature is intended to support architecture-independent code.

## Flags

Debug flags (interpreted as integer) can be used as input. Flags are prefixed with an `_` followed by the flag name. Valid flags are: `_cf`, `_pf`, `_af`, `_zf`, `_sf`, `_tf`, `_if`, `_df`, `_of`, `_rf`, `_vm`, `_ac`, `_vif`, `_vip` and `_id`.

## Memory locations

You can read/write from/to a memory location by using one of the following expressions:
- `[addr]` read a DWORD/QWORD from `addr`.
- `n:[addr]` read n bytes from `addr`.
- `seg:[addr]` read a DWORD/QWORD from a segment at `addr`.
- `byte:[addr]` read a BYTE from `addr`.
- `word:[addr]` read a WORD from `addr`.
- `dword:[addr]` read a DWORD from `addr`.
- `qword:[addr]` read a QWORD from `addr` (x64 only).

- `n` is the amount of bytes to read, this can be anything not greater than 4 on x32 and not greater than 8 on x64 when specified, otherwise there will be an error.
- `seg` can be `gs`, `es`, `cs`, `fs`, `ds`, `ss`. Only `fs` and `gs` have an effect.

Dereferencing an invalid address causes an error, which can be problematic for [conditional breakpoints](./ConditionalBreakpoint.md) or when scripting. You can use the `ReadByte(addr)` family of [expression functions](./Expression-functions.md) to return 0 on error instead.

## Labels/Symbols

User-defined labels and symbols are a valid expressions (they resolve to the address of said label/symbol).

## Module Data

### DLL exports

Type `GetProcAddress` and it will automatically be resolved to the actual address of the function. To explicitly define from which module to load the API, use: `module.dll:api` or `module:api`. In a similar way you can resolve ordinals, try `module:ordinal`. Another macro allows you to get the loaded base of a module. When `module` is an empty string (`:myexport` for example), the module that is currently selected in the CPU will be used. Using a `.` instead of a `:` is equivalent.

```
ntdll.dll:ZwContinue
ntdll:memcmp
ntdll.memcmp // same as above
ntdll:1D // Ordinal 0x1D
:myexport // Export 'myexport' in the current module
```

Forwarded exports are resolved to their final address. To prevent this you can use a `?` instead of `:`.

```
kernel32:EnterCriticalSection // resolves to ntdll:RtlEnterCriticalSection
kernel32?EnterCriticalSection // resolves to the export in kernel32
```

### Loaded module bases

If you want to access the loaded module base, you can write: `module`, `module:0`, `module:base`, `module:imagebase` or `module:header`.

### RVA/File offset

If you want to access a module RVA you can either write `module + rva` or you can write `module:$rva`. If you want to convert a file offset to a VA you can use `module:#offset`. When `module` is an empty string (`:$123` for example), the module that is currently selected in the CPU will be used.

```
// File offset 0x400
ntdll.dll:#400
:#400
// RVA 0x1000
ntdll.dll:$1000 // RVA 0x1000
:$1000
```

### Module entry points

To access a module entry point you can write `module:entry`, `module:oep` or `module:ep`. Notice that when there are exports with the names `entry`, `oep` or `ep` the address of these will be returned instead.
