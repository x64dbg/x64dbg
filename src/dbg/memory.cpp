/**
 @file memory.cpp

 @brief Implements the memory class.
 */

#include "memory.h"
#include "debugger.h"
#include "patches.h"
#include "threading.h"
#include "thread.h"
#include "module.h"
#include "taskthread.h"

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
    pageVector.reserve(200); //TODO: provide a better estimate
    {
        SIZE_T numBytes = 0;
        duint pageStart = 0;
        duint allocationBase = 0;

        do
        {
            // Query memory attributes
            MEMORY_BASIC_INFORMATION mbi;
            memset(&mbi, 0, sizeof(mbi));

            numBytes = VirtualQueryEx(fdProcessInfo->hProcess, (LPVOID)pageStart, &mbi, sizeof(mbi));

            // Only allow pages that are committed/reserved (exclude free memory)
            if(mbi.State != MEM_FREE)
            {
                auto bReserved = mbi.State == MEM_RESERVE; //check if the current page is reserved.
                auto bPrevReserved = pageVector.size() ? pageVector.back().mbi.State == MEM_RESERVE : false; //back if the previous page was reserved (meaning this one won't be so it has to be added to the map)
                // Only list allocation bases, unless if forced to list all
                if(bListAllPages || bReserved || bPrevReserved || allocationBase != duint(mbi.AllocationBase))
                {
                    // Set the new allocation base page
                    allocationBase = duint(mbi.AllocationBase);

                    MEMPAGE curPage;
                    memset(&curPage, 0, sizeof(MEMPAGE));
                    memcpy(&curPage.mbi, &mbi, sizeof(mbi));

                    if(bReserved)
                    {
                        if(duint(curPage.mbi.BaseAddress) != allocationBase)
                            sprintf_s(curPage.info, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Reserved (%p)")), allocationBase);
                        else
                            strcpy_s(curPage.info, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Reserved")));
                    }
                    else if(!ModNameFromAddr(pageStart, curPage.info, true))
                    {
                        // Module lookup failed; check if it's a file mapping
                        wchar_t szMappedName[sizeof(curPage.info)] = L"";
                        if((mbi.Type == MEM_MAPPED) &&
                                (GetMappedFileNameW(fdProcessInfo->hProcess, mbi.AllocationBase, szMappedName, MAX_MODULE_SIZE) != 0))
                        {
                            auto bFileNameOnly = false; //TODO: setting for this
                            auto fileStart = wcsrchr(szMappedName, L'\\');
                            if(bFileNameOnly && fileStart)
                                strcpy_s(curPage.info, StringUtils::Utf16ToUtf8(fileStart + 1).c_str());
                            else
                                strcpy_s(curPage.info, StringUtils::Utf16ToUtf8(szMappedName).c_str());
                        }
                    }

                    pageVector.push_back(curPage);
                }
                else
                {
                    // Otherwise append the page to the last created entry
                    if(pageVector.size())  //make sure to not dereference an invalid pointer
                        pageVector.back().mbi.RegionSize += mbi.RegionSize;
                }
            }

            // Calculate the next page start
            duint newAddress = duint(mbi.BaseAddress) + mbi.RegionSize;

            if(newAddress <= pageStart)
                break;

            pageStart = newAddress;
        }
        while(numBytes);
    }

    // Process file sections
    int pagecount = (int)pageVector.size();
    char curMod[MAX_MODULE_SIZE] = "";
    for(int i = pagecount - 1; i > -1; i--)
    {
        auto & currentPage = pageVector.at(i);
        if(!currentPage.info[0] || (scmp(curMod, currentPage.info) && !bListAllPages))   //there is a module
            continue; //skip non-modules
        strcpy(curMod, pageVector.at(i).info);
        if(!ModBaseFromName(currentPage.info))
            continue;
        auto base = duint(currentPage.mbi.AllocationBase);
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
                duint SectionSize = currentSection.size;
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
            duint start = (duint)currentPage.mbi.BaseAddress;
            duint end = start + currentPage.mbi.RegionSize;
            for(int j = 0, k = 0; j < SectionNumber; j++)
            {
                const auto & currentSection = sections.at(j);
                duint secStart = currentSection.addr;
                duint SectionSize = currentSection.size;
                if(SectionSize % PAGE_SIZE)  //unaligned page size
                    SectionSize += PAGE_SIZE - (SectionSize % PAGE_SIZE); //fix this
                duint secEnd = secStart + SectionSize;
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

    // Get a list of threads for information about Kernel/PEB/TEB/Stack ranges
    THREADLIST threadList;
    ThreadGetList(&threadList);

    for(auto & page : pageVector)
    {
        const duint pageBase = (duint)page.mbi.BaseAddress;
        const duint pageSize = (duint)page.mbi.RegionSize;

        // Check for windows specific data
        if(pageBase == 0x7FFE0000)
        {
            strcpy_s(page.info, "KUSER_SHARED_DATA");
            continue;
        }

        // Check in threads
        for(int i = 0; i < threadList.count; i++)
        {
            DWORD threadId = threadList.list[i].BasicInfo.ThreadId;

            // Mark TEB
            //
            // TebBase:      Points to 32/64 TEB
            // TebBaseWow64: Points to 64 TEB in a 32bit process
            duint tebBase = threadList.list[i].BasicInfo.ThreadLocalBase;
            duint tebBaseWow64 = tebBase - (2 * PAGE_SIZE);

            if(pageBase == tebBase)
            {
                sprintf_s(page.info, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Thread %X TEB")), threadId);
                break;
            }
            else if(pageBase == tebBaseWow64)
            {
#ifndef _WIN64
                if(pageSize == (3 * PAGE_SIZE))
                {
                    sprintf_s(page.info, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Thread %X WoW64 TEB")), threadId);
                    break;
                }
#endif // ndef _WIN64
            }

            // Mark stack
            //
            // Read TEB::Tib to get stack information
            NT_TIB tib;
            if(!ThreadGetTib(tebBase, &tib))
                continue;

            // The stack will be a specific range only, not always the base address
            duint stackAddr = (duint)tib.StackLimit;

            if(stackAddr >= pageBase && stackAddr < (pageBase + pageSize))
                sprintf_s(page.info, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Thread %X Stack")), threadId);
        }
    }

    // Only free thread data if it was allocated
    if(threadList.list)
        BridgeFree(threadList.list);

    // Convert the vector to a map
    EXCLUSIVE_ACQUIRE(LockMemoryPages);
    memoryPages.clear();

    for(auto & page : pageVector)
    {
        duint start = (duint)page.mbi.BaseAddress;
        duint size = (duint)page.mbi.RegionSize;
        memoryPages.insert(std::make_pair(std::make_pair(start, start + size - 1), page));
    }
}

static DWORD WINAPI memUpdateMap()
{
    if(DbgIsDebugging())
    {
        MemUpdateMap();
        GuiUpdateMemoryView();
    }
    return 0;
}

void MemUpdateMapAsync()
{
    static TaskThread_<decltype(&memUpdateMap)> memUpdateMapTask(&memUpdateMap, 1000);
    memUpdateMapTask.WakeUp();
}

duint MemFindBaseAddr(duint Address, duint* Size, bool Refresh)
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

bool MemRead(duint BaseAddress, void* Buffer, duint Size, duint* NumberOfBytesRead, bool cache)
{
    if(!MemIsCanonicalAddress(BaseAddress))
        return false;

    if(cache && !MemIsValidReadPtr(BaseAddress, cache))
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
        duint offset = 0;
        duint readBase = BaseAddress;
        duint readSize = ROUND_TO_PAGES(readBase) - readBase;

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

bool MemReadUnsafe(duint BaseAddress, void* Buffer, duint Size, duint* NumberOfBytesRead)
{
    SIZE_T read = 0;
    auto result = !!ReadProcessMemory(fdProcessInfo->hProcess, LPCVOID(BaseAddress), Buffer, Size, &read);
    if(NumberOfBytesRead)
        *NumberOfBytesRead = read;
    return result;
}

bool MemWrite(duint BaseAddress, const void* Buffer, duint Size, duint* NumberOfBytesWritten)
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
        duint offset = 0;
        duint writeBase = BaseAddress;
        duint writeSize = ROUND_TO_PAGES(writeBase) - writeBase;

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

bool MemPatch(duint BaseAddress, const void* Buffer, duint Size, duint* NumberOfBytesWritten)
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

    // Are we able to write on this page?
    if(MemWrite(BaseAddress, Buffer, Size, NumberOfBytesWritten))
    {
        for(duint i = 0; i < Size; i++)
            PatchSet(BaseAddress + i, oldData()[i], ((const unsigned char*)Buffer)[i]);

        // Done
        return true;
    }

    // Unable to write memory
    return false;
}

bool MemIsValidReadPtr(duint Address, bool cache)
{
    if(cache)
        return MemFindBaseAddr(Address, nullptr) != 0;
    unsigned char ch;
    return MemRead(Address, &ch, sizeof(ch));
}

bool MemIsValidReadPtrUnsafe(duint Address, bool cache)
{
    if(cache)
        return MemFindBaseAddr(Address, nullptr) != 0;
    unsigned char ch;
    return MemReadUnsafe(Address, &ch, sizeof(ch));
}

bool MemIsCanonicalAddress(duint Address)
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

bool MemIsCodePage(duint Address, bool Refresh)
{
    MEMPAGE pageInfo;
    if(!MemGetPageInfo(Address, &pageInfo, Refresh))
        return false;

    return (pageInfo.mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
}

duint MemAllocRemote(duint Address, duint Size, DWORD Type, DWORD Protect)
{
    return (duint)VirtualAllocEx(fdProcessInfo->hProcess, (LPVOID)Address, Size, Type, Protect);
}

bool MemFreeRemote(duint Address)
{
    return !!VirtualFreeEx(fdProcessInfo->hProcess, (LPVOID)Address, 0, MEM_RELEASE);
}

bool MemGetPageInfo(duint Address, MEMPAGE* PageInfo, bool Refresh)
{
    // Update the memory map if needed
    if(Refresh)
        MemUpdateMap();

    SHARED_ACQUIRE(LockMemoryPages);

    // Search for the memory page address
    auto found = memoryPages.find(std::make_pair(Address, Address));

    if(found == memoryPages.end())
        return false;

    // Return the data when possible
    if(PageInfo)
        *PageInfo = found->second;

    return true;
}

bool MemSetPageRights(duint Address, const char* Rights)
{
    // Align address to page base
    Address = PAGE_ALIGN(Address);

    // String -> bit mask
    DWORD protect;
    if(!MemPageRightsFromString(&protect, Rights))
        return false;

    DWORD oldProtect;
    return !!VirtualProtectEx(fdProcessInfo->hProcess, (void*)Address, PAGE_SIZE, protect, &oldProtect);
}

bool MemGetPageRights(duint Address, char* Rights)
{
    // Align address to page base
    Address = PAGE_ALIGN(Address);

    MEMORY_BASIC_INFORMATION mbi;
    memset(&mbi, 0, sizeof(MEMORY_BASIC_INFORMATION));

    if(!VirtualQueryEx(fdProcessInfo->hProcess, (void*)Address, &mbi, sizeof(mbi)))
        return false;

    return MemPageRightsToString(mbi.Protect, Rights);
}

bool MemPageRightsToString(DWORD Protect, char* Rights)
{
    if(!Protect)  //reserved pages don't have a protection (https://goo.gl/Izkk0c)
    {
        *Rights = '\0';
        return true;
    }
    switch(Protect & 0xFF)
    {
    case PAGE_NOACCESS:
        strcpy_s(Rights, RIGHTS_STRING_SIZE, "----");
        break;
    case PAGE_READONLY:
        strcpy_s(Rights, RIGHTS_STRING_SIZE, "-R--");
        break;
    case PAGE_READWRITE:
        strcpy_s(Rights, RIGHTS_STRING_SIZE, "-RW-");
        break;
    case PAGE_WRITECOPY:
        strcpy_s(Rights, RIGHTS_STRING_SIZE, "-RWC");
        break;
    case PAGE_EXECUTE:
        strcpy_s(Rights, RIGHTS_STRING_SIZE, "E---");
        break;
    case PAGE_EXECUTE_READ:
        strcpy_s(Rights, RIGHTS_STRING_SIZE, "ER--");
        break;
    case PAGE_EXECUTE_READWRITE:
        strcpy_s(Rights, RIGHTS_STRING_SIZE, "ERW-");
        break;
    case PAGE_EXECUTE_WRITECOPY:
        strcpy_s(Rights, RIGHTS_STRING_SIZE, "ERWC");
        break;
    default:
        memset(Rights, 0, RIGHTS_STRING_SIZE);
        break;
    }

    strcat_s(Rights, RIGHTS_STRING_SIZE, ((Protect & PAGE_GUARD) == PAGE_GUARD) ? "G" : "-");
    //  Rights[5] = ((Protect & PAGE_NOCACHE) == PAGE_NOCACHE) ? '' : '-';
    //  Rights[6] = ((Protect & PAGE_WRITECOMBINE) == PAGE_GUARD) ? '' : '-';

    return true;
}

bool MemPageRightsFromString(DWORD* Protect, const char* Rights)
{
    ASSERT_TRUE(strlen(Rights) >= 2);
    *Protect = 0;

    // Check for the PAGE_GUARD flag
    if(Rights[0] == 'G' || Rights[0] == 'g')
    {
        *Protect |= PAGE_GUARD;
        Rights++;
    }

    if(_strcmpi(Rights, "Execute") == 0)
        *Protect |= PAGE_EXECUTE;
    else if(_strcmpi(Rights, "ExecuteRead") == 0)
        *Protect |= PAGE_EXECUTE_READ;
    else if(_strcmpi(Rights, "ExecuteReadWrite") == 0)
        *Protect |= PAGE_EXECUTE_READWRITE;
    else if(_strcmpi(Rights, "ExecuteWriteCopy") == 0)
        *Protect |= PAGE_EXECUTE_WRITECOPY;
    else if(_strcmpi(Rights, "NoAccess") == 0)
        *Protect |= PAGE_NOACCESS;
    else if(_strcmpi(Rights, "ReadOnly") == 0)
        *Protect |= PAGE_READONLY;
    else if(_strcmpi(Rights, "ReadWrite") == 0)
        *Protect |= PAGE_READWRITE;
    else if(_strcmpi(Rights, "WriteCopy") == 0)
        *Protect |= PAGE_WRITECOPY;

    return (*Protect != 0);
}

bool MemFindInPage(const SimplePage & page, duint startoffset, const std::vector<PatternByte> & pattern, std::vector<duint> & results, duint maxresults)
{
    if(startoffset >= page.size || results.size() >= maxresults)
        return false;

    //TODO: memory read limit
    Memory<unsigned char*> data(page.size);
    if(!MemRead(page.address, data(), data.size()))
        return false;

    duint maxFind = maxresults;
    duint foundCount = results.size();
    duint i = 0;
    duint findSize = data.size() - startoffset;
    while(foundCount < maxFind)
    {
        duint foundoffset = patternfind(data() + startoffset + i, findSize - i, pattern);
        if(foundoffset == -1)
            break;
        i += foundoffset + 1;
        duint result = page.address + startoffset + i - 1;
        results.push_back(result);
        foundCount++;
    }
    return true;
}

bool MemFindInMap(const std::vector<SimplePage> & pages, const std::vector<PatternByte> & pattern, std::vector<duint> & results, duint maxresults, bool progress)
{
    duint count = 0;
    duint total = pages.size();
    for(const auto page : pages)
    {
        if(!MemFindInPage(page, 0, pattern, results, maxresults))
            continue;
        if(progress)
            GuiReferenceSetProgress(int(floor((float(count) / float(total)) * 100.0f)));
        if(results.size() >= maxresults)
            break;
        count++;
    }
    if(progress)
    {
        GuiReferenceSetProgress(100);
        GuiReferenceReloadData();
    }
    return true;
}

template<class T>
static T ror(T x, unsigned int moves)
{
    return (x >> moves) | (x << (sizeof(T) * 8 - moves));
}

template<class T>
static T rol(T x, unsigned int moves)
{
    return (x << moves) | (x >> (sizeof(T) * 8 - moves));
}

bool MemDecodePointer(duint* Pointer, bool vistaPlus)
{
    // Decode a pointer that has been encoded with a special "process cookie"
    // http://doxygen.reactos.org/dd/dc6/lib_2rtl_2process_8c_ad52c0f8f48ce65475a02a5c334b3e959.html
    typedef NTSTATUS(NTAPI * pfnNtQueryInformationProcess)(
        IN  HANDLE ProcessHandle,
        IN  LONG ProcessInformationClass,
        OUT PVOID ProcessInformation,
        IN  ULONG ProcessInformationLength,
        OUT PULONG ReturnLength
    );

    static auto NtQIP = (pfnNtQueryInformationProcess)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess");

    // Verify
    if(!NtQIP || !Pointer)
        return false;

    // Query the kernel for XOR key
    ULONG cookie;

    if(NtQIP(fdProcessInfo->hProcess, /* ProcessCookie */36, &cookie, sizeof(ULONG), nullptr) < 0)
        return false;

    // Pointer adjustment (Windows Vista+)
    if(vistaPlus)
#ifdef _WIN64
        *Pointer = ror(*Pointer, (0x40 - (cookie & 0x3F)) & 0xFF);
#else
        *Pointer = ror(*Pointer, (0x20 - (cookie & 0x1F)) & 0xFF);
#endif //_WIN64

    // XOR pointer with key
    *Pointer ^= cookie;

    return true;
}