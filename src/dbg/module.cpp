#include "module.h"
#include "TitanEngine/TitanEngine.h"
#include "ntdll/ntdll.h"
#include "threading.h"
#include "symbolinfo.h"
#include "murmurhash.h"
#include "symbolsourcedia.h"
#include "memory.h"
#include "label.h"
#include <algorithm>
#include <shlwapi.h>
#include "console.h"
#include "debugger.h"
#include <memory>

std::map<Range, std::unique_ptr<MODINFO>, RangeCompare> modinfo;
std::unordered_map<duint, std::string> hashNameMap;

// RtlImageNtHeaderEx is much better than the non-Ex version due to stricter validation, but isn't available on XP x86.
// This is essentially a fallback replacement that does the same thing
static NTSTATUS ImageNtHeaders(duint base, duint size, PIMAGE_NT_HEADERS* outHeaders)
{
    PIMAGE_NT_HEADERS ntHeaders;

    __try
    {
        if(base == 0 || outHeaders == nullptr)
            return STATUS_INVALID_PARAMETER;
        if(size < sizeof(IMAGE_DOS_HEADER))
            return STATUS_INVALID_IMAGE_FORMAT;

        const PIMAGE_DOS_HEADER dosHeaders = (PIMAGE_DOS_HEADER)base;
        if(dosHeaders->e_magic != IMAGE_DOS_SIGNATURE)
            return STATUS_INVALID_IMAGE_FORMAT;

        const ULONG e_lfanew = dosHeaders->e_lfanew;
        const ULONG sizeOfPeSignature = sizeof('PE00');
        if(e_lfanew >= size ||
                e_lfanew >= (ULONG_MAX - sizeOfPeSignature - sizeof(IMAGE_FILE_HEADER)) ||
                (e_lfanew + sizeOfPeSignature + sizeof(IMAGE_FILE_HEADER)) >= size)
            return STATUS_INVALID_IMAGE_FORMAT;

        ntHeaders = (PIMAGE_NT_HEADERS)((PCHAR)base + e_lfanew);

        // RtlImageNtHeaderEx verifies that the range does not cross the UM <-> KM boundary here,
        // but it would cost a syscall to query this address as it varies between OS versions // TODO: or do we already have this info somewhere?
        if(!MemIsCanonicalAddress((duint)ntHeaders + sizeof(IMAGE_NT_HEADERS)))
            return STATUS_INVALID_IMAGE_FORMAT;
        if(ntHeaders->Signature != IMAGE_NT_SIGNATURE)
            return STATUS_INVALID_IMAGE_FORMAT;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    *outHeaders = ntHeaders;
    return STATUS_SUCCESS;
}

// Use only with SEC_COMMIT mappings, not SEC_IMAGE! (in that case, just do VA = base + rva...)
static ULONG64 RvaToVa(ULONG64 base, PIMAGE_NT_HEADERS ntHeaders, ULONG64 rva)
{
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);
    for(WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i)
    {
        if(rva >= section->VirtualAddress &&
                rva < section->VirtualAddress + section->SizeOfRawData)
        {
            ASSERT_TRUE(rva != 0); // Following garbage in is garbage out, RVA 0 should always yield VA 0
            return base + (rva - section->VirtualAddress) + section->PointerToRawData;
        }
        section++;
    }
    return 0;
}

static void ReadExportDirectory(MODINFO & Info, ULONG_PTR FileMapVA)
{
    // Get the export directory and its size
    ULONG exportDirSize;
    auto exportDir = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData((PVOID)FileMapVA,
                     FALSE,
                     IMAGE_DIRECTORY_ENTRY_EXPORT,
                     &exportDirSize);
    if(exportDirSize == 0 || exportDir == nullptr || exportDir->NumberOfFunctions == 0)
        return;

    auto rva2offset = [&Info](ULONG64 rva)
    {
        return RvaToVa(0, Info.headers, rva);
    };

    auto addressOfFunctionsOffset = rva2offset(exportDir->AddressOfFunctions);
    if(!addressOfFunctionsOffset)
        return;

    auto addressOfFunctions = PDWORD(addressOfFunctionsOffset + FileMapVA);

    auto addressOfNamesOffset = rva2offset(exportDir->AddressOfNames);
    auto addressOfNames = PDWORD(addressOfNamesOffset ? addressOfNamesOffset + FileMapVA : 0);

    auto addressOfNameOrdinalsOffset = rva2offset(exportDir->AddressOfNameOrdinals);
    auto addressOfNameOrdinals = PWORD(addressOfNameOrdinalsOffset ? addressOfNameOrdinalsOffset + FileMapVA : 0);

    Info.exports.reserve(exportDir->NumberOfFunctions);
    Info.exportOrdinalBase = exportDir->Base;

    // TODO: 'invalid address' below means an RVA that is obviously invalid, like being greater than SizeOfImage.
    // In that case rva2offset will return a VA of 0 and we can ignore it. However the ntdll loader (and this code)
    // will still crash on corrupt or malicious inputs that are seemingly valid. Find out how common this is
    // (i.e. does it warrant wrapping everything in try/except?) and whether there are better solutions.
    // Note that we're loading this file because the debuggee did; that makes it at least somewhat plausible that we will also survive
    for(DWORD i = 0; i < exportDir->NumberOfFunctions; i++)
    {
        Info.exports.emplace_back();
        auto & entry = Info.exports.back();
        entry.ordinal = i + exportDir->Base;
        entry.rva = addressOfFunctions[i];
        const auto entryVa = RvaToVa(FileMapVA, Info.headers, entry.rva);
        entry.forwarded = entryVa >= (ULONG64)exportDir;
        if(entry.forwarded && entryVa < (ULONG64)exportDir + exportDirSize)
        {
            auto forwardNameOffset = rva2offset(entry.rva);
            if(forwardNameOffset) // Silent ignore (1) by ntdll loader: invalid forward names or addresses of forward names
                entry.forwardName = String((const char*)(forwardNameOffset + FileMapVA));
        }
    }

    for(DWORD i = 0; i < exportDir->NumberOfNames; i++)
    {
        DWORD index = addressOfNameOrdinals[i];
        if(index < Info.exports.size()) // Silent ignore (2) by ntdll loader: bogus AddressOfNameOrdinals indices
        {
            auto nameOffset = rva2offset(addressOfNames[i]);
            if(nameOffset) // Silent ignore (3) by ntdll loader: invalid names or addresses of names
                Info.exports[index].name = String((const char*)(nameOffset + FileMapVA));
        }
    }

    // prepare sorted vectors
    Info.exportsByName.resize(Info.exports.size());
    Info.exportsByRva.resize(Info.exports.size());
    for(size_t i = 0; i < Info.exports.size(); i++)
    {
        Info.exportsByName[i] = i;
        Info.exportsByRva[i] = i;
    }

    std::sort(Info.exportsByName.begin(), Info.exportsByName.end(), [&Info](size_t a, size_t b)
    {
        return Info.exports.at(a).name < Info.exports.at(b).name;
    });

    std::sort(Info.exportsByRva.begin(), Info.exportsByRva.end(), [&Info](size_t a, size_t b)
    {
        return Info.exports.at(a).rva < Info.exports.at(b).rva;
    });
}

static void ReadImportDirectory(MODINFO & Info, ULONG_PTR FileMapVA)
{
    // Get the import directory and its size
    ULONG importDirSize;
    auto importDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData((PVOID)FileMapVA,
                            FALSE,
                            IMAGE_DIRECTORY_ENTRY_IMPORT,
                            &importDirSize);
    if(importDirSize == 0 || importDescriptor == nullptr)
        return;

    const ULONG64 ordinalFlag = IMAGE64(Info.headers) ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32;
    auto rva2offset = [&Info](ULONG64 rva)
    {
        return RvaToVa(0, Info.headers, rva);
    };

    for(size_t moduleIndex = 0; importDescriptor->Name != 0; ++importDescriptor, ++moduleIndex)
    {
        auto moduleNameOffset = rva2offset(importDescriptor->Name);
        if(!moduleNameOffset) // If the module name VA is invalid, the loader crashes with an access violation. Try to avoid this
            break;

        // Prefer OFTs over FTs. If they differ, the FT is a bounded import and has a 0% chance of being correct due to ASLR
        auto thunkOffset = rva2offset(importDescriptor->OriginalFirstThunk != 0
                                      ? importDescriptor->OriginalFirstThunk
                                      : importDescriptor->FirstThunk);

        // If there is no FT, the loader ignores the descriptor and moves on to the next DLL instead of crashing. Wise move
        if(importDescriptor->FirstThunk == 0)
            continue;

        Info.importModules.emplace_back((const char*)(moduleNameOffset + FileMapVA));
        unsigned char* thunkData = (unsigned char*)FileMapVA + thunkOffset;

        for(auto iatRva = importDescriptor->FirstThunk;
                THUNK_VAL(Info.headers, thunkData, u1.AddressOfData) != 0;
                thunkData += IMAGE64(Info.headers) ? sizeof(IMAGE_THUNK_DATA64) : sizeof(IMAGE_THUNK_DATA32), iatRva += IMAGE64(Info.headers) ? sizeof(ULONG64) : sizeof(DWORD))
        {
            // Get AddressOfData, check whether the ordinal flag was set, and then strip it because the RVA is not valid with it set
            ULONG64 addressOfDataValue = THUNK_VAL(Info.headers, thunkData, u1.AddressOfData);
            const bool ordinalFlagSet = (addressOfDataValue & ordinalFlag) == ordinalFlag; // NB: both variables are ULONG64 to force this test to be 64 bit
            addressOfDataValue &= ~ordinalFlag;

            auto addressOfDataOffset = rva2offset(addressOfDataValue);
            if(!addressOfDataOffset) // Invalid entries are ignored. Of course the app will crash if it ever calls the function, but whose fault is that?
                continue;

            Info.imports.emplace_back();
            auto & entry = Info.imports.back();
            entry.iatRva = iatRva;
            entry.moduleIndex = moduleIndex;

            auto importByName = PIMAGE_IMPORT_BY_NAME(addressOfDataOffset + FileMapVA);
            if(!ordinalFlagSet && importByName->Name[0] != '\0')
            {
                // Import by name
                entry.name = String((const char*)importByName->Name);
                entry.ordinal = -1;
            }
            else
            {
                // Import by ordinal
                entry.ordinal = THUNK_VAL(Info.headers, thunkData, u1.Ordinal) & 0xffff;
                char buf[18];
                sprintf_s(buf, "Ordinal%u", (ULONG)entry.ordinal);
                entry.name = String((const char*)buf);
            }
        }
    }

    // prepare sorted vectors
    Info.importsByRva.resize(Info.imports.size());
    for(size_t i = 0; i < Info.imports.size(); i++)
        Info.importsByRva[i] = i;
    std::sort(Info.importsByRva.begin(), Info.importsByRva.end(), [&Info](size_t a, size_t b)
    {
        return Info.imports[a].iatRva < Info.imports[b].iatRva;
    });
}

static void ReadTlsCallbacks(MODINFO & Info, ULONG_PTR FileMapVA)
{
    // Clear TLS callbacks
    Info.tlsCallbacks.clear();

    // Get the TLS directory
    ULONG tlsDirSize;
    auto tlsDir = (PIMAGE_TLS_DIRECTORY)RtlImageDirectoryEntryToData((PVOID)FileMapVA,
                  FALSE,
                  IMAGE_DIRECTORY_ENTRY_TLS,
                  &tlsDirSize);
    if(tlsDir == nullptr /*|| tlsDirSize == 0*/) // The loader completely ignores the directory size. Setting it to 0 is an anti-debug trick
        return;

    ULONG64 addressOfCallbacks = IMAGE64(Info.headers)
                                 ? ((PIMAGE_TLS_DIRECTORY64)tlsDir)->AddressOfCallBacks
                                 : (ULONG64)((PIMAGE_TLS_DIRECTORY32)tlsDir)->AddressOfCallBacks;
    if(!addressOfCallbacks)
        return;

    auto imageBase = HEADER_FIELD(Info.headers, ImageBase);
    auto tlsArrayOffset = RvaToVa(0, Info.headers, tlsDir->AddressOfCallBacks - imageBase);
    if(!tlsArrayOffset)
        return;

    // TODO: proper bounds checking
    auto tlsArray = PULONG_PTR(tlsArrayOffset + FileMapVA);
    while(*tlsArray)
        Info.tlsCallbacks.push_back(*tlsArray++ - imageBase + Info.base);
}

#ifndef IMAGE_REL_BASED_RESERVED
#define IMAGE_REL_BASED_RESERVED 6
#endif // IMAGE_REL_BASED_RESERVED

#ifndef IMAGE_REL_BASED_MACHINE_SPECIFIC_7
#define IMAGE_REL_BASED_MACHINE_SPECIFIC_7 7
#endif // IMAGE_REL_BASED_MACHINE_SPECIFIC_7

static void ReadBaseRelocationTable(MODINFO & Info, ULONG_PTR FileMapVA)
{
    // Clear relocations
    Info.relocations.clear();

    // Ignore files with relocation info stripped
    if((Info.headers->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED) == IMAGE_FILE_RELOCS_STRIPPED)
        return;

    // Get address and size of base relocation table
    ULONG totalBytes;
    auto baseRelocBlock = (PIMAGE_BASE_RELOCATION)RtlImageDirectoryEntryToData((PVOID)FileMapVA,
                          FALSE,
                          IMAGE_DIRECTORY_ENTRY_BASERELOC,
                          &totalBytes);
    if(baseRelocBlock == nullptr || totalBytes == 0 || (ULONG_PTR)baseRelocBlock + totalBytes > FileMapVA + Info.loadedSize)
        return;

    // Until we reach the end of the relocation table
    while(totalBytes > 0)
    {
        ULONG blockSize = baseRelocBlock->SizeOfBlock;
        if(blockSize == 0 || blockSize > totalBytes) // The loader allows incorrect relocation dir sizes/block counts, but it won't relocate the image
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid relocation block for module %s%s!\n"), Info.name, Info.extension);
            return;
        }

        // Process the relocation block
        totalBytes -= blockSize;
        blockSize = (blockSize - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(USHORT);
        PUSHORT nextOffset = (PUSHORT)((PCHAR)baseRelocBlock + sizeof(IMAGE_BASE_RELOCATION));

        while(blockSize--)
        {
            const auto type = (UCHAR)((*nextOffset) >> 12);
            const auto offset = (USHORT)(*nextOffset & 0xfff);

            if(baseRelocBlock->VirtualAddress + offset > FileMapVA + HEADER_FIELD(Info.headers, SizeOfImage))
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid relocation entry for module %s%s!\n"), Info.name, Info.extension);
                return;
            }

            switch(type)
            {
            case IMAGE_REL_BASED_HIGHLOW:
                Info.relocations.push_back(MODRELOCATIONINFO{ baseRelocBlock->VirtualAddress + offset, type, sizeof(ULONG) });
                break;
            case IMAGE_REL_BASED_DIR64:
                Info.relocations.push_back(MODRELOCATIONINFO{ baseRelocBlock->VirtualAddress + offset, type, sizeof(ULONG64) });
                break;
            case IMAGE_REL_BASED_HIGH:
            case IMAGE_REL_BASED_LOW:
            case IMAGE_REL_BASED_HIGHADJ:
                Info.relocations.push_back(MODRELOCATIONINFO{ baseRelocBlock->VirtualAddress + offset, type, sizeof(USHORT) });
                break;
            case IMAGE_REL_BASED_ABSOLUTE:
            case IMAGE_REL_BASED_RESERVED: // IMAGE_REL_BASED_SECTION; ignored by loader
            case IMAGE_REL_BASED_MACHINE_SPECIFIC_7: // IMAGE_REL_BASED_REL32; ignored by loader
                break;
            default:
                dprintf(QT_TRANSLATE_NOOP("DBG", "Illegal relocation type 0x%02X for module %s%s!\n"), type, Info.name, Info.extension);
                return;
            }
            ++nextOffset;
        }
        baseRelocBlock = (PIMAGE_BASE_RELOCATION)nextOffset;
    }

    std::sort(Info.relocations.begin(), Info.relocations.end(), [](MODRELOCATIONINFO const & a, MODRELOCATIONINFO const & b)
    {
        return a.rva < b.rva;
    });
}

//Useful information: http://www.debuginfo.com/articles/debuginfomatch.html
void ReadDebugDirectory(MODINFO & Info, ULONG_PTR FileMapVA)
{
    // Get the debug directory and its size
    ULONG debugDirSize;
    auto debugDir = (PIMAGE_DEBUG_DIRECTORY)RtlImageDirectoryEntryToData((PVOID)FileMapVA,
                    FALSE,
                    IMAGE_DIRECTORY_ENTRY_DEBUG,
                    &debugDirSize);
    if(debugDirSize == 0 || debugDir == nullptr)
        return;

    struct CV_HEADER
    {
        DWORD Signature;
        DWORD Offset;
    };

    struct CV_INFO_PDB20
    {
        CV_HEADER CvHeader; //CvHeader.Signature = "NB10"
        DWORD Signature;
        DWORD Age;
        BYTE PdbFileName[1];
    };

    struct CV_INFO_PDB70
    {
        DWORD CvSignature; //"RSDS"
        GUID Signature;
        DWORD Age;
        BYTE PdbFileName[1];
    };

    const auto supported = [&Info](PIMAGE_DEBUG_DIRECTORY entry)
    {
        // Check for valid RVA
        const auto offset = RvaToVa(0, Info.headers, entry->AddressOfRawData);
        if(!offset)
            return false;

        // Check size is sane and end of data lies within the image
        if(entry->SizeOfData < sizeof(CV_INFO_PDB20) /*smallest supported type*/ ||
                entry->AddressOfRawData + entry->SizeOfData > HEADER_FIELD(Info.headers, SizeOfImage))
            return false;

        // Choose from one of our many supported types such as codeview
        if(entry->Type == IMAGE_DEBUG_TYPE_CODEVIEW) // TODO: support other types (DBG)?
        {
            // Get the CV signature and do a final size check if it is valid
            auto signature = *(DWORD*)(Info.fileMapVA + offset);
            if(signature == '01BN')
                return entry->SizeOfData >= sizeof(CV_INFO_PDB20) && entry->SizeOfData < sizeof(CV_INFO_PDB20) + DOS_MAX_PATH_LENGTH;
            else if(signature == 'SDSR')
                return entry->SizeOfData >= sizeof(CV_INFO_PDB70) && entry->SizeOfData < sizeof(CV_INFO_PDB70) + DOS_MAX_PATH_LENGTH;
            else
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "Unknown CodeView signature %08X for module %s%s...\n"), signature, Info.name, Info.extension);
                return false;
            }
        }
        return false;
    };

    // Iterate over entries until we find a CV one or the end of the directory
    PIMAGE_DEBUG_DIRECTORY entry = debugDir;
    while(debugDirSize >= sizeof(IMAGE_DEBUG_DIRECTORY))
    {
        if(supported(entry))
            break;

        const auto typeName = [](DWORD type)
        {
            switch(type)
            {
            case IMAGE_DEBUG_TYPE_UNKNOWN:
                return "IMAGE_DEBUG_TYPE_UNKNOWN";
            case IMAGE_DEBUG_TYPE_COFF:
                return "IMAGE_DEBUG_TYPE_COFF";
            case IMAGE_DEBUG_TYPE_CODEVIEW:
                return "IMAGE_DEBUG_TYPE_CODEVIEW";
            case IMAGE_DEBUG_TYPE_FPO:
                return "IMAGE_DEBUG_TYPE_FPO";
            case IMAGE_DEBUG_TYPE_MISC:
                return "IMAGE_DEBUG_TYPE_MISC";
            case IMAGE_DEBUG_TYPE_EXCEPTION:
                return "IMAGE_DEBUG_TYPE_EXCEPTION";
            case IMAGE_DEBUG_TYPE_FIXUP:
                return "IMAGE_DEBUG_TYPE_FIXUP";
            case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
                return "IMAGE_DEBUG_TYPE_OMAP_TO_SRC";
            case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
                return "IMAGE_DEBUG_TYPE_OMAP_FROM_SRC";
            case IMAGE_DEBUG_TYPE_BORLAND:
                return "IMAGE_DEBUG_TYPE_BORLAND";
            case IMAGE_DEBUG_TYPE_RESERVED10:
                return "IMAGE_DEBUG_TYPE_RESERVED10";
            case IMAGE_DEBUG_TYPE_CLSID:
                return "IMAGE_DEBUG_TYPE_CLSID";
            // The following types aren't defined in older Windows SDKs, so just count up from here so we can still return the names for them
            case(IMAGE_DEBUG_TYPE_CLSID + 1):
                return "IMAGE_DEBUG_TYPE_VC_FEATURE"; // How to kill: /NOVCFEATURE linker switch
            case(IMAGE_DEBUG_TYPE_CLSID + 2):
                return "IMAGE_DEBUG_TYPE_POGO"; // How to kill: /NOCOFFGRPINFO linker switch
            case(IMAGE_DEBUG_TYPE_CLSID + 3):
                return "IMAGE_DEBUG_TYPE_ILTCG";
            case(IMAGE_DEBUG_TYPE_CLSID + 4):
                return "IMAGE_DEBUG_TYPE_MPX";
            case(IMAGE_DEBUG_TYPE_CLSID + 5):
                return "IMAGE_DEBUG_TYPE_REPRO";
            default:
                return "unknown";
            }
        }(entry->Type);

        /*dprintf("IMAGE_DEBUG_DIRECTORY:\nCharacteristics: %08X\nTimeDateStamp: %08X\nMajorVersion: %04X\nMinorVersion: %04X\nType: %s\nSizeOfData: %08X\nAddressOfRawData: %08X\nPointerToRawData: %08X\n",
                debugDir->Characteristics, debugDir->TimeDateStamp, debugDir->MajorVersion, debugDir->MinorVersion, typeName, debugDir->SizeOfData, debugDir->AddressOfRawData, debugDir->PointerToRawData);*/

        dprintf(QT_TRANSLATE_NOOP("DBG", "Skipping unsupported debug type %s in module %s%s...\n"), typeName, Info.name, Info.extension);
        entry++;
        debugDirSize -= sizeof(IMAGE_DEBUG_DIRECTORY);
    }

    if(!supported(entry))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Did not find any supported debug types in module %s%s!\n"), Info.name, Info.extension);
        return;
    }

    // At this point we know the entry is a valid CV one
    auto cvData = (unsigned char*)(FileMapVA + RvaToVa(0, Info.headers, entry->AddressOfRawData));
    auto signature = *(DWORD*)cvData;
    if(signature == '01BN')
    {
        auto cv = (CV_INFO_PDB20*)cvData;
        Info.pdbSignature = StringUtils::sprintf("%X%X", cv->Signature, cv->Age);
        Info.pdbFile = String((const char*)cv->PdbFileName, entry->SizeOfData - FIELD_OFFSET(CV_INFO_PDB20, PdbFileName));
        Info.pdbValidation.signature = cv->Signature;
        Info.pdbValidation.age = cv->Age;
    }
    else if(signature == 'SDSR')
    {
        auto cv = (CV_INFO_PDB70*)cvData;
        Info.pdbSignature = StringUtils::sprintf("%08X%04X%04X%s%X",
                            cv->Signature.Data1, cv->Signature.Data2, cv->Signature.Data3,
                            StringUtils::ToHex(cv->Signature.Data4, 8).c_str(),
                            cv->Age);
        Info.pdbFile = String((const char*)cv->PdbFileName, entry->SizeOfData - FIELD_OFFSET(CV_INFO_PDB70, PdbFileName));
        memcpy(&Info.pdbValidation.guid, &cv->Signature, sizeof(GUID));
        Info.pdbValidation.age = cv->Age;
    }

    //dprintf("%s%s pdbSignature: %s, pdbFile: \"%s\"\n", Info.name, Info.extension, Info.pdbSignature.c_str(), Info.pdbFile.c_str());

    if(!Info.pdbFile.empty())
    {
        // Get the directory/filename from the debug directory PDB path
        String dir, file;
        auto lastIdx = Info.pdbFile.rfind('\\');
        if(lastIdx == String::npos)
            file = Info.pdbFile;
        else
        {
            dir = Info.pdbFile.substr(0, lastIdx - 1);
            file = Info.pdbFile.substr(lastIdx + 1);
        }

        // TODO: this order is exactly the wrong way around :P
        // It should be: symbol cache (by far the most likely location, also why it exists) -> PDB path in PE -> program directory.
        // (this is also the search order used by WinDbg/symchk/dumpbin and anything that uses symsrv)
        // WinDbg even tries HTTP servers before the path in the PE, but that might be taking it a bit too far

        // Program directory
        char pdbPath[MAX_PATH];
        strcpy_s(pdbPath, Info.path);
        auto lastBack = strrchr(pdbPath, '\\');
        if(lastBack)
        {
            lastBack[1] = '\0';
            strncat_s(pdbPath, file.c_str(), _TRUNCATE);
            Info.pdbPaths.push_back(pdbPath);
        }

        // Debug directory full path
        const bool bAllowUncPathsInDebugDirectory = false; // TODO: create setting for this
        if(!dir.empty() && (bAllowUncPathsInDebugDirectory || !PathIsUNCW(StringUtils::Utf8ToUtf16(Info.pdbFile).c_str())))
            Info.pdbPaths.push_back(Info.pdbFile);

        // Symbol cache
        auto cachePath = String(szSymbolCachePath);
        if(cachePath.back() != '\\')
            cachePath += '\\';
        cachePath += StringUtils::sprintf("%s\\%s\\%s", file.c_str(), Info.pdbSignature.c_str(), file.c_str());
        Info.pdbPaths.push_back(cachePath);
    }
}

void GetModuleInfo(MODINFO & Info, ULONG_PTR FileMapVA)
{
    // Get the PE headers
    if(!NT_SUCCESS(ImageNtHeaders(FileMapVA, Info.loadedSize, &Info.headers)))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Module %s%s: invalid PE file!\n"), Info.name, Info.extension);
        return;
    }

    // Get the entry point
    duint moduleOEP = HEADER_FIELD(Info.headers, AddressOfEntryPoint);

    // Fix a problem where the OEP is set to zero (non-existent).
    // OEP can't start at the PE header/offset 0 -- except if module is an EXE.
    Info.entry = moduleOEP + Info.base;

    if(!moduleOEP)
    {
        // If this wasn't an exe, invalidate the entry point
        if((Info.headers->FileHeader.Characteristics & IMAGE_FILE_DLL) == IMAGE_FILE_DLL)
            Info.entry = 0;
    }

    // Enumerate all PE sections
    Info.sections.clear();
    WORD sectionCount = Info.headers->FileHeader.NumberOfSections;
    PIMAGE_SECTION_HEADER ntSection = IMAGE_FIRST_SECTION(Info.headers);

    for(WORD i = 0; i < sectionCount; i++)
    {
        MODSECTIONINFO curSection;
        memset(&curSection, 0, sizeof(MODSECTIONINFO));

        curSection.addr = ntSection->VirtualAddress + Info.base;
        curSection.size = ntSection->Misc.VirtualSize;

        // Null-terminate section name
        char sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];
        strncpy_s(sectionName, (const char*)ntSection->Name, IMAGE_SIZEOF_SHORT_NAME);

        // Escape section name when needed
        strcpy_s(curSection.name, StringUtils::Escape(sectionName).c_str());

        // Add entry to the vector
        Info.sections.push_back(curSection);
        ntSection++;
    }

    ReadExportDirectory(Info, FileMapVA);
    ReadImportDirectory(Info, FileMapVA);
    dprintf("[%s%s] read %d imports and %d exports\n", Info.name, Info.extension, Info.imports.size(), Info.exports.size());
    ReadTlsCallbacks(Info, FileMapVA);
    ReadBaseRelocationTable(Info, FileMapVA);
    ReadDebugDirectory(Info, FileMapVA);
}

bool ModLoad(duint Base, duint Size, const char* FullPath)
{
    // Handle a new module being loaded
    if(!Base || !Size || !FullPath)
        return false;

    auto infoPtr = std::make_unique<MODINFO>();
    auto & info = *infoPtr;

    // Copy the module path in the struct
    strcpy_s(info.path, FullPath);

    // Break the module path into a directory and file name
    char file[MAX_MODULE_SIZE];
    {
        char dir[MAX_PATH];
        memset(dir, 0, sizeof(dir));

        // Dir <- lowercase(file path)
        strcpy_s(dir, FullPath);
        _strlwr_s(dir);

        // Find the last instance of a path delimiter (slash)
        char* fileStart = strrchr(dir, '\\');

        if(fileStart)
        {
            strcpy_s(file, fileStart + 1);
            fileStart[0] = '\0';
        }
        else
            strcpy_s(file, FullPath);
    }

    // Calculate module hash from full file name
    info.hash = ModHashFromName(file);

    // Copy the extension into the module struct
    {
        char* extensionPos = strrchr(file, '.');

        if(extensionPos)
        {
            strcpy_s(info.extension, extensionPos);
            extensionPos[0] = '\0';
        }
    }

    // Copy information to struct
    strcpy_s(info.name, file);
    info.base = Base;
    info.size = Size;
    info.fileHandle = nullptr;
    info.loadedSize = 0;
    info.fileMap = nullptr;
    info.fileMapVA = 0;

    // Determine whether the module is located in system
    wchar_t sysdir[MAX_PATH];
    GetEnvironmentVariableW(L"windir", sysdir, _countof(sysdir));
    String Utf8Sysdir = StringUtils::Utf16ToUtf8(sysdir);
    Utf8Sysdir.append("\\");
    if(_memicmp(Utf8Sysdir.c_str(), FullPath, Utf8Sysdir.size()) == 0)
    {
        info.party = 1;
    }
    else
    {
        info.party = 0;
    }

    // Load module data
    bool virtualModule = strstr(FullPath, "virtual:\\") == FullPath;

    if(!virtualModule)
    {
        auto wszFullPath = StringUtils::Utf8ToUtf16(FullPath);

        // Load the physical module from disk
        if(StaticFileLoadW(wszFullPath.c_str(), UE_ACCESS_READ, false, &info.fileHandle, &info.loadedSize, &info.fileMap, &info.fileMapVA))
        {
            GetModuleInfo(info, info.fileMapVA);

            Size = GetPE32DataFromMappedFile(info.fileMapVA, 0, UE_SIZEOFIMAGE);
            info.size = Size;
        }
        else
        {
            info.fileHandle = nullptr;
            info.loadedSize = 0;
            info.fileMap = nullptr;
            info.fileMapVA = 0;
        }
    }
    else
    {
        // This was a virtual module -> read it remotely
        Memory<unsigned char*> data(Size);
        MemRead(Base, data(), data.size());

        // Get information from the local buffer
        // TODO: this does not properly work for file offset -> rva conversions (since virtual modules are SEC_IMAGE)
        GetModuleInfo(info, (ULONG_PTR)data());
    }

    // TODO: setting to auto load symbols
    info.loadSymbols();

    // Add module to list
    EXCLUSIVE_ACQUIRE(LockModules);
    modinfo.insert(std::make_pair(Range(Base, Base + Size - 1), std::move(infoPtr)));
    EXCLUSIVE_RELEASE();

    // Put labels for virtual module exports
    if(virtualModule)
    {
        if(info.entry >= Base && info.entry < Base + Size)
            LabelSet(info.entry, "EntryPoint", false);

        apienumexports(Base, [](duint base, const char* mod, const char* name, duint addr)
        {
            LabelSet(addr, name, false);
        });
    }

    SymUpdateModuleList();
    return true;
}

bool ModUnload(duint Base)
{
    EXCLUSIVE_ACQUIRE(LockModules);

    // Find the iterator index
    const auto found = modinfo.find(Range(Base, Base));

    if(found == modinfo.end())
        return false;

    // Remove it from the list
    modinfo.erase(found);
    EXCLUSIVE_RELEASE();

    // Update symbols
    SymUpdateModuleList();
    return true;
}

void ModClear()
{
    {
        // Clean up all the modules
        EXCLUSIVE_ACQUIRE(LockModules);
        modinfo.clear();
    }

    {
        // Clean up the reverse hash map
        EXCLUSIVE_ACQUIRE(LockModuleHashes);
        hashNameMap.clear();
    }

    // Tell the symbol updater
    GuiSymbolUpdateModuleList(0, nullptr);
}

MODINFO* ModInfoFromAddr(duint Address)
{
    //
    // NOTE: THIS DOES _NOT_ USE LOCKS
    //
    auto found = modinfo.find(Range(Address, Address));

    // Was the module found with this address?
    if(found == modinfo.end())
        return nullptr;

    return found->second.get();
}

bool ModNameFromAddr(duint Address, char* Name, bool Extension)
{
    ASSERT_NONNULL(Name);
    SHARED_ACQUIRE(LockModules);

    // Get a pointer to module information
    auto module = ModInfoFromAddr(Address);

    if(!module)
    {
        Name[0] = '\0';
        return false;
    }

    // Copy initial module name
    strcpy_s(Name, MAX_MODULE_SIZE, module->name);

    if(Extension)
        strcat_s(Name, MAX_MODULE_SIZE, module->extension);

    return true;
}

duint ModBaseFromAddr(duint Address)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return 0;

    return module->base;
}

duint ModHashFromAddr(duint Address)
{
    // Returns a unique hash from a virtual address
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return Address;

    return module->hash + (Address - module->base);
}

duint ModContentHashFromAddr(duint Address)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return 0;

    if(module->fileMapVA != 0 && module->loadedSize > 0)
        return murmurhash((void*)module->fileMapVA, module->loadedSize);
    else
        return 0;
}

duint ModHashFromName(const char* Module)
{
    // return MODINFO.hash (based on the name)
    ASSERT_NONNULL(Module);
    auto len = int(strlen(Module));
    if(!len)
        return 0;
    auto hash = murmurhash(Module, len);

    //update the hash cache
    SHARED_ACQUIRE(LockModuleHashes);
    auto hashInCache = hashNameMap.find(hash) != hashNameMap.end();
    SHARED_RELEASE();
    if(!hashInCache)
    {
        EXCLUSIVE_ACQUIRE(LockModuleHashes);
        hashNameMap[hash] = Module;
    }

    return hash;
}

duint ModBaseFromName(const char* Module)
{
    ASSERT_NONNULL(Module);
    auto len = int(strlen(Module));
    if(!len)
        return 0;
    ASSERT_TRUE(len < MAX_MODULE_SIZE);
    SHARED_ACQUIRE(LockModules);

    //TODO: refactor this to query from a map
    duint candidate = 0;
    for(const auto & i : modinfo)
    {
        const auto & currentModule = i.second;
        char currentModuleName[MAX_MODULE_SIZE];
        strcpy_s(currentModuleName, currentModule->name);
        strcat_s(currentModuleName, currentModule->extension);

        // Compare with extension (perfect match)
        if(!_stricmp(currentModuleName, Module))
            return currentModule->base;

        // Compare without extension, possible candidate (thanks to chessgod101 for finding this)
        if(!candidate && !_stricmp(currentModule->name, Module))
            candidate = currentModule->base;
    }

    return candidate;
}

duint ModSizeFromAddr(duint Address)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return 0;

    return module->size;
}

std::string ModNameFromHash(duint Hash)
{
    SHARED_ACQUIRE(LockModuleHashes);
    auto found = hashNameMap.find(Hash);
    if(found == hashNameMap.end())
        return std::string();
    return found->second;
}

bool ModSectionsFromAddr(duint Address, std::vector<MODSECTIONINFO>* Sections)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return false;

    // Copy vector <-> vector
    *Sections = module->sections;
    return true;
}

duint ModEntryFromAddr(duint Address)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return 0;

    return module->entry;
}

int ModPathFromAddr(duint Address, char* Path, int Size)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return 0;

    strcpy_s(Path, Size, module->path);
    return (int)strlen(Path);
}

int ModPathFromName(const char* Module, char* Path, int Size)
{
    return ModPathFromAddr(ModBaseFromName(Module), Path, Size);
}

void ModEnum(const std::function<void(const MODINFO &)> & cbEnum)
{
    SHARED_ACQUIRE(LockModules);
    for(const auto & mod : modinfo)
        cbEnum(*mod.second);
}

int ModGetParty(duint Address)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    // If the module is not found, it is an user module
    if(!module)
        return 0;

    return module->party;
}

void ModSetParty(duint Address, int Party)
{
    EXCLUSIVE_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    // If the module is not found, it is an user module
    if(!module)
        return;

    module->party = Party;
}

bool ModRelocationsFromAddr(duint Address, std::vector<MODRELOCATIONINFO> & Relocations)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module || module->relocations.empty())
        return false;

    Relocations = module->relocations;

    return true;
}

bool ModRelocationAtAddr(duint Address, MODRELOCATIONINFO* Relocation)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module || module->relocations.empty())
        return false;

    DWORD rva = (DWORD)(Address - module->base);

    // We assume there are no overlapping relocations
    auto ub = std::upper_bound(module->relocations.cbegin(), module->relocations.cend(), rva,
                               [](DWORD a, MODRELOCATIONINFO const & b)
    {
        return a < b.rva;
    });
    if(ub != module->relocations.begin() && (--ub)->Contains(rva))
    {
        if(Relocation)
            *Relocation = *ub;
        return true;
    }

    return false;
}

bool ModRelocationsInRange(duint Address, duint Size, std::vector<MODRELOCATIONINFO> & Relocations)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module || module->relocations.empty())
        return false;

    DWORD rva = (DWORD)(Address - module->base);

    // We assume there are no overlapping relocations
    auto ub = std::upper_bound(module->relocations.cbegin(), module->relocations.cend(), rva,
                               [](DWORD a, MODRELOCATIONINFO const & b)
    {
        return a < b.rva;
    });
    if(ub != module->relocations.begin())
        ub--;

    Relocations.clear();
    while(ub != module->relocations.end() && ub->rva < rva + Size)
    {
        if(ub->rva >= rva)
            Relocations.push_back(*ub);
        ub++;
    }

    return !Relocations.empty();
}

bool MODINFO::loadSymbols()
{
    unloadSymbols();
    symbols = &EmptySymbolSource; // empty symbol source per default

    // Try DIA
    if(symbols == &EmptySymbolSource && SymbolSourceDIA::isLibraryAvailable())
    {
        // TODO: do something with searchPaths
        DiaValidationData_t validationData;
        memcpy(&validationData.guid, &pdbValidation.guid, sizeof(GUID));
        validationData.signature = pdbValidation.signature;
        validationData.age = pdbValidation.age;
        SymbolSourceDIA* symSource = new SymbolSourceDIA();
        for(const auto & pdbPath : pdbPaths)
        {
            if(!FileExists(pdbPath.c_str()))
            {
                GuiSymbolLogAdd(StringUtils::sprintf("[DIA] Skipping non-existent PDB: %s\n", pdbPath.c_str()).c_str());
            }
            else if(symSource->loadPDB(pdbPath, base, size, &validationData))
            {
                symSource->resizeSymbolBitmap(size);

                symbols = symSource;

                std::string msg;
                if(symSource->isLoading())
                    msg = StringUtils::sprintf("[DIA] Loading PDB (async): %s\n", pdbPath.c_str());
                else
                    msg = StringUtils::sprintf("[DIA] Loaded PDB: %s\n", pdbPath.c_str());
                GuiSymbolLogAdd(msg.c_str());

                return true;
            }
            else
            {
                // TODO: more detailled error codes?
                GuiSymbolLogAdd(StringUtils::sprintf("[DIA] Failed to load PDB: %s\n", pdbPath.c_str()).c_str());
            }
        }
        delete symSource;
    }
    if(symbols == &EmptySymbolSource && true) // TODO: try loading from other sources?
    {
    }

    if(!symbols->isOpen())
    {
        std::string msg = StringUtils::sprintf("No symbols loaded for: %s%s\n", name, extension);
        GuiSymbolLogAdd(msg.c_str());
        return false;
    }

    return true;
}

void MODINFO::unloadSymbols()
{
    if(symbols != nullptr && symbols != &EmptySymbolSource)
    {
        delete symbols;
        symbols = &EmptySymbolSource;
    }
}

void MODINFO::unmapFile()
{
    // Unload the mapped file from memory
    if(fileMapVA)
        StaticFileUnloadW(StringUtils::Utf8ToUtf16(path).c_str(), false, fileHandle, loadedSize, fileMap, fileMapVA);
}

void MODIMPORT::convertToGuiSymbol(duint base, SYMBOLINFO* info) const
{
    info->addr = base + iatRva;
    info->type = sym_import;
    info->decoratedSymbol = (char*)name.c_str();
    info->undecoratedSymbol = "";
    info->freeDecorated = info->freeUndecorated = false;
}

void MODEXPORT::convertToGuiSymbol(duint base, SYMBOLINFO* info) const
{
    info->addr = base + rva;
    info->type = sym_export;
    info->decoratedSymbol = (char*)name.c_str();
    info->undecoratedSymbol = "";
    info->freeDecorated = info->freeUndecorated = false;
}
