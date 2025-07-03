# String Formatting

This section explains the simple string formatter built into x64dbg.

The basic syntax is `{?:expression}` where `?` is the optional type of the [expression](./Expressions.md). To output `{` or `}` in the result, escape them as `{{` or `}}`.

## Types

- `d` signed **d**ecimal: `-3`
- `u` **u**nsigned decimal: `57329171`
- `p` zero prefixed **p**ointer: `0000000410007683`
- `s` **s**tring pointer: `"this is a string"` (**not recommended**, use `{utf8@address}` instead)
- `x` he**x**: `3C28A` (default for integer values)
- `a` **a**ddress info: `00401010 <module.EntryPoint>`
- `i` **i**nstruction text: `jmp 0x77ac3c87`
- `f` single precision floating-point pointer or register: If `10001234` is an address of a single precision floating-point number 3.14, `{f:10001234}` will print `3.14`. It can also accept XMM, YMM and ZMM registers: `{f:XMM0}` prints the single precision floating-point number at XMM0 bit 31:0, `{f:YMM7[7]}` prints the single precision floating-point number at YMM7 bit 255:224. x87 registers are currently not supported.
-   `F` double precision floating-point pointer or register: Similar to `f`, except that the data is interpreted as double precision floating-point number. It can also accept XMM, YMM and ZMM registers: `{F:YMM7[3]}` prints the double precision floating-point number at YMM7 bit 255:192.

**Note**: XMM, YMM and ZMM registers may only be used with the `f`/`F` floating-point type. (Issue 2826 links to details about why)

## Complex Type

- `{mem;size@address}` will print the `size` bytes starting at `address` in hex.
- `{winerror@code}` will print the name of windows error code(returned with `GetLastError()`) and the description of it(with `FormatMessage`). It is similar to ErrLookup utility.
- `{winerrorname@code}` will print the name of windows error code(returned with `GetLastError()`) only.
- `{ntstatus@code}` will print the name of NTSTATUS error code and the description of it(with `FormatMessage`).
- `{ntstatusname@code}` will print the name of NTSTATUS error code only.
- `{ascii[;length]@address}` will print the ASCII string at `address` with an optional `length` (in bytes).
- `{ansi[;length]@address}` will print the ANSI (local codepage) string at `address` with an optional `length` (in bytes).
- `{utf8[;length]@address}` will print the UTF-8 string at `address` with an optional `length` (in bytes).
- `{utf16[;length]@address}` will print the UTF-16 string at `address` with an optional `length` (in words).
- `{disasm@address}` will print the disassembly at `address` (equivalent to `{i:address}`).
- `{modname@address}` will print the name of the module at `address`.
- `{bswap[;size]@value}` will byte-swap `value` for a specified `size` (size of pointer per default).
- `{label@address}` will print the (auto)label at `address`.
- `{comment@address}` will print the (auto)comment at `address`.

## Examples

- `rax: {rax}` formats to `rax: 4C76`
- `password: {utf16@4*ecx+0x402000}` formats to `password: s3cret`
- `function type: {mem;1@[ebp]+0xa}` formats to `function type: 01`
- `{x:bswap(rax)}` where `rax=0000000078D333E0` formats to `E033D37800000000` because of bswap fun which reverse the hex value
- `{bswap;4@rax}` where `rax=1122334455667788` formats to `88776655`
- `mnemonic: {dis.mnemonic(dis.sel())}` formats to `mnemonic: push`
- `return address: `{a:[rsp]}` formats to `00401010 <module.myfunction+N>`

## Logging

When using the `log` command you should put quotes around the format string (`log "{mem;8@rax}"`) to avoid ambiguity with the `;` (which separates two commands). See [issue #1931](https://github.com/x64dbg/x64dbg/issues/1931) for more details.

## Plugins

Plugins can use [`_plugin_registerformatfunction`](../developers/plugins/API/registerformatfunction.rst) to register custom string formatting functions. The syntax is `{type;arg1;arg2;argN@expression}` where `type` is the name of the registered function, `argN` is any string (these are passed to the formatting function as arguments) and `expression` is any valid expression.
