/**
 @file memory.cpp

 @brief Implements the memory class.
 */

#include "memory.h"
#include "debugger.h"
#include "patches.h"
#include "threading.h"
#include "module.h"

#define PAGE_SHIFT              (12)
//#define PAGE_SIZE               (4096)
#define PAGE_ALIGN(Va)          ((ULONG_PTR)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))
#define BYTES_TO_PAGES(Size)    (((Size) >> PAGE_SHIFT) + (((Size) & (PAGE_SIZE - 1)) != 0))
#define ROUND_TO_PAGES(Size)    (((ULONG_PTR)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

std::map<Range, MEMPAGE, RangeCompare> memoryPages;
bool bListAllPages = false;

void MemUpdateMap()
{
    // First gather all possible pages in the memory range
    std::vector<MEMPAGE> pageVector;
    {
        SIZE_T numBytes = 0;
        uint pageStart = 0;
        uint allocationBase = 0;

        do
        {
            // Query memory attributes
            MEMORY_BASIC_INFORMATION mbi;
            memset(&mbi, 0, sizeof(mbi));

            numBytes = VirtualQueryEx(fdProcessInfo->hProcess, (LPVOID)pageStart, &mbi, sizeof(mbi));

            // Only allow pages that are committed to memory (exclude reserved/mapped)
            if(mbi.State == MEM_COMMIT)
            {
                // Only list allocation bases, unless if forced to list all
                if(bListAllPages || allocationBase != (uint)mbi.AllocationBase)
                {
                    // Set the new allocation base page
                    allocationBase = (uint)mbi.AllocationBase;

                    MEMPAGE curPage;
                    memset(&curPage, 0, sizeof(MEMPAGE));
                    memcpy(&curPage.mbi, &mbi, sizeof(mbi));

                    ModNameFromAddr(pageStart, curPage.info, true);
                    pageVector.push_back(curPage);
                }
                else
                {
                    // Otherwise append the page to the last created entry
                    pageVector.back().mbi.RegionSize += mbi.RegionSize;
                }
            }

            // Calculate the next page start
            uint newAddress = (uint)mbi.BaseAddress + mbi.RegionSize;

            if(newAddress <= pageStart)
                break;

            pageStart = newAddress;
        }
        while(numBytes);
    }

    //process file sections
    int pagecount = (int)pageVector.size();
    char curMod[MAX_MODULE_SIZE] = "";
    for(int i = pagecount - 1; i > -1; i--)
    {
        auto & currentPage = pageVector.at(i);
        if(!currentPage.info[0] || (scmp(curMod, currentPage.info) && !bListAllPages))   //there is a module
            continue; //skip non-modules
        strcpy(curMod, pageVector.at(i).info);
        uint base = ModBaseFromName(currentPage.info);
        if(!base)
            continue;
        std::vector<MODSECTIONINFO> sections;
        if(!ModSectionsFromAddr(base, &sections))
            continue;
        int SectionNumber = (int)sections.size();
        if(!SectionNumber)  //no sections = skip
            continue;
        if(!bListAllPages)  //normal view
        {
            MEMPAGE newPage;
            //remove the current module page (page = size of module at this point) and insert the module sections
            pageVector.erase(pageVector.begin() + i); //remove the SizeOfImage page
            for(int j = SectionNumber - 1; j > -1; j--)
            {
                const auto & currentSection = sections.at(j);
                memset(&newPage, 0, sizeof(MEMPAGE));
                VirtualQueryEx(fdProcessInfo->hProcess, (LPCVOID)currentSection.addr, &newPage.mbi, sizeof(MEMORY_BASIC_INFORMATION));
                uint SectionSize = currentSection.size;
                if(SectionSize % PAGE_SIZE)  //unaligned page size
                    SectionSize += PAGE_SIZE - (SectionSize % PAGE_SIZE); //fix this
                if(SectionSize)
                    newPage.mbi.RegionSize = SectionSize;
                sprintf_s(newPage.info, " \"%s\"", currentSection.name);
                pageVector.insert(pageVector.begin() + i, newPage);
            }
            //insert the module itself (the module header)
            memset(&newPage, 0, sizeof(MEMPAGE));
            VirtualQueryEx(fdProcessInfo->hProcess, (LPCVOID)base, &newPage.mbi, sizeof(MEMORY_BASIC_INFORMATION));
            strcpy_s(newPage.info, curMod);
            pageVector.insert(pageVector.begin() + i, newPage);
        }
        else //list all pages
        {
            uint start = (uint)currentPage.mbi.BaseAddress;
            uint end = start + currentPage.mbi.RegionSize;
            for(int j = 0, k = 0; j < SectionNumber; j++)
            {
                const auto & currentSection = sections.at(j);
                uint secStart = currentSection.addr;
                uint SectionSize = currentSection.size;
                if(SectionSize % PAGE_SIZE)  //unaligned page size
                    SectionSize += PAGE_SIZE - (SectionSize % PAGE_SIZE); //fix this
                uint secEnd = secStart + SectionSize;
                if(secStart >= start && secEnd <= end)  //section is inside the memory page
                {
                    if(k)
                        k += sprintf_s(currentPage.info + k, MAX_MODULE_SIZE - k, ",");
                    k += sprintf_s(currentPage.info + k, MAX_MODULE_SIZE - k, " \"%s\"", currentSection.name);
                }
                else if(start >= secStart && end <= secEnd)  //memory page is inside the section
                {
                    if(k)
                        k += sprintf_s(currentPage.info + k, MAX_MODULE_SIZE - k, ",");
                    k += sprintf_s(currentPage.info + k, MAX_MODULE_SIZE - k, " \"%s\"", currentSection.name);
                }
            }
        }
    }

    // Convert the vector to a map
    EXCLUSIVE_ACQUIRE(LockMemoryPages);
    memoryPages.clear();

    for(auto & page : pageVector)
    {
        uint start = (uint)page.mbi.BaseAddress;
        uint size = (uint)page.mbi.RegionSize;
        memoryPages.insert(std::make_pair(std::make_pair(start, start + size - 1), page));
    }
}

uint MemFindBaseAddr(uint Address, uint* Size, bool Refresh)
{
    // Update the memory map if needed
    if(Refresh)
        MemUpdateMap();

    SHARED_ACQUIRE(LockMemoryPages);

    // Search for the memory page address
    auto found = memoryPages.find(std::make_pair(Address, Address));

    if(found == memoryPages.end())
        return 0;

    // Return the allocation region size when requested
    if(Size)
        *Size = found->second.mbi.RegionSize;

    return found->first.first;
}

bool MemRead(uint BaseAddress, void* Buffer, uint Size, uint* NumberOfBytesRead)
{
    if(!MemIsCanonicalAddress(BaseAddress))
        return false;

    // Buffer must be supplied and size must be greater than 0
    if(!Buffer || Size <= 0)
        return false;

    // If the 'bytes read' parameter is null, use a temp
    SIZE_T bytesReadTemp = 0;

    if(!NumberOfBytesRead)
        NumberOfBytesRead = &bytesReadTemp;

    // Normal single-call read
    bool ret = MemoryReadSafe(fdProcessInfo->hProcess, (LPVOID)BaseAddress, Buffer, Size, NumberOfBytesRead);

    if(ret && *NumberOfBytesRead == Size)
        return true;

    // Read page-by-page (Skip if only 1 page exists)
    // If (SIZE > PAGE_SIZE) or (ADDRESS exceeds boundary), multiple reads will be needed
    SIZE_T pageCount = BYTES_TO_PAGES(Size);

    if(pageCount > 1)
    {
        // Determine the number of bytes between ADDRESS and the next page
        uint offset = 0;
        uint readBase = BaseAddress;
        uint readSize = ROUND_TO_PAGES(readBase) - readBase;

        // Reset the bytes read count
        *NumberOfBytesRead = 0;

        for(SIZE_T i = 0; i < pageCount; i++)
        {
            SIZE_T bytesRead = 0;

            if(MemoryReadSafe(fdProcessInfo->hProcess, (PVOID)readBase, ((PBYTE)Buffer + offset), readSize, &bytesRead))
                *NumberOfBytesRead += bytesRead;

            offset += readSize;
            readBase += readSize;

            Size -= readSize;
            readSize = min(PAGE_SIZE, Size);
        }
    }

    SetLastError(ERROR_PARTIAL_COPY);
    return (*NumberOfBytesRead > 0);
}

bool MemWrite(uint BaseAddress, const void* Buffer, uint Size, uint* NumberOfBytesWritten)
{
    if(!MemIsCanonicalAddress(BaseAddress))
        return false;

    // Buffer must be supplied and size must be greater than 0
    if(!Buffer || Size <= 0)
        return false;

    // If the 'bytes written' parameter is null, use a temp
    SIZE_T bytesWrittenTemp = 0;

    if(!NumberOfBytesWritten)
        NumberOfBytesWritten = &bytesWrittenTemp;

    // Try a regular WriteProcessMemory call
    bool ret = MemoryWriteSafe(fdProcessInfo->hProcess, (LPVOID)BaseAddress, Buffer, Size, NumberOfBytesWritten);

    if(ret && *NumberOfBytesWritten == Size)
        return true;

    // Write page-by-page (Skip if only 1 page exists)
    // See: MemRead
    SIZE_T pageCount = BYTES_TO_PAGES(Size);

    if(pageCount > 1)
    {
        // Determine the number of bytes between ADDRESS and the next page
        uint offset = 0;
        uint writeBase = BaseAddress;
        uint writeSize = ROUND_TO_PAGES(writeBase) - writeBase;

        // Reset the bytes read count
        *NumberOfBytesWritten = 0;

        for(SIZE_T i = 0; i < pageCount; i++)
        {
            SIZE_T bytesWritten = 0;

            if(MemoryWriteSafe(fdProcessInfo->hProcess, (PVOID)writeBase, ((PBYTE)Buffer + offset), writeSize, &bytesWritten))
                *NumberOfBytesWritten += bytesWritten;

            offset += writeSize;
            writeBase += writeSize;

            Size -= writeSize;
            writeSize = min(PAGE_SIZE, Size);
        }
    }

    SetLastError(ERROR_PARTIAL_COPY);
    return (*NumberOfBytesWritten > 0);
}

bool MemPatch(uint BaseAddress, const void* Buffer, uint Size, uint* NumberOfBytesWritten)
{
    // Buffer and size must be valid
    if(!Buffer || Size <= 0)
        return false;

    // Allocate the memory
    Memory<unsigned char*> oldData(Size, "mempatch:oldData");

    if(!MemRead(BaseAddress, oldData(), Size))
    {
        // If no memory can be read, no memory can be written. Fail out
        // of this function.
        return false;
    }

    for(uint i = 0; i < Size; i++)
        PatchSet(BaseAddress + i, oldData()[i], ((const unsigned char*)Buffer)[i]);

    return MemWrite(BaseAddress, Buffer, Size, NumberOfBytesWritten);
}

bool MemIsValidReadPtr(uint Address)
{
    unsigned char a = 0;
    return MemRead(Address, &a, sizeof(unsigned char));
}

bool MemIsCanonicalAddress(uint Address)
{
#ifndef _WIN64
    // 32-bit mode only supports 4GB max, so limits are
    // not an issue
    return true;
#else
    // The most-significant 16 bits must be all 1 or all 0.
    // (64 - 16) = 48bit linear address range.
    //
    // 0xFFFF800000000000 = Significant 16 bits set
    // 0x0000800000000000 = 48th bit set
    return (((Address & 0xFFFF800000000000) + 0x800000000000) & ~0x800000000000) == 0;
#endif // ndef _WIN64
}

uint MemAllocRemote(uint Address, uint Size, DWORD Type, DWORD Protect)
{
    return (uint)VirtualAllocEx(fdProcessInfo->hProcess, (LPVOID)Address, Size, Type, Protect);
}

bool MemFreeRemote(uint Address)
{
    return !!VirtualFreeEx(fdProcessInfo->hProcess, (LPVOID)Address, 0, MEM_RELEASE);
}

bool MemGetPageInfo(uint Address, MEMPAGE* PageInfo, bool Refresh)
{
    // Update the memory map if needed
    if (Refresh)
        MemUpdateMap();

    SHARED_ACQUIRE(LockMemoryPages);

    // Search for the memory page address
    auto found = memoryPages.find(std::make_pair(Address, Address));

    if (found == memoryPages.end())
        return false;

    // Return the data when possible
    if (PageInfo)
        *PageInfo = found->second;

    return true;
}

bool MemIsCodePage(uint Address, bool Refresh)
{
    MEMPAGE PageInfo;
    if (!MemGetPageInfo(Address, &PageInfo, Refresh))
        return false;
    return (PageInfo.mbi.Protect & PAGE_EXECUTE) == PAGE_EXECUTE;
}