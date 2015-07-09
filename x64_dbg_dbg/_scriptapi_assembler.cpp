#include "_scriptapi_assembler.h"
#include "assemble.h"

SCRIPT_EXPORT bool Script::Assembler::Assemble(duint addr, unsigned char* dest, int* size, const char* instruction)
{
    return assemble(addr, dest, size, instruction, nullptr);
}

SCRIPT_EXPORT bool Script::Assembler::AssembleEx(duint addr, unsigned char* dest, int* size, const char* instruction, char* error)
{
    return assemble(addr, dest, size, instruction, error);
}

SCRIPT_EXPORT bool Script::Assembler::AssembleMem(duint addr, const char* instruction)
{
    return assembleat(addr, instruction, nullptr, nullptr, false);
}

SCRIPT_EXPORT bool Script::Assembler::AssembleMemEx(duint addr, const char* instruction, int* size, char* error, bool fillnop)
{
    return assembleat(addr, instruction, size, error, fillnop);
}