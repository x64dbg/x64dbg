#include "memory.h"
#include "debugger.h"
#include "patches.h"
#include "console.h"
#include "threading.h"
#include "module.h"

MemoryMap memoryPages;
bool bListAllPages = false;

void memupdatemap(HANDLE hProcess)
{
    CriticalSectionLocker locker(LockMemoryPages);
    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T numBytes;
    uint MyAddress = 0, newAddress = 0;
    uint curAllocationBase = 0;

    std::vector<MEMPAGE> pageVector;
    do
    {
        numBytes = VirtualQueryEx(hProcess, (LPCVOID)MyAddress, &mbi, sizeof(mbi));
        if(mbi.State == MEM_COMMIT)
        {
            if(bListAllPages || curAllocationBase != (uint)mbi.AllocationBase) //only list allocation bases
            {
                curAllocationBase = (uint)mbi.AllocationBase;
                MEMPAGE curPage;
                *curPage.info = 0;
                ModNameFromAddr(MyAddress, curPage.info, true);
                memcpy(&curPage.mbi, &mbi, sizeof(mbi));
                pageVector.push_back(curPage);
            }
            else
                pageVector.at(pageVector.size() - 1).mbi.RegionSize += mbi.RegionSize;
        }
        newAddress = (uint)mbi.BaseAddress + mbi.RegionSize;
        if(newAddress <= MyAddress)
            numBytes = 0;
        else
            MyAddress = newAddress;
    }
    while(numBytes);

    //process file sections
    int pagecount = (int)pageVector.size();
    char curMod[MAX_MODULE_SIZE] = "";
    for(int i = pagecount - 1; i > -1; i--)
    {
        if(!pageVector.at(i).info[0] || (scmp(curMod, pageVector.at(i).info) && !bListAllPages)) //there is a module
            continue; //skip non-modules
        strcpy(curMod, pageVector.at(i).info);
        uint base = ModBaseFromName(pageVector.at(i).info);
        if(!base)
            continue;
        std::vector<MODSECTIONINFO> sections;
        if(!ModSectionsFromAddr(base, &sections))
            continue;
        int SectionNumber = (int)sections.size();
        if(!SectionNumber) //no sections = skip
            continue;
        if(!bListAllPages) //normal view
        {
            MEMPAGE newPage;
            //remove the current module page (page = size of module at this point) and insert the module sections
            pageVector.erase(pageVector.begin() + i); //remove the SizeOfImage page
            for(int j = SectionNumber - 1; j > -1; j--)
            {
                memset(&newPage, 0, sizeof(MEMPAGE));
                VirtualQueryEx(hProcess, (LPCVOID)sections.at(j).addr, &newPage.mbi, sizeof(MEMORY_BASIC_INFORMATION));
                uint SectionSize = sections.at(j).size;
                if(SectionSize % PAGE_SIZE) //unaligned page size
                    SectionSize += PAGE_SIZE - (SectionSize % PAGE_SIZE); //fix this
                if(SectionSize)
                    newPage.mbi.RegionSize = SectionSize;
                sprintf(newPage.info, " \"%s\"", sections.at(j).name);
                pageVector.insert(pageVector.begin() + i, newPage);
            }
            //insert the module itself (the module header)
            memset(&newPage, 0, sizeof(MEMPAGE));
            VirtualQueryEx(hProcess, (LPCVOID)base, &newPage.mbi, sizeof(MEMORY_BASIC_INFORMATION));
            strcpy(newPage.info, curMod);
            pageVector.insert(pageVector.begin() + i, newPage);
        }
        else //list all pages
        {
            uint start = (uint)pageVector.at(i).mbi.BaseAddress;
            uint end = start + pageVector.at(i).mbi.RegionSize;
            char section[50] = "";
            for(int j = 0, k = 0; j < SectionNumber; j++)
            {
                uint secStart = sections.at(j).addr;
                uint SectionSize = sections.at(j).size;
                if(SectionSize % PAGE_SIZE) //unaligned page size
                    SectionSize += PAGE_SIZE - (SectionSize % PAGE_SIZE); //fix this
                uint secEnd = secStart + SectionSize;

                if(secStart >= start && secEnd <= end) //section is inside the memory page
                {
                    if(k)
                        k += sprintf(pageVector.at(i).info + k, ",");
                    k += sprintf(pageVector.at(i).info + k, " \"%s\"", sections.at(j).name);
                }
                else if(start >= secStart && end <= secEnd) //memory page is inside the section
                {
                    if(k)
                        k += sprintf(pageVector.at(i).info + k, ",");
                    k += sprintf(pageVector.at(i).info + k, " \"%s\"", sections.at(j).name);
                }
            }
        }
    }

    //convert to memoryPages map
    pagecount = (int)pageVector.size();
    memoryPages.clear();
    for(int i = 0; i < pagecount; i++)
    {
        const MEMPAGE & curPage = pageVector.at(i);
        uint start = (uint)curPage.mbi.BaseAddress;
        uint size = curPage.mbi.RegionSize;
        memoryPages.insert(std::make_pair(std::make_pair(start, start + size - 1), curPage));
    }
}

uint memfindbaseaddr(uint addr, uint* size, bool refresh)
{
    if(refresh)
        memupdatemap(fdProcessInfo->hProcess); //update memory map
    CriticalSectionLocker locker(LockMemoryPages);
    MemoryMap::iterator found = memoryPages.find(std::make_pair(addr, addr));
    if(found == memoryPages.end())
        return 0;
    if(size)
        *size = found->second.mbi.RegionSize;
    return found->first.first;
}

bool memread(HANDLE hProcess, const void* lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
{
    if(!hProcess or !lpBaseAddress or !lpBuffer or !nSize) //generic failures
        return false;
    SIZE_T read = 0;
    DWORD oldprotect = 0;
    bool ret = MemoryReadSafe(hProcess, (void*)lpBaseAddress, lpBuffer, nSize, &read); //try 'normal' RPM
    if(ret and read == nSize) //'normal' RPM worked!
    {
        if(lpNumberOfBytesRead)
            *lpNumberOfBytesRead = read;
        return true;
    }
    for(uint i = 0; i < nSize; i++) //read byte-per-byte
    {
        unsigned char* curaddr = (unsigned char*)lpBaseAddress + i;
        unsigned char* curbuf = (unsigned char*)lpBuffer + i;
        ret = MemoryReadSafe(hProcess, curaddr, curbuf, 1, 0); //try 'normal' RPM
        if(!ret) //we failed
        {
            if(lpNumberOfBytesRead)
                *lpNumberOfBytesRead = i;
            SetLastError(ERROR_PARTIAL_COPY);
            return false;
        }
    }
    return true;
}

bool memwrite(HANDLE hProcess, void* lpBaseAddress, const void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten)
{
    if(!hProcess or !lpBaseAddress or !lpBuffer or !nSize) //generic failures
        return false;
    SIZE_T written = 0;
    DWORD oldprotect = 0;
    bool ret = MemoryWriteSafe(hProcess, lpBaseAddress, lpBuffer, nSize, &written);
    if(ret and written == nSize) //'normal' WPM worked!
    {
        if(lpNumberOfBytesWritten)
            *lpNumberOfBytesWritten = written;
        return true;
    }
    for(uint i = 0; i < nSize; i++) //write byte-per-byte
    {
        unsigned char* curaddr = (unsigned char*)lpBaseAddress + i;
        unsigned char* curbuf = (unsigned char*)lpBuffer + i;
        ret = MemoryWriteSafe(hProcess, curaddr, curbuf, 1, 0); //try 'normal' WPM
        if(!ret) //we failed
        {
            if(lpNumberOfBytesWritten)
                *lpNumberOfBytesWritten = i;
            SetLastError(ERROR_PARTIAL_COPY);
            return false;
        }
    }
    return true;
}

bool mempatch(HANDLE hProcess, void* lpBaseAddress, const void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten)
{
    if(!hProcess or !lpBaseAddress or !lpBuffer or !nSize) //generic failures
        return false;
    Memory<unsigned char*> olddata(nSize, "mempatch:olddata");
    if(!memread(hProcess, lpBaseAddress, olddata, nSize, 0))
        return memwrite(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
    unsigned char* newdata = (unsigned char*)lpBuffer;
    for(uint i = 0; i < nSize; i++)
        patchset((uint)lpBaseAddress + i, olddata[i], newdata[i]);
    return memwrite(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
}

bool memisvalidreadptr(HANDLE hProcess, uint addr)
{
    unsigned char a = 0;
    return memread(hProcess, (void*)addr, &a, 1, 0);
}

void* memalloc(HANDLE hProcess, uint addr, SIZE_T size, DWORD fdProtect)
{
    return VirtualAllocEx(hProcess, (void*)addr, size, MEM_RESERVE | MEM_COMMIT, fdProtect);
}

void memfree(HANDLE hProcess, uint addr)
{
    VirtualFreeEx(hProcess, (void*)addr, 0, MEM_RELEASE);
}