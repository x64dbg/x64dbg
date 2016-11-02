#ifndef _DBGHELP_
#define _DBGHELP_


// As a general principal always call the 64 bit version
// of every API, if a choice exists.  The 64 bit version
// works great on 32 bit platforms, and is forward
// compatible to 64 bit platforms.

#ifdef _WIN64
#ifndef _IMAGEHLP64
#define _IMAGEHLP64
#endif
#endif

#pragma pack(push,8)

// For those without specstrings.h
// Since there are different versions of this header, I need to
// individually test each item and define it if it is not around.

#ifndef __in
#define __in
#endif
#ifndef __out
#define __out
#endif
#ifndef __inout
#define __inout
#endif
#ifndef __in_opt
#define __in_opt
#endif
#ifndef __out_opt
#define __out_opt
#endif
#ifndef __inout_opt
#define __inout_opt
#endif
#ifndef __in_ecount
#define __in_ecount(x)
#endif
#ifndef __out_ecount
#define __out_ecount(x)
#endif
#ifndef __inout_ecount
#define __inout_ecount(x)
#endif
#ifndef __in_bcount
#define __in_bcount(x)
#endif
#ifndef __out_bcount
#define __out_bcount(x)
#endif
#ifndef __inout_bcount
#define __inout_bcount(x)
#endif
#ifndef __out_xcount
#define __out_xcount(x)
#endif
#ifndef __deref_opt_out
#define __deref_opt_out
#endif
#ifndef __deref_out
#define __deref_out
#endif
#ifndef __out_ecount_opt
#define __out_ecount_opt(x)
#endif
#ifndef __in_bcount_opt
#define __in_bcount_opt(x)
#endif
#ifndef __out_bcount_opt
#define __out_bcount_opt(x)
#endif
#ifndef __deref_out_opt
#define __deref_out_opt
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _IMAGEHLP_SOURCE_
#define IMAGEAPI __stdcall
#define DBHLP_DEPRECIATED
#else
#define IMAGEAPI DECLSPEC_IMPORT __stdcall
#if (_MSC_VER >= 1300) && !defined(MIDL_PASS)
#define DBHLP_DEPRECIATED   __declspec(deprecated)
#else
#define DBHLP_DEPRECIATED
#endif
#endif

#define DBHLPAPI IMAGEAPI

#define IMAGE_SEPARATION (64*1024)

// Observant readers may notice that 2 new fields,
// 'fReadOnly' and 'Version' have been added to
// the LOADED_IMAGE structure after 'fDOSImage'.
// This does not change the size of the structure
// from previous headers.  That is because while
// 'fDOSImage' is a byte, it is padded by the
// compiler to 4 bytes.  So the 2 new fields are
// slipped into the extra space.

typedef struct _LOADED_IMAGE
{
    PSTR                  ModuleName;
    HANDLE                hFile;
    PUCHAR                MappedAddress;
#ifdef _IMAGEHLP64
    PIMAGE_NT_HEADERS64   FileHeader;
#else
    PIMAGE_NT_HEADERS32   FileHeader;
#endif
    PIMAGE_SECTION_HEADER LastRvaSection;
    ULONG                 NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
    ULONG                 Characteristics;
    BOOLEAN               fSystemImage;
    BOOLEAN               fDOSImage;
    BOOLEAN               fReadOnly;
    UCHAR                 Version;
    LIST_ENTRY            Links;
    ULONG                 SizeOfImage;
} LOADED_IMAGE, *PLOADED_IMAGE;

#define MAX_SYM_NAME            2000


// Error codes set by dbghelp functions.  Call GetLastError
// to see them.
// Dbghelp also sets error codes found in winerror.h

#define ERROR_IMAGE_NOT_STRIPPED    0x8800  // the image is not stripped.  No dbg file available.
#define ERROR_NO_DBG_POINTER        0x8801  // image is stripped but there is no pointer to a dbg file
#define ERROR_NO_PDB_POINTER        0x8802  // image does not point to a pdb file

typedef BOOL
(CALLBACK* PFIND_DEBUG_FILE_CALLBACK)(
    __in HANDLE FileHandle,
    __in PCSTR FileName,
    __in PVOID CallerData
);

HANDLE
IMAGEAPI
SymFindDebugInfoFile(
    __in HANDLE hProcess,
    __in PCSTR FileName,
    __out_ecount(MAX_PATH + 1) PSTR DebugFilePath,
    __in_opt PFIND_DEBUG_FILE_CALLBACK Callback,
    __in_opt PVOID CallerData
);

typedef BOOL
(CALLBACK* PFIND_DEBUG_FILE_CALLBACKW)(
    __in HANDLE FileHandle,
    __in PCWSTR FileName,
    __in PVOID  CallerData
);

HANDLE
IMAGEAPI
SymFindDebugInfoFileW(
    __in HANDLE hProcess,
    __in PCWSTR FileName,
    __out_ecount(MAX_PATH + 1) PWSTR DebugFilePath,
    __in_opt PFIND_DEBUG_FILE_CALLBACKW Callback,
    __in_opt PVOID CallerData
);

HANDLE
IMAGEAPI
FindDebugInfoFile(
    __in PCSTR FileName,
    __in PCSTR SymbolPath,
    __out_ecount(MAX_PATH + 1) PSTR DebugFilePath
);

HANDLE
IMAGEAPI
FindDebugInfoFileEx(
    __in PCSTR FileName,
    __in PCSTR SymbolPath,
    __out_ecount(MAX_PATH + 1) PSTR  DebugFilePath,
    __in_opt PFIND_DEBUG_FILE_CALLBACK Callback,
    __in_opt PVOID CallerData
);

HANDLE
IMAGEAPI
FindDebugInfoFileExW(
    __in PCWSTR FileName,
    __in PCWSTR SymbolPath,
    __out_ecount(MAX_PATH + 1) PWSTR DebugFilePath,
    __in_opt PFIND_DEBUG_FILE_CALLBACKW Callback,
    __in_opt PVOID CallerData
);

typedef BOOL
(CALLBACK* PFINDFILEINPATHCALLBACK)(
    __in PCSTR filename,
    __in PVOID context
);

BOOL
IMAGEAPI
SymFindFileInPath(
    __in HANDLE hprocess,
    __in_opt PCSTR SearchPath,
    __in PCSTR FileName,
    __in_opt PVOID id,
    __in DWORD two,
    __in DWORD three,
    __in DWORD flags,
    __out_ecount(MAX_PATH + 1) PSTR FoundFile,
    __in_opt PFINDFILEINPATHCALLBACK callback,
    __in_opt PVOID context
);

typedef BOOL
(CALLBACK* PFINDFILEINPATHCALLBACKW)(
    __in PCWSTR filename,
    __in PVOID context
);

BOOL
IMAGEAPI
SymFindFileInPathW(
    __in HANDLE hprocess,
    __in_opt PCWSTR SearchPath,
    __in PCWSTR FileName,
    __in_opt PVOID id,
    __in DWORD two,
    __in DWORD three,
    __in DWORD flags,
    __out_ecount(MAX_PATH + 1) PWSTR FoundFile,
    __in_opt PFINDFILEINPATHCALLBACKW callback,
    __in_opt PVOID context
);

typedef BOOL
(CALLBACK* PFIND_EXE_FILE_CALLBACK)(
    __in HANDLE FileHandle,
    __in PCSTR FileName,
    __in_opt PVOID CallerData
);

HANDLE
IMAGEAPI
SymFindExecutableImage(
    __in HANDLE hProcess,
    __in PCSTR FileName,
    __out_ecount(MAX_PATH + 1) PSTR ImageFilePath,
    __in PFIND_EXE_FILE_CALLBACK Callback,
    __in PVOID CallerData
);

typedef BOOL
(CALLBACK* PFIND_EXE_FILE_CALLBACKW)(
    __in HANDLE FileHandle,
    __in PCWSTR FileName,
    __in_opt PVOID CallerData
);

HANDLE
IMAGEAPI
SymFindExecutableImageW(
    __in HANDLE hProcess,
    __in PCWSTR FileName,
    __out_ecount(MAX_PATH + 1) PWSTR ImageFilePath,
    __in PFIND_EXE_FILE_CALLBACKW Callback,
    __in PVOID CallerData
);

HANDLE
IMAGEAPI
FindExecutableImage(
    __in PCSTR FileName,
    __in PCSTR SymbolPath,
    __out_ecount(MAX_PATH + 1) PSTR ImageFilePath
);

HANDLE
IMAGEAPI
FindExecutableImageEx(
    __in PCSTR FileName,
    __in PCSTR SymbolPath,
    __out_ecount(MAX_PATH + 1) PSTR ImageFilePath,
    __in_opt PFIND_EXE_FILE_CALLBACK Callback,
    __in_opt PVOID CallerData
);

HANDLE
IMAGEAPI
FindExecutableImageExW(
    __in PCWSTR FileName,
    __in PCWSTR SymbolPath,
    __out_ecount(MAX_PATH + 1) PWSTR ImageFilePath,
    __in_opt PFIND_EXE_FILE_CALLBACKW Callback,
    __in PVOID CallerData
);

PIMAGE_NT_HEADERS
IMAGEAPI
ImageNtHeader(
    __in PVOID Base
);

PVOID
IMAGEAPI
ImageDirectoryEntryToDataEx(
    __in PVOID Base,
    __in BOOLEAN MappedAsImage,
    __in USHORT DirectoryEntry,
    __out PULONG Size,
    __out_opt PIMAGE_SECTION_HEADER* FoundHeader
);

PVOID
IMAGEAPI
ImageDirectoryEntryToData(
    __in PVOID Base,
    __in BOOLEAN MappedAsImage,
    __in USHORT DirectoryEntry,
    __out PULONG Size
);

PIMAGE_SECTION_HEADER
IMAGEAPI
ImageRvaToSection(
    __in PIMAGE_NT_HEADERS NtHeaders,
    __in PVOID Base,
    __in ULONG Rva
);

PVOID
IMAGEAPI
ImageRvaToVa(
    __in PIMAGE_NT_HEADERS NtHeaders,
    __in PVOID Base,
    __in ULONG Rva,
    __in_opt OUT PIMAGE_SECTION_HEADER* LastRvaSection
);

#ifndef _WIN64
// This api won't be ported to Win64 - Fix your code.

typedef struct _IMAGE_DEBUG_INFORMATION
{
    LIST_ENTRY List;
    DWORD ReservedSize;
    PVOID ReservedMappedBase;
    USHORT ReservedMachine;
    USHORT ReservedCharacteristics;
    DWORD ReservedCheckSum;
    DWORD ImageBase;
    DWORD SizeOfImage;

    DWORD ReservedNumberOfSections;
    PIMAGE_SECTION_HEADER ReservedSections;

    DWORD ReservedExportedNamesSize;
    PSTR ReservedExportedNames;

    DWORD ReservedNumberOfFunctionTableEntries;
    PIMAGE_FUNCTION_ENTRY ReservedFunctionTableEntries;
    DWORD ReservedLowestFunctionStartingAddress;
    DWORD ReservedHighestFunctionEndingAddress;

    DWORD ReservedNumberOfFpoTableEntries;
    PFPO_DATA ReservedFpoTableEntries;

    DWORD SizeOfCoffSymbols;
    PIMAGE_COFF_SYMBOLS_HEADER CoffSymbols;

    DWORD ReservedSizeOfCodeViewSymbols;
    PVOID ReservedCodeViewSymbols;

    PSTR ImageFilePath;
    PSTR ImageFileName;
    PSTR ReservedDebugFilePath;

    DWORD ReservedTimeDateStamp;

    BOOL  ReservedRomImage;
    PIMAGE_DEBUG_DIRECTORY ReservedDebugDirectory;
    DWORD ReservedNumberOfDebugDirectories;

    DWORD ReservedOriginalFunctionTableBaseAddress;

    DWORD Reserved[ 2 ];

} IMAGE_DEBUG_INFORMATION, *PIMAGE_DEBUG_INFORMATION;


PIMAGE_DEBUG_INFORMATION
IMAGEAPI
MapDebugInformation(
    __in_opt HANDLE FileHandle,
    __in PCSTR FileName,
    __in_opt PCSTR SymbolPath,
    __in ULONG ImageBase
);

BOOL
IMAGEAPI
UnmapDebugInformation(
    __out_xcount(unknown) PIMAGE_DEBUG_INFORMATION DebugInfo
);

#endif

BOOL
IMAGEAPI
SearchTreeForFile(
    __in PCSTR RootPath,
    __in PCSTR InputPathName,
    __out_ecount(MAX_PATH + 1) PSTR OutputPathBuffer
);

BOOL
IMAGEAPI
SearchTreeForFileW(
    __in PCWSTR RootPath,
    __in PCWSTR InputPathName,
    __out_ecount(MAX_PATH + 1) PWSTR OutputPathBuffer
);

typedef BOOL
(CALLBACK* PENUMDIRTREE_CALLBACK)(
    __in PCSTR FilePath,
    __in_opt PVOID CallerData
);

BOOL
IMAGEAPI
EnumDirTree(
    __in_opt HANDLE hProcess,
    __in PCSTR RootPath,
    __in PCSTR InputPathName,
    __out_ecount_opt(MAX_PATH + 1) PSTR OutputPathBuffer,
    __in_opt PENUMDIRTREE_CALLBACK cb,
    __in_opt PVOID data
);

typedef BOOL
(CALLBACK* PENUMDIRTREE_CALLBACKW)(
    __in PCWSTR FilePath,
    __in_opt PVOID CallerData
);

BOOL
IMAGEAPI
EnumDirTreeW(
    __in_opt HANDLE hProcess,
    __in PCWSTR RootPath,
    __in PCWSTR InputPathName,
    __out_ecount_opt(MAX_PATH + 1) PWSTR OutputPathBuffer,
    __in_opt PENUMDIRTREE_CALLBACKW cb,
    __in_opt PVOID data
);

BOOL
IMAGEAPI
MakeSureDirectoryPathExists(
    __in PCSTR DirPath
);

//
// UnDecorateSymbolName Flags
//

#define UNDNAME_COMPLETE                 (0x0000)  // Enable full undecoration
#define UNDNAME_NO_LEADING_UNDERSCORES   (0x0001)  // Remove leading underscores from MS extended keywords
#define UNDNAME_NO_MS_KEYWORDS           (0x0002)  // Disable expansion of MS extended keywords
#define UNDNAME_NO_FUNCTION_RETURNS      (0x0004)  // Disable expansion of return type for primary declaration
#define UNDNAME_NO_ALLOCATION_MODEL      (0x0008)  // Disable expansion of the declaration model
#define UNDNAME_NO_ALLOCATION_LANGUAGE   (0x0010)  // Disable expansion of the declaration language specifier
#define UNDNAME_NO_MS_THISTYPE           (0x0020)  // NYI Disable expansion of MS keywords on the 'this' type for primary declaration
#define UNDNAME_NO_CV_THISTYPE           (0x0040)  // NYI Disable expansion of CV modifiers on the 'this' type for primary declaration
#define UNDNAME_NO_THISTYPE              (0x0060)  // Disable all modifiers on the 'this' type
#define UNDNAME_NO_ACCESS_SPECIFIERS     (0x0080)  // Disable expansion of access specifiers for members
#define UNDNAME_NO_THROW_SIGNATURES      (0x0100)  // Disable expansion of 'throw-signatures' for functions and pointers to functions
#define UNDNAME_NO_MEMBER_TYPE           (0x0200)  // Disable expansion of 'static' or 'virtual'ness of members
#define UNDNAME_NO_RETURN_UDT_MODEL      (0x0400)  // Disable expansion of MS model for UDT returns
#define UNDNAME_32_BIT_DECODE            (0x0800)  // Undecorate 32-bit decorated names
#define UNDNAME_NAME_ONLY                (0x1000)  // Crack only the name for primary declaration;
//  return just [scope::]name.  Does expand template params
#define UNDNAME_NO_ARGUMENTS             (0x2000)  // Don't undecorate arguments to function
#define UNDNAME_NO_SPECIAL_SYMS          (0x4000)  // Don't undecorate special names (v-table, vcall, vector xxx, metatype, etc)

DWORD
IMAGEAPI
WINAPI
UnDecorateSymbolName(
    __in PCSTR name,
    __out_ecount(maxStringLength) PSTR outputString,
    __in DWORD maxStringLength,
    __in DWORD flags
);

DWORD
IMAGEAPI
WINAPI
UnDecorateSymbolNameW(
    __in PCWSTR name,
    __out_ecount(maxStringLength) PWSTR outputString,
    __in DWORD maxStringLength,
    __in DWORD flags
);

//
// these values are used for synthesized file types
// that can be passed in as image headers instead of
// the standard ones from ntimage.h
//

#define DBHHEADER_DEBUGDIRS     0x1
#define DBHHEADER_CVMISC        0x2
#define DBHHEADER_PDBGUID       0x3
typedef struct _MODLOAD_DATA
{
    DWORD   ssize;                  // size of this struct
    DWORD   ssig;                   // signature identifying the passed data
    PVOID   data;                   // pointer to passed data
    DWORD   size;                   // size of passed data
    DWORD   flags;                  // options
} MODLOAD_DATA, *PMODLOAD_DATA;

typedef struct _MODLOAD_CVMISC
{
    DWORD   oCV;                    // ofset to the codeview record
    size_t  cCV;                    // size of the codeview record
    DWORD   oMisc;                  // offset to the misc record
    size_t  cMisc;                  // size of the misc record
    DWORD   dtImage;                // datetime stamp of the image
    DWORD   cImage;                 // size of the image
} MODLOAD_CVMISC, *PMODLOAD_CVMISC;

typedef struct _MODLOAD_PDBGUID_PDBAGE
{
    GUID    PdbGuid;                // Pdb Guid
    DWORD   PdbAge;                 // Pdb Age
} MODLOAD_PDBGUID_PDBAGE, *PMODLOAD_PDBGUID_PDBAGE;

//
// StackWalking API
//

typedef enum
{
    AddrMode1616,
    AddrMode1632,
    AddrModeReal,
    AddrModeFlat
} ADDRESS_MODE;

typedef struct _tagADDRESS64
{
    DWORD64       Offset;
    WORD          Segment;
    ADDRESS_MODE  Mode;
} ADDRESS64, *LPADDRESS64;

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define ADDRESS ADDRESS64
#define LPADDRESS LPADDRESS64
#else
typedef struct _tagADDRESS
{
    DWORD         Offset;
    WORD          Segment;
    ADDRESS_MODE  Mode;
} ADDRESS, *LPADDRESS;

__inline
void
Address32To64(
    __in LPADDRESS a32,
    __out LPADDRESS64 a64
)
{
    a64->Offset = (ULONG64)(LONG64)(LONG)a32->Offset;
    a64->Segment = a32->Segment;
    a64->Mode = a32->Mode;
}

__inline
void
Address64To32(
    __in LPADDRESS64 a64,
    __out LPADDRESS a32
)
{
    a32->Offset = (ULONG)a64->Offset;
    a32->Segment = a64->Segment;
    a32->Mode = a64->Mode;
}
#endif

//
// This structure is included in the STACKFRAME structure,
// and is used to trace through usermode callbacks in a thread's
// kernel stack.  The values must be copied by the kernel debugger
// from the DBGKD_GET_VERSION and WAIT_STATE_CHANGE packets.
//

//
// New KDHELP structure for 64 bit system support.
// This structure is preferred in new code.
//
typedef struct _KDHELP64
{

    //
    // address of kernel thread object, as provided in the
    // WAIT_STATE_CHANGE packet.
    //
    DWORD64   Thread;

    //
    // offset in thread object to pointer to the current callback frame
    // in kernel stack.
    //
    DWORD   ThCallbackStack;

    //
    // offset in thread object to pointer to the current callback backing
    // store frame in kernel stack.
    //
    DWORD   ThCallbackBStore;

    //
    // offsets to values in frame:
    //
    // address of next callback frame
    DWORD   NextCallback;

    // address of saved frame pointer (if applicable)
    DWORD   FramePointer;


    //
    // Address of the kernel function that calls out to user mode
    //
    DWORD64   KiCallUserMode;

    //
    // Address of the user mode dispatcher function
    //
    DWORD64   KeUserCallbackDispatcher;

    //
    // Lowest kernel mode address
    //
    DWORD64   SystemRangeStart;

    //
    // Address of the user mode exception dispatcher function.
    // Added in API version 10.
    //
    DWORD64   KiUserExceptionDispatcher;

    //
    // Stack bounds, added in API version 11.
    //
    DWORD64   StackBase;
    DWORD64   StackLimit;

    DWORD64   Reserved[5];

} KDHELP64, *PKDHELP64;

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define KDHELP KDHELP64
#define PKDHELP PKDHELP64
#else
typedef struct _KDHELP
{

    //
    // address of kernel thread object, as provided in the
    // WAIT_STATE_CHANGE packet.
    //
    DWORD   Thread;

    //
    // offset in thread object to pointer to the current callback frame
    // in kernel stack.
    //
    DWORD   ThCallbackStack;

    //
    // offsets to values in frame:
    //
    // address of next callback frame
    DWORD   NextCallback;

    // address of saved frame pointer (if applicable)
    DWORD   FramePointer;

    //
    // Address of the kernel function that calls out to user mode
    //
    DWORD   KiCallUserMode;

    //
    // Address of the user mode dispatcher function
    //
    DWORD   KeUserCallbackDispatcher;

    //
    // Lowest kernel mode address
    //
    DWORD   SystemRangeStart;

    //
    // offset in thread object to pointer to the current callback backing
    // store frame in kernel stack.
    //
    DWORD   ThCallbackBStore;

    //
    // Address of the user mode exception dispatcher function.
    // Added in API version 10.
    //
    DWORD   KiUserExceptionDispatcher;

    //
    // Stack bounds, added in API version 11.
    //
    DWORD   StackBase;
    DWORD   StackLimit;

    DWORD   Reserved[5];

} KDHELP, *PKDHELP;

__inline
void
KdHelp32To64(
    __in PKDHELP p32,
    __out PKDHELP64 p64
)
{
    p64->Thread = p32->Thread;
    p64->ThCallbackStack = p32->ThCallbackStack;
    p64->NextCallback = p32->NextCallback;
    p64->FramePointer = p32->FramePointer;
    p64->KiCallUserMode = p32->KiCallUserMode;
    p64->KeUserCallbackDispatcher = p32->KeUserCallbackDispatcher;
    p64->SystemRangeStart = p32->SystemRangeStart;
    p64->KiUserExceptionDispatcher = p32->KiUserExceptionDispatcher;
    p64->StackBase = p32->StackBase;
    p64->StackLimit = p32->StackLimit;
}
#endif

typedef struct _tagSTACKFRAME64
{
    ADDRESS64   AddrPC;               // program counter
    ADDRESS64   AddrReturn;           // return address
    ADDRESS64   AddrFrame;            // frame pointer
    ADDRESS64   AddrStack;            // stack pointer
    ADDRESS64   AddrBStore;           // backing store pointer
    PVOID       FuncTableEntry;       // pointer to pdata/fpo or NULL
    DWORD64     Params[4];            // possible arguments to the function
    BOOL        Far;                  // WOW far call
    BOOL        Virtual;              // is this a virtual frame?
    DWORD64     Reserved[3];
    KDHELP64    KdHelp;
} STACKFRAME64, *LPSTACKFRAME64;

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define STACKFRAME STACKFRAME64
#define LPSTACKFRAME LPSTACKFRAME64
#else
typedef struct _tagSTACKFRAME
{
    ADDRESS     AddrPC;               // program counter
    ADDRESS     AddrReturn;           // return address
    ADDRESS     AddrFrame;            // frame pointer
    ADDRESS     AddrStack;            // stack pointer
    PVOID       FuncTableEntry;       // pointer to pdata/fpo or NULL
    DWORD       Params[4];            // possible arguments to the function
    BOOL        Far;                  // WOW far call
    BOOL        Virtual;              // is this a virtual frame?
    DWORD       Reserved[3];
    KDHELP      KdHelp;
    ADDRESS     AddrBStore;           // backing store pointer
} STACKFRAME, *LPSTACKFRAME;
#endif


typedef
BOOL
(__stdcall* PREAD_PROCESS_MEMORY_ROUTINE64)(
    __in HANDLE hProcess,
    __in DWORD64 qwBaseAddress,
    __out_bcount(nSize) PVOID lpBuffer,
    __in DWORD nSize,
    __out LPDWORD lpNumberOfBytesRead
);

typedef
PVOID
(__stdcall* PFUNCTION_TABLE_ACCESS_ROUTINE64)(
    __in HANDLE ahProcess,
    __in DWORD64 AddrBase
);

typedef
DWORD64
(__stdcall* PGET_MODULE_BASE_ROUTINE64)(
    __in HANDLE hProcess,
    __in DWORD64 Address
);

typedef
DWORD64
(__stdcall* PTRANSLATE_ADDRESS_ROUTINE64)(
    __in HANDLE hProcess,
    __in HANDLE hThread,
    __in LPADDRESS64 lpaddr
);

BOOL
IMAGEAPI
StackWalk64(
    __in DWORD MachineType,
    __in HANDLE hProcess,
    __in HANDLE hThread,
    __inout LPSTACKFRAME64 StackFrame,
    __inout PVOID ContextRecord,
    __in_opt PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    __in_opt PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    __in_opt PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    __in_opt PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)

#define PREAD_PROCESS_MEMORY_ROUTINE PREAD_PROCESS_MEMORY_ROUTINE64
#define PFUNCTION_TABLE_ACCESS_ROUTINE PFUNCTION_TABLE_ACCESS_ROUTINE64
#define PGET_MODULE_BASE_ROUTINE PGET_MODULE_BASE_ROUTINE64
#define PTRANSLATE_ADDRESS_ROUTINE PTRANSLATE_ADDRESS_ROUTINE64

#define StackWalk StackWalk64

#else

typedef
BOOL
(__stdcall* PREAD_PROCESS_MEMORY_ROUTINE)(
    __in HANDLE hProcess,
    __in DWORD lpBaseAddress,
    __out_bcount(nSize) PVOID lpBuffer,
    __in DWORD nSize,
    __out PDWORD lpNumberOfBytesRead
);

typedef
PVOID
(__stdcall* PFUNCTION_TABLE_ACCESS_ROUTINE)(
    __in HANDLE hProcess,
    __in DWORD AddrBase
);

typedef
DWORD
(__stdcall* PGET_MODULE_BASE_ROUTINE)(
    __in HANDLE hProcess,
    __in DWORD Address
);

typedef
DWORD
(__stdcall* PTRANSLATE_ADDRESS_ROUTINE)(
    __in HANDLE hProcess,
    __in HANDLE hThread,
    __out LPADDRESS lpaddr
);

BOOL
IMAGEAPI
StackWalk(
    DWORD MachineType,
    __in HANDLE hProcess,
    __in HANDLE hThread,
    __inout LPSTACKFRAME StackFrame,
    __inout PVOID ContextRecord,
    __in_opt PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
    __in_opt PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
    __in_opt PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
    __in_opt PTRANSLATE_ADDRESS_ROUTINE TranslateAddress
);

#endif


#define API_VERSION_NUMBER 11

typedef struct API_VERSION
{
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    USHORT  Revision;
    USHORT  Reserved;
} API_VERSION, *LPAPI_VERSION;

LPAPI_VERSION
IMAGEAPI
ImagehlpApiVersion(
    VOID
);

LPAPI_VERSION
IMAGEAPI
ImagehlpApiVersionEx(
    __in LPAPI_VERSION AppVersion
);

DWORD
IMAGEAPI
GetTimestampForLoadedLibrary(
    __in HMODULE Module
);

//
// typedefs for function pointers
//
typedef BOOL
(CALLBACK* PSYM_ENUMMODULES_CALLBACK64)(
    __in PCSTR ModuleName,
    __in DWORD64 BaseOfDll,
    __in_opt PVOID UserContext
);

typedef BOOL
(CALLBACK* PSYM_ENUMMODULES_CALLBACKW64)(
    __in PCWSTR ModuleName,
    __in DWORD64 BaseOfDll,
    __in_opt PVOID UserContext
);

typedef BOOL
(CALLBACK* PENUMLOADED_MODULES_CALLBACK64)(
    __in PCSTR ModuleName,
    __in DWORD64 ModuleBase,
    __in ULONG ModuleSize,
    __in_opt PVOID UserContext
);

typedef BOOL
(CALLBACK* PENUMLOADED_MODULES_CALLBACKW64)(
    __in PCWSTR ModuleName,
    __in DWORD64 ModuleBase,
    __in ULONG ModuleSize,
    __in_opt PVOID UserContext
);

typedef BOOL
(CALLBACK* PSYM_ENUMSYMBOLS_CALLBACK64)(
    __in PCSTR SymbolName,
    __in DWORD64 SymbolAddress,
    __in ULONG SymbolSize,
    __in_opt PVOID UserContext
);

typedef BOOL
(CALLBACK* PSYM_ENUMSYMBOLS_CALLBACK64W)(
    __in PCWSTR SymbolName,
    __in DWORD64 SymbolAddress,
    __in ULONG SymbolSize,
    __in_opt PVOID UserContext
);

typedef BOOL
(CALLBACK* PSYMBOL_REGISTERED_CALLBACK64)(
    __in HANDLE hProcess,
    __in ULONG ActionCode,
    __in_opt ULONG64 CallbackData,
    __in_opt ULONG64 UserContext
);

typedef
PVOID
(CALLBACK* PSYMBOL_FUNCENTRY_CALLBACK)(
    __in HANDLE hProcess,
    __in DWORD AddrBase,
    __in_opt PVOID UserContext
);

typedef
PVOID
(CALLBACK* PSYMBOL_FUNCENTRY_CALLBACK64)(
    __in HANDLE hProcess,
    __in ULONG64 AddrBase,
    __in ULONG64 UserContext
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)

#define PSYM_ENUMMODULES_CALLBACK PSYM_ENUMMODULES_CALLBACK64
#define PSYM_ENUMSYMBOLS_CALLBACK PSYM_ENUMSYMBOLS_CALLBACK64
#define PSYM_ENUMSYMBOLS_CALLBACKW PSYM_ENUMSYMBOLS_CALLBACK64W
#define PENUMLOADED_MODULES_CALLBACK PENUMLOADED_MODULES_CALLBACK64
#define PSYMBOL_REGISTERED_CALLBACK PSYMBOL_REGISTERED_CALLBACK64
#define PSYMBOL_FUNCENTRY_CALLBACK PSYMBOL_FUNCENTRY_CALLBACK64

#else

typedef BOOL
(CALLBACK* PSYM_ENUMMODULES_CALLBACK)(
    __in PCSTR ModuleName,
    __in ULONG BaseOfDll,
    __in_opt PVOID UserContext
);

typedef BOOL
(CALLBACK* PSYM_ENUMSYMBOLS_CALLBACK)(
    __in PCSTR SymbolName,
    __in ULONG SymbolAddress,
    __in ULONG SymbolSize,
    __in_opt PVOID UserContext
);

typedef BOOL
(CALLBACK* PSYM_ENUMSYMBOLS_CALLBACKW)(
    __in PCWSTR SymbolName,
    __in ULONG SymbolAddress,
    __in ULONG SymbolSize,
    __in_opt PVOID UserContext
);

typedef BOOL
(CALLBACK* PENUMLOADED_MODULES_CALLBACK)(
    __in PCSTR ModuleName,
    __in ULONG ModuleBase,
    __in ULONG ModuleSize,
    __in_opt PVOID UserContext
);

typedef BOOL
(CALLBACK* PSYMBOL_REGISTERED_CALLBACK)(
    __in HANDLE hProcess,
    __in ULONG ActionCode,
    __in_opt PVOID CallbackData,
    __in_opt PVOID UserContext
);

#endif


// values found in SYMBOL_INFO.Tag
//
// This was taken from cvconst.h and should
// not override any values found there.
//
// #define _NO_CVCONST_H_ if you don't
// have access to that file...

#ifdef _NO_CVCONST_H

// DIA enums

enum SymTagEnum
{
    SymTagNull,
    SymTagExe,
    SymTagCompiland,
    SymTagCompilandDetails,
    SymTagCompilandEnv,
    SymTagFunction,
    SymTagBlock,
    SymTagData,
    SymTagAnnotation,
    SymTagLabel,
    SymTagPublicSymbol,
    SymTagUDT,
    SymTagEnum,
    SymTagFunctionType,
    SymTagPointerType,
    SymTagArrayType,
    SymTagBaseType,
    SymTagTypedef,
    SymTagBaseClass,
    SymTagFriend,
    SymTagFunctionArgType,
    SymTagFuncDebugStart,
    SymTagFuncDebugEnd,
    SymTagUsingNamespace,
    SymTagVTableShape,
    SymTagVTable,
    SymTagCustom,
    SymTagThunk,
    SymTagCustomType,
    SymTagManagedType,
    SymTagDimension,
    SymTagMax
};

#endif

//
// flags found in SYMBOL_INFO.Flags
//

#define SYMFLAG_VALUEPRESENT     0x00000001
#define SYMFLAG_REGISTER         0x00000008
#define SYMFLAG_REGREL           0x00000010
#define SYMFLAG_FRAMEREL         0x00000020
#define SYMFLAG_PARAMETER        0x00000040
#define SYMFLAG_LOCAL            0x00000080
#define SYMFLAG_CONSTANT         0x00000100
#define SYMFLAG_EXPORT           0x00000200
#define SYMFLAG_FORWARDER        0x00000400
#define SYMFLAG_FUNCTION         0x00000800
#define SYMFLAG_VIRTUAL          0x00001000
#define SYMFLAG_THUNK            0x00002000
#define SYMFLAG_TLSREL           0x00004000
#define SYMFLAG_SLOT             0x00008000
#define SYMFLAG_ILREL            0x00010000
#define SYMFLAG_METADATA         0x00020000
#define SYMFLAG_CLR_TOKEN        0x00040000

// this resets SymNext/Prev to the beginning
// of the module passed in the address field

#define SYMFLAG_RESET            0x80000000

//
// symbol type enumeration
//
typedef enum
{
    SymNone = 0,
    SymCoff,
    SymCv,
    SymPdb,
    SymExport,
    SymDeferred,
    SymSym,       // .sym file
    SymDia,
    SymVirtual,
    NumSymTypes
} SYM_TYPE;

//
// symbol data structure
//

typedef struct _IMAGEHLP_SYMBOL64
{
    DWORD   SizeOfStruct;           // set to sizeof(IMAGEHLP_SYMBOL64)
    DWORD64 Address;                // virtual address including dll base address
    DWORD   Size;                   // estimated size of symbol, can be zero
    DWORD   Flags;                  // info about the symbols, see the SYMF defines
    DWORD   MaxNameLength;          // maximum size of symbol name in 'Name'
    CHAR    Name[1];                // symbol name (null terminated string)
} IMAGEHLP_SYMBOL64, *PIMAGEHLP_SYMBOL64;

typedef struct _IMAGEHLP_SYMBOL64_PACKAGE
{
    IMAGEHLP_SYMBOL64 sym;
    CHAR              name[MAX_SYM_NAME + 1];
} IMAGEHLP_SYMBOL64_PACKAGE, *PIMAGEHLP_SYMBOL64_PACKAGE;

typedef struct _IMAGEHLP_SYMBOLW64
{
    DWORD   SizeOfStruct;           // set to sizeof(IMAGEHLP_SYMBOLW64)
    DWORD64 Address;                // virtual address including dll base address
    DWORD   Size;                   // estimated size of symbol, can be zero
    DWORD   Flags;                  // info about the symbols, see the SYMF defines
    DWORD   MaxNameLength;          // maximum size of symbol name in 'Name'
    WCHAR   Name[1];                // symbol name (null terminated string)
} IMAGEHLP_SYMBOLW64, *PIMAGEHLP_SYMBOLW64;

typedef struct _IMAGEHLP_SYMBOLW64_PACKAGE
{
    IMAGEHLP_SYMBOLW64 sym;
    WCHAR              name[MAX_SYM_NAME + 1];
} IMAGEHLP_SYMBOLW64_PACKAGE, *PIMAGEHLP_SYMBOLW64_PACKAGE;

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)

#define IMAGEHLP_SYMBOL IMAGEHLP_SYMBOL64
#define PIMAGEHLP_SYMBOL PIMAGEHLP_SYMBOL64
#define IMAGEHLP_SYMBOL_PACKAGE IMAGEHLP_SYMBOL64_PACKAGE
#define PIMAGEHLP_SYMBOL_PACKAGE PIMAGEHLP_SYMBOL64_PACKAGE
#define IMAGEHLP_SYMBOLW IMAGEHLP_SYMBOLW64
#define PIMAGEHLP_SYMBOLW PIMAGEHLP_SYMBOLW64
#define IMAGEHLP_SYMBOLW_PACKAGE IMAGEHLP_SYMBOLW64_PACKAGE
#define PIMAGEHLP_SYMBOLW_PACKAGE PIMAGEHLP_SYMBOLW64_PACKAGE

#else

typedef struct _IMAGEHLP_SYMBOL
{
    DWORD SizeOfStruct;           // set to sizeof(IMAGEHLP_SYMBOL)
    DWORD Address;                // virtual address including dll base address
    DWORD Size;                   // estimated size of symbol, can be zero
    DWORD Flags;                  // info about the symbols, see the SYMF defines
    DWORD                       MaxNameLength;          // maximum size of symbol name in 'Name'
    CHAR                        Name[1];                // symbol name (null terminated string)
} IMAGEHLP_SYMBOL, *PIMAGEHLP_SYMBOL;

typedef struct _IMAGEHLP_SYMBOL_PACKAGE
{
    IMAGEHLP_SYMBOL sym;
    CHAR            name[MAX_SYM_NAME + 1];
} IMAGEHLP_SYMBOL_PACKAGE, *PIMAGEHLP_SYMBOL_PACKAGE;

typedef struct _IMAGEHLP_SYMBOLW
{
    DWORD SizeOfStruct;           // set to sizeof(IMAGEHLP_SYMBOLW)
    DWORD Address;                // virtual address including dll base address
    DWORD Size;                   // estimated size of symbol, can be zero
    DWORD Flags;                  // info about the symbols, see the SYMF defines
    DWORD                       MaxNameLength;          // maximum size of symbol name in 'Name'
    WCHAR                       Name[1];                // symbol name (null terminated string)
} IMAGEHLP_SYMBOLW, *PIMAGEHLP_SYMBOLW;

typedef struct _IMAGEHLP_SYMBOLW_PACKAGE
{
    IMAGEHLP_SYMBOLW sym;
    WCHAR            name[MAX_SYM_NAME + 1];
} IMAGEHLP_SYMBOLW_PACKAGE, *PIMAGEHLP_SYMBOLW_PACKAGE;

#endif

//
// module data structure
//

typedef struct _IMAGEHLP_MODULE64
{
    DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_MODULE64)
    DWORD64  BaseOfImage;            // base load address of module
    DWORD    ImageSize;              // virtual size of the loaded module
    DWORD    TimeDateStamp;          // date/time stamp from pe header
    DWORD    CheckSum;               // checksum from the pe header
    DWORD    NumSyms;                // number of symbols in the symbol table
    SYM_TYPE SymType;                // type of symbols loaded
    CHAR     ModuleName[32];         // module name
    CHAR     ImageName[256];         // image name
    CHAR     LoadedImageName[256];   // symbol file name
    // new elements: 07-Jun-2002
    CHAR     LoadedPdbName[256];     // pdb file name
    DWORD    CVSig;                  // Signature of the CV record in the debug directories
    CHAR     CVData[MAX_PATH * 3];   // Contents of the CV record
    DWORD    PdbSig;                 // Signature of PDB
    GUID     PdbSig70;               // Signature of PDB (VC 7 and up)
    DWORD    PdbAge;                 // DBI age of pdb
    BOOL     PdbUnmatched;           // loaded an unmatched pdb
    BOOL     DbgUnmatched;           // loaded an unmatched dbg
    BOOL     LineNumbers;            // we have line number information
    BOOL     GlobalSymbols;          // we have internal symbol information
    BOOL     TypeInfo;               // we have type information
    // new elements: 17-Dec-2003
    BOOL     SourceIndexed;          // pdb supports source server
    BOOL     Publics;                // contains public symbols
} IMAGEHLP_MODULE64, *PIMAGEHLP_MODULE64;

typedef struct _IMAGEHLP_MODULEW64
{
    DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_MODULE64)
    DWORD64  BaseOfImage;            // base load address of module
    DWORD    ImageSize;              // virtual size of the loaded module
    DWORD    TimeDateStamp;          // date/time stamp from pe header
    DWORD    CheckSum;               // checksum from the pe header
    DWORD    NumSyms;                // number of symbols in the symbol table
    SYM_TYPE SymType;                // type of symbols loaded
    WCHAR    ModuleName[32];         // module name
    WCHAR    ImageName[256];         // image name
    // new elements: 07-Jun-2002
    WCHAR    LoadedImageName[256];   // symbol file name
    WCHAR    LoadedPdbName[256];     // pdb file name
    DWORD    CVSig;                  // Signature of the CV record in the debug directories
    WCHAR        CVData[MAX_PATH * 3];   // Contents of the CV record
    DWORD    PdbSig;                 // Signature of PDB
    GUID     PdbSig70;               // Signature of PDB (VC 7 and up)
    DWORD    PdbAge;                 // DBI age of pdb
    BOOL     PdbUnmatched;           // loaded an unmatched pdb
    BOOL     DbgUnmatched;           // loaded an unmatched dbg
    BOOL     LineNumbers;            // we have line number information
    BOOL     GlobalSymbols;          // we have internal symbol information
    BOOL     TypeInfo;               // we have type information
    // new elements: 17-Dec-2003
    BOOL     SourceIndexed;          // pdb supports source server
    BOOL     Publics;                // contains public symbols
} IMAGEHLP_MODULEW64, *PIMAGEHLP_MODULEW64;

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define IMAGEHLP_MODULE IMAGEHLP_MODULE64
#define PIMAGEHLP_MODULE PIMAGEHLP_MODULE64
#define IMAGEHLP_MODULEW IMAGEHLP_MODULEW64
#define PIMAGEHLP_MODULEW PIMAGEHLP_MODULEW64
#else
typedef struct _IMAGEHLP_MODULE
{
    DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_MODULE)
    DWORD    BaseOfImage;            // base load address of module
    DWORD    ImageSize;              // virtual size of the loaded module
    DWORD    TimeDateStamp;          // date/time stamp from pe header
    DWORD    CheckSum;               // checksum from the pe header
    DWORD    NumSyms;                // number of symbols in the symbol table
    SYM_TYPE SymType;                // type of symbols loaded
    CHAR     ModuleName[32];         // module name
    CHAR     ImageName[256];         // image name
    CHAR     LoadedImageName[256];   // symbol file name
} IMAGEHLP_MODULE, *PIMAGEHLP_MODULE;

typedef struct _IMAGEHLP_MODULEW
{
    DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_MODULE)
    DWORD    BaseOfImage;            // base load address of module
    DWORD    ImageSize;              // virtual size of the loaded module
    DWORD    TimeDateStamp;          // date/time stamp from pe header
    DWORD    CheckSum;               // checksum from the pe header
    DWORD    NumSyms;                // number of symbols in the symbol table
    SYM_TYPE SymType;                // type of symbols loaded
    WCHAR    ModuleName[32];         // module name
    WCHAR    ImageName[256];         // image name
    WCHAR    LoadedImageName[256];   // symbol file name
} IMAGEHLP_MODULEW, *PIMAGEHLP_MODULEW;
#endif

//
// source file line data structure
//

typedef struct _IMAGEHLP_LINE64
{
    DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_LINE64)
    PVOID    Key;                    // internal
    DWORD    LineNumber;             // line number in file
    PCHAR    FileName;               // full filename
    DWORD64  Address;                // first instruction of line
} IMAGEHLP_LINE64, *PIMAGEHLP_LINE64;

typedef struct _IMAGEHLP_LINEW64
{
    DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_LINE64)
    PVOID    Key;                    // internal
    DWORD    LineNumber;             // line number in file
    PWSTR    FileName;               // full filename
    DWORD64  Address;                // first instruction of line
} IMAGEHLP_LINEW64, *PIMAGEHLP_LINEW64;

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define IMAGEHLP_LINE IMAGEHLP_LINE64
#define PIMAGEHLP_LINE PIMAGEHLP_LINE64
#else
typedef struct _IMAGEHLP_LINE
{
    DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_LINE)
    PVOID    Key;                    // internal
    DWORD    LineNumber;             // line number in file
    PCHAR    FileName;               // full filename
    DWORD    Address;                // first instruction of line
} IMAGEHLP_LINE, *PIMAGEHLP_LINE;

typedef struct _IMAGEHLP_LINEW
{
    DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_LINE64)
    PVOID    Key;                    // internal
    DWORD    LineNumber;             // line number in file
    PCHAR    FileName;               // full filename
    DWORD64  Address;                // first instruction of line
} IMAGEHLP_LINEW, *PIMAGEHLP_LINEW;
#endif

//
// source file structure
//

typedef struct _SOURCEFILE
{
    DWORD64  ModBase;                // base address of loaded module
    PCHAR    FileName;               // full filename of source
} SOURCEFILE, *PSOURCEFILE;

typedef struct _SOURCEFILEW
{
    DWORD64  ModBase;                // base address of loaded module
    PWSTR    FileName;               // full filename of source
} SOURCEFILEW, *PSOURCEFILEW;

//
// data structures used for registered symbol callbacks
//

#define CBA_DEFERRED_SYMBOL_LOAD_START          0x00000001
#define CBA_DEFERRED_SYMBOL_LOAD_COMPLETE       0x00000002
#define CBA_DEFERRED_SYMBOL_LOAD_FAILURE        0x00000003
#define CBA_SYMBOLS_UNLOADED                    0x00000004
#define CBA_DUPLICATE_SYMBOL                    0x00000005
#define CBA_READ_MEMORY                         0x00000006
#define CBA_DEFERRED_SYMBOL_LOAD_CANCEL         0x00000007
#define CBA_SET_OPTIONS                         0x00000008
#define CBA_EVENT                               0x00000010
#define CBA_DEFERRED_SYMBOL_LOAD_PARTIAL        0x00000020
#define CBA_DEBUG_INFO                          0x10000000
#define CBA_SRCSRV_INFO                         0x20000000
#define CBA_SRCSRV_EVENT                        0x40000000

typedef struct _IMAGEHLP_CBA_READ_MEMORY
{
    DWORD64   addr;                                     // address to read from
    PVOID     buf;                                      // buffer to read to
    DWORD     bytes;                                    // amount of bytes to read
    DWORD*    bytesread;                                // pointer to store amount of bytes read
} IMAGEHLP_CBA_READ_MEMORY, *PIMAGEHLP_CBA_READ_MEMORY;

enum
{
    sevInfo = 0,
    sevProblem,
    sevAttn,
    sevFatal,
    sevMax  // unused
};

#define EVENT_SRCSPEW_START 100
#define EVENT_SRCSPEW       100
#define EVENT_SRCSPEW_END   199

typedef struct _IMAGEHLP_CBA_EVENT
{
    DWORD severity;                                     // values from sevInfo to sevFatal
    DWORD code;                                         // numerical code IDs the error
    PCHAR desc;                                         // may contain a text description of the error
    PVOID object;                                       // value dependant upon the error code
} IMAGEHLP_CBA_EVENT, *PIMAGEHLP_CBA_EVENT;

typedef struct _IMAGEHLP_CBA_EVENTW
{
    DWORD  severity;                                     // values from sevInfo to sevFatal
    DWORD  code;                                         // numerical code IDs the error
    PCWSTR desc;                                         // may contain a text description of the error
    PVOID  object;                                       // value dependant upon the error code
} IMAGEHLP_CBA_EVENTW, *PIMAGEHLP_CBA_EVENTW;

typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOAD64
{
    DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_DEFERRED_SYMBOL_LOAD64)
    DWORD64  BaseOfImage;            // base load address of module
    DWORD    CheckSum;               // checksum from the pe header
    DWORD    TimeDateStamp;          // date/time stamp from pe header
    CHAR     FileName[MAX_PATH];     // symbols file or image name
    BOOLEAN  Reparse;                // load failure reparse
    HANDLE   hFile;                  // file handle, if passed
    DWORD    Flags;                     //
} IMAGEHLP_DEFERRED_SYMBOL_LOAD64, *PIMAGEHLP_DEFERRED_SYMBOL_LOAD64;

typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOADW64
{
    DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_DEFERRED_SYMBOL_LOADW64)
    DWORD64  BaseOfImage;            // base load address of module
    DWORD    CheckSum;               // checksum from the pe header
    DWORD    TimeDateStamp;          // date/time stamp from pe header
    WCHAR    FileName[MAX_PATH + 1]; // symbols file or image name
    BOOLEAN  Reparse;                // load failure reparse
    HANDLE   hFile;                  // file handle, if passed
    DWORD    Flags;         //
} IMAGEHLP_DEFERRED_SYMBOL_LOADW64, *PIMAGEHLP_DEFERRED_SYMBOL_LOADW64;

#define DSLFLAG_MISMATCHED_PDB  0x1
#define DSLFLAG_MISMATCHED_DBG  0x2

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define IMAGEHLP_DEFERRED_SYMBOL_LOAD IMAGEHLP_DEFERRED_SYMBOL_LOAD64
#define PIMAGEHLP_DEFERRED_SYMBOL_LOAD PIMAGEHLP_DEFERRED_SYMBOL_LOAD64
#else
typedef struct _IMAGEHLP_DEFERRED_SYMBOL_LOAD
{
    DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_DEFERRED_SYMBOL_LOAD)
    DWORD    BaseOfImage;            // base load address of module
    DWORD    CheckSum;               // checksum from the pe header
    DWORD    TimeDateStamp;          // date/time stamp from pe header
    CHAR     FileName[MAX_PATH];     // symbols file or image name
    BOOLEAN  Reparse;                // load failure reparse
    HANDLE   hFile;                  // file handle, if passed
} IMAGEHLP_DEFERRED_SYMBOL_LOAD, *PIMAGEHLP_DEFERRED_SYMBOL_LOAD;
#endif

typedef struct _IMAGEHLP_DUPLICATE_SYMBOL64
{
    DWORD              SizeOfStruct;           // set to sizeof(IMAGEHLP_DUPLICATE_SYMBOL64)
    DWORD              NumberOfDups;           // number of duplicates in the Symbol array
    PIMAGEHLP_SYMBOL64 Symbol;                 // array of duplicate symbols
    DWORD              SelectedSymbol;         // symbol selected (-1 to start)
} IMAGEHLP_DUPLICATE_SYMBOL64, *PIMAGEHLP_DUPLICATE_SYMBOL64;

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define IMAGEHLP_DUPLICATE_SYMBOL IMAGEHLP_DUPLICATE_SYMBOL64
#define PIMAGEHLP_DUPLICATE_SYMBOL PIMAGEHLP_DUPLICATE_SYMBOL64
#else
typedef struct _IMAGEHLP_DUPLICATE_SYMBOL
{
    DWORD            SizeOfStruct;           // set to sizeof(IMAGEHLP_DUPLICATE_SYMBOL)
    DWORD            NumberOfDups;           // number of duplicates in the Symbol array
    PIMAGEHLP_SYMBOL Symbol;                 // array of duplicate symbols
    DWORD            SelectedSymbol;         // symbol selected (-1 to start)
} IMAGEHLP_DUPLICATE_SYMBOL, *PIMAGEHLP_DUPLICATE_SYMBOL;
#endif

// If dbghelp ever needs to display graphical UI, it will use this as the parent window.

BOOL
IMAGEAPI
SymSetParentWindow(
    __in HWND hwnd
);

PCHAR
IMAGEAPI
SymSetHomeDirectory(
    __in_opt HANDLE hProcess,
    __in_opt PCSTR dir
);

PWSTR
IMAGEAPI
SymSetHomeDirectoryW(
    __in_opt HANDLE hProcess,
    __in_opt PCWSTR dir
);

PCHAR
IMAGEAPI
SymGetHomeDirectory(
    __in DWORD type,
    __out_ecount(size) PSTR dir,
    __in size_t size
);

PWSTR
IMAGEAPI
SymGetHomeDirectoryW(
    __in DWORD type,
    __out_ecount(size) PWSTR dir,
    __in size_t size
);

enum
{
    hdBase = 0, // root directory for dbghelp
    hdSym,      // where symbols are stored
    hdSrc,      // where source is stored
    hdMax       // end marker
};

typedef struct _OMAP
{
    ULONG  rva;
    ULONG  rvaTo;
} OMAP, *POMAP;

BOOL
IMAGEAPI
SymGetOmaps(
    __in HANDLE hProcess,
    __in DWORD64 BaseOfDll,
    __out POMAP* OmapTo,
    __out PDWORD64 cOmapTo,
    __out POMAP* OmapFrom,
    __out PDWORD64 cOmapFrom
);

//
// options that are set/returned by SymSetOptions() & SymGetOptions()
// these are used as a mask
//
#define SYMOPT_CASE_INSENSITIVE          0x00000001
#define SYMOPT_UNDNAME                   0x00000002
#define SYMOPT_DEFERRED_LOADS            0x00000004
#define SYMOPT_NO_CPP                    0x00000008
#define SYMOPT_LOAD_LINES                0x00000010
#define SYMOPT_OMAP_FIND_NEAREST         0x00000020
#define SYMOPT_LOAD_ANYTHING             0x00000040
#define SYMOPT_IGNORE_CVREC              0x00000080
#define SYMOPT_NO_UNQUALIFIED_LOADS      0x00000100
#define SYMOPT_FAIL_CRITICAL_ERRORS      0x00000200
#define SYMOPT_EXACT_SYMBOLS             0x00000400
#define SYMOPT_ALLOW_ABSOLUTE_SYMBOLS    0x00000800
#define SYMOPT_IGNORE_NT_SYMPATH         0x00001000
#define SYMOPT_INCLUDE_32BIT_MODULES     0x00002000
#define SYMOPT_PUBLICS_ONLY              0x00004000
#define SYMOPT_NO_PUBLICS                0x00008000
#define SYMOPT_AUTO_PUBLICS              0x00010000
#define SYMOPT_NO_IMAGE_SEARCH           0x00020000
#define SYMOPT_SECURE                    0x00040000
#define SYMOPT_NO_PROMPTS                0x00080000
#define SYMOPT_OVERWRITE                 0x00100000
#define SYMOPT_IGNORE_IMAGEDIR           0x00200000
#define SYMOPT_FLAT_DIRECTORY            0x00400000
#define SYMOPT_FAVOR_COMPRESSED          0x00800000
#define SYMOPT_ALLOW_ZERO_ADDRESS        0x01000000
#define SYMOPT_DISABLE_SYMSRV_AUTODETECT 0x02000000

#define SYMOPT_DEBUG                     0x80000000

DWORD
IMAGEAPI
SymSetOptions(
    __in DWORD   SymOptions
);

DWORD
IMAGEAPI
SymGetOptions(
    VOID
);

BOOL
IMAGEAPI
SymCleanup(
    __in HANDLE hProcess
);

BOOL
IMAGEAPI
SymMatchString(
    __in PCSTR string,
    __in PCSTR expression,
    __in BOOL fCase
);

BOOL
IMAGEAPI
SymMatchStringA(
    __in PCSTR string,
    __in PCSTR expression,
    __in BOOL fCase
);

BOOL
IMAGEAPI
SymMatchStringW(
    __in PCWSTR string,
    __in PCWSTR expression,
    __in BOOL fCase
);

typedef BOOL
(CALLBACK* PSYM_ENUMSOURCEFILES_CALLBACK)(
    __in PSOURCEFILE pSourceFile,
    __in_opt PVOID UserContext
);

// for backwards compatibility - don't use this
#define PSYM_ENUMSOURCFILES_CALLBACK PSYM_ENUMSOURCEFILES_CALLBACK

BOOL
IMAGEAPI
SymEnumSourceFiles(
    __in HANDLE hProcess,
    __in ULONG64 ModBase,
    __in_opt PCSTR Mask,
    __in PSYM_ENUMSOURCEFILES_CALLBACK cbSrcFiles,
    __in_opt PVOID UserContext
);

typedef BOOL
(CALLBACK* PSYM_ENUMSOURCEFILES_CALLBACKW)(
    __in PSOURCEFILEW pSourceFile,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymEnumSourceFilesW(
    __in HANDLE hProcess,
    __in ULONG64 ModBase,
    __in_opt PCWSTR Mask,
    __in PSYM_ENUMSOURCEFILES_CALLBACKW cbSrcFiles,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymEnumerateModules64(
    __in HANDLE hProcess,
    __in PSYM_ENUMMODULES_CALLBACK64 EnumModulesCallback,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymEnumerateModulesW64(
    __in HANDLE hProcess,
    __in PSYM_ENUMMODULES_CALLBACKW64 EnumModulesCallback,
    __in_opt PVOID UserContext
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymEnumerateModules SymEnumerateModules64
#else
BOOL
IMAGEAPI
SymEnumerateModules(
    __in HANDLE hProcess,
    __in PSYM_ENUMMODULES_CALLBACK EnumModulesCallback,
    __in_opt PVOID UserContext
);
#endif

BOOL
IMAGEAPI
EnumerateLoadedModulesEx(
    __in HANDLE hProcess,
    __in PENUMLOADED_MODULES_CALLBACK64 EnumLoadedModulesCallback,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
EnumerateLoadedModulesExW(
    __in HANDLE hProcess,
    __in PENUMLOADED_MODULES_CALLBACKW64 EnumLoadedModulesCallback,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
EnumerateLoadedModules64(
    __in HANDLE hProcess,
    __in PENUMLOADED_MODULES_CALLBACK64 EnumLoadedModulesCallback,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
EnumerateLoadedModulesW64(
    __in HANDLE hProcess,
    __in PENUMLOADED_MODULES_CALLBACKW64 EnumLoadedModulesCallback,
    __in_opt PVOID UserContext
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define EnumerateLoadedModules EnumerateLoadedModules64
#else
BOOL
IMAGEAPI
EnumerateLoadedModules(
    __in HANDLE hProcess,
    __in PENUMLOADED_MODULES_CALLBACK EnumLoadedModulesCallback,
    __in_opt PVOID UserContext
);
#endif

PVOID
IMAGEAPI
SymFunctionTableAccess64(
    __in HANDLE hProcess,
    __in DWORD64 AddrBase
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymFunctionTableAccess SymFunctionTableAccess64
#else
PVOID
IMAGEAPI
SymFunctionTableAccess(
    __in HANDLE hProcess,
    __in DWORD AddrBase
);
#endif

BOOL
IMAGEAPI
SymGetUnwindInfo(
    __in HANDLE hProcess,
    __in DWORD64 Address,
    __out_bcount_opt(*Size) PVOID Buffer,
    __inout PULONG Size
);

BOOL
IMAGEAPI
SymGetModuleInfo64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr,
    __out PIMAGEHLP_MODULE64 ModuleInfo
);

BOOL
IMAGEAPI
SymGetModuleInfoW64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr,
    __out PIMAGEHLP_MODULEW64 ModuleInfo
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymGetModuleInfo   SymGetModuleInfo64
#define SymGetModuleInfoW  SymGetModuleInfoW64
#else
BOOL
IMAGEAPI
SymGetModuleInfo(
    __in HANDLE hProcess,
    __in DWORD dwAddr,
    __out PIMAGEHLP_MODULE ModuleInfo
);

BOOL
IMAGEAPI
SymGetModuleInfoW(
    __in HANDLE hProcess,
    __in DWORD dwAddr,
    __out PIMAGEHLP_MODULEW ModuleInfo
);
#endif

DWORD64
IMAGEAPI
SymGetModuleBase64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymGetModuleBase SymGetModuleBase64
#else
DWORD
IMAGEAPI
SymGetModuleBase(
    __in HANDLE hProcess,
    __in DWORD dwAddr
);
#endif

typedef struct _SRCCODEINFO
{
    DWORD   SizeOfStruct;           // set to sizeof(SRCCODEINFO)
    PVOID   Key;                    // not used
    DWORD64 ModBase;                // base address of module this applies to
    CHAR    Obj[MAX_PATH + 1];      // the object file within the module
    CHAR    FileName[MAX_PATH + 1]; // full filename
    DWORD   LineNumber;             // line number in file
    DWORD64 Address;                // first instruction of line
} SRCCODEINFO, *PSRCCODEINFO;

typedef struct _SRCCODEINFOW
{
    DWORD   SizeOfStruct;           // set to sizeof(SRCCODEINFO)
    PVOID   Key;                    // not used
    DWORD64 ModBase;                // base address of module this applies to
    WCHAR   Obj[MAX_PATH + 1];      // the object file within the module
    WCHAR   FileName[MAX_PATH + 1]; // full filename
    DWORD   LineNumber;             // line number in file
    DWORD64 Address;                // first instruction of line
} SRCCODEINFOW, *PSRCCODEINFOW;

typedef BOOL
(CALLBACK* PSYM_ENUMLINES_CALLBACK)(
    __in PSRCCODEINFO LineInfo,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymEnumLines(
    __in HANDLE hProcess,
    __in ULONG64 Base,
    __in_opt PCSTR Obj,
    __in_opt PCSTR File,
    __in PSYM_ENUMLINES_CALLBACK EnumLinesCallback,
    __in_opt PVOID UserContext
);

typedef BOOL
(CALLBACK* PSYM_ENUMLINES_CALLBACKW)(
    __in PSRCCODEINFOW LineInfo,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymEnumLinesW(
    __in HANDLE hProcess,
    __in ULONG64 Base,
    __in_opt PCWSTR Obj,
    __in_opt PCWSTR File,
    __in PSYM_ENUMLINES_CALLBACKW EnumLinesCallback,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymGetLineFromAddr64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr,
    __out PDWORD pdwDisplacement,
    __out PIMAGEHLP_LINE64 Line64
);

BOOL
IMAGEAPI
SymGetLineFromAddrW64(
    __in HANDLE hProcess,
    __in DWORD64 dwAddr,
    __out PDWORD pdwDisplacement,
    __out PIMAGEHLP_LINEW64 Line
);

BOOL
IMAGEAPI
SymEnumSourceLines(
    __in HANDLE hProcess,
    __in ULONG64 Base,
    __in_opt PCSTR Obj,
    __in_opt PCSTR File,
    __in_opt DWORD Line,
    __in DWORD Flags,
    __in PSYM_ENUMLINES_CALLBACK EnumLinesCallback,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymEnumSourceLinesW(
    __in HANDLE hProcess,
    __in ULONG64 Base,
    __in_opt PCWSTR Obj,
    __in_opt PCWSTR File,
    __in_opt DWORD Line,
    __in DWORD Flags,
    __in PSYM_ENUMLINES_CALLBACKW EnumLinesCallback,
    __in_opt PVOID UserContext
);

// flags for SymEnumSourceLines

#define ESLFLAG_FULLPATH        0x1
#define ESLFLAG_NEAREST         0x2
#define ESLFLAG_PREV            0x4
#define ESLFLAG_NEXT            0x8

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymGetLineFromAddr SymGetLineFromAddr64
#define SymGetLineFromAddrW SymGetLineFromAddrW64
#else
BOOL
IMAGEAPI
SymGetLineFromAddr(
    __in HANDLE hProcess,
    __in DWORD dwAddr,
    __out PDWORD pdwDisplacement,
    __out PIMAGEHLP_LINE Line
);

BOOL
IMAGEAPI
SymGetLineFromAddrW(
    __in HANDLE hProcess,
    __in DWORD dwAddr,
    __out PDWORD pdwDisplacement,
    __out PIMAGEHLP_LINEW Line
);
#endif

BOOL
IMAGEAPI
SymGetLineFromName64(
    __in HANDLE hProcess,
    __in_opt PCSTR ModuleName,
    __in_opt PCSTR FileName,
    __in DWORD dwLineNumber,
    __out PLONG plDisplacement,
    __inout PIMAGEHLP_LINE64 Line
);

BOOL
IMAGEAPI
SymGetLineFromNameW64(
    __in HANDLE hProcess,
    __in_opt PCWSTR ModuleName,
    __in_opt PCWSTR FileName,
    __in DWORD dwLineNumber,
    __out PLONG plDisplacement,
    __inout PIMAGEHLP_LINEW64 Line
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymGetLineFromName SymGetLineFromName64
#else
BOOL
IMAGEAPI
SymGetLineFromName(
    __in HANDLE hProcess,
    __in_opt PCSTR ModuleName,
    __in_opt PCSTR FileName,
    __in DWORD dwLineNumber,
    __out PLONG plDisplacement,
    __inout PIMAGEHLP_LINE Line
);
#endif

BOOL
IMAGEAPI
SymGetLineNext64(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_LINE64 Line
);

BOOL
IMAGEAPI
SymGetLineNextW64(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_LINEW64 Line
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymGetLineNext SymGetLineNext64
#else
BOOL
IMAGEAPI
SymGetLineNext(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_LINE Line
);

BOOL
IMAGEAPI
SymGetLineNextW(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_LINEW Line
);
#endif

BOOL
IMAGEAPI
SymGetLinePrev64(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_LINE64 Line
);

BOOL
IMAGEAPI
SymGetLinePrevW64(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_LINEW64 Line
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymGetLinePrev SymGetLinePrev64
#else
BOOL
IMAGEAPI
SymGetLinePrev(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_LINE Line
);

BOOL
IMAGEAPI
SymGetLinePrevW(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_LINEW Line
);
#endif

ULONG
IMAGEAPI
SymGetFileLineOffsets64(
    __in HANDLE hProcess,
    __in_opt PCSTR ModuleName,
    __in PCSTR FileName,
    __out_ecount(BufferLines) PDWORD64 Buffer,
    __in ULONG BufferLines
);

BOOL
IMAGEAPI
SymMatchFileName(
    __in PCSTR FileName,
    __in PCSTR Match,
    __deref_opt_out PSTR* FileNameStop,
    __deref_opt_out PSTR* MatchStop
);

BOOL
IMAGEAPI
SymMatchFileNameW(
    __in PCWSTR FileName,
    __in PCWSTR Match,
    __deref_opt_out PWSTR* FileNameStop,
    __deref_opt_out PWSTR* MatchStop
);

BOOL
IMAGEAPI
SymGetSourceFile(
    __in HANDLE hProcess,
    __in ULONG64 Base,
    __in_opt PCSTR Params,
    __in PCSTR FileSpec,
    __out_ecount(Size) PSTR FilePath,
    __in DWORD Size
);

BOOL
IMAGEAPI
SymGetSourceFileW(
    __in HANDLE hProcess,
    __in ULONG64 Base,
    __in_opt PCWSTR Params,
    __in PCWSTR FileSpec,
    __out_ecount(Size) PWSTR FilePath,
    __in DWORD Size
);

BOOL
IMAGEAPI
SymGetSourceFileToken(
    __in HANDLE hProcess,
    __in ULONG64 Base,
    __in PCSTR FileSpec,
    __deref_out PVOID* Token,
    __out DWORD* Size
);

BOOL
IMAGEAPI
SymGetSourceFileTokenW(
    __in HANDLE hProcess,
    __in ULONG64 Base,
    __in PCWSTR FileSpec,
    __deref_out PVOID* Token,
    __out DWORD* Size
);

BOOL
IMAGEAPI
SymGetSourceFileFromToken(
    __in HANDLE hProcess,
    __in PVOID Token,
    __in_opt PCSTR Params,
    __out_ecount(Size) PSTR FilePath,
    __in DWORD Size
);

BOOL
IMAGEAPI
SymGetSourceFileFromTokenW(
    __in HANDLE hProcess,
    __in PVOID Token,
    __in_opt PCWSTR Params,
    __out_ecount(Size) PWSTR FilePath,
    __in DWORD Size
);

BOOL
IMAGEAPI
SymGetSourceVarFromToken(
    __in HANDLE hProcess,
    __in PVOID Token,
    __in_opt PCSTR Params,
    __in PCSTR VarName,
    __out_ecount(Size) PSTR Value,
    __in DWORD Size
);

BOOL
IMAGEAPI
SymGetSourceVarFromTokenW(
    __in HANDLE hProcess,
    __in PVOID Token,
    __in_opt PCWSTR Params,
    __in PCWSTR VarName,
    __out_ecount(Size) PWSTR Value,
    __in DWORD Size
);

typedef BOOL (CALLBACK* PENUMSOURCEFILETOKENSCALLBACK)(__in PVOID token,  __in size_t size);

BOOL
IMAGEAPI
SymEnumSourceFileTokens(
    __in HANDLE hProcess,
    __in ULONG64 Base,
    __in PENUMSOURCEFILETOKENSCALLBACK Callback
);

BOOL
IMAGEAPI
SymInitialize(
    __in HANDLE hProcess,
    __in_opt PCSTR UserSearchPath,
    __in BOOL fInvadeProcess
);

BOOL
IMAGEAPI
SymInitializeW(
    __in HANDLE hProcess,
    __in_opt PCWSTR UserSearchPath,
    __in BOOL fInvadeProcess
);

BOOL
IMAGEAPI
SymGetSearchPath(
    __in HANDLE hProcess,
    __out_ecount(SearchPathLength) PSTR SearchPath,
    __in DWORD SearchPathLength
);

BOOL
IMAGEAPI
SymGetSearchPathW(
    __in HANDLE hProcess,
    __out_ecount(SearchPathLength) PWSTR SearchPath,
    __in DWORD SearchPathLength
);

BOOL
IMAGEAPI
SymSetSearchPath(
    __in HANDLE hProcess,
    __in_opt PCSTR SearchPath
);

BOOL
IMAGEAPI
SymSetSearchPathW(
    __in HANDLE hProcess,
    __in_opt PCWSTR SearchPath
);

#define SLMFLAG_VIRTUAL     0x1
#define SLMFLAG_ALT_INDEX   0x2
#define SLMFLAG_NO_SYMBOLS  0x4

DWORD64
IMAGEAPI
SymLoadModuleEx(
    __in HANDLE hProcess,
    __in_opt HANDLE hFile,
    __in_opt PCSTR ImageName,
    __in_opt PCSTR ModuleName,
    __in DWORD64 BaseOfDll,
    __in DWORD DllSize,
    __in_opt PMODLOAD_DATA Data,
    __in_opt DWORD Flags
);

DWORD64
IMAGEAPI
SymLoadModuleExW(
    __in HANDLE hProcess,
    __in_opt HANDLE hFile,
    __in_opt PCWSTR ImageName,
    __in_opt PCWSTR ModuleName,
    __in DWORD64 BaseOfDll,
    __in DWORD DllSize,
    __in_opt PMODLOAD_DATA Data,
    __in_opt DWORD Flags
);

BOOL
IMAGEAPI
SymUnloadModule64(
    __in HANDLE hProcess,
    __in DWORD64 BaseOfDll
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymUnloadModule SymUnloadModule64
#else
BOOL
IMAGEAPI
SymUnloadModule(
    __in HANDLE hProcess,
    __in DWORD BaseOfDll
);
#endif

BOOL
IMAGEAPI
SymUnDName64(
    __in PIMAGEHLP_SYMBOL64 sym,            // Symbol to undecorate
    __out_ecount(UnDecNameLength) PSTR UnDecName,   // Buffer to store undecorated name in
    __in DWORD UnDecNameLength              // Size of the buffer
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymUnDName SymUnDName64
#else
BOOL
IMAGEAPI
SymUnDName(
    __in PIMAGEHLP_SYMBOL sym,              // Symbol to undecorate
    __out_ecount(UnDecNameLength) PSTR UnDecName,   // Buffer to store undecorated name in
    __in DWORD UnDecNameLength              // Size of the buffer
);
#endif

BOOL
IMAGEAPI
SymRegisterCallback64(
    __in HANDLE hProcess,
    __in PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,
    __in ULONG64 UserContext
);

BOOL
IMAGEAPI
SymRegisterCallbackW64(
    __in HANDLE hProcess,
    __in PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,
    __in ULONG64 UserContext
);

BOOL
IMAGEAPI
SymRegisterFunctionEntryCallback64(
    __in HANDLE hProcess,
    __in PSYMBOL_FUNCENTRY_CALLBACK64 CallbackFunction,
    __in ULONG64 UserContext
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymRegisterCallback SymRegisterCallback64
#define SymRegisterFunctionEntryCallback SymRegisterFunctionEntryCallback64
#else
BOOL
IMAGEAPI
SymRegisterCallback(
    __in HANDLE hProcess,
    __in PSYMBOL_REGISTERED_CALLBACK CallbackFunction,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymRegisterFunctionEntryCallback(
    __in HANDLE hProcess,
    __in PSYMBOL_FUNCENTRY_CALLBACK CallbackFunction,
    __in_opt PVOID UserContext
);
#endif


typedef struct _IMAGEHLP_SYMBOL_SRC
{
    DWORD sizeofstruct;
    DWORD type;
    char  file[MAX_PATH];
} IMAGEHLP_SYMBOL_SRC, *PIMAGEHLP_SYMBOL_SRC;

typedef struct _MODULE_TYPE_INFO   // AKA TYPTYP
{
    USHORT      dataLength;
    USHORT      leaf;
    BYTE        data[1];
} MODULE_TYPE_INFO, *PMODULE_TYPE_INFO;

typedef struct _SYMBOL_INFO
{
    ULONG       SizeOfStruct;
    ULONG       TypeIndex;        // Type Index of symbol
    ULONG64     Reserved[2];
    ULONG       Index;
    ULONG       Size;
    ULONG64     ModBase;          // Base Address of module comtaining this symbol
    ULONG       Flags;
    ULONG64     Value;            // Value of symbol, ValuePresent should be 1
    ULONG64     Address;          // Address of symbol including base address of module
    ULONG       Register;         // register holding value or pointer to value
    ULONG       Scope;            // scope of the symbol
    ULONG       Tag;              // pdb classification
    ULONG       NameLen;          // Actual length of name
    ULONG       MaxNameLen;
    CHAR        Name[1];          // Name of symbol
} SYMBOL_INFO, *PSYMBOL_INFO;

typedef struct _SYMBOL_INFO_PACKAGE
{
    SYMBOL_INFO si;
    CHAR        name[MAX_SYM_NAME + 1];
} SYMBOL_INFO_PACKAGE, *PSYMBOL_INFO_PACKAGE;

typedef struct _SYMBOL_INFOW
{
    ULONG       SizeOfStruct;
    ULONG       TypeIndex;        // Type Index of symbol
    ULONG64     Reserved[2];
    ULONG       Index;
    ULONG       Size;
    ULONG64     ModBase;          // Base Address of module comtaining this symbol
    ULONG       Flags;
    ULONG64     Value;            // Value of symbol, ValuePresent should be 1
    ULONG64     Address;          // Address of symbol including base address of module
    ULONG       Register;         // register holding value or pointer to value
    ULONG       Scope;            // scope of the symbol
    ULONG       Tag;              // pdb classification
    ULONG       NameLen;          // Actual length of name
    ULONG       MaxNameLen;
    WCHAR       Name[1];          // Name of symbol
} SYMBOL_INFOW, *PSYMBOL_INFOW;

typedef struct _SYMBOL_INFO_PACKAGEW
{
    SYMBOL_INFOW si;
    WCHAR        name[MAX_SYM_NAME + 1];
} SYMBOL_INFO_PACKAGEW, *PSYMBOL_INFO_PACKAGEW;

typedef struct _IMAGEHLP_STACK_FRAME
{
    ULONG64 InstructionOffset;
    ULONG64 ReturnOffset;
    ULONG64 FrameOffset;
    ULONG64 StackOffset;
    ULONG64 BackingStoreOffset;
    ULONG64 FuncTableEntry;
    ULONG64 Params[4];
    ULONG64 Reserved[5];
    BOOL    Virtual;
    ULONG   Reserved2;
} IMAGEHLP_STACK_FRAME, *PIMAGEHLP_STACK_FRAME;

typedef VOID IMAGEHLP_CONTEXT, *PIMAGEHLP_CONTEXT;


BOOL
IMAGEAPI
SymSetContext(
    __in HANDLE hProcess,
    __in PIMAGEHLP_STACK_FRAME StackFrame,
    __in_opt PIMAGEHLP_CONTEXT Context
);

BOOL
IMAGEAPI
SymSetScopeFromAddr(
    __in HANDLE hProcess,
    __in ULONG64 Address
);

BOOL
IMAGEAPI
SymSetScopeFromIndex(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in DWORD Index
);

typedef BOOL
(CALLBACK* PSYM_ENUMPROCESSES_CALLBACK)(
    __in HANDLE hProcess,
    __in PVOID UserContext
);

BOOL
IMAGEAPI
SymEnumProcesses(
    __in PSYM_ENUMPROCESSES_CALLBACK EnumProcessesCallback,
    __in PVOID UserContext
);

BOOL
IMAGEAPI
SymFromAddr(
    __in HANDLE hProcess,
    __in DWORD64 Address,
    __out_opt PDWORD64 Displacement,
    __inout PSYMBOL_INFO Symbol
);

BOOL
IMAGEAPI
SymFromAddrW(
    __in HANDLE hProcess,
    __in DWORD64 Address,
    __out_opt PDWORD64 Displacement,
    __inout PSYMBOL_INFOW Symbol
);

BOOL
IMAGEAPI
SymFromToken(
    __in HANDLE hProcess,
    __in DWORD64 Base,
    __in DWORD Token,
    __inout PSYMBOL_INFO Symbol
);

BOOL
IMAGEAPI
SymFromTokenW(
    __in HANDLE hProcess,
    __in DWORD64 Base,
    __in DWORD Token,
    __inout PSYMBOL_INFOW Symbol
);

BOOL
IMAGEAPI
SymNext(
    __in HANDLE hProcess,
    __inout PSYMBOL_INFO si
);

BOOL
IMAGEAPI
SymNextW(
    __in HANDLE hProcess,
    __inout PSYMBOL_INFOW siw
);

BOOL
IMAGEAPI
SymPrev(
    __in HANDLE hProcess,
    __inout PSYMBOL_INFO si
);

BOOL
IMAGEAPI
SymPrevW(
    __in HANDLE hProcess,
    __inout PSYMBOL_INFOW siw
);

// While SymFromName will provide a symbol from a name,
// SymEnumSymbols can provide the same matching information
// for ALL symbols with a matching name, even regular
// expressions.  That way you can search across modules
// and differentiate between identically named symbols.

BOOL
IMAGEAPI
SymFromName(
    __in HANDLE hProcess,
    __in PCSTR Name,
    __inout PSYMBOL_INFO Symbol
);

BOOL
IMAGEAPI
SymFromNameW(
    __in HANDLE hProcess,
    __in PCWSTR Name,
    __inout PSYMBOL_INFOW Symbol
);

typedef BOOL
(CALLBACK* PSYM_ENUMERATESYMBOLS_CALLBACK)(
    __in PSYMBOL_INFO pSymInfo,
    __in ULONG SymbolSize,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymEnumSymbols(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in_opt PCSTR Mask,
    __in PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
    __in_opt PVOID UserContext
);

typedef BOOL
(CALLBACK* PSYM_ENUMERATESYMBOLS_CALLBACKW)(
    __in PSYMBOL_INFOW pSymInfo,
    __in ULONG SymbolSize,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymEnumSymbolsW(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in_opt PCWSTR Mask,
    __in PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymEnumSymbolsForAddr(
    __in HANDLE hProcess,
    __in DWORD64 Address,
    __in PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymEnumSymbolsForAddrW(
    __in HANDLE hProcess,
    __in DWORD64 Address,
    __in PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
    __in_opt PVOID UserContext
);

#define SYMSEARCH_MASKOBJS      0x01    // used internally to implement other APIs
#define SYMSEARCH_RECURSE       0X02    // recurse scopes
#define SYMSEARCH_GLOBALSONLY   0X04    // search only for global symbols
#define SYMSEARCH_ALLITEMS      0X08    // search for everything in the pdb, not just normal scoped symbols

BOOL
IMAGEAPI
SymSearch(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in_opt DWORD Index,
    __in_opt DWORD SymTag,
    __in_opt PCSTR Mask,
    __in_opt DWORD64 Address,
    __in PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
    __in_opt PVOID UserContext,
    __in DWORD Options
);

BOOL
IMAGEAPI
SymSearchW(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in_opt DWORD Index,
    __in_opt DWORD SymTag,
    __in_opt PCWSTR Mask,
    __in_opt DWORD64 Address,
    __in PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
    __in_opt PVOID UserContext,
    __in DWORD Options
);

BOOL
IMAGEAPI
SymGetScope(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in DWORD Index,
    __inout PSYMBOL_INFO Symbol
);

BOOL
IMAGEAPI
SymGetScopeW(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in DWORD Index,
    __inout PSYMBOL_INFOW Symbol
);

BOOL
IMAGEAPI
SymFromIndex(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in DWORD Index,
    __inout PSYMBOL_INFO Symbol
);

BOOL
IMAGEAPI
SymFromIndexW(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in DWORD Index,
    __inout PSYMBOL_INFOW Symbol
);

typedef enum _IMAGEHLP_SYMBOL_TYPE_INFO
{
    TI_GET_SYMTAG,
    TI_GET_SYMNAME,
    TI_GET_LENGTH,
    TI_GET_TYPE,
    TI_GET_TYPEID,
    TI_GET_BASETYPE,
    TI_GET_ARRAYINDEXTYPEID,
    TI_FINDCHILDREN,
    TI_GET_DATAKIND,
    TI_GET_ADDRESSOFFSET,
    TI_GET_OFFSET,
    TI_GET_VALUE,
    TI_GET_COUNT,
    TI_GET_CHILDRENCOUNT,
    TI_GET_BITPOSITION,
    TI_GET_VIRTUALBASECLASS,
    TI_GET_VIRTUALTABLESHAPEID,
    TI_GET_VIRTUALBASEPOINTEROFFSET,
    TI_GET_CLASSPARENTID,
    TI_GET_NESTED,
    TI_GET_SYMINDEX,
    TI_GET_LEXICALPARENT,
    TI_GET_ADDRESS,
    TI_GET_THISADJUST,
    TI_GET_UDTKIND,
    TI_IS_EQUIV_TO,
    TI_GET_CALLING_CONVENTION,
    TI_IS_CLOSE_EQUIV_TO,
    TI_GTIEX_REQS_VALID,
    TI_GET_VIRTUALBASEOFFSET,
    TI_GET_VIRTUALBASEDISPINDEX,
    TI_GET_IS_REFERENCE,
    TI_GET_INDIRECTVIRTUALBASECLASS,
    IMAGEHLP_SYMBOL_TYPE_INFO_MAX,
} IMAGEHLP_SYMBOL_TYPE_INFO;

typedef struct _TI_FINDCHILDREN_PARAMS
{
    ULONG Count;
    ULONG Start;
    ULONG ChildId[1];
} TI_FINDCHILDREN_PARAMS;

BOOL
IMAGEAPI
SymGetTypeInfo(
    __in HANDLE hProcess,
    __in DWORD64 ModBase,
    __in ULONG TypeId,
    __in IMAGEHLP_SYMBOL_TYPE_INFO GetType,
    __out PVOID pInfo
);

#define IMAGEHLP_GET_TYPE_INFO_UNCACHED 0x00000001
#define IMAGEHLP_GET_TYPE_INFO_CHILDREN 0x00000002

typedef struct _IMAGEHLP_GET_TYPE_INFO_PARAMS
{
    IN  ULONG    SizeOfStruct;
    IN  ULONG    Flags;
    IN  ULONG    NumIds;
    IN  PULONG   TypeIds;
    IN  ULONG64  TagFilter;
    IN  ULONG    NumReqs;
    IN  IMAGEHLP_SYMBOL_TYPE_INFO* ReqKinds;
    IN  PULONG_PTR ReqOffsets;
    IN  PULONG   ReqSizes;
    IN  ULONG_PTR ReqStride;
    IN  ULONG_PTR BufferSize;
    OUT PVOID    Buffer;
    OUT ULONG    EntriesMatched;
    OUT ULONG    EntriesFilled;
    OUT ULONG64  TagsFound;
    OUT ULONG64  AllReqsValid;
    IN  ULONG    NumReqsValid;
    OUT PULONG64 ReqsValid OPTIONAL;
} IMAGEHLP_GET_TYPE_INFO_PARAMS, *PIMAGEHLP_GET_TYPE_INFO_PARAMS;

BOOL
IMAGEAPI
SymGetTypeInfoEx(
    __in HANDLE hProcess,
    __in DWORD64 ModBase,
    __inout PIMAGEHLP_GET_TYPE_INFO_PARAMS Params
);

BOOL
IMAGEAPI
SymEnumTypes(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymEnumTypesW(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymEnumTypesByName(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in_opt PCSTR mask,
    __in PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymEnumTypesByNameW(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in_opt PCWSTR mask,
    __in PSYM_ENUMERATESYMBOLS_CALLBACKW EnumSymbolsCallback,
    __in_opt PVOID UserContext
);

BOOL
IMAGEAPI
SymGetTypeFromName(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in PCSTR Name,
    __inout PSYMBOL_INFO Symbol
);

BOOL
IMAGEAPI
SymGetTypeFromNameW(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in PCWSTR Name,
    __inout PSYMBOL_INFOW Symbol
);

BOOL
IMAGEAPI
SymAddSymbol(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in PCSTR Name,
    __in DWORD64 Address,
    __in DWORD Size,
    __in DWORD Flags
);

BOOL
IMAGEAPI
SymAddSymbolW(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in PCWSTR Name,
    __in DWORD64 Address,
    __in DWORD Size,
    __in DWORD Flags
);

BOOL
IMAGEAPI
SymDeleteSymbol(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in_opt PCSTR Name,
    __in DWORD64 Address,
    __in DWORD Flags
);

BOOL
IMAGEAPI
SymDeleteSymbolW(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in_opt PCWSTR Name,
    __in DWORD64 Address,
    __in DWORD Flags
);

BOOL
IMAGEAPI
SymRefreshModuleList(
    __in HANDLE hProcess
);

BOOL
IMAGEAPI
SymAddSourceStream(
    __in HANDLE hProcess,
    __in ULONG64 Base,
    __in_opt PCSTR StreamFile,
    __in_bcount_opt(Size) PBYTE Buffer,
    __in size_t Size
);

typedef BOOL (WINAPI* SYMADDSOURCESTREAM)(HANDLE, ULONG64, PCSTR, PBYTE, size_t);

BOOL
IMAGEAPI
SymAddSourceStreamA(
    __in HANDLE hProcess,
    __in ULONG64 Base,
    __in_opt PCSTR StreamFile,
    __in_bcount_opt(Size) PBYTE Buffer,
    __in size_t Size
);

typedef BOOL (WINAPI* SYMADDSOURCESTREAMA)(HANDLE, ULONG64, PCSTR, PBYTE, size_t);

BOOL
IMAGEAPI
SymAddSourceStreamW(
    __in HANDLE hProcess,
    __in ULONG64 Base,
    __in_opt PCWSTR FileSpec,
    __in_bcount_opt(Size) PBYTE Buffer,
    __in size_t Size
);

BOOL
IMAGEAPI
SymSrvIsStoreW(
    __in_opt HANDLE hProcess,
    __in PCWSTR path
);

BOOL
IMAGEAPI
SymSrvIsStore(
    __in_opt HANDLE hProcess,
    __in PCSTR path
);

PCSTR
IMAGEAPI
SymSrvDeltaName(
    __in HANDLE hProcess,
    __in_opt PCSTR SymPath,
    __in PCSTR Type,
    __in PCSTR File1,
    __in PCSTR File2
);

PCWSTR
IMAGEAPI
SymSrvDeltaNameW(
    __in HANDLE hProcess,
    __in_opt PCWSTR SymPath,
    __in PCWSTR Type,
    __in PCWSTR File1,
    __in PCWSTR File2
);

PCSTR
IMAGEAPI
SymSrvGetSupplement(
    __in HANDLE hProcess,
    __in_opt PCSTR SymPath,
    __in PCSTR Node,
    __in PCSTR File
);

PCWSTR
IMAGEAPI
SymSrvGetSupplementW(
    __in HANDLE hProcess,
    __in_opt PCWSTR SymPath,
    __in PCWSTR Node,
    __in PCWSTR File
);

BOOL
IMAGEAPI
SymSrvGetFileIndexes(
    __in PCSTR File,
    __out GUID* Id,
    __out PDWORD Val1,
    __out_opt PDWORD Val2,
    __in DWORD Flags
);

BOOL
IMAGEAPI
SymSrvGetFileIndexesW(
    __in PCWSTR File,
    __out GUID* Id,
    __out PDWORD Val1,
    __out_opt PDWORD Val2,
    __in DWORD Flags
);

BOOL
IMAGEAPI
SymSrvGetFileIndexStringW(
    __in HANDLE hProcess,
    __in_opt PCWSTR SrvPath,
    __in PCWSTR File,
    __out_ecount(Size) PWSTR Index,
    __in size_t Size,
    __in DWORD Flags
);

BOOL
IMAGEAPI
SymSrvGetFileIndexString(
    __in HANDLE hProcess,
    __in_opt PCSTR SrvPath,
    __in PCSTR File,
    __out_ecount(Size) PSTR Index,
    __in size_t Size,
    __in DWORD Flags
);

typedef struct
{
    DWORD sizeofstruct;
    char file[MAX_PATH + 1];
    BOOL  stripped;
    DWORD timestamp;
    DWORD size;
    char dbgfile[MAX_PATH + 1];
    char pdbfile[MAX_PATH + 1];
    GUID  guid;
    DWORD sig;
    DWORD age;
} SYMSRV_INDEX_INFO, *PSYMSRV_INDEX_INFO;

typedef struct
{
    DWORD sizeofstruct;
    WCHAR file[MAX_PATH + 1];
    BOOL  stripped;
    DWORD timestamp;
    DWORD size;
    WCHAR dbgfile[MAX_PATH + 1];
    WCHAR pdbfile[MAX_PATH + 1];
    GUID  guid;
    DWORD sig;
    DWORD age;
} SYMSRV_INDEX_INFOW, *PSYMSRV_INDEX_INFOW;

BOOL
IMAGEAPI
SymSrvGetFileIndexInfo(
    __in PCSTR File,
    __out PSYMSRV_INDEX_INFO Info,
    __in DWORD Flags
);

BOOL
IMAGEAPI
SymSrvGetFileIndexInfoW(
    __in PCWSTR File,
    __out PSYMSRV_INDEX_INFOW Info,
    __in DWORD Flags
);

PCSTR
IMAGEAPI
SymSrvStoreSupplement(
    __in HANDLE hProcess,
    __in_opt PCSTR SrvPath,
    __in PCSTR Node,
    __in PCSTR File,
    __in DWORD Flags
);

PCWSTR
IMAGEAPI
SymSrvStoreSupplementW(
    __in HANDLE hProcess,
    __in_opt PCWSTR SymPath,
    __in PCWSTR Node,
    __in PCWSTR File,
    __in DWORD Flags
);

PCSTR
IMAGEAPI
SymSrvStoreFile(
    __in HANDLE hProcess,
    __in_opt PCSTR SrvPath,
    __in PCSTR File,
    __in DWORD Flags
);

PCWSTR
IMAGEAPI
SymSrvStoreFileW(
    __in HANDLE hProcess,
    __in_opt PCWSTR SrvPath,
    __in PCWSTR File,
    __in DWORD Flags
);

// used by SymGetSymbolFile's "Type" parameter

enum
{
    sfImage = 0,
    sfDbg,
    sfPdb,
    sfMpd,
    sfMax
};

BOOL
IMAGEAPI
SymGetSymbolFile(
    __in_opt HANDLE hProcess,
    __in_opt PCSTR SymPath,
    __in PCSTR ImageFile,
    __in DWORD Type,
    __out_ecount(cSymbolFile) PSTR SymbolFile,
    __in size_t cSymbolFile,
    __out_ecount(cDbgFile) PSTR DbgFile,
    __in size_t cDbgFile
);

BOOL
IMAGEAPI
SymGetSymbolFileW(
    __in_opt HANDLE hProcess,
    __in_opt PCWSTR SymPath,
    __in PCWSTR ImageFile,
    __in DWORD Type,
    __out_ecount(cSymbolFile) PWSTR SymbolFile,
    __in size_t cSymbolFile,
    __out_ecount(cDbgFile) PWSTR DbgFile,
    __in size_t cDbgFile
);

//
// Full user-mode dump creation.
//

typedef BOOL (WINAPI* PDBGHELP_CREATE_USER_DUMP_CALLBACK)(
    __in DWORD DataType,
    __in PVOID* Data,
    __out LPDWORD DataLength,
    __in_opt PVOID UserData
);

BOOL
WINAPI
DbgHelpCreateUserDump(
    __in_opt LPCSTR FileName,
    __in PDBGHELP_CREATE_USER_DUMP_CALLBACK Callback,
    __in_opt PVOID UserData
);

BOOL
WINAPI
DbgHelpCreateUserDumpW(
    __in_opt LPCWSTR FileName,
    __in PDBGHELP_CREATE_USER_DUMP_CALLBACK Callback,
    __in_opt PVOID UserData
);

// -----------------------------------------------------------------
// The following 4 legacy APIs are fully supported, but newer
// ones are recommended.  SymFromName and SymFromAddr provide
// much more detailed info on the returned symbol.

BOOL
IMAGEAPI
SymGetSymFromAddr64(
    __in HANDLE hProcess,
    __in DWORD64 qwAddr,
    __out_opt PDWORD64 pdwDisplacement,
    __inout PIMAGEHLP_SYMBOL64  Symbol
);


#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymGetSymFromAddr SymGetSymFromAddr64
#else
BOOL
IMAGEAPI
SymGetSymFromAddr(
    __in HANDLE hProcess,
    __in DWORD dwAddr,
    __out_opt PDWORD pdwDisplacement,
    __inout PIMAGEHLP_SYMBOL Symbol
);
#endif

// While following two APIs will provide a symbol from a name,
// SymEnumSymbols can provide the same matching information
// for ALL symbols with a matching name, even regular
// expressions.  That way you can search across modules
// and differentiate between identically named symbols.

BOOL
IMAGEAPI
SymGetSymFromName64(
    __in HANDLE hProcess,
    __in PCSTR Name,
    __inout PIMAGEHLP_SYMBOL64 Symbol
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymGetSymFromName SymGetSymFromName64
#else
BOOL
IMAGEAPI
SymGetSymFromName(
    __in HANDLE hProcess,
    __in PCSTR Name,
    __inout PIMAGEHLP_SYMBOL Symbol
);
#endif


// Symbol server exports

typedef BOOL (WINAPI* PSYMBOLSERVERPROC)(PCSTR, PCSTR, PVOID, DWORD, DWORD, PSTR);
typedef BOOL (WINAPI* PSYMBOLSERVERPROCA)(PCSTR, PCSTR, PVOID, DWORD, DWORD, PSTR);
typedef BOOL (WINAPI* PSYMBOLSERVERPROCW)(PCWSTR, PCWSTR, PVOID, DWORD, DWORD, PWSTR);
typedef BOOL (WINAPI* PSYMBOLSERVERBYINDEXPROC)(PCSTR, PCSTR, PCSTR, PSTR);
typedef BOOL (WINAPI* PSYMBOLSERVERBYINDEXPROCA)(PCSTR, PCSTR, PCSTR, PSTR);
typedef BOOL (WINAPI* PSYMBOLSERVERBYINDEXPROCW)(PCWSTR, PCWSTR, PCWSTR, PWSTR);
typedef BOOL (WINAPI* PSYMBOLSERVEROPENPROC)(VOID);
typedef BOOL (WINAPI* PSYMBOLSERVERCLOSEPROC)(VOID);
typedef BOOL (WINAPI* PSYMBOLSERVERSETOPTIONSPROC)(UINT_PTR, ULONG64);
typedef BOOL (WINAPI* PSYMBOLSERVERSETOPTIONSWPROC)(UINT_PTR, ULONG64);
typedef BOOL (CALLBACK WINAPI* PSYMBOLSERVERCALLBACKPROC)(UINT_PTR action, ULONG64 data, ULONG64 context);
typedef UINT_PTR(WINAPI* PSYMBOLSERVERGETOPTIONSPROC)();
typedef BOOL (WINAPI* PSYMBOLSERVERPINGPROC)(PCSTR);
typedef BOOL (WINAPI* PSYMBOLSERVERPINGPROCA)(PCSTR);
typedef BOOL (WINAPI* PSYMBOLSERVERPINGPROCW)(PCWSTR);
typedef BOOL (WINAPI* PSYMBOLSERVERGETVERSION)(LPAPI_VERSION);
typedef BOOL (WINAPI* PSYMBOLSERVERDELTANAME)(PCSTR, PVOID, DWORD, DWORD, PVOID, DWORD, DWORD, PSTR, size_t);
typedef BOOL (WINAPI* PSYMBOLSERVERDELTANAMEW)(PCWSTR, PVOID, DWORD, DWORD, PVOID, DWORD, DWORD, PWSTR, size_t);
typedef BOOL (WINAPI* PSYMBOLSERVERGETSUPPLEMENT)(PCSTR, PCSTR, PCSTR, PSTR, size_t);
typedef BOOL (WINAPI* PSYMBOLSERVERGETSUPPLEMENTW)(PCWSTR, PCWSTR, PCWSTR, PWSTR, size_t);
typedef BOOL (WINAPI* PSYMBOLSERVERSTORESUPPLEMENT)(PCSTR, PCSTR, PCSTR, PSTR, size_t, DWORD);
typedef BOOL (WINAPI* PSYMBOLSERVERSTORESUPPLEMENTW)(PCWSTR, PCWSTR, PCWSTR, PWSTR, size_t, DWORD);
typedef BOOL (WINAPI* PSYMBOLSERVERGETINDEXSTRING)(PVOID, DWORD, DWORD, PSTR, size_t);
typedef BOOL (WINAPI* PSYMBOLSERVERGETINDEXSTRINGW)(PVOID, DWORD, DWORD, PWSTR, size_t);
typedef BOOL (WINAPI* PSYMBOLSERVERSTOREFILE)(PCSTR, PCSTR, PVOID, DWORD, DWORD, PSTR, size_t, DWORD);
typedef BOOL (WINAPI* PSYMBOLSERVERSTOREFILEW)(PCWSTR, PCWSTR, PVOID, DWORD, DWORD, PWSTR, size_t, DWORD);
typedef BOOL (WINAPI* PSYMBOLSERVERISSTORE)(PCSTR);
typedef BOOL (WINAPI* PSYMBOLSERVERISSTOREW)(PCWSTR);
typedef DWORD (WINAPI* PSYMBOLSERVERVERSION)();
typedef BOOL (CALLBACK WINAPI* PSYMBOLSERVERMESSAGEPROC)(UINT_PTR action, ULONG64 data, ULONG64 context);

#define SYMSRV_VERSION              2

#define SSRVOPT_CALLBACK            0x00000001
#define SSRVOPT_DWORD               0x00000002
#define SSRVOPT_DWORDPTR            0x00000004
#define SSRVOPT_GUIDPTR             0x00000008
#define SSRVOPT_OLDGUIDPTR          0x00000010
#define SSRVOPT_UNATTENDED          0x00000020
#define SSRVOPT_NOCOPY              0x00000040
#define SSRVOPT_GETPATH             0x00000040
#define SSRVOPT_PARENTWIN           0x00000080
#define SSRVOPT_PARAMTYPE           0x00000100
#define SSRVOPT_SECURE              0x00000200
#define SSRVOPT_TRACE               0x00000400
#define SSRVOPT_SETCONTEXT          0x00000800
#define SSRVOPT_PROXY               0x00001000
#define SSRVOPT_DOWNSTREAM_STORE    0x00002000
#define SSRVOPT_OVERWRITE           0x00004000
#define SSRVOPT_RESETTOU            0x00008000
#define SSRVOPT_CALLBACKW           0x00010000
#define SSRVOPT_FLAT_DEFAULT_STORE  0x00020000
#define SSRVOPT_PROXYW              0x00040000
#define SSRVOPT_MESSAGE             0x00080000
#define SSRVOPT_SERVICE             0x00100000   // deprecated
#define SSRVOPT_FAVOR_COMPRESSED    0x00200000
#define SSRVOPT_STRING              0x00400000
#define SSRVOPT_WINHTTP             0x00800000
#define SSRVOPT_WININET             0x01000000

#define SSRVOPT_MAX                 0x0100000

#define SSRVOPT_RESET               ((ULONG_PTR)-1)


#define NUM_SSRVOPTS                30

#define SSRVACTION_TRACE        1
#define SSRVACTION_QUERYCANCEL  2
#define SSRVACTION_EVENT        3
#define SSRVACTION_EVENTW       4
#define SSRVACTION_SIZE         5

#define SYMSTOREOPT_COMPRESS        0x01
#define SYMSTOREOPT_OVERWRITE       0x02
#define SYMSTOREOPT_RETURNINDEX     0x04
#define SYMSTOREOPT_POINTER         0x08
#define SYMSTOREOPT_ALT_INDEX       0x10
#define SYMSTOREOPT_UNICODE         0x20
#define SYMSTOREOPT_PASS_IF_EXISTS  0x40

#ifdef DBGHELP_TRANSLATE_TCHAR
#define SymInitialize                     SymInitializeW
#define SymAddSymbol                      SymAddSymbolW
#define SymDeleteSymbol                   SymDeleteSymbolW
#define SearchTreeForFile                 SearchTreeForFileW
#define UnDecorateSymbolName              UnDecorateSymbolNameW
#define SymGetLineFromName64              SymGetLineFromNameW64
#define SymGetLineFromAddr64              SymGetLineFromAddrW64
#define SymGetLineNext64                  SymGetLineNextW64
#define SymGetLinePrev64                  SymGetLinePrevW64
#define SymFromName                       SymFromNameW
#define SymFindExecutableImage            SymFindExecutableImageW
#define FindExecutableImageEx             FindExecutableImageExW
#define SymSearch                         SymSearchW
#define SymEnumLines                      SymEnumLinesW
#define SymEnumSourceLines                SymEnumSourceLinesW
#define SymGetTypeFromName                SymGetTypeFromNameW
#define SymEnumSymbolsForAddr             SymEnumSymbolsForAddrW
#define SymFromAddr                       SymFromAddrW
#define SymMatchString                    SymMatchStringW
#define SymEnumSourceFiles                SymEnumSourceFilesW
#define SymEnumSymbols                    SymEnumSymbolsW
#define SymLoadModuleEx                   SymLoadModuleExW
#define SymSetSearchPath                  SymSetSearchPathW
#define SymGetSearchPath                  SymGetSearchPathW
#define EnumDirTree                       EnumDirTreeW
#define SymFromToken                      SymFromTokenW
#define SymFromIndex                      SymFromIndexW
#define SymGetScope                       SymGetScopeW
#define SymNext                           SymNextW
#define SymPrev                           SymPrevW
#define SymEnumTypes                      SymEnumTypesW
#define SymEnumTypesByName                SymEnumTypesByNameW
#define SymRegisterCallback64             SymRegisterCallbackW64
#define SymFindDebugInfoFile              SymFindDebugInfoFileW
#define FindDebugInfoFileEx               FindDebugInfoFileExW
#define SymFindFileInPath                 SymFindFileInPathW
#define SymEnumerateModules64             SymEnumerateModulesW64
#define SymSetHomeDirectory               SymSetHomeDirectoryW
#define SymGetHomeDirectory               SymGetHomeDirectoryW
#define SymGetSourceFile                  SymGetSourceFileW
#define SymGetSourceFileToken             SymGetSourceFileTokenW
#define SymGetSourceFileFromToken         SymGetSourceFileFromTokenW
#define SymGetSourceVarFromToken          SymGetSourceVarFromTokenW
#define SymGetSourceFileToken             SymGetSourceFileTokenW
#define SymGetFileLineOffsets64           SymGetFileLineOffsetsW64
#define SymFindFileInPath                 SymFindFileInPathW
#define SymMatchFileName                  SymMatchFileNameW
#define SymGetSourceFileFromToken         SymGetSourceFileFromTokenW
#define SymGetSourceVarFromToken          SymGetSourceVarFromTokenW
#define SymGetModuleInfo64                SymGetModuleInfoW64
#define SymSrvIsStore                     SymSrvIsStoreW
#define SymSrvDeltaName                   SymSrvDeltaNameW
#define SymSrvGetSupplement               SymSrvGetSupplementW
#define SymSrvStoreSupplement             SymSrvStoreSupplementW
#define SymSrvGetFileIndexes              SymSrvGetFileIndexes
#define SymSrvGetFileIndexString          SymSrvGetFileIndexStringW
#define SymSrvStoreFile                   SymSrvStoreFileW
#define SymGetSymbolFile                  SymGetSymbolFileW
#define EnumerateLoadedModules64          EnumerateLoadedModulesW64
#define EnumerateLoadedModulesEx          EnumerateLoadedModulesExW
#define SymSrvGetFileIndexInfo            SymSrvGetFileIndexInfoW

#define IMAGEHLP_LINE64                   IMAGEHLP_LINEW64
#define PIMAGEHLP_LINE64                  PIMAGEHLP_LINEW64
#define SYMBOL_INFO                       SYMBOL_INFOW
#define PSYMBOL_INFO                      PSYMBOL_INFOW
#define SYMBOL_INFO_PACKAGE               SYMBOL_INFO_PACKAGEW
#define PSYMBOL_INFO_PACKAGE              PSYMBOL_INFO_PACKAGEW
#define FIND_EXE_FILE_CALLBACK            FIND_EXE_FILE_CALLBACKW
#define PFIND_EXE_FILE_CALLBACK           PFIND_EXE_FILE_CALLBACKW
#define SYM_ENUMERATESYMBOLS_CALLBACK     SYM_ENUMERATESYMBOLS_CALLBACKW
#define PSYM_ENUMERATESYMBOLS_CALLBACK    PSYM_ENUMERATESYMBOLS_CALLBACKW
#define SRCCODEINFO                       SRCCODEINFOW
#define PSRCCODEINFO                      PSRCCODEINFOW
#define SOURCEFILE                        SOURCEFILEW
#define PSOURCEFILE                       PSOURCEFILEW
#define SYM_ENUMSOURECFILES_CALLBACK      SYM_ENUMSOURCEFILES_CALLBACKW
#define PSYM_ENUMSOURCEFILES_CALLBACK     PSYM_ENUMSOURECFILES_CALLBACKW
#define IMAGEHLP_CBA_EVENT                IMAGEHLP_CBA_EVENTW
#define PIMAGEHLP_CBA_EVENT               PIMAGEHLP_CBA_EVENTW
#define PENUMDIRTREE_CALLBACK             PENUMDIRTREE_CALLBACKW
#define IMAGEHLP_DEFERRED_SYMBOL_LOAD64   IMAGEHLP_DEFERRED_SYMBOL_LOADW64
#define PIMAGEHLP_DEFERRED_SYMBOL_LOAD64  PIMAGEHLP_DEFERRED_SYMBOL_LOADW64
#define PFIND_DEBUG_FILE_CALLBACK         PFIND_DEBUG_FILE_CALLBACKW
#define PFINDFILEINPATHCALLBACK           PFINDFILEINPATHCALLBACKW
#define IMAGEHLP_MODULE64                 IMAGEHLP_MODULEW64
#define PIMAGEHLP_MODULE64                PIMAGEHLP_MODULEW64
#define SYMSRV_INDEX_INFO                 SYMSRV_INDEX_INFOW
#define PSYMSRV_INDEX_INFO                PSYMSRV_INDEX_INFOW

#define PSYMBOLSERVERPROC                 PSYMBOLSERVERPROCW
#define PSYMBOLSERVERPINGPROC             PSYMBOLSERVERPINGPROCW
#endif

// -----------------------------------------------------------------
// The following APIs exist only for backwards compatibility
// with a pre-release version documented in an MSDN release.

// You should use SymFindFileInPath if you want to maintain
// future compatibility.

DBHLP_DEPRECIATED
BOOL
IMAGEAPI
FindFileInPath(
    __in HANDLE hprocess,
    __in PCSTR SearchPath,
    __in PCSTR FileName,
    __in PVOID id,
    __in DWORD two,
    __in DWORD three,
    __in DWORD flags,
    __out_ecount(MAX_PATH + 1) PSTR FilePath
);

// You should use SymFindFileInPath if you want to maintain
// future compatibility.

DBHLP_DEPRECIATED
BOOL
IMAGEAPI
FindFileInSearchPath(
    __in HANDLE hprocess,
    __in PCSTR SearchPath,
    __in PCSTR FileName,
    __in DWORD one,
    __in DWORD two,
    __in DWORD three,
    __out_ecount(MAX_PATH + 1) PSTR FilePath
);

DBHLP_DEPRECIATED
BOOL
IMAGEAPI
SymEnumSym(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback,
    __in_opt PVOID UserContext
);

DBHLP_DEPRECIATED
BOOL
IMAGEAPI
SymEnumerateSymbols64(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in PSYM_ENUMSYMBOLS_CALLBACK64 EnumSymbolsCallback,
    __in_opt PVOID UserContext
);

DBHLP_DEPRECIATED
BOOL
IMAGEAPI
SymEnumerateSymbolsW64(
    __in HANDLE hProcess,
    __in ULONG64 BaseOfDll,
    __in PSYM_ENUMSYMBOLS_CALLBACK64W EnumSymbolsCallback,
    __in_opt PVOID UserContext
);


#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymEnumerateSymbols SymEnumerateSymbols64
#define SymEnumerateSymbolsW SymEnumerateSymbolsW64
#else
DBHLP_DEPRECIATED
BOOL
IMAGEAPI
SymEnumerateSymbols(
    __in HANDLE hProcess,
    __in ULONG BaseOfDll,
    __in PSYM_ENUMSYMBOLS_CALLBACK EnumSymbolsCallback,
    __in_opt PVOID UserContext
);

DBHLP_DEPRECIATED
BOOL
IMAGEAPI
SymEnumerateSymbolsW(
    __in HANDLE hProcess,
    __in ULONG BaseOfDll,
    __in PSYM_ENUMSYMBOLS_CALLBACKW EnumSymbolsCallback,
    __in_opt PVOID UserContext
);
#endif

// use SymLoadModuleEx

DWORD64
IMAGEAPI
SymLoadModule64(
    __in HANDLE hProcess,
    __in_opt HANDLE hFile,
    __in_opt PCSTR ImageName,
    __in_opt PCSTR ModuleName,
    __in DWORD64 BaseOfDll,
    __in DWORD SizeOfDll
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymLoadModule SymLoadModule64
#else
DWORD
IMAGEAPI
SymLoadModule(
    __in HANDLE hProcess,
    __in_opt HANDLE hFile,
    __in_opt PCSTR ImageName,
    __in_opt PCSTR ModuleName,
    __in DWORD BaseOfDll,
    __in DWORD SizeOfDll
);
#endif

BOOL
IMAGEAPI
SymGetSymNext64(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_SYMBOL64 Symbol
);

BOOL
IMAGEAPI
SymGetSymNextW64(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_SYMBOLW64 Symbol
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymGetSymNext SymGetSymNext64
#define SymGetSymNextW SymGetSymNextW64
#else
BOOL
IMAGEAPI
SymGetSymNext(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_SYMBOL Symbol
);

BOOL
IMAGEAPI
SymGetSymNextW(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_SYMBOLW Symbol
);
#endif

BOOL
IMAGEAPI
SymGetSymPrev64(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_SYMBOL64 Symbol
);

BOOL
IMAGEAPI
SymGetSymPrevW64(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_SYMBOLW64 Symbol
);

#if !defined(_IMAGEHLP_SOURCE_) && defined(_IMAGEHLP64)
#define SymGetSymPrev SymGetSymPrev64
#define SymGetSymPrevW SymGetSymPrevW64
#else
BOOL
IMAGEAPI
SymGetSymPrev(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_SYMBOL Symbol
);

BOOL
IMAGEAPI
SymGetSymPrevW(
    __in HANDLE hProcess,
    __inout PIMAGEHLP_SYMBOLW Symbol
);
#endif


// These values should not be used.
// They have been replaced by SYMFLAG_ values.

#define SYMF_OMAP_GENERATED   0x00000001
#define SYMF_OMAP_MODIFIED    0x00000002
#define SYMF_REGISTER         0x00000008
#define SYMF_REGREL           0x00000010
#define SYMF_FRAMEREL         0x00000020
#define SYMF_PARAMETER        0x00000040
#define SYMF_LOCAL            0x00000080
#define SYMF_CONSTANT         0x00000100
#define SYMF_EXPORT           0x00000200
#define SYMF_FORWARDER        0x00000400
#define SYMF_FUNCTION         0x00000800
#define SYMF_VIRTUAL          0x00001000
#define SYMF_THUNK            0x00002000
#define SYMF_TLSREL           0x00004000

// These values should also not be used.
// They have been replaced by SYMFLAG_ values.

#define IMAGEHLP_SYMBOL_INFO_VALUEPRESENT          1
#define IMAGEHLP_SYMBOL_INFO_REGISTER              SYMF_REGISTER        // 0x0008
#define IMAGEHLP_SYMBOL_INFO_REGRELATIVE           SYMF_REGREL          // 0x0010
#define IMAGEHLP_SYMBOL_INFO_FRAMERELATIVE         SYMF_FRAMEREL        // 0x0020
#define IMAGEHLP_SYMBOL_INFO_PARAMETER             SYMF_PARAMETER       // 0x0040
#define IMAGEHLP_SYMBOL_INFO_LOCAL                 SYMF_LOCAL           // 0x0080
#define IMAGEHLP_SYMBOL_INFO_CONSTANT              SYMF_CONSTANT        // 0x0100
#define IMAGEHLP_SYMBOL_FUNCTION                   SYMF_FUNCTION        // 0x0800
#define IMAGEHLP_SYMBOL_VIRTUAL                    SYMF_VIRTUAL         // 0x1000
#define IMAGEHLP_SYMBOL_THUNK                      SYMF_THUNK           // 0x2000
#define IMAGEHLP_SYMBOL_INFO_TLSRELATIVE           SYMF_TLSREL          // 0x4000

#pragma pack(pop)


#pragma pack(push,4)

#define MINIDUMP_SIGNATURE ('PMDM')
#define MINIDUMP_VERSION   (42899)
typedef DWORD RVA;
typedef ULONG64 RVA64;

typedef struct _MINIDUMP_LOCATION_DESCRIPTOR
{
    ULONG32 DataSize;
    RVA Rva;
} MINIDUMP_LOCATION_DESCRIPTOR;

typedef struct _MINIDUMP_LOCATION_DESCRIPTOR64
{
    ULONG64 DataSize;
    RVA64 Rva;
} MINIDUMP_LOCATION_DESCRIPTOR64;


typedef struct _MINIDUMP_MEMORY_DESCRIPTOR
{
    ULONG64 StartOfMemoryRange;
    MINIDUMP_LOCATION_DESCRIPTOR Memory;
} MINIDUMP_MEMORY_DESCRIPTOR, *PMINIDUMP_MEMORY_DESCRIPTOR;

// DESCRIPTOR64 is used for full-memory minidumps where
// all of the raw memory is laid out sequentially at the
// end of the dump.  There is no need for individual RVAs
// as the RVA is the base RVA plus the sum of the preceeding
// data blocks.
typedef struct _MINIDUMP_MEMORY_DESCRIPTOR64
{
    ULONG64 StartOfMemoryRange;
    ULONG64 DataSize;
} MINIDUMP_MEMORY_DESCRIPTOR64, *PMINIDUMP_MEMORY_DESCRIPTOR64;


typedef struct _MINIDUMP_HEADER
{
    ULONG32 Signature;
    ULONG32 Version;
    ULONG32 NumberOfStreams;
    RVA StreamDirectoryRva;
    ULONG32 CheckSum;
    union
    {
        ULONG32 Reserved;
        ULONG32 TimeDateStamp;
    };
    ULONG64 Flags;
} MINIDUMP_HEADER, *PMINIDUMP_HEADER;

//
// The MINIDUMP_HEADER field StreamDirectoryRva points to
// an array of MINIDUMP_DIRECTORY structures.
//

typedef struct _MINIDUMP_DIRECTORY
{
    ULONG32 StreamType;
    MINIDUMP_LOCATION_DESCRIPTOR Location;
} MINIDUMP_DIRECTORY, *PMINIDUMP_DIRECTORY;


typedef struct _MINIDUMP_STRING
{
    ULONG32 Length;         // Length in bytes of the string
    WCHAR   Buffer [0];     // Variable size buffer
} MINIDUMP_STRING, *PMINIDUMP_STRING;



//
// The MINIDUMP_DIRECTORY field StreamType may be one of the following types.
// Types will be added in the future, so if a program reading the minidump
// header encounters a stream type it does not understand it should ignore
// the data altogether. Any tag above LastReservedStream will not be used by
// the system and is reserved for program-specific information.
//

typedef enum _MINIDUMP_STREAM_TYPE
{

    UnusedStream                = 0,
    ReservedStream0             = 1,
    ReservedStream1             = 2,
    ThreadListStream            = 3,
    ModuleListStream            = 4,
    MemoryListStream            = 5,
    ExceptionStream             = 6,
    SystemInfoStream            = 7,
    ThreadExListStream          = 8,
    Memory64ListStream          = 9,
    CommentStreamA              = 10,
    CommentStreamW              = 11,
    HandleDataStream            = 12,
    FunctionTableStream         = 13,
    UnloadedModuleListStream    = 14,
    MiscInfoStream              = 15,
    MemoryInfoListStream        = 16,
    ThreadInfoListStream        = 17,
    HandleOperationListStream   = 18,
    TokenStream                 = 19,

    ceStreamNull                = 0x8000,
    ceStreamSystemInfo          = 0x8001,
    ceStreamException           = 0x8002,
    ceStreamModuleList          = 0x8003,
    ceStreamProcessList         = 0x8004,
    ceStreamThreadList          = 0x8005,
    ceStreamThreadContextList   = 0x8006,
    ceStreamThreadCallStackList = 0x8007,
    ceStreamMemoryVirtualList   = 0x8008,
    ceStreamMemoryPhysicalList  = 0x8009,
    ceStreamBucketParameters    = 0x800A,
    ceStreamProcessModuleMap    = 0x800B,
    ceStreamDiagnosisList       = 0x800C,

    LastReservedStream          = 0xffff

} MINIDUMP_STREAM_TYPE;


//
// The minidump system information contains processor and
// Operating System specific information.
//

//
// CPU information is obtained from one of two places.
//
//  1) On x86 computers, CPU_INFORMATION is obtained from the CPUID
//     instruction. You must use the X86 portion of the union for X86
//     computers.
//
//  2) On non-x86 architectures, CPU_INFORMATION is obtained by calling
//     IsProcessorFeatureSupported().
//

typedef union _CPU_INFORMATION
{

    //
    // X86 platforms use CPUID function to obtain processor information.
    //

    struct
    {

        //
        // CPUID Subfunction 0, register EAX (VendorId [0]),
        // EBX (VendorId [1]) and ECX (VendorId [2]).
        //

        ULONG32 VendorId [ 3 ];

        //
        // CPUID Subfunction 1, register EAX
        //

        ULONG32 VersionInformation;

        //
        // CPUID Subfunction 1, register EDX
        //

        ULONG32 FeatureInformation;


        //
        // CPUID, Subfunction 80000001, register EBX. This will only
        // be obtained if the vendor id is "AuthenticAMD".
        //

        ULONG32 AMDExtendedCpuFeatures;

    } X86CpuInfo;

    //
    // Non-x86 platforms use processor feature flags.
    //

    struct
    {

        ULONG64 ProcessorFeatures [ 2 ];

    } OtherCpuInfo;

} CPU_INFORMATION, *PCPU_INFORMATION;

typedef struct _MINIDUMP_SYSTEM_INFO
{

    //
    // ProcessorArchitecture, ProcessorLevel and ProcessorRevision are all
    // taken from the SYSTEM_INFO structure obtained by GetSystemInfo( ).
    //

    USHORT ProcessorArchitecture;
    USHORT ProcessorLevel;
    USHORT ProcessorRevision;

    union
    {
        USHORT Reserved0;
        struct
        {
            UCHAR NumberOfProcessors;
            UCHAR ProductType;
        };
    };

    //
    // MajorVersion, MinorVersion, BuildNumber, PlatformId and
    // CSDVersion are all taken from the OSVERSIONINFO structure
    // returned by GetVersionEx( ).
    //

    ULONG32 MajorVersion;
    ULONG32 MinorVersion;
    ULONG32 BuildNumber;
    ULONG32 PlatformId;

    //
    // RVA to a CSDVersion string in the string table.
    //

    RVA CSDVersionRva;

    union
    {
        ULONG32 Reserved1;
        struct
        {
            USHORT SuiteMask;
            USHORT Reserved2;
        };
    };

    CPU_INFORMATION Cpu;

} MINIDUMP_SYSTEM_INFO, *PMINIDUMP_SYSTEM_INFO;


//
// The minidump thread contains standard thread
// information plus an RVA to the memory for this
// thread and an RVA to the CONTEXT structure for
// this thread.
//


//
// ThreadId must be 4 bytes on all architectures.
//

C_ASSERT(sizeof(((PPROCESS_INFORMATION)0)->dwThreadId) == 4);

typedef struct _MINIDUMP_THREAD
{
    ULONG32 ThreadId;
    ULONG32 SuspendCount;
    ULONG32 PriorityClass;
    ULONG32 Priority;
    ULONG64 Teb;
    MINIDUMP_MEMORY_DESCRIPTOR Stack;
    MINIDUMP_LOCATION_DESCRIPTOR ThreadContext;
} MINIDUMP_THREAD, *PMINIDUMP_THREAD;

//
// The thread list is a container of threads.
//

typedef struct _MINIDUMP_THREAD_LIST
{
    ULONG32 NumberOfThreads;
    MINIDUMP_THREAD Threads [0];
} MINIDUMP_THREAD_LIST, *PMINIDUMP_THREAD_LIST;


typedef struct _MINIDUMP_THREAD_EX
{
    ULONG32 ThreadId;
    ULONG32 SuspendCount;
    ULONG32 PriorityClass;
    ULONG32 Priority;
    ULONG64 Teb;
    MINIDUMP_MEMORY_DESCRIPTOR Stack;
    MINIDUMP_LOCATION_DESCRIPTOR ThreadContext;
    MINIDUMP_MEMORY_DESCRIPTOR BackingStore;
} MINIDUMP_THREAD_EX, *PMINIDUMP_THREAD_EX;

//
// The thread list is a container of threads.
//

typedef struct _MINIDUMP_THREAD_EX_LIST
{
    ULONG32 NumberOfThreads;
    MINIDUMP_THREAD_EX Threads [0];
} MINIDUMP_THREAD_EX_LIST, *PMINIDUMP_THREAD_EX_LIST;


//
// The MINIDUMP_EXCEPTION is the same as EXCEPTION on Win64.
//

typedef struct _MINIDUMP_EXCEPTION
{
    ULONG32 ExceptionCode;
    ULONG32 ExceptionFlags;
    ULONG64 ExceptionRecord;
    ULONG64 ExceptionAddress;
    ULONG32 NumberParameters;
    ULONG32 __unusedAlignment;
    ULONG64 ExceptionInformation [ EXCEPTION_MAXIMUM_PARAMETERS ];
} MINIDUMP_EXCEPTION, *PMINIDUMP_EXCEPTION;


//
// The exception information stream contains the id of the thread that caused
// the exception (ThreadId), the exception record for the exception
// (ExceptionRecord) and an RVA to the thread context where the exception
// occured.
//

typedef struct MINIDUMP_EXCEPTION_STREAM
{
    ULONG32 ThreadId;
    ULONG32  __alignment;
    MINIDUMP_EXCEPTION ExceptionRecord;
    MINIDUMP_LOCATION_DESCRIPTOR ThreadContext;
} MINIDUMP_EXCEPTION_STREAM, *PMINIDUMP_EXCEPTION_STREAM;


//
// The MINIDUMP_MODULE contains information about a
// a specific module. It includes the CheckSum and
// the TimeDateStamp for the module so the module
// can be reloaded during the analysis phase.
//

typedef struct _MINIDUMP_MODULE
{
    ULONG64 BaseOfImage;
    ULONG32 SizeOfImage;
    ULONG32 CheckSum;
    ULONG32 TimeDateStamp;
    RVA ModuleNameRva;
    VS_FIXEDFILEINFO VersionInfo;
    MINIDUMP_LOCATION_DESCRIPTOR CvRecord;
    MINIDUMP_LOCATION_DESCRIPTOR MiscRecord;
    ULONG64 Reserved0;                          // Reserved for future use.
    ULONG64 Reserved1;                          // Reserved for future use.
} MINIDUMP_MODULE, *PMINIDUMP_MODULE;


//
// The minidump module list is a container for modules.
//

typedef struct _MINIDUMP_MODULE_LIST
{
    ULONG32 NumberOfModules;
    MINIDUMP_MODULE Modules [ 0 ];
} MINIDUMP_MODULE_LIST, *PMINIDUMP_MODULE_LIST;


//
// Memory Ranges
//

typedef struct _MINIDUMP_MEMORY_LIST
{
    ULONG32 NumberOfMemoryRanges;
    MINIDUMP_MEMORY_DESCRIPTOR MemoryRanges [0];
} MINIDUMP_MEMORY_LIST, *PMINIDUMP_MEMORY_LIST;

typedef struct _MINIDUMP_MEMORY64_LIST
{
    ULONG64 NumberOfMemoryRanges;
    RVA64 BaseRva;
    MINIDUMP_MEMORY_DESCRIPTOR64 MemoryRanges [0];
} MINIDUMP_MEMORY64_LIST, *PMINIDUMP_MEMORY64_LIST;


//
// Support for user supplied exception information.
//

typedef struct _MINIDUMP_EXCEPTION_INFORMATION
{
    DWORD ThreadId;
    PEXCEPTION_POINTERS ExceptionPointers;
    BOOL ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION, *PMINIDUMP_EXCEPTION_INFORMATION;

typedef struct _MINIDUMP_EXCEPTION_INFORMATION64
{
    DWORD ThreadId;
    ULONG64 ExceptionRecord;
    ULONG64 ContextRecord;
    BOOL ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION64, *PMINIDUMP_EXCEPTION_INFORMATION64;


//
// Support for capturing system handle state at the time of the dump.
//

// Per-handle object information varies according to
// the OS, the OS version, the processor type and
// so on.  The minidump gives a minidump identifier
// to each possible data format for identification
// purposes but does not control nor describe the actual data.
typedef enum _MINIDUMP_HANDLE_OBJECT_INFORMATION_TYPE
{
    MiniHandleObjectInformationNone,
    MiniThreadInformation1,
    MiniMutantInformation1,
    MiniMutantInformation2,
    MiniProcessInformation1,
    MiniProcessInformation2,
    MiniHandleObjectInformationTypeMax
} MINIDUMP_HANDLE_OBJECT_INFORMATION_TYPE;

typedef struct _MINIDUMP_HANDLE_OBJECT_INFORMATION
{
    RVA NextInfoRva;
    ULONG32 InfoType;
    ULONG32 SizeOfInfo;
    // Raw information follows.
} MINIDUMP_HANDLE_OBJECT_INFORMATION;

typedef struct _MINIDUMP_HANDLE_DESCRIPTOR
{
    ULONG64 Handle;
    RVA TypeNameRva;
    RVA ObjectNameRva;
    ULONG32 Attributes;
    ULONG32 GrantedAccess;
    ULONG32 HandleCount;
    ULONG32 PointerCount;
} MINIDUMP_HANDLE_DESCRIPTOR, *PMINIDUMP_HANDLE_DESCRIPTOR;

typedef struct _MINIDUMP_HANDLE_DESCRIPTOR_2
{
    ULONG64 Handle;
    RVA TypeNameRva;
    RVA ObjectNameRva;
    ULONG32 Attributes;
    ULONG32 GrantedAccess;
    ULONG32 HandleCount;
    ULONG32 PointerCount;
    RVA ObjectInfoRva;
    ULONG32 Reserved0;
} MINIDUMP_HANDLE_DESCRIPTOR_2, *PMINIDUMP_HANDLE_DESCRIPTOR_2;

// The latest MINIDUMP_HANDLE_DESCRIPTOR definition.
typedef MINIDUMP_HANDLE_DESCRIPTOR_2 MINIDUMP_HANDLE_DESCRIPTOR_N;
typedef MINIDUMP_HANDLE_DESCRIPTOR_N* PMINIDUMP_HANDLE_DESCRIPTOR_N;

typedef struct _MINIDUMP_HANDLE_DATA_STREAM
{
    ULONG32 SizeOfHeader;
    ULONG32 SizeOfDescriptor;
    ULONG32 NumberOfDescriptors;
    ULONG32 Reserved;
} MINIDUMP_HANDLE_DATA_STREAM, *PMINIDUMP_HANDLE_DATA_STREAM;

// Some operating systems can track the last operations
// performed on a handle.  For example, Application Verifier
// can enable this for some versions of Windows.  The
// handle operation list collects handle operations
// known for the dump target.
// Each entry is an AVRF_HANDLE_OPERATION.
typedef struct _MINIDUMP_HANDLE_OPERATION_LIST
{
    ULONG32 SizeOfHeader;
    ULONG32 SizeOfEntry;
    ULONG32 NumberOfEntries;
    ULONG32 Reserved;
} MINIDUMP_HANDLE_OPERATION_LIST, *PMINIDUMP_HANDLE_OPERATION_LIST;


//
// Support for capturing dynamic function table state at the time of the dump.
//

typedef struct _MINIDUMP_FUNCTION_TABLE_DESCRIPTOR
{
    ULONG64 MinimumAddress;
    ULONG64 MaximumAddress;
    ULONG64 BaseAddress;
    ULONG32 EntryCount;
    ULONG32 SizeOfAlignPad;
} MINIDUMP_FUNCTION_TABLE_DESCRIPTOR, *PMINIDUMP_FUNCTION_TABLE_DESCRIPTOR;

typedef struct _MINIDUMP_FUNCTION_TABLE_STREAM
{
    ULONG32 SizeOfHeader;
    ULONG32 SizeOfDescriptor;
    ULONG32 SizeOfNativeDescriptor;
    ULONG32 SizeOfFunctionEntry;
    ULONG32 NumberOfDescriptors;
    ULONG32 SizeOfAlignPad;
} MINIDUMP_FUNCTION_TABLE_STREAM, *PMINIDUMP_FUNCTION_TABLE_STREAM;


//
// The MINIDUMP_UNLOADED_MODULE contains information about a
// a specific module that was previously loaded but no
// longer is.  This can help with diagnosing problems where
// callers attempt to call code that is no longer loaded.
//

typedef struct _MINIDUMP_UNLOADED_MODULE
{
    ULONG64 BaseOfImage;
    ULONG32 SizeOfImage;
    ULONG32 CheckSum;
    ULONG32 TimeDateStamp;
    RVA ModuleNameRva;
} MINIDUMP_UNLOADED_MODULE, *PMINIDUMP_UNLOADED_MODULE;


//
// The minidump unloaded module list is a container for unloaded modules.
//

typedef struct _MINIDUMP_UNLOADED_MODULE_LIST
{
    ULONG32 SizeOfHeader;
    ULONG32 SizeOfEntry;
    ULONG32 NumberOfEntries;
} MINIDUMP_UNLOADED_MODULE_LIST, *PMINIDUMP_UNLOADED_MODULE_LIST;


//
// The miscellaneous information stream contains a variety
// of small pieces of information.  A member is valid if
// it's within the available size and its corresponding
// bit is set.
//

#define MINIDUMP_MISC1_PROCESS_ID            0x00000001
#define MINIDUMP_MISC1_PROCESS_TIMES         0x00000002
#define MINIDUMP_MISC1_PROCESSOR_POWER_INFO  0x00000004
#define MINIDUMP_MISC3_PROCESS_INTEGRITY     0x00000010
#define MINIDUMP_MISC3_PROCESS_EXECUTE_FLAGS 0x00000020
#define MINIDUMP_MISC3_TIMEZONE              0x00000040
#define MINIDUMP_MISC3_PROTECTED_PROCESS     0x00000080

typedef struct _MINIDUMP_MISC_INFO
{
    ULONG32 SizeOfInfo;
    ULONG32 Flags1;
    ULONG32 ProcessId;
    ULONG32 ProcessCreateTime;
    ULONG32 ProcessUserTime;
    ULONG32 ProcessKernelTime;
} MINIDUMP_MISC_INFO, *PMINIDUMP_MISC_INFO;

typedef struct _MINIDUMP_MISC_INFO_2
{
    ULONG32 SizeOfInfo;
    ULONG32 Flags1;
    ULONG32 ProcessId;
    ULONG32 ProcessCreateTime;
    ULONG32 ProcessUserTime;
    ULONG32 ProcessKernelTime;
    ULONG32 ProcessorMaxMhz;
    ULONG32 ProcessorCurrentMhz;
    ULONG32 ProcessorMhzLimit;
    ULONG32 ProcessorMaxIdleState;
    ULONG32 ProcessorCurrentIdleState;
} MINIDUMP_MISC_INFO_2, *PMINIDUMP_MISC_INFO_2;

typedef struct _MINIDUMP_MISC_INFO_3
{
    ULONG32 SizeOfInfo;
    ULONG32 Flags1;
    ULONG32 ProcessId;
    ULONG32 ProcessCreateTime;
    ULONG32 ProcessUserTime;
    ULONG32 ProcessKernelTime;
    ULONG32 ProcessorMaxMhz;
    ULONG32 ProcessorCurrentMhz;
    ULONG32 ProcessorMhzLimit;
    ULONG32 ProcessorMaxIdleState;
    ULONG32 ProcessorCurrentIdleState;
    ULONG32 ProcessIntegrityLevel;
    ULONG32 ProcessExecuteFlags;
    ULONG32 ProtectedProcess;
    ULONG32 TimeZoneId;
    TIME_ZONE_INFORMATION TimeZone;
} MINIDUMP_MISC_INFO_3, *PMINIDUMP_MISC_INFO_3;

// The latest MINIDUMP_MISC_INFO definition.
typedef MINIDUMP_MISC_INFO_3 MINIDUMP_MISC_INFO_N;
typedef MINIDUMP_MISC_INFO_N* PMINIDUMP_MISC_INFO_N;


//
// The memory information stream contains memory region
// description information.  This stream corresponds to
// what VirtualQuery would return for the process the
// dump was created for.
//

typedef struct _MINIDUMP_MEMORY_INFO
{
    ULONG64 BaseAddress;
    ULONG64 AllocationBase;
    ULONG32 AllocationProtect;
    ULONG32 __alignment1;
    ULONG64 RegionSize;
    ULONG32 State;
    ULONG32 Protect;
    ULONG32 Type;
    ULONG32 __alignment2;
} MINIDUMP_MEMORY_INFO, *PMINIDUMP_MEMORY_INFO;

typedef struct _MINIDUMP_MEMORY_INFO_LIST
{
    ULONG SizeOfHeader;
    ULONG SizeOfEntry;
    ULONG64 NumberOfEntries;
} MINIDUMP_MEMORY_INFO_LIST, *PMINIDUMP_MEMORY_INFO_LIST;


//
// The memory information stream contains memory region
// description information.  This stream corresponds to
// what VirtualQuery would return for the process the
// dump was created for.
//

// Thread dump writer status flags.
#define MINIDUMP_THREAD_INFO_ERROR_THREAD    0x00000001
#define MINIDUMP_THREAD_INFO_WRITING_THREAD  0x00000002
#define MINIDUMP_THREAD_INFO_EXITED_THREAD   0x00000004
#define MINIDUMP_THREAD_INFO_INVALID_INFO    0x00000008
#define MINIDUMP_THREAD_INFO_INVALID_CONTEXT 0x00000010
#define MINIDUMP_THREAD_INFO_INVALID_TEB     0x00000020

typedef struct _MINIDUMP_THREAD_INFO
{
    ULONG32 ThreadId;
    ULONG32 DumpFlags;
    ULONG32 DumpError;
    ULONG32 ExitStatus;
    ULONG64 CreateTime;
    ULONG64 ExitTime;
    ULONG64 KernelTime;
    ULONG64 UserTime;
    ULONG64 StartAddress;
    ULONG64 Affinity;
} MINIDUMP_THREAD_INFO, *PMINIDUMP_THREAD_INFO;

typedef struct _MINIDUMP_THREAD_INFO_LIST
{
    ULONG SizeOfHeader;
    ULONG SizeOfEntry;
    ULONG NumberOfEntries;
} MINIDUMP_THREAD_INFO_LIST, *PMINIDUMP_THREAD_INFO_LIST;

//
// Support for token information.
//
typedef struct _MINIDUMP_TOKEN_INFO_HEADER
{
    ULONG   TokenSize;   // The size of the token structure.
    ULONG   TokenId;     // The PID in NtOpenProcessToken() call or TID in NtOpenThreadToken() call.
    ULONG64 TokenHandle; // The handle value returned.
} MINIDUMP_TOKEN_INFO_HEADER, *PMINIDUMP_TOKEN_INFO_HEADER;

typedef struct _MINIDUMP_TOKEN_INFO_LIST
{
    ULONG TokenListSize;
    ULONG TokenListEntries;
    ULONG ListHeaderSize;
    ULONG ElementHeaderSize;
} MINIDUMP_TOKEN_INFO_LIST, *PMINIDUMP_TOKEN_INFO_LIST;

//
// Support for arbitrary user-defined information.
//

typedef struct _MINIDUMP_USER_RECORD
{
    ULONG32 Type;
    MINIDUMP_LOCATION_DESCRIPTOR Memory;
} MINIDUMP_USER_RECORD, *PMINIDUMP_USER_RECORD;


typedef struct _MINIDUMP_USER_STREAM
{
    ULONG32 Type;
    ULONG BufferSize;
    PVOID Buffer;

} MINIDUMP_USER_STREAM, *PMINIDUMP_USER_STREAM;


typedef struct _MINIDUMP_USER_STREAM_INFORMATION
{
    ULONG UserStreamCount;
    PMINIDUMP_USER_STREAM UserStreamArray;
} MINIDUMP_USER_STREAM_INFORMATION, *PMINIDUMP_USER_STREAM_INFORMATION;

//
// Callback support.
//

typedef enum _MINIDUMP_CALLBACK_TYPE
{
    ModuleCallback,
    ThreadCallback,
    ThreadExCallback,
    IncludeThreadCallback,
    IncludeModuleCallback,
    MemoryCallback,
    CancelCallback,
    WriteKernelMinidumpCallback,
    KernelMinidumpStatusCallback,
    RemoveMemoryCallback,
    IncludeVmRegionCallback,
    IoStartCallback,
    IoWriteAllCallback,
    IoFinishCallback,
    ReadMemoryFailureCallback,
    SecondaryFlagsCallback,
} MINIDUMP_CALLBACK_TYPE;


typedef struct _MINIDUMP_THREAD_CALLBACK
{
    ULONG ThreadId;
    HANDLE ThreadHandle;
    CONTEXT Context;
    ULONG SizeOfContext;
    ULONG64 StackBase;
    ULONG64 StackEnd;
} MINIDUMP_THREAD_CALLBACK, *PMINIDUMP_THREAD_CALLBACK;


typedef struct _MINIDUMP_THREAD_EX_CALLBACK
{
    ULONG ThreadId;
    HANDLE ThreadHandle;
    CONTEXT Context;
    ULONG SizeOfContext;
    ULONG64 StackBase;
    ULONG64 StackEnd;
    ULONG64 BackingStoreBase;
    ULONG64 BackingStoreEnd;
} MINIDUMP_THREAD_EX_CALLBACK, *PMINIDUMP_THREAD_EX_CALLBACK;


typedef struct _MINIDUMP_INCLUDE_THREAD_CALLBACK
{
    ULONG ThreadId;
} MINIDUMP_INCLUDE_THREAD_CALLBACK, *PMINIDUMP_INCLUDE_THREAD_CALLBACK;


typedef enum _THREAD_WRITE_FLAGS
{
    ThreadWriteThread            = 0x0001,
    ThreadWriteStack             = 0x0002,
    ThreadWriteContext           = 0x0004,
    ThreadWriteBackingStore      = 0x0008,
    ThreadWriteInstructionWindow = 0x0010,
    ThreadWriteThreadData        = 0x0020,
    ThreadWriteThreadInfo        = 0x0040,
} THREAD_WRITE_FLAGS;

typedef struct _MINIDUMP_MODULE_CALLBACK
{
    PWCHAR FullPath;
    ULONG64 BaseOfImage;
    ULONG SizeOfImage;
    ULONG CheckSum;
    ULONG TimeDateStamp;
    VS_FIXEDFILEINFO VersionInfo;
    PVOID CvRecord;
    ULONG SizeOfCvRecord;
    PVOID MiscRecord;
    ULONG SizeOfMiscRecord;
} MINIDUMP_MODULE_CALLBACK, *PMINIDUMP_MODULE_CALLBACK;


typedef struct _MINIDUMP_INCLUDE_MODULE_CALLBACK
{
    ULONG64 BaseOfImage;
} MINIDUMP_INCLUDE_MODULE_CALLBACK, *PMINIDUMP_INCLUDE_MODULE_CALLBACK;


typedef enum _MODULE_WRITE_FLAGS
{
    ModuleWriteModule        = 0x0001,
    ModuleWriteDataSeg       = 0x0002,
    ModuleWriteMiscRecord    = 0x0004,
    ModuleWriteCvRecord      = 0x0008,
    ModuleReferencedByMemory = 0x0010,
    ModuleWriteTlsData       = 0x0020,
    ModuleWriteCodeSegs      = 0x0040,
} MODULE_WRITE_FLAGS;


typedef struct _MINIDUMP_IO_CALLBACK
{
    HANDLE Handle;
    ULONG64 Offset;
    PVOID Buffer;
    ULONG BufferBytes;
} MINIDUMP_IO_CALLBACK, *PMINIDUMP_IO_CALLBACK;


typedef struct _MINIDUMP_READ_MEMORY_FAILURE_CALLBACK
{
    ULONG64 Offset;
    ULONG Bytes;
    HRESULT FailureStatus;
} MINIDUMP_READ_MEMORY_FAILURE_CALLBACK,
*PMINIDUMP_READ_MEMORY_FAILURE_CALLBACK;


typedef struct _MINIDUMP_CALLBACK_INPUT
{
    ULONG ProcessId;
    HANDLE ProcessHandle;
    ULONG CallbackType;
    union
    {
        HRESULT Status;
        MINIDUMP_THREAD_CALLBACK Thread;
        MINIDUMP_THREAD_EX_CALLBACK ThreadEx;
        MINIDUMP_MODULE_CALLBACK Module;
        MINIDUMP_INCLUDE_THREAD_CALLBACK IncludeThread;
        MINIDUMP_INCLUDE_MODULE_CALLBACK IncludeModule;
        MINIDUMP_IO_CALLBACK Io;
        MINIDUMP_READ_MEMORY_FAILURE_CALLBACK ReadMemoryFailure;
        ULONG SecondaryFlags;
    };
} MINIDUMP_CALLBACK_INPUT, *PMINIDUMP_CALLBACK_INPUT;

typedef struct _MINIDUMP_CALLBACK_OUTPUT
{
    union
    {
        ULONG ModuleWriteFlags;
        ULONG ThreadWriteFlags;
        ULONG SecondaryFlags;
        struct
        {
            ULONG64 MemoryBase;
            ULONG MemorySize;
        };
        struct
        {
            BOOL CheckCancel;
            BOOL Cancel;
        };
        HANDLE Handle;
        struct
        {
            MINIDUMP_MEMORY_INFO VmRegion;
            BOOL Continue;
        };
        HRESULT Status;
    };
} MINIDUMP_CALLBACK_OUTPUT, *PMINIDUMP_CALLBACK_OUTPUT;


//
// A normal minidump contains just the information
// necessary to capture stack traces for all of the
// existing threads in a process.
//
// A minidump with data segments includes all of the data
// sections from loaded modules in order to capture
// global variable contents.  This can make the dump much
// larger if many modules have global data.
//
// A minidump with full memory includes all of the accessible
// memory in the process and can be very large.  A minidump
// with full memory always has the raw memory data at the end
// of the dump so that the initial structures in the dump can
// be mapped directly without having to include the raw
// memory information.
//
// Stack and backing store memory can be filtered to remove
// data unnecessary for stack walking.  This can improve
// compression of stacks and also deletes data that may
// be private and should not be stored in a dump.
// Memory can also be scanned to see what modules are
// referenced by stack and backing store memory to allow
// omission of other modules to reduce dump size.
// In either of these modes the ModuleReferencedByMemory flag
// is set for all modules referenced before the base
// module callbacks occur.
//
// On some operating systems a list of modules that were
// recently unloaded is kept in addition to the currently
// loaded module list.  This information can be saved in
// the dump if desired.
//
// Stack and backing store memory can be scanned for referenced
// pages in order to pick up data referenced by locals or other
// stack memory.  This can increase the size of a dump significantly.
//
// Module paths may contain undesired information such as user names
// or other important directory names so they can be stripped.  This
// option reduces the ability to locate the proper image later
// and should only be used in certain situations.
//
// Complete operating system per-process and per-thread information can
// be gathered and stored in the dump.
//
// The virtual address space can be scanned for various types
// of memory to be included in the dump.
//
// Code which is concerned with potentially private information
// getting into the minidump can set a flag that automatically
// modifies all existing and future flags to avoid placing
// unnecessary data in the dump.  Basic data, such as stack
// information, will still be included but optional data, such
// as indirect memory, will not.
//
// When doing a full memory dump it's possible to store all
// of the enumerated memory region descriptive information
// in a memory information stream.
//
// Additional thread information beyond the basic thread
// structure can be collected if desired.
//
// A minidump with code segments includes all of the code
// and code-related sections from loaded modules in order
// to capture executable content.
//
// MiniDumpWithoutAuxiliaryState turns off any secondary,
// auxiliary-supported memory gathering.
//
// MiniDumpWithFullAuxiliaryState asks any present auxiliary
// data providers to include all of their state in the dump.
// The exact set of what is provided depends on the auxiliary.
// This can be quite large.
//

typedef enum _MINIDUMP_TYPE
{
    MiniDumpNormal                         = 0x00000000,
    MiniDumpWithDataSegs                   = 0x00000001,
    MiniDumpWithFullMemory                 = 0x00000002,
    MiniDumpWithHandleData                 = 0x00000004,
    MiniDumpFilterMemory                   = 0x00000008,
    MiniDumpScanMemory                     = 0x00000010,
    MiniDumpWithUnloadedModules            = 0x00000020,
    MiniDumpWithIndirectlyReferencedMemory = 0x00000040,
    MiniDumpFilterModulePaths              = 0x00000080,
    MiniDumpWithProcessThreadData          = 0x00000100,
    MiniDumpWithPrivateReadWriteMemory     = 0x00000200,
    MiniDumpWithoutOptionalData            = 0x00000400,
    MiniDumpWithFullMemoryInfo             = 0x00000800,
    MiniDumpWithThreadInfo                 = 0x00001000,
    MiniDumpWithCodeSegs                   = 0x00002000,
    MiniDumpWithoutAuxiliaryState          = 0x00004000,
    MiniDumpWithFullAuxiliaryState         = 0x00008000,
    MiniDumpWithPrivateWriteCopyMemory     = 0x00010000,
    MiniDumpIgnoreInaccessibleMemory       = 0x00020000,
    MiniDumpWithTokenInformation           = 0x00040000,
    MiniDumpValidTypeFlags                 = 0x0007ffff,
} MINIDUMP_TYPE;

//
// In addition to the primary flags provided to
// MiniDumpWriteDump there are additional, less
// frequently used options queried via the secondary
// flags callback.
//
// MiniSecondaryWithoutPowerInfo suppresses the minidump
// query that retrieves processor power information for
// MINIDUMP_MISC_INFO.
//

typedef enum _MINIDUMP_SECONDARY_FLAGS
{
    MiniSecondaryWithoutPowerInfo = 0x00000001,

    MiniSecondaryValidFlags       = 0x00000001,
} MINIDUMP_SECONDARY_FLAGS;


//
// The minidump callback should modify the FieldsToWrite parameter to reflect
// what portions of the specified thread or module should be written to the
// file.
//

typedef
BOOL
(WINAPI* MINIDUMP_CALLBACK_ROUTINE)(
    __inout PVOID CallbackParam,
    __in    PMINIDUMP_CALLBACK_INPUT CallbackInput,
    __inout PMINIDUMP_CALLBACK_OUTPUT CallbackOutput
);

typedef struct _MINIDUMP_CALLBACK_INFORMATION
{
    MINIDUMP_CALLBACK_ROUTINE CallbackRoutine;
    PVOID CallbackParam;
} MINIDUMP_CALLBACK_INFORMATION, *PMINIDUMP_CALLBACK_INFORMATION;



//++
//
// PVOID
// RVA_TO_ADDR(
//     PVOID Mapping,
//     ULONG Rva
//     )
//
// Routine Description:
//
//     Map an RVA that is contained within a mapped file to it's associated
//     flat address.
//
// Arguments:
//
//     Mapping - Base address of mapped file containing the RVA.
//
//     Rva - An Rva to fixup.
//
// Return Values:
//
//     A pointer to the desired data.
//
//--

#define RVA_TO_ADDR(Mapping,Rva) ((PVOID)(((ULONG_PTR) (Mapping)) + (Rva)))

BOOL
WINAPI
MiniDumpWriteDump(
    __in HANDLE hProcess,
    __in DWORD ProcessId,
    __in HANDLE hFile,
    __in MINIDUMP_TYPE DumpType,
    __in_opt PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    __in_opt PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    __in_opt PMINIDUMP_CALLBACK_INFORMATION CallbackParam
);

BOOL
WINAPI
MiniDumpReadDumpStream(
    __in PVOID BaseOfDump,
    __in ULONG StreamNumber,
    __deref_out_opt PMINIDUMP_DIRECTORY* Dir,
    __deref_out_opt PVOID* StreamPointer,
    __out_opt ULONG* StreamSize
);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif


#endif // _DBGHELP_

