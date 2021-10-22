#ifndef _SCRIPTAPI_MEMORY_H
#define _SCRIPTAPI_MEMORY_H

#include "_scriptapi.h"

namespace Script
{
    namespace Memory
    {
        SCRIPT_EXPORT bool Read(duint addr, void* data, duint size, duint* sizeRead);
        SCRIPT_EXPORT bool Write(duint addr, const void* data, duint size, duint* sizeWritten);
        SCRIPT_EXPORT bool IsValidPtr(duint addr);
        SCRIPT_EXPORT duint RemoteAlloc(duint addr, duint size);
        SCRIPT_EXPORT bool RemoteFree(duint addr);
        SCRIPT_EXPORT unsigned int GetProtect(duint addr, bool reserved = false, bool cache = true);
        SCRIPT_EXPORT bool SetProtect(duint addr, unsigned int protect);
        SCRIPT_EXPORT duint GetBase(duint addr, bool reserved = false, bool cache = true);
        SCRIPT_EXPORT duint GetSize(duint addr, bool reserved = false, bool cache = true);

        SCRIPT_EXPORT unsigned char ReadByte(duint addr);
        SCRIPT_EXPORT bool WriteByte(duint addr, unsigned char data);
        SCRIPT_EXPORT unsigned short ReadWord(duint addr);
        SCRIPT_EXPORT bool WriteWord(duint addr, unsigned short data);
        SCRIPT_EXPORT unsigned int ReadDword(duint addr);
        SCRIPT_EXPORT bool WriteDword(duint addr, unsigned int data);
        SCRIPT_EXPORT unsigned long long ReadQword(duint addr);
        SCRIPT_EXPORT bool WriteQword(duint addr, unsigned long long data);
        SCRIPT_EXPORT duint ReadPtr(duint addr);
        SCRIPT_EXPORT bool WritePtr(duint addr, duint data);
    }; //Memory
}; //Script

#endif //_SCRIPTAPI_MEMORY_H