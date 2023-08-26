#pragma once

/*
This should probably take some inspiration from Zydis:
- Address space min/max (64 vs 32 bit basically)
- Disassembly architecture (likely should return a reference to a disassembler)

*/
class Architecture
{
public:
    virtual ~Architecture() = default;

    // TODO: replace this with something about address space
    virtual bool disasm64() const = 0;
    virtual bool addr64() const = 0;
};
