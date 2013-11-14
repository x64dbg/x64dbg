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
    if(!hProcess or !lpBaseAddress or !lpBuffer or !nSize)
        return false;

    uint addr=(uint)lpBaseAddress;
    uint startRva=addr&(PAGE_SIZE-1); //get start rva
    uint addrStart=addr-startRva; //round down one page
    uint pages=nSize/PAGE_SIZE+1;
    SIZE_T sizeRead=0;
    unsigned char curPage[PAGE_SIZE]; //current page memory
    unsigned char* destBuffer=(unsigned char*)lpBuffer;

    for(uint i=0; i<pages; i++)
    {
        SIZE_T readBytes=0;
        void* curAddr=(void*)(addrStart+i*PAGE_SIZE);
        bool ret=ReadProcessMemory(hProcess, curAddr, curPage, PAGE_SIZE, &readBytes);
        if(!ret or readBytes!=PAGE_SIZE)
        {
            DWORD oldprotect=0;
            VirtualProtectEx(hProcess, curAddr, PAGE_SIZE, PAGE_EXECUTE_READWRITE, &oldprotect);
            ret=ReadProcessMemory(hProcess, curAddr, curPage, PAGE_SIZE, &readBytes);
            VirtualProtectEx(hProcess, curAddr, PAGE_SIZE, oldprotect, &oldprotect);
            if(!ret or readBytes!=PAGE_SIZE)
                return false;
        }
        if(sizeRead+PAGE_SIZE>nSize) //do not overflow the buffer
            memcpy(destBuffer, curPage+startRva, nSize-sizeRead);
        else //default case
            memcpy(destBuffer, curPage+startRva, PAGE_SIZE-startRva);
        sizeRead+=(PAGE_SIZE-startRva);
        destBuffer+=(PAGE_SIZE-startRva);
        if(!i)
            startRva=0;
    }
    return true;
}

void* memalloc(HANDLE hProcess, uint addr, DWORD size, DWORD fdProtect)
{
    return VirtualAllocEx(hProcess, (void*)addr, size, MEM_RESERVE|MEM_COMMIT, fdProtect);
}
