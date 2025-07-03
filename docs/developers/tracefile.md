# x64dbg trace file format specification
x64dbg trace file is a binary file that contains all information about program execution. Each trace file is composed of 3 parts: **Magic word**, **JSON header** and **binary trace blocks**. x64dbg trace file is little-endian.
## Magic word
Every trace file will begin with 4 bytes, "TRAC" (encoded in ASCII).
## Header
Header is located after header at offset 4. It is composed of a 4-byte length field, followed by a JSON blob. The JSON blob might not be null-terminated and might not be aligned to 4-byte boundary.
## Binary trace blocks
Binary trace data is immediately after header without any padding and might not be aligned to 4-byte boundary. It is defined as a sequence of blocks. Currently, only block type 0 is defined.

Every block is started with a 1-byte type number. This type number must be 0, which means it is a block that describes an instruction traced.

If the type number is 0, then the block will contain the following data:

```c++
struct {
    uint8_t BlockType; //BlockType is 0, indicating it describes an instruction execution.
    uint8_t RegisterChanges;
    uint8_t MemoryAccesses;
    uint8_t BlockFlagsAndOpcodeSize; //Bitfield

    DWORD ThreadId;
    uint8_t Opcode[];

    uint8_t RegisterChangePosition[];
    duint RegisterChangeNewData[];

    uint8_t MemoryAccessFlags[];
    duint MemoryAccessAddress[];
    duint MemoryAccessOldData[];
    duint MemoryAccessNewData[];
};
```

`RegisterChanges` is a unsigned byte that counts the number of elements in the array `RegisterChangePosition` and `RegisterChangeNewData`.

`MemoryAccesses` is a unsigned byte that counts the number of elements in the array `MemoryAccessFlags`.

`BlockFlagsAndOpcodeSize` is a bitfield. The most significant bit is ThreadId bit. When this bit is set, `ThreadId` field is available and indicates the thread id which executed the instruction. When this bit is clear, the thread id that executed the instruction is the same as last instruction, so it is not stored in file. The least 4 significant bits specify the length of `Opcode` field, in number of bytes. Other bits are reserved and set to 0. `Opcode` field contains the opcode of current instruction.

`RegisterChangePosition` is an array of unsigned bytes. Each element indicates a pointer-sized integer in struct `REGDUMP` that is updated after execution of current instruction, as an offset to previous location. The absolute index is computed by adding the absolute index of previous element +1 (or 0 if it is first element) with this relative index. `RegisterChangeNewData` is an array of pointer-sized integers that contains the new value of register, that is recorded before the instruction is executed. `REGDUMP` structure is given below.

```c++
typedef struct
{
    REGISTERCONTEXT regcontext;
    FLAGS flags;
    X87FPUREGISTER x87FPURegisters[8];
    unsigned long long mmx[8];
    MXCSRFIELDS MxCsrFields;
    X87STATUSWORDFIELDS x87StatusWordFields;
    X87CONTROLWORDFIELDS x87ControlWordFields;
    LASTERROR lastError;
    //LASTSTATUS lastStatus; //This field is not supported and not included in trace file.
} REGDUMP;
```

For example, `ccx` is the second member of `regcontext`. On x64 architecture, it is at byte offset 8 and on x86 architecture it is at byte offset 4. On both architectures, it is at index 1 and `cax` is at index 0. Therefore, when `RegisterChangePosition[0]` = 0, `RegisterChangeNewData[0]` contains the new value of `cax`. If `RegisterChangePosition[1]` = 0, `RegisterChangeNewData[1]` contains the new value of `ccx`, since the absolute index is computed by 0+0+1=1. The use of relative indexing helps achieve better data compression if a lossless compression is then applied to trace file, and also allow future expansion of `REGDUMP` structure without increasing size of `RegisterChanges` and `RegisterChangePosition` beyond a byte. Note: the file reader can locate the address of the instruction using `cip` register in this structure.

x64dbg will save all registers at the start of trace, and every 512 instructions (this number might be changed in future versions to have different tradeoff between speed and space). A block with all registers saved will have `RegisterChanges`=172 on 64-bit platform and 216 on 32-bit platform. This allows x64dbg trace file to be randomly accessed. x64dbg might be unable to open a trace file that has a sequence of instruction longer than an implementation-defined limit without all registers saved.

`MemoryAccessFlags` is an array of bytes that indicates properties of memory access. Currently, only bit 0 is defined and all other bits are reserved and set to 0. When bit 0 is set, it indicates the memory is not changed (This could mean it is read, or it is overwritten with identical value), so `MemoryAccessNewData` will not have an entry for this memory access. The file reader may use a disassembler to determine the true type of memory access.

`MemoryAccessAddress` is an array of pointers that indicates the address of memory access.

`MemoryAccessOldData` is an array of pointer-sized integers that stores the old content of memory.

`MemoryAccessNewData` is an array of pointer-sized integers that stores the new content of memory.
