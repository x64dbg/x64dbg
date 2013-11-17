#include "memory.h"

uint memfindbaseaddr(HANDLE hProcess, uint addr, uint* size)
{
    MEMORY_BASIC_INFORMATION mbi;
    DWORD numBytes;
    uint MyAddress=0, newAddress=0;
    do
    {
        numBytes=VirtualQueryEx(hProcess, (LPCVOID)MyAddress, &mbi, sizeof(mbi));
        newAddress=(uint)mbi.BaseAddress+mbi.RegionSize;
        if(mbi.State==MEM_COMMIT and addr<newAddress and addr>=MyAddress)
        {
            if(size)
                *size=mbi.RegionSize;
            return (uint)mbi.BaseAddress;
        }
        if(newAddress<=MyAddress)
            numBytes=0;
        else
            MyAddress=newAddress;
    }
    while(numBytes);
    return 0;
}

bool memread(HANDLE hProcess, const void* lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
{
    if(!hProcess or !lpBaseAddress or !lpBuffer or !nSize) //generic failures
        return false;

    SIZE_T read=0;
    DWORD oldprotect=0;
    bool ret=ReadProcessMemory(hProcess, (void*)lpBaseAddress, lpBuffer, nSize, &read); //try 'normal' RPM
    if(!ret or read!=nSize) //failed
    {
        VirtualProtectEx(hProcess, (void*)lpBaseAddress, nSize, PAGE_EXECUTE_READWRITE, &oldprotect); //change page protection
        ret=ReadProcessMemory(hProcess, (void*)lpBaseAddress, lpBuffer, nSize, &read); //try 'normal' RPM again
        VirtualProtectEx(hProcess, (void*)lpBaseAddress, nSize, oldprotect, &oldprotect); //restore page protection
    }
    if(ret and read==nSize) //'normal' RPM worked!
    {
        if(lpNumberOfBytesRead)
            *lpNumberOfBytesRead=read;
        return true;
    }

    for(uint i=0; i<nSize; i++) //read byte-per-byte
    {
        unsigned char* curaddr=(unsigned char*)lpBaseAddress+i;
        unsigned char* curbuf=(unsigned char*)lpBuffer+i;
        ret=ReadProcessMemory(hProcess, curaddr, curbuf, 1, 0); //try 'normal' RPM
        if(!ret) //we failed
        {
            VirtualProtectEx(hProcess, curaddr, 1, PAGE_EXECUTE_READWRITE, &oldprotect); //change page protection
            ret=ReadProcessMemory(hProcess, curaddr, curbuf, PAGE_SIZE, 0); //try 'normal' RPM again
            VirtualProtectEx(hProcess, curaddr, 1, oldprotect, &oldprotect); //restore page protection
            if(!ret) //complete failure
                return false;
        }
    }
    return true;
}

bool memisvalidreadptr(HANDLE hProcess, uint addr)
{
    unsigned char a=0;
    return memread(hProcess, (void*)addr, &a, 1, 0);
}

void* memalloc(HANDLE hProcess, uint addr, DWORD size, DWORD fdProtect)
{
    return VirtualAllocEx(hProcess, (void*)addr, size, MEM_RESERVE|MEM_COMMIT, fdProtect);
}
