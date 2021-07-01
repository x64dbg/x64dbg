#include "_scriptapi_memory.h"
#include "memory.h"
#include "threading.h"

SCRIPT_EXPORT bool Script::Memory::Read(duint addr, void* data, duint size, duint* sizeRead)
{
    return MemRead(addr, data, size, sizeRead);
}

SCRIPT_EXPORT bool Script::Memory::Write(duint addr, const void* data, duint size, duint* sizeWritten)
{
    return MemWrite(addr, data, size, sizeWritten);
}

SCRIPT_EXPORT bool Script::Memory::IsValidPtr(duint addr)
{
    return MemIsValidReadPtr(addr);
}

SCRIPT_EXPORT duint Script::Memory::RemoteAlloc(duint addr, duint size)
{
    return MemAllocRemote(addr, size);
}

SCRIPT_EXPORT bool Script::Memory::RemoteFree(duint addr)
{
    return MemFreeRemote(addr);
}

SCRIPT_EXPORT unsigned int Script::Memory::GetProtect(duint addr, bool reserved, bool cache)
{
    if(!cache)
        MemUpdateMap();
    SHARED_ACQUIRE(LockMemoryPages);
    auto found = memoryPages.find({ addr, addr });
    if(found == memoryPages.end())
        return 0;
    if(!reserved && found->second.mbi.State == MEM_RESERVE) //check if the current page is reserved.
        return 0;
    return found->second.mbi.Protect;
}

SCRIPT_EXPORT duint Script::Memory::GetBase(duint addr, bool reserved, bool cache)
{
    return MemFindBaseAddr(addr, nullptr, !cache, reserved);
}

SCRIPT_EXPORT duint Script::Memory::GetSize(duint addr, bool reserved, bool cache)
{
    duint size = 0;
    MemFindBaseAddr(addr, &size, !cache, reserved);
    return size;
}

SCRIPT_EXPORT unsigned char Script::Memory::ReadByte(duint addr)
{
    unsigned char data = 0;
    Read(addr, &data, sizeof(data), nullptr);
    return data;
}

SCRIPT_EXPORT bool Script::Memory::WriteByte(duint addr, unsigned char data)
{
    return Write(addr, &data, sizeof(data), nullptr);
}

SCRIPT_EXPORT unsigned short Script::Memory::ReadWord(duint addr)
{
    unsigned short data = 0;
    Read(addr, &data, sizeof(data), nullptr);
    return data;
}

SCRIPT_EXPORT bool Script::Memory::WriteWord(duint addr, unsigned short data)
{
    return Write(addr, &data, sizeof(data), nullptr);
}

SCRIPT_EXPORT unsigned int Script::Memory::ReadDword(duint addr)
{
    unsigned int data = 0;
    Read(addr, &data, sizeof(data), nullptr);
    return data;
}

SCRIPT_EXPORT bool Script::Memory::WriteDword(duint addr, unsigned int data)
{
    return Write(addr, &data, sizeof(data), nullptr);
}

SCRIPT_EXPORT unsigned long long Script::Memory::ReadQword(duint addr)
{
    unsigned long long data = 0;
    Read(addr, &data, sizeof(data), nullptr);
    return data;
}

SCRIPT_EXPORT bool Script::Memory::WriteQword(duint addr, unsigned long long data)
{
    return Write(addr, &data, sizeof(data), nullptr);
}

SCRIPT_EXPORT duint Script::Memory::ReadPtr(duint addr)
{
    duint data = 0;
    Read(addr, &data, sizeof(data), nullptr);
    return data;
}

SCRIPT_EXPORT bool Script::Memory::WritePtr(duint addr, duint data)
{
    return Write(addr, &data, sizeof(data), nullptr);
}