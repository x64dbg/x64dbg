/*
Copyright (c) 2013. The YARA Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma pack(push, 1)

#ifdef _WIN32
#include <windows.h>

// These definitions are not present in older Windows headers.

#ifndef IMAGE_FILE_MACHINE_ARMNT
#define IMAGE_FILE_MACHINE_ARMNT             0x01c4
#endif

#ifndef IMAGE_FILE_MACHINE_ARM64
#define IMAGE_FILE_MACHINE_ARM64             0xaa64
#endif

#else

#include <stdint.h>
#include <stdlib.h>

typedef uint8_t   BYTE;
typedef uint16_t  WORD;
typedef uint32_t  DWORD;
typedef int32_t   LONG;
typedef uint32_t  ULONG;
typedef uint64_t  ULONGLONG;


#define FIELD_OFFSET(type, field)    ((size_t)&(((type *)0)->field))

#ifndef _MAC

#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_OS2_SIGNATURE                 0x454E      // NE
#define IMAGE_OS2_SIGNATURE_LE              0x454C      // LE
#define IMAGE_VXD_SIGNATURE                 0x454C      // LE
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00

#else

#define IMAGE_DOS_SIGNATURE                 0x4D5A      // MZ
#define IMAGE_OS2_SIGNATURE                 0x4E45      // NE
#define IMAGE_OS2_SIGNATURE_LE              0x4C45      // LE
#define IMAGE_NT_SIGNATURE                  0x50450000  // PE00

#endif

#pragma pack(push, 2)

typedef struct _IMAGE_DOS_HEADER        // DOS .EXE header
{
    WORD   e_magic;                     // Magic number
    WORD   e_cblp;                      // Bytes on last page of file
    WORD   e_cp;                        // Pages in file
    WORD   e_crlc;                      // Relocations
    WORD   e_cparhdr;                   // Size of header in paragraphs
    WORD   e_minalloc;                  // Minimum extra paragraphs needed
    WORD   e_maxalloc;                  // Maximum extra paragraphs needed
    WORD   e_ss;                        // Initial (relative) SS value
    WORD   e_sp;                        // Initial SP value
    WORD   e_csum;                      // Checksum
    WORD   e_ip;                        // Initial IP value
    WORD   e_cs;                        // Initial (relative) CS value
    WORD   e_lfarlc;                    // File address of relocation table
    WORD   e_ovno;                      // Overlay number
    WORD   e_res[4];                    // Reserved words
    WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
    WORD   e_oeminfo;                   // OEM information; e_oemid specific
    WORD   e_res2[10];                  // Reserved words
    LONG   e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

#pragma pack(pop)

//
// File header format.
//

#pragma pack(push,4)

typedef struct _IMAGE_FILE_HEADER
{
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;



#define IMAGE_SIZEOF_FILE_HEADER             20


#define IMAGE_FILE_RELOCS_STRIPPED           0x0001  // Relocation info stripped from file.
#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002  // File is executable  (i.e. no unresolved externel references).
#define IMAGE_FILE_LINE_NUMS_STRIPPED        0x0004  // Line nunbers stripped from file.
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED       0x0008  // Local symbols stripped from file.
#define IMAGE_FILE_AGGRESIVE_WS_TRIM         0x0010  // Agressively trim working set
#define IMAGE_FILE_LARGE_ADDRESS_AWARE       0x0020  // App can handle >2gb addresses
#define IMAGE_FILE_BYTES_REVERSED_LO         0x0080  // Bytes of machine word are reversed.
#define IMAGE_FILE_32BIT_MACHINE             0x0100  // 32 bit word machine.
#define IMAGE_FILE_DEBUG_STRIPPED            0x0200  // Debugging info stripped from file in .DBG file
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   0x0400  // If Image is on removable media, copy and run from the swap file.
#define IMAGE_FILE_NET_RUN_FROM_SWAP         0x0800  // If Image is on Net, copy and run from the swap file.
#define IMAGE_FILE_SYSTEM                    0x1000  // System File.
#define IMAGE_FILE_DLL                       0x2000  // File is a DLL.
#define IMAGE_FILE_UP_SYSTEM_ONLY            0x4000  // File should only be run on a UP machine
#define IMAGE_FILE_BYTES_REVERSED_HI         0x8000  // Bytes of machine word are reversed.


#define IMAGE_FILE_MACHINE_UNKNOWN           0x0000
#define IMAGE_FILE_MACHINE_AM33              0x01d3
#define IMAGE_FILE_MACHINE_AMD64             0x8664
#define IMAGE_FILE_MACHINE_ARM               0x01c0
#define IMAGE_FILE_MACHINE_ARMNT             0x01c4
#define IMAGE_FILE_MACHINE_ARM64             0xaa64
#define IMAGE_FILE_MACHINE_EBC               0x0ebc
#define IMAGE_FILE_MACHINE_I386              0x014c
#define IMAGE_FILE_MACHINE_IA64              0x0200
#define IMAGE_FILE_MACHINE_M32R              0x9041
#define IMAGE_FILE_MACHINE_MIPS16            0x0266
#define IMAGE_FILE_MACHINE_MIPSFPU           0x0366
#define IMAGE_FILE_MACHINE_MIPSFPU16         0x0466
#define IMAGE_FILE_MACHINE_POWERPC           0x01f0
#define IMAGE_FILE_MACHINE_POWERPCFP         0x01f1
#define IMAGE_FILE_MACHINE_R4000             0x0166
#define IMAGE_FILE_MACHINE_SH3               0x01a2
#define IMAGE_FILE_MACHINE_SH3DSP            0x01a3
#define IMAGE_FILE_MACHINE_SH4               0x01a6
#define IMAGE_FILE_MACHINE_SH5               0x01a8
#define IMAGE_FILE_MACHINE_THUMB             0x01c2
#define IMAGE_FILE_MACHINE_WCEMIPSV2         0x0169

// Section characteristics
#define IMAGE_SCN_CNT_CODE                   0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA       0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA     0x00000080
#define IMAGE_SCN_GPREL                      0x00008000
#define IMAGE_SCN_MEM_16BIT                  0x00020000
#define IMAGE_SCN_LNK_NRELOC_OVFL            0x01000000
#define IMAGE_SCN_MEM_DISCARDABLE            0x02000000
#define IMAGE_SCN_MEM_NOT_CACHED             0x04000000
#define IMAGE_SCN_MEM_NOT_PAGED              0x08000000
#define IMAGE_SCN_MEM_SHARED                 0x10000000
#define IMAGE_SCN_MEM_EXECUTE                0x20000000
#define IMAGE_SCN_MEM_READ                   0x40000000
#define IMAGE_SCN_MEM_WRITE                  0x80000000

//
// Directory format.
//

typedef struct _IMAGE_DATA_DIRECTORY
{
    DWORD   VirtualAddress;
    DWORD   Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16


#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
#define IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7   // (X86 usage)
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor


//
// Optional header format.
//

typedef struct _IMAGE_OPTIONAL_HEADER32
{
    WORD Magic;
    BYTE MajorLinkerVersion;
    BYTE MinorLinkerVersion;
    DWORD SizeOfCode;
    DWORD SizeOfInitializedData;
    DWORD SizeOfUninitializedData;
    DWORD AddressOfEntryPoint;
    DWORD BaseOfCode;
    DWORD BaseOfData;
    DWORD ImageBase;
    DWORD SectionAlignment;
    DWORD FileAlignment;
    WORD MajorOperatingSystemVersion;
    WORD MinorOperatingSystemVersion;
    WORD MajorImageVersion;
    WORD MinorImageVersion;
    WORD MajorSubsystemVersion;
    WORD MinorSubsystemVersion;
    DWORD Win32VersionValue;
    DWORD SizeOfImage;
    DWORD SizeOfHeaders;
    DWORD CheckSum;
    WORD Subsystem;
    WORD DllCharacteristics;
    DWORD SizeOfStackReserve;
    DWORD SizeOfStackCommit;
    DWORD SizeOfHeapReserve;
    DWORD SizeOfHeapCommit;
    DWORD LoaderFlags;
    DWORD NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];

} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;


typedef struct _IMAGE_OPTIONAL_HEADER64
{
    WORD Magic;
    BYTE MajorLinkerVersion;
    BYTE MinorLinkerVersion;
    DWORD SizeOfCode;
    DWORD SizeOfInitializedData;
    DWORD SizeOfUninitializedData;
    DWORD AddressOfEntryPoint;
    DWORD BaseOfCode;
    ULONGLONG ImageBase;
    DWORD SectionAlignment;
    DWORD FileAlignment;
    WORD MajorOperatingSystemVersion;
    WORD MinorOperatingSystemVersion;
    WORD MajorImageVersion;
    WORD MinorImageVersion;
    WORD MajorSubsystemVersion;
    WORD MinorSubsystemVersion;
    DWORD Win32VersionValue;
    DWORD SizeOfImage;
    DWORD SizeOfHeaders;
    DWORD CheckSum;
    WORD Subsystem;
    WORD DllCharacteristics;
    ULONGLONG SizeOfStackReserve;
    ULONGLONG SizeOfStackCommit;
    ULONGLONG SizeOfHeapReserve;
    ULONGLONG SizeOfHeapCommit;
    DWORD LoaderFlags;
    DWORD NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];

} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;


#define IMAGE_NT_OPTIONAL_HDR32_MAGIC      0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC      0x20b


typedef struct _IMAGE_NT_HEADERS32
{
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;

} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;


typedef struct _IMAGE_NT_HEADERS64
{
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;

} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;


// IMAGE_FIRST_SECTION doesn't need 32/64 versions since the file header is
// the same either way.

#define IMAGE_FIRST_SECTION( ntheader ) ((PIMAGE_SECTION_HEADER) \
    ((BYTE*)ntheader + \
     FIELD_OFFSET( IMAGE_NT_HEADERS32, OptionalHeader ) + \
     ((PIMAGE_NT_HEADERS32)(ntheader))->FileHeader.SizeOfOptionalHeader \
    ))

// Subsystem Values

#define IMAGE_SUBSYSTEM_UNKNOWN              0   // Unknown subsystem.
#define IMAGE_SUBSYSTEM_NATIVE               1   // Image doesn't require a subsystem.
#define IMAGE_SUBSYSTEM_WINDOWS_GUI          2   // Image runs in the Windows GUI subsystem.
#define IMAGE_SUBSYSTEM_WINDOWS_CUI          3   // Image runs in the Windows character subsystem.
#define IMAGE_SUBSYSTEM_OS2_CUI              5   // image runs in the OS/2 character subsystem.
#define IMAGE_SUBSYSTEM_POSIX_CUI            7   // image runs in the Posix character subsystem.
#define IMAGE_SUBSYSTEM_NATIVE_WINDOWS       8   // image is a native Win9x driver.

//
// Section header format.
//

#define IMAGE_SIZEOF_SHORT_NAME              8

typedef struct _IMAGE_SECTION_HEADER
{
    BYTE    Name[IMAGE_SIZEOF_SHORT_NAME];
    union
    {
        DWORD   PhysicalAddress;
        DWORD   VirtualSize;
    } Misc;
    DWORD   VirtualAddress;
    DWORD   SizeOfRawData;
    DWORD   PointerToRawData;
    DWORD   PointerToRelocations;
    DWORD   PointerToLinenumbers;
    WORD    NumberOfRelocations;
    WORD    NumberOfLinenumbers;
    DWORD   Characteristics;

} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define IMAGE_SIZEOF_SECTION_HEADER          40


typedef struct _IMAGE_EXPORT_DIRECTORY
{
    DWORD Characteristics;
    DWORD TimeDateStamp;
    WORD  MajorVersion;
    WORD  MinorVersion;
    DWORD Name;
    DWORD Base;
    DWORD NumberOfFunctions;
    DWORD NumberOfNames;
    DWORD AddressOfFunctions;
    DWORD AddressOfNames;
    DWORD AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;


typedef struct _IMAGE_IMPORT_DESCRIPTOR
{
    union
    {
        DWORD Characteristics;
        DWORD OriginalFirstThunk;
    } ;
    DWORD TimeDateStamp;
    DWORD ForwarderChain;
    DWORD Name;
    DWORD FirstThunk;

} IMAGE_IMPORT_DESCRIPTOR, *PIMAGE_IMPORT_DESCRIPTOR;


typedef struct _IMAGE_IMPORT_BY_NAME
{
    WORD Hint;
    BYTE Name[1];

} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

typedef struct _IMAGE_THUNK_DATA32
{
    union
    {
        DWORD ForwarderString;
        DWORD Function;
        DWORD Ordinal;
        DWORD AddressOfData;
    } u1;

} IMAGE_THUNK_DATA32, *PIMAGE_THUNK_DATA32;


#define IMAGE_ORDINAL_FLAG32  0x80000000
#define IMAGE_ORDINAL_FLAG64  0x8000000000000000L

typedef struct _IMAGE_THUNK_DATA64
{
    union
    {
        ULONGLONG ForwarderString;
        ULONGLONG Function;
        ULONGLONG Ordinal;
        ULONGLONG AddressOfData;
    } u1;

} IMAGE_THUNK_DATA64, *PIMAGE_THUNK_DATA64;


typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY
{
    DWORD Name;
    DWORD OffsetToData;
} IMAGE_RESOURCE_DIRECTORY_ENTRY, *PIMAGE_RESOURCE_DIRECTORY_ENTRY;


typedef struct _IMAGE_RESOURCE_DATA_ENTRY
{
    DWORD OffsetToData;
    DWORD Size;
    DWORD CodePage;
    DWORD Reserved;
} IMAGE_RESOURCE_DATA_ENTRY, *PIMAGE_RESOURCE_DATA_ENTRY;


typedef struct _IMAGE_RESOURCE_DIRECTORY
{
    DWORD Characteristics;
    DWORD TimeDateStamp;
    WORD  MajorVersion;
    WORD  MinorVersion;
    WORD  NumberOfNamedEntries;
    WORD  NumberOfIdEntries;
} IMAGE_RESOURCE_DIRECTORY, *PIMAGE_RESOURCE_DIRECTORY;

#pragma pack(pop)

#endif  // _WIN32

typedef struct _VERSION_INFO
{
    WORD   Length;
    WORD   ValueLength;
    WORD   Type;
    char   Key[0];
} VERSION_INFO, *PVERSION_INFO;


#define WIN_CERT_REVISION_1_0 0x0100
#define WIN_CERT_REVISION_2_0 0x0200

#define WIN_CERT_TYPE_X509             0x0001
#define WIN_CERT_TYPE_PKCS_SIGNED_DATA 0x0002
#define WIN_CERT_TYPE_RESERVED_1       0x0003
#define WIN_CERT_TYPE_TS_STACK_SIGNED  0x0004

typedef struct _WIN_CERTIFICATE
{
    DWORD Length;
    WORD  Revision;
    WORD  CertificateType;
    BYTE  Certificate[1];
} WIN_CERTIFICATE, *PWIN_CERTIFICATE;


//
// Rich signature.
// http://www.ntcore.com/files/richsign.htm
//

typedef struct _RICH_SIGNATURE
{
    DWORD dans;
    DWORD key1;
    DWORD key2;
    DWORD key3;
} RICH_SIGNATURE, *PRICH_SIGNATURE;

#define RICH_DANS 0x536e6144 // "DanS"
#define RICH_RICH 0x68636952 // "Rich"

typedef struct _RICH_DATA
{
    size_t len;
    BYTE* raw_data;
    BYTE* clear_data;
} RICH_DATA, *PRICH_DATA;

#pragma pack(pop)
