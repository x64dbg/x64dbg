#include "_scriptapi_stack.h"
#include "_scriptapi_memory.h"
#include "_scriptapi_register.h"

SCRIPT_EXPORT duint Script::Stack::Pop()
{
    duint csp = Register::GetCSP();
    duint top = Memory::ReadPtr(csp);
    Register::SetCSP(csp + sizeof(duint));
    return top;
}

SCRIPT_EXPORT duint Script::Stack::Push(duint value)
{
    duint csp = Register::GetCSP();
    Register::SetCSP(csp - sizeof(duint));
    Memory::WritePtr(csp - sizeof(duint), value);
    return Memory::ReadPtr(csp);
}

SCRIPT_EXPORT duint Script::Stack::Peek(int offset)
{
    return Memory::ReadPtr(Register::GetCSP() + offset * sizeof(duint));
}