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
#include "value.h"

#define PAGE_SHIFT              (12)
//#define PAGE_SIZE               (4096)
#define PAGE_ALIGN(Va)          ((ULONG_PTR)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))
#define BYTES_TO_PAGES(Size)    (((Size) >> PAGE_SHIFT) + (((Size) & (PAGE_SIZE - 1)) != 0))
#define ROUND_TO_PAGES(Size)    (((ULONG_PTR)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

static ULONG fallbackCookie = 0;
std::map<Range, MEMPAGE, RangeCompare> memoryPages;
bool bListAllPages = false;
bool bQueryWorkingSet = false;

// Get system information once.
static const SYSTEM_INFO systemInfo = []()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si;
}();

static std::vector<MEMPAGE> QueryMemPages()
{
    // First gather all possible pages in the memory range
    std::vector<MEMPAGE> pages;
    pages.reserve(200); //TODO: provide a better estimate

    SIZE_T numBytes = 0;
    duint pageStart = (duint)systemInfo.lpMinimumApplicationAddress;
    duint allocationBase = 0;

    do
    {
        if(!DbgIsDebugging())
            return {};

        // Query memory attributes
        MEMORY_BASIC_INFORMATION mbi;
        memset(&mbi, 0, sizeof(mbi));

        numBytes = VirtualQueryEx(fdProcessInfo->hProcess, (LPVOID)pageStart, &mbi, sizeof(mbi));

        // Only allow pages that are committed/reserved (exclude free memory)
        if(mbi.State != MEM_FREE)
        {
            auto bReserved = mbi.State == MEM_RESERVE; //check if the current page is reserved.
            auto bPrevReserved = pages.size() ? pages.back().mbi.State == MEM_RESERVE : false; //back if the previous page was reserved (meaning this one won't be so it has to be added to the map)
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

                pages.push_back(curPage);
            }
            else
            {
                // Otherwise append the page to the last created entry
                if(pages.size()) //make sure to not dereference an invalid pointer
                    pages.back().mbi.RegionSize += mbi.RegionSize;
            }
        }

        // Calculate the next page start
        duint newAddress = duint(mbi.BaseAddress) + mbi.RegionSize;

        if(newAddress <= pageStart)
            break;

        pageStart = newAddress;
    }
    while(numBytes);

    return pages;
}

static void ProcessFileSections(std::vector<MEMPAGE> & pageVector, std::vector<String> & errors)
{
    if(pageVector.empty())
        return;
    const auto pagecount = (int)pageVector.size();
    // NOTE: this iteration is backwards
    for(int i = pagecount - 1; i > -1; i--)
    {
        if(!DbgIsDebugging())
            return;

        auto & currentPage = pageVector.at(i);

        // Nothing to do for reserved or free pages
        if(currentPage.mbi.State == MEM_RESERVE || currentPage.mbi.State == MEM_FREE)
            continue;

        auto pageBase = duint(currentPage.mbi.BaseAddress);
        auto pageSize = currentPage.mbi.RegionSize;

        // Retrieve module info
        duint modBase = 0;
        std::vector<MODSECTIONINFO> sections;
        duint sectionAlignment = 0;
        duint modSize = 0;
        duint sizeOfImage = 0;
        {
            SHARED_ACQUIRE(LockModules);
            auto modInfo = ModInfoFromAddr(pageBase);

            // Nothing to do for non-modules
            if(!modInfo)
                continue;

            modBase = modInfo->base;
            modSize = modInfo->size;
            sections = modInfo->sections;
            if(modInfo->headers)
            {
                sectionAlignment = modInfo->headers->OptionalHeader.SectionAlignment;
                sizeOfImage = ROUND_TO_PAGES(modInfo->headers->OptionalHeader.SizeOfImage);
            }
            else
            {
                // This appears to happen under unknown circumstances
                // https://github.com/x64dbg/x64dbg/issues/2945
                // https://github.com/x64dbg/x64dbg/issues/2931
                sectionAlignment = 0x1000;
                sizeOfImage = modSize;

                // Spam the user with errors to hopefully get more information
                std::string summary;
                summary = StringUtils::sprintf("The module at %p (%s%s) triggers a weird bug, please report an issue\n",  modBase, modInfo->name, modInfo->extension);
                errors.push_back(std::move(summary));
            }
        }

        // Nothing to do if the module doesn't have sections
        if(sections.empty())
            continue;

        // It looks like a section alignment of 0x10000 becomes PAGE_SIZE in reality
        // The rest of the space is mapped as RESERVED
        sectionAlignment = min(sectionAlignment, PAGE_SIZE);

        auto sectionAlign = [sectionAlignment](duint value)
        {
            return (value + (sectionAlignment - 1)) & ~(sectionAlignment - 1);
        };

        // Align the sections
        for(size_t j = 0; j < sections.size(); j++)
        {
            auto & section = sections[j];
            section.addr = sectionAlign(section.addr);
            section.size = (section.size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);

            // Extend the last section to SizeOfImage under the right circumstances
            if(j + 1 == sections.size() && sectionAlignment < PAGE_SIZE)
            {
                auto totalSize = section.addr + section.size;
                totalSize -= modBase;
                if(sizeOfImage > totalSize)
                    section.size += sizeOfImage - totalSize;
            }
        }

        auto listIncludedSections = [&]()
        {
            size_t infoOffset = 0;
            // Display module name in first region (useful if PE header and first section have same protection)
            if(pageBase == modBase)
                infoOffset = strlen(currentPage.info);
            for(size_t j = 0; j < sections.size() && infoOffset + 1 < _countof(currentPage.info); j++)
            {
                const auto & currentSection = sections.at(j);
                duint sectionStart = currentSection.addr;
                duint sectionEnd = sectionStart + currentSection.size;
                // Check if the section and page overlap
                if(pageBase < sectionEnd && pageBase + pageSize > sectionStart)
                {
                    if(infoOffset)
                        infoOffset += _snprintf_s(currentPage.info + infoOffset, sizeof(currentPage.info) - infoOffset, _TRUNCATE, ",");
                    infoOffset += _snprintf_s(currentPage.info + infoOffset, sizeof(currentPage.info) - infoOffset, _TRUNCATE, " \"%s\"", currentSection.name);
                }
            }
        };

        // Section view
        if(!bListAllPages)
        {
            std::vector<MEMPAGE> newPages;
            newPages.reserve(sections.size() + 1);

            // Insert a page for the header if this is the header page
            if(pageBase == modBase)
            {
                MEMPAGE headerPage = {};
                VirtualQueryEx(fdProcessInfo->hProcess, (LPCVOID)modBase, &headerPage.mbi, sizeof(MEMORY_BASIC_INFORMATION));
                headerPage.mbi.RegionSize = min(sections.front().addr - modBase, pageSize);
                strcpy_s(headerPage.info, currentPage.info);
                newPages.push_back(headerPage);
            }

            // Insert pages for the sections
            for(const auto & section : sections)
            {
                // Only insert the sections that are within the current page
                if(section.addr >= pageBase && section.addr + section.size <= pageBase + currentPage.mbi.RegionSize)
                {
                    MEMPAGE sectionPage = {};
                    VirtualQueryEx(fdProcessInfo->hProcess, (LPCVOID)section.addr, &sectionPage.mbi, sizeof(MEMORY_BASIC_INFORMATION));
                    sectionPage.mbi.BaseAddress = (PVOID)section.addr;
                    sectionPage.mbi.RegionSize = section.size;
                    sprintf_s(sectionPage.info, " \"%s\"", section.name);
                    newPages.push_back(sectionPage);
                }
            }

            // Make sure the total size matches the new fake pages
            size_t totalSize = 0;
            for(const auto & page : newPages)
                totalSize += page.mbi.RegionSize;

            if(totalSize < pageSize)
            {
                // On Windows 11 24H2+, modules that support hotpatching contain an extra page mapped into their image region
                MEMPAGE imageExtensionPage = {};
                imageExtensionPage.mbi.BaseAddress = (PVOID)(pageBase + totalSize);
                imageExtensionPage.mbi.RegionSize = pageSize - totalSize;
                sprintf_s(imageExtensionPage.info, "ImageExtension");
                newPages.push_back(imageExtensionPage);
                totalSize = pageSize;
            }

            // Replace the single module page with the sections
            // newPages should never be empty, but try to avoid data loss if it is
            if(!newPages.empty() && totalSize == pageSize)
            {
                pageVector.erase(pageVector.begin() + i); // remove the SizeOfImage page
                pageVector.insert(pageVector.begin() + i, newPages.begin(), newPages.end());
            }
            else
            {
                // Report an error to make it easier to debug
                auto summary = StringUtils::sprintf("Error replacing page: %p[%p] (%s)\n", pageBase, pageSize, currentPage.info);
                summary += "Sections:\n";
                for(const auto & section : sections)
                    summary += StringUtils::sprintf("  \"%s\": %p[%p]\n", section.name, section.addr, section.size);
                summary += "New pages:\n";
                for(const auto & page : newPages)
                    summary += StringUtils::sprintf(" \"%s\": %p[%p]\n", page.info, page.mbi.BaseAddress, page.mbi.RegionSize);
                summary += "Please report an issue!\n";
                errors.push_back(std::move(summary));

                // Fall back to the 'list all pages' algorithm
                listIncludedSections();
            }
        }
        // List all pages
        else
        {
            listIncludedSections();
        }
    }
}

#define MAX_HEAPS 1000

static void ProcessSystemPages(std::vector<MEMPAGE> & pageVector)
{
    THREADLIST threadList;
    ThreadGetList(&threadList);
    auto pebBase = (duint)GetPEBLocation(fdProcessInfo->hProcess);
    std::vector<duint> stackAddrs;
    for(int i = 0; i < threadList.count; i++)
    {
        DWORD threadId = threadList.list[i].BasicInfo.ThreadId;

        // Read TEB::Tib to get stack information
        NT_TIB tib;
        if(!ThreadGetTib(threadList.list[i].BasicInfo.ThreadLocalBase, &tib))
            tib.StackLimit = nullptr;

        // The stack will be a specific range only, not always the base address
        stackAddrs.push_back((duint)tib.StackLimit);
    }

    ULONG NumberOfHeaps = 0;
    MemRead(pebBase + offsetof(PEB, NumberOfHeaps), &NumberOfHeaps, sizeof(NumberOfHeaps));
    duint ProcessHeapsPtr = 0;
    MemRead(pebBase + offsetof(PEB, ProcessHeaps), &ProcessHeapsPtr, sizeof(ProcessHeapsPtr));

    duint ProcessHeaps[MAX_HEAPS] = {};
    auto HeapCount = min(_countof(ProcessHeaps), NumberOfHeaps);
    MemRead(ProcessHeapsPtr, ProcessHeaps, sizeof(duint) * HeapCount);
    std::unordered_map<duint, uint32_t> processHeapIds;
    processHeapIds.reserve(HeapCount);
    for(uint32_t i = 0; i < HeapCount; i++)
        processHeapIds.emplace(ProcessHeaps[i], i);

    for(auto & page : pageVector)
    {
        const duint pageBase = (duint)page.mbi.BaseAddress;
        const duint pageSize = (duint)page.mbi.RegionSize;

        auto inRange = [pageBase, pageSize](duint addr)
        {
            return addr >= pageBase && addr < pageBase + pageSize;
        };

        auto appendInfo = [&page](const char* str)
        {
            if(*page.info)
            {
                strncat_s(page.info, ", ", _TRUNCATE);
            }
            strncat_s(page.info, str, _TRUNCATE);
        };

        // Check for windows specific data
        if(inRange(0x7FFE0000))
        {
            appendInfo("KUSER_SHARED_DATA");
            continue;
        }

        // Mark PEB
        if(inRange(pebBase))
        {
            appendInfo("PEB");
        }

        // Check in threads
        char temp[256] = "";
        for(int i = 0; i < threadList.count; i++)
        {
            DWORD threadId = threadList.list[i].BasicInfo.ThreadId;
            auto tidStr = formatpidtid(threadId);

            // Mark TEB
            //
            // TebBase:      Points to 32/64 TEB
            // TebBaseWow64: Points to 64 TEB in a 32bit process
            duint tebBase = threadList.list[i].BasicInfo.ThreadLocalBase;
            duint tebBaseWow64 = tebBase - (2 * PAGE_SIZE);

            if(inRange(tebBase))
            {
                sprintf_s(temp, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "TEB (%s)")), tidStr.c_str());
                appendInfo(temp);
            }

            if(inRange(tebBaseWow64))
            {
#ifndef _WIN64
                sprintf_s(temp, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "WoW64 TEB (%s)")), tidStr.c_str());
                appendInfo(temp);
#endif //_WIN64
            }

            // The stack will be a specific range only, not always the base address
            duint stackAddr = stackAddrs[i];
            if(inRange(stackAddr))
            {
                sprintf_s(temp, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Stack (%s)")), tidStr.c_str());
                appendInfo(temp);
            }
        }

        auto heapItr = processHeapIds.find(pageBase);
        if(heapItr != processHeapIds.end())
        {
            sprintf_s(temp, "Heap (ID %u)", heapItr->second);
            appendInfo(temp);
        }
    }

    // Only free thread data if it was allocated
    if(threadList.list)
        BridgeFree(threadList.list);
}

void MemUpdateMap()
{
    // First gather all possible pages in the memory range
    std::vector<MEMPAGE> pageVector = QueryMemPages();

    // Process file sections
    std::vector<String> errors;
    ProcessFileSections(pageVector, errors);

    // Get a list of threads for information about Kernel/PEB/TEB/Stack ranges
    ProcessSystemPages(pageVector);

    // Convert the vector to a map
    EXCLUSIVE_ACQUIRE(LockMemoryPages);
    memoryPages.clear();

    // Print the annoying errors only once
    static std::unordered_set<String> shownErrors;
    for(auto & error : errors)
    {
        if(shownErrors.count(error) == 0)
        {
            GuiAddLogMessage(error.c_str());
            shownErrors.insert(std::move(error));
        }
    }

    for(const auto & page : pageVector)
    {
        duint start = (duint)page.mbi.BaseAddress;
        duint size = (duint)page.mbi.RegionSize;
        memoryPages.emplace(std::make_pair(start, start + size - 1), page);
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

duint MemFindBaseAddr(duint Address, duint* Size, bool Refresh, bool FindReserved)
{
    // Update the memory map if needed
    if(Refresh)
        MemUpdateMap();

    SHARED_ACQUIRE(LockMemoryPages);

    // Search for the memory page address
    auto found = memoryPages.find(std::make_pair(Address, Address));

    if(found == memoryPages.end())
        return 0;

    if(!FindReserved && found->second.mbi.State == MEM_RESERVE) //check if the current page is reserved.
        return 0;

    // Return the allocation region size when requested
    if(Size)
        *Size = found->second.mbi.RegionSize;

    return found->first.first;
}

//http://www.triplefault.io/2017/08/detecting-debuggers-by-abusing-bad.html
//TODO: name this function properly
static bool IgnoreThisRead(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
{
    typedef BOOL(WINAPI * QUERYWORKINGSETEX)(HANDLE, PVOID, DWORD);
    static auto fnQueryWorkingSetEx = QUERYWORKINGSETEX(GetProcAddress(GetModuleHandleW(L"psapi.dll"), "QueryWorkingSetEx"));
    if(!bQueryWorkingSet || !fnQueryWorkingSetEx)
        return false;
    PSAPI_WORKING_SET_EX_INFORMATION wsi;
    wsi.VirtualAddress = (PVOID)PAGE_ALIGN(lpBaseAddress);
    if(fnQueryWorkingSetEx(hProcess, &wsi, sizeof(wsi)) && !wsi.VirtualAttributes.Valid)
    {
        MEMORY_BASIC_INFORMATION mbi;
        if(VirtualQueryEx(hProcess, wsi.VirtualAddress, &mbi, sizeof(mbi)) && mbi.State == MEM_COMMIT/* && mbi.Type == MEM_PRIVATE*/)
        {
            memset(lpBuffer, 0, nSize);
            if(lpNumberOfBytesRead)
                *lpNumberOfBytesRead = nSize;
            return true;
        }
    }
    return false;
}

bool MemoryReadSafePage(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
{
#if 0
    //TODO: remove when proven stable, this function checks if reads are always within page boundaries
    auto base = duint(lpBaseAddress);
    if(nSize > PAGE_SIZE - (base & (PAGE_SIZE - 1)))
        __debugbreak();
#endif
    if(IgnoreThisRead(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead))
        return true;
    return MemoryReadSafe(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
}

bool MemRead(duint BaseAddress, void* Buffer, duint Size, duint* NumberOfBytesRead, bool cache)
{
    if(!MemIsCanonicalAddress(BaseAddress) || !DbgIsDebugging())
        return false;

    if(cache && !MemIsValidReadPtr(BaseAddress, true))
        return false;

    if(!Buffer)
        return false;

    duint bytesReadTemp = 0;
    if(!NumberOfBytesRead)
        NumberOfBytesRead = &bytesReadTemp;
    *NumberOfBytesRead = 0;

    duint offset = 0;
    duint requestedSize = Size;
    duint sizeLeftInFirstPage = PAGE_SIZE - (BaseAddress & (PAGE_SIZE - 1));
    duint readSize = min(sizeLeftInFirstPage, requestedSize);

    while(readSize)
    {
        SIZE_T bytesRead = 0;
        auto readSuccess = MemoryReadSafePage(fdProcessInfo->hProcess, (PVOID)(BaseAddress + offset), (PBYTE)Buffer + offset, readSize, &bytesRead);
        *NumberOfBytesRead += bytesRead;
        if(!readSuccess)
            break;

        offset += readSize;
        requestedSize -= readSize;
        readSize = min(PAGE_SIZE, requestedSize);

        if(readSize && (BaseAddress + offset) % PAGE_SIZE)
            __debugbreak(); //TODO: remove when proven stable, this checks if (BaseAddress + offset) is aligned to PAGE_SIZE after the first call
    }

    auto success = *NumberOfBytesRead == Size;
    SetLastError(success ? ERROR_SUCCESS : ERROR_PARTIAL_COPY);
    return success;
}

bool MemReadUnsafePage(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
{
    //TODO: remove when proven stable, this function checks if reads are always within page boundaries
    auto base = duint(lpBaseAddress);
    if(nSize > PAGE_SIZE - (base & (PAGE_SIZE - 1)))
        __debugbreak();
    if(IgnoreThisRead(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead))
        return true;
    return !!ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
}

bool MemReadUnsafe(duint BaseAddress, void* Buffer, duint Size, duint* NumberOfBytesRead)
{
    if(!MemIsCanonicalAddress(BaseAddress) || BaseAddress < PAGE_SIZE || !DbgIsDebugging())
        return false;

    if(!Buffer)
        return false;

    duint bytesReadTemp = 0;
    if(!NumberOfBytesRead)
        NumberOfBytesRead = &bytesReadTemp;
    *NumberOfBytesRead = 0;

    duint offset = 0;
    duint requestedSize = Size;
    duint sizeLeftInFirstPage = PAGE_SIZE - (BaseAddress & (PAGE_SIZE - 1));
    duint readSize = min(sizeLeftInFirstPage, requestedSize);

    while(readSize)
    {
        SIZE_T bytesRead = 0;
        auto readSuccess = MemReadUnsafePage(fdProcessInfo->hProcess, (PVOID)(BaseAddress + offset), (PBYTE)Buffer + offset, readSize, &bytesRead);
        *NumberOfBytesRead += bytesRead;
        if(!readSuccess)
            break;

        offset += readSize;
        requestedSize -= readSize;
        readSize = min(PAGE_SIZE, requestedSize);

        if(readSize && (BaseAddress + offset) % PAGE_SIZE)
            __debugbreak(); //TODO: remove when proven stable, this checks if (BaseAddress + offset) is aligned to PAGE_SIZE after the first call
    }

    auto success = *NumberOfBytesRead == Size;
    SetLastError(success ? ERROR_SUCCESS : ERROR_PARTIAL_COPY);
    return success;
}

static bool MemoryWriteSafePage(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten)
{
    //TODO: remove when proven stable, this function checks if writes are always within page boundaries
    auto base = duint(lpBaseAddress);
    if(nSize > PAGE_SIZE - (base & (PAGE_SIZE - 1)))
        __debugbreak();
    return MemoryWriteSafe(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
}

bool MemWrite(duint BaseAddress, const void* Buffer, duint Size, duint* NumberOfBytesWritten)
{
    if(!MemIsCanonicalAddress(BaseAddress))
        return false;

    if(!Buffer)
        return false;

    SIZE_T bytesWrittenTemp = 0;
    if(!NumberOfBytesWritten)
        NumberOfBytesWritten = &bytesWrittenTemp;
    *NumberOfBytesWritten = 0;

    duint offset = 0;
    duint requestedSize = Size;
    duint sizeLeftInFirstPage = PAGE_SIZE - (BaseAddress & (PAGE_SIZE - 1));
    duint writeSize = min(sizeLeftInFirstPage, requestedSize);

    while(writeSize)
    {
        SIZE_T bytesWritten = 0;
        auto writeSuccess = MemoryWriteSafePage(fdProcessInfo->hProcess, (PVOID)(BaseAddress + offset), (PBYTE)Buffer + offset, writeSize, &bytesWritten);
        *NumberOfBytesWritten += bytesWritten;
        if(!writeSuccess)
            break;

        offset += writeSize;
        requestedSize -= writeSize;
        writeSize = min(PAGE_SIZE, requestedSize);

        if(writeSize && (BaseAddress + offset) % PAGE_SIZE)
            __debugbreak(); //TODO: remove when proven stable, this checks if (BaseAddress + offset) is aligned to PAGE_SIZE after the first call
    }

    auto success = *NumberOfBytesWritten == Size;
    SetLastError(success ? ERROR_SUCCESS : ERROR_PARTIAL_COPY);
    return success;
}

bool MemPatch(duint BaseAddress, const void* Buffer, duint Size, duint* NumberOfBytesWritten)
{
    // Buffer must be valid
    if(!Buffer)
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
#endif //_WIN64
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
    DWORD protect;
    if(!MemPageRightsFromString(&protect, Rights))
        return false;

    return MemSetProtect(Address, protect, PAGE_SIZE);
}

bool MemGetPageRights(duint Address, char* Rights)
{
    unsigned int protect = 0;
    if(!MemGetProtect(Address, true, false, &protect))
        return false;

    return MemPageRightsToString(protect, Rights);
}

bool MemPageRightsToString(DWORD Protect, char* Rights)
{
    if(!Protect) //reserved pages don't have a protection (https://goo.gl/Izkk0c)
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

    // Verify
    if(!Pointer)
        return false;

    // Query the kernel for XOR key
    ULONG cookie;

    if(!NT_SUCCESS(NtQueryInformationProcess(fdProcessInfo->hProcess, ProcessCookie, &cookie, sizeof(ULONG), nullptr)))
    {
        if(!fallbackCookie)
            return false;
        cookie = fallbackCookie;
    }

    // Pointer adjustment (Windows Vista+)
    if(vistaPlus)
#ifdef _WIN64
        * Pointer = ror(*Pointer, (0x40 - (cookie & 0x3F)) & 0xFF);
#else
        * Pointer = ror(*Pointer, (0x20 - (cookie & 0x1F)) & 0xFF);
#endif //_WIN64

    // XOR pointer with key
    * Pointer ^= cookie;

    return true;
}

void MemInitRemoteProcessCookie(ULONG cookie)
{
    // Clear previous session's cookie
    fallbackCookie = cookie;

    // Allow a non-zero cookie to ignore the brute force
    if(fallbackCookie)
        return;

    // Windows XP/Vista/7 are unable to obtain remote process cookies using NtQueryInformationProcess
    // Guess the cookie by brute-forcing all possible hashes and validate it using known encodings
    duint RtlpUnhandledExceptionFilter = 0;
    duint UnhandledExceptionFilter = 0;
    duint SingleHandler = 0;
    duint DefaultHandler = 0;

    auto RtlpUnhandledExceptionFilterSymbol = ArchValue("_RtlpUnhandledExceptionFilter", "RtlpUnhandledExceptionFilter");
    auto UnhandledExceptionFilterSymbol = ArchValue("_UnhandledExceptionFilter@4", "UnhandledExceptionFilter");
    auto SingleHandlerSymbol = ArchValue("_SingleHandler", "SingleHandler");
    auto DefaultHandlerSymbol = ArchValue("_DefaultHandler@4", "DefaultHandler");

    if(!valfromstring(RtlpUnhandledExceptionFilterSymbol, &RtlpUnhandledExceptionFilter) ||
            !valfromstring(UnhandledExceptionFilterSymbol, &UnhandledExceptionFilter) ||
            !valfromstring(SingleHandlerSymbol, &SingleHandler) ||
            !valfromstring(DefaultHandlerSymbol, &DefaultHandler))
        return;

    // Pointer encodings known at System Breakpoint. These may be invalid if attaching to a process.
    // *ntdll.RtlpUnhandledExceptionFilter = EncodePointer(kernel32.UnhandledExceptionFilter)
    duint encodedUnhandledExceptionFilter = 0;
    if(!MemRead(RtlpUnhandledExceptionFilter, &encodedUnhandledExceptionFilter, sizeof(encodedUnhandledExceptionFilter)))
        return;

    // *kernel32.SingleHandler = EncodePointer(kernel32.DefaultHandler)
    duint encodedDefaultHandler = 0;
    if(!MemRead(SingleHandler, &encodedDefaultHandler, sizeof(encodedDefaultHandler)))
        return;

    auto isValidEncoding = [](ULONG CookieGuess, duint EncodedValue, duint DecodedValue)
    {
        return DecodedValue == (ror(EncodedValue, 0x40 - (CookieGuess & 0x3F)) ^ CookieGuess);
    };

    cookie = 0;
    for(int i = 64; i > 0; i--)
    {
        const ULONG guess = ULONG(ror(encodedDefaultHandler, i) ^ DefaultHandler);
        if(isValidEncoding(guess, encodedUnhandledExceptionFilter, UnhandledExceptionFilter) &&
                isValidEncoding(guess, encodedDefaultHandler, DefaultHandler))
        {
            // cookie collision, we're unable to determine which cookie is correct
            if(cookie && guess != cookie)
                return;
            cookie = guess;
        }
    }

    fallbackCookie = cookie;
}

//Workaround for modules that have holes between sections, it keeps parts it couldn't read the same as the input
bool MemReadDumb(duint BaseAddress, void* Buffer, duint Size)
{
    if(!MemIsCanonicalAddress(BaseAddress) || !Buffer || !Size)
        return false;

    duint offset = 0;
    duint requestedSize = Size;
    duint sizeLeftInFirstPage = PAGE_SIZE - (BaseAddress & (PAGE_SIZE - 1));
    duint readSize = min(sizeLeftInFirstPage, requestedSize);

    bool success = true;
    while(readSize)
    {
        SIZE_T bytesRead = 0;
        if(!MemoryReadSafePage(fdProcessInfo->hProcess, (PVOID)(BaseAddress + offset), (PBYTE)Buffer + offset, readSize, &bytesRead))
            success = false;
        offset += readSize;
        requestedSize -= readSize;
        readSize = min(PAGE_SIZE, requestedSize);
    }
    return success;
}

bool MemGetProtect(duint Address, bool Reserved, bool Cache, unsigned int* Protect)
{
    if(!Protect)
        return false;

    // Align address to page base
    Address = PAGE_ALIGN(Address);

    if(Cache)
    {
        SHARED_ACQUIRE(LockMemoryPages);
        auto found = memoryPages.find({ Address, Address });
        if(found == memoryPages.end())
            return false;
        if(!Reserved && found->second.mbi.State == MEM_RESERVE) //check if the current page is reserved.
            return false;

        *Protect = found->second.mbi.Protect;
    }
    else
    {
        MEMORY_BASIC_INFORMATION mbi;
        memset(&mbi, 0, sizeof(MEMORY_BASIC_INFORMATION));

        if(!VirtualQueryEx(fdProcessInfo->hProcess, (void*)Address, &mbi, sizeof(mbi)))
            return false;

        *Protect = mbi.Protect;
    }

    return true;
}

static void MemSplitRange(duint Address, duint Size)
{
    Size = PAGE_ALIGN(Size);

    auto found = memoryPages.find({ Address, Address });
    if(found == memoryPages.end())
        return;

    auto & range = found->first;
    if(range.second > (Address + Size))
    {
        // Requires split.
        MEMPAGE firstPage = found->second;
        firstPage.mbi.RegionSize = Size;
        Range firstRange{ Address, Address + Size };

        MEMPAGE secondPage = found->second;
        secondPage.mbi.RegionSize = (range.second - (Address + Size));
        Range secondRange{ Address + Size, range.second };

        memoryPages.erase(found);
        memoryPages.emplace(firstRange, firstPage);
        memoryPages.emplace(secondRange, secondPage);
    }
}

bool MemSetProtect(duint Address, unsigned int Protect, duint Size)
{
    // Align address to page base
    Address = PAGE_ALIGN(Address);

    DWORD oldProtect;
    if(VirtualProtectEx(fdProcessInfo->hProcess, (void*)Address, Size, Protect, &oldProtect) == FALSE)
        return false;

    // Update cache.
    SHARED_ACQUIRE(LockMemoryPages);

    // When the protection changes we can't treat this as a single page anymore.
    if(bListAllPages)
    {
        // But we only need to split if the view requests it.
        MemSplitRange(Address, Size);
    }

    // Update protection info.
    auto found = memoryPages.find({ Address, Address });
    while(found != memoryPages.end())
    {
        if(found->first.second > (Address + Size))
            break;
        found->second.mbi.Protect = Protect;
        found++;
    }

    return true;
}
