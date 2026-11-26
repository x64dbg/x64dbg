# findmnem/mnemfind

Find instructions by mnemonic, optionally narrowed down by their operands. Unlike [findasm](./findasm.md), the pattern does not need to be a complete instruction that assembles. `xor` finds every `xor`, whatever its operands.

## arguments

`arg1` The pattern to look for (see below). Quote it when it contains a comma, for example `findmnem "xor eax, eax"`, otherwise the part after the comma is taken as the next argument. You can use [String Formatting](../../introduction/Formatting.md) here.

`[arg2]` Address of/inside a memory page to look in. When not specified CIP will be used.

`[arg3]` The number of bytes to search, starting at `arg2`. When not specified (or `0`) the whole memory region `arg2` is in is searched.

`[arg4]` Where to search. `0` searches the memory region `arg2` is in (the default), `1` the module `arg2` is in, `2` all modules, `3` all user modules and `4` all system modules. With `1` to `4`, `arg3` is ignored, and with `2` to `4` so is `arg2`.

## pattern

A pattern is a mnemonic, optionally followed by a space and an operand pattern:

- `xor` matches every `xor`, whatever the operands.
- `xor*` matches `xor`, `xorps`, `xorpd`, ...
- `j*` matches every instruction starting with `j`, which is all conditional jumps and `jmp`.
- `jmp|call` matches every `jmp` and every `call`.
- `xor eax` matches `xor` whose first operand is `eax`: `xor eax, eax`, `xor eax, ebx`.
- `xor *eax*` matches `xor` mentioning `eax` anywhere in its operands, including `xor ebx, eax`.
- `mov *,*[*` matches `mov` whose second operand is a memory operand.
- `call *[*` matches indirect calls through memory, like `call qword ptr ds:[0x00007FF6A1B02010]`.
- `je 0x00401*` matches `je` to a destination in `0x00401000`-`0x00401FFF` (x32).

The rules:

- `*` matches any run of characters, including none. `?` matches exactly one character.
- `|` separates alternatives. An instruction is found when it matches any of them. Each alternative has its own mnemonic and operands, so `xor eax|jmp` finds `xor eax, ...` and every `jmp`.
- Matching is case insensitive. `XOR EAX` and `xor eax` are the same pattern.
- The mnemonic must match in full: `xor` does not match `xorps`. Add a `*` (`xor*`) to match longer mnemonics.
- The operand pattern only has to match the start of the operands: `xor eax` means `xor eax*`. To find a register anywhere in the operands, put a `*` in front of it too (`xor *eax*`).
- Spaces around commas are ignored and other runs of spaces count as one. `eax,eax`, `eax, eax` and `eax ,  eax` are the same pattern, as are `dword ptr` and `dword  ptr`.
- Without an operand pattern the operands are not looked at, so `xor` is as fast as a pattern can be.

## what is matched

The mnemonic is matched against the name shown in the CPU view. The operands are matched against the plain disassembler text, which differs from the CPU view in several ways. To narrow a search by operands, write the operand pattern in the form shown in the last column (x64):

```
bytes               mnemonic  operand text
31 C0               xor       eax, eax
74 05               je        0x0000000140001007
48 8B 44 24 10      mov       rax, qword ptr ss:[rsp+0x10]
48 8B 45 F8         mov       rax, qword ptr ss:[rbp-0x08]
89 04 8B            mov       dword ptr ds:[rbx+rcx*4], eax
0F 1F 44 00 00      nop       dword ptr ds:[rax+rax*1], eax
8B 05 00 10 00 00   mov       eax, dword ptr ds:[0x0000000140002006]
B8 04 00 00 00      mov       eax, 0x04
F0 0F C1 08         xadd      dword ptr ds:[rax], ecx
F3 AA               stosb
62 F1 74 49 58 C2   vaddps    zmm0 {k1}, zmm1, zmm2
```

The `F0` (`lock`) and `F3` (`rep`) prefixes in the last rows are part of neither the mnemonic nor the operand text. On x32 the `je` above would read `0x00401007`.

In detail:

- Numbers are hexadecimal with a `0x` prefix and at least two digits: `mov eax, 0x04` matches, `mov eax, 4` and `mov eax, 0x4` do not. Use a wildcard when unsure: `mov eax, *4`.
- Addresses (jump and call destinations, absolute memory operands) are written out in full with leading zeros: 16 digits on x64, 8 on x32.
- Memory operands always carry their size and segment, as in `qword ptr ss:[rsp+0x10]`, so the pattern `mov rax, [rsp+0x10]` finds nothing. Start the operand with a `*` to skip the size and segment, as in `mov *[rsp+0x10]`.
- `rip` relative operands are shown as the absolute address they point to. `[rip+0x1000]` reads as `[0x0000000140002006]` in the example above.
- There are no spaces inside brackets, as in `[rbx+rcx*4]`. A pattern with `[rbx + rcx*4]` finds nothing.

## what can and cannot be matched

You can match:

- Any instruction by its mnemonic alone, with or without wildcards.
- Instructions that have a prefix (`lock`, `rep`, `repne`, `bnd`, `notrack`, ...), by their mnemonic: `xadd` finds `lock xadd`, `stosb` finds `rep stosb`.
- Conditional jumps, `cmovcc` and `setcc` by the names shown in the CPU view. These include the aliases the CPU view uses: `je`/`jne`, `ja`/`jae`, `jg`/`jge`, `cmove`/`cmovne`, `cmova`/`cmovae`, `cmovg`/`cmovge`, `sete`/`setne`, `seta`/`setae`, `setg`/`setge`.
- Registers, numbers, sizes (`byte ptr`, `dword ptr`, ...), segments and addresses in the operands, written as described under "what is matched".

You cannot match:

- Prefixes. `lock`, `rep`, `rep stosb`, `lock xadd` and the like find nothing, because the prefix is not part of the mnemonic or the operands. Search for the mnemonic (`xadd`, `stosb`) and look at the results instead.
- Mnemonic spellings other than the ones the CPU view shows. For example `jz`, `jnz`, `cmovz`, `setz`, `jc` and `jnae` find nothing. Search for `je`, `jne`, `cmove`, `sete` and `jb` (for both `jc` and `jnae`) instead.
- Labels, symbols, comments or module names, such as `call <kernel32.CreateFileW>` or `call MyLabel`. These are added by the CPU view and are not part of the operand text. Search for the address instead, or use [modcallfind](./modcallfind.md) for calls into other modules.
- Numbers written differently from the disassembler text, such as decimal (`4`), without leading zeros (`0x4`) or with an `h` suffix (`4h`). Hexadecimal digits match in either case.
- A literal `*`, `?` or `|`, as these always have their wildcard meaning. A `*` still matches a literal `*` (as in `[rbx+rcx*4]`), it just matches other characters as well.
- Two instructions in sequence (for example a `cmp` followed by a `jne`). Each instruction is matched on its own.
- Regular expressions. Only `*`, `?` and `|` have a special meaning.
- Instructions the search does not reach. The search disassembles from the start of the searched range, one instruction after the other, and skips bytes marked as data in the CPU view. An instruction that starts in the middle of another instruction (as in obfuscated code) is not found.

Braces start [String Formatting](../../introduction/Formatting.md), so `{k1}` is replaced by the value of the expression `k1` before the search starts. Write `{{k1}}` to search for the text `{k1}`, or use a wildcard in its place.

## examples

Find every `xor` in the memory region CIP is in:

```
findmnem xor
```

Find every instruction that zeroes a register by `xor`ing it with itself, in the current module:

```
findmnem "xor eax, eax|xor ecx, ecx|xor edx, edx", cip, 0, 1
```

Find every call and jump through a memory operand, in all user modules:

```
findmnem "call *[*|jmp *[*", cip, 0, 3
```

Find every `mov` into `eax`:

```
findmnem "mov eax"
```

## result

The `$result` variable is set to the number of instructions found.

## remarks

The search is also available from the CPU view: right click, `Search for`, choose where to search and then `Mnemonic`. The dialog is pre-filled with the mnemonic of the selected instruction.

The contents of the reference view can be iterated in a script with the `ref.addr` [expression function](../../introduction/Expression-functions.md):

```
i = 0
loop:
  addr = ref.addr(i)
  log "reference {d:i} = {p:addr}"
  i++
  cmp i, ref.count()
  jne loop
```
