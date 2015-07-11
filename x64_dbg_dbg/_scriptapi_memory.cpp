#include "_scriptapi_memory.h"
#include "memory.h"

SCRIPT_EXPORT bool Script::Memory::Read(duint addr, void* data, duint size, duint* sizeRead)
{
    return MemRead(addr, data, size, sizeRead);
}

SCRIPT_EXPORT bool Script::Memory::Write(duint addr, const void* data, duint size, duint* sizeWritten)
{
    return MemWrite(addr, (void*)data, size, sizeWritten);
}

SCRIPT_EXPORT bool Script::Memory::IsValidPtr(duint addr)
{
    return MemIsValidReadPtr(addr);
}

SCRIPT_EXPORT duint Script::Memory::RemoteAlloc(duint addr, duint size)
{
    return (duint)MemAllocRemote(addr, size, PAGE_EXECUTE_READWRITE);
}

SCRIPT_EXPORT bool Script::Memory::RemoteFree(duint addr)
{
    return MemFreeRemote(addr);
}

SCRIPT_EXPORT unsigned char Script::Memory::ReadByte(duint addr)
{
    unsigned char data;
    Read(addr, &data, sizeof(data), nullptr);
    return data;
}

SCRIPT_EXPORT bool Script::Memory::WriteByte(duint addr, unsigned char data)
{
    return Write(addr, &data, sizeof(data), nullptr);
}

SCRIPT_EXPORT unsigned short Script::Memory::ReadWord(duint addr)
{
    unsigned short data;
    Read(addr, &data, sizeof(data), nullptr);
    return data;
}

SCRIPT_EXPORT bool Script::Memory::WriteWord(duint addr, unsigned short data)
{
    return Write(addr, &data, sizeof(data), nullptr);
}

SCRIPT_EXPORT unsigned int Script::Memory::ReadDword(duint addr)
{
    unsigned int data;
    Read(addr, &data, sizeof(data), nullptr);
    return data;
}

SCRIPT_EXPORT bool Script::Memory::WriteDword(duint addr, unsigned int data)
{
    return Write(addr, &data, sizeof(data), nullptr);
}

#ifdef _WIN64
SCRIPT_EXPORT unsigned long long Script::Memory::ReadQword(duint addr)
{
    unsigned long long data;
    Read(addr, &data, sizeof(data), nullptr);
    return data;
}

SCRIPT_EXPORT bool Script::Memory::WriteQword(duint addr, unsigned long long data)
{
    return Write(addr, &data, sizeof(data), nullptr);
}
#endif //_WIN64

SCRIPT_EXPORT duint Script::Memory::ReadPtr(duint addr)
{
    duint data;
    Read(addr, &data, sizeof(data), nullptr);
    return data;
}

SCRIPT_EXPORT bool Script::Memory::WritePtr(duint addr, duint data)
{
    return Write(addr, &data, sizeof(data), nullptr);
}