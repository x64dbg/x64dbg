#include "memory.h"
#include "debugger.h"
#include "patches.h"
#include "console.h"
#include "threading.h"
#include "module.h"

MemoryMap memoryPages;
bool bListAllPages = false;

void MemUpdateMap(HANDLE hProcess)
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

uint MemFindBaseAddr(uint addr, uint* Size, bool refresh)
{
    // Update the memory map if needed
    if(refresh)
        MemUpdateMap(fdProcessInfo->hProcess);

    SHARED_ACQUIRE(LockMemoryPages);

    // Search for the memory page address
    auto found = memoryPages.find(std::make_pair(addr, addr));

    if(found == memoryPages.end())
        return 0;

    // Return the allocation region size when requested
    if(Size)
        *Size = found->second.mbi.RegionSize;

    return found->first.first;
}

bool MemRead(void* BaseAddress, void* Buffer, SIZE_T Size, SIZE_T* NumberOfBytesRead)
{
    // Buffer must be supplied and size must be greater than 0
    if(!Buffer || Size <= 0)
        return false;

    // If the 'bytes read' parameter is null, use a temp
    SIZE_T bytesReadTemp = 0;

    if(!NumberOfBytesRead)
        NumberOfBytesRead = &bytesReadTemp;

    // Normal single-call read
    bool ret = MemoryReadSafe(fdProcessInfo->hProcess, BaseAddress, Buffer, Size, NumberOfBytesRead);

    // Did the normal memory read work?
    if(ret && *NumberOfBytesRead == Size)
        return true;

    // Read byte-by-byte
    // TODO: Replace this with a better method (PAGE_SIZE)
    for(uint i = 0; i < Size; i++)
    {
        unsigned char* curaddr  = (unsigned char*)BaseAddress + i;
        unsigned char* curbuf   = (unsigned char*)Buffer + i;

        ret = MemoryReadSafe(fdProcessInfo->hProcess, curaddr, curbuf, 1, nullptr);

        if(!ret)
        {
            *NumberOfBytesRead = i;

            SetLastError(ERROR_PARTIAL_COPY);
            return false;
        }
    }

    return true;
}

bool MemWrite(void* BaseAddress, void* Buffer, SIZE_T Size, SIZE_T* NumberOfBytesWritten)
{
    // Buffer must be supplied and size must be greater than 0
    if(!Buffer || Size <= 0)
        return false;

    // If the 'bytes written' parameter is null, use a temp
    SIZE_T bytesWrittenTemp = 0;

    if(!NumberOfBytesWritten)
        NumberOfBytesWritten = &bytesWrittenTemp;

    // Try a regular WriteProcessMemory call
    bool ret = MemoryWriteSafe(fdProcessInfo->hProcess, BaseAddress, Buffer, Size, NumberOfBytesWritten);

    if(ret and * NumberOfBytesWritten == Size)
        return true;

    // Fallback: Write byte-by-byte
    for(SIZE_T i = 0; i < Size; i++)
    {
        unsigned char* curaddr  = (unsigned char*)BaseAddress + i;
        unsigned char* curbuf   = (unsigned char*)Buffer + i;

        ret = MemoryWriteSafe(fdProcessInfo->hProcess, curaddr, curbuf, 1, nullptr);

        if(!ret)
        {
            *NumberOfBytesWritten = i;

            SetLastError(ERROR_PARTIAL_COPY);
            return false;
        }
    }

    return true;
}

bool MemPatch(void* BaseAddress, void* Buffer, SIZE_T Size, SIZE_T* NumberOfBytesWritten)
{
    // Buffer and size must be valid
    if(!Buffer || Size <= 0)
        return false;

    // Allocate the memory
    Memory<unsigned char*> olddata(Size, "mempatch:olddata");

    if(!MemRead(BaseAddress, olddata, Size, nullptr))
    {
        // If no memory can be read, no memory can be written. Fail out
        // of this function.
        return false;
    }

    for(SIZE_T i = 0; i < Size; i++)
        patchset((uint)BaseAddress + i, olddata[i], ((unsigned char*)Buffer)[i]);

    return MemWrite(BaseAddress, Buffer, Size, NumberOfBytesWritten);
}

bool MemIsValidReadPtr(uint Address)
{
    unsigned char a = 0;
    return MemRead((void*)Address, &a, 1, nullptr);
}

void* MemAllocRemote(uint Address, SIZE_T Size, DWORD Protect)
{
    return VirtualAllocEx(fdProcessInfo->hProcess, (void*)Address, Size, MEM_RESERVE | MEM_COMMIT, Protect);
}

void MemFreeRemote(uint Address)
{
    VirtualFreeEx(fdProcessInfo->hProcess, (void*)Address, 0, MEM_RELEASE);
}