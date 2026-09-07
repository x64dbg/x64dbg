#pragma once

namespace ElfBug
{
    enum class BreakpointType
    {
        Software,
        Hardware,
        Memory
    };

    enum class SoftwareType
    {
        ShortInt3
    };

    enum class HardwareSlot
    {
        Dr0,
        Dr1,
        Dr2,
        Dr3
    };

    enum class HardwareType
    {
        Execute,
        Write,
        Access
    };

    enum class HardwareSize
    {
        Byte,
        Word,
        Dword,
        Qword
    };

    enum class MemoryType
    {
        Access,
        Read,
        Write,
        Execute
    };

    enum class StepOverKind
    {
        None,
        Call,
        Rep,
        Pushf
    };
}
