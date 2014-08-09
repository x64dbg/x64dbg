#ifndef TITANENGINE
#define TITANENGINE

#define TITCALL

#if _MSC_VER > 1000
#pragma once
#endif

#include <windows.h>

#pragma pack(push, 1)

// Global.Constant.Structure.Declaration:
// Engine.External:
#define UE_STRUCT_PE32STRUCT 1
#define UE_STRUCT_PE64STRUCT 2
#define UE_STRUCT_PESTRUCT 3
#define UE_STRUCT_IMPORTENUMDATA 4
#define UE_STRUCT_THREAD_ITEM_DATA 5
#define UE_STRUCT_LIBRARY_ITEM_DATA 6
#define UE_STRUCT_LIBRARY_ITEM_DATAW 7
#define UE_STRUCT_PROCESS_ITEM_DATA 8
#define UE_STRUCT_HANDLERARRAY 9
#define UE_STRUCT_PLUGININFORMATION 10
#define UE_STRUCT_HOOK_ENTRY 11
#define UE_STRUCT_FILE_STATUS_INFO 12
#define UE_STRUCT_FILE_FIX_INFO 13

#define UE_ACCESS_READ 0
#define UE_ACCESS_WRITE 1
#define UE_ACCESS_ALL 2

#define UE_HIDE_PEBONLY 0
#define UE_HIDE_BASIC 1

#define UE_PLUGIN_CALL_REASON_PREDEBUG 1
#define UE_PLUGIN_CALL_REASON_EXCEPTION 2
#define UE_PLUGIN_CALL_REASON_POSTDEBUG 3
#define UE_PLUGIN_CALL_REASON_UNHANDLEDEXCEPTION 4

#define TEE_HOOK_NRM_JUMP 1
#define TEE_HOOK_NRM_CALL 3
#define TEE_HOOK_IAT 5

#define UE_ENGINE_ALOW_MODULE_LOADING 1
#define UE_ENGINE_AUTOFIX_FORWARDERS 2
#define UE_ENGINE_PASS_ALL_EXCEPTIONS 3
#define UE_ENGINE_NO_CONSOLE_WINDOW 4
#define UE_ENGINE_BACKUP_FOR_CRITICAL_FUNCTIONS 5
#define UE_ENGINE_CALL_PLUGIN_CALLBACK 6
#define UE_ENGINE_RESET_CUSTOM_HANDLER 7
#define UE_ENGINE_CALL_PLUGIN_DEBUG_CALLBACK 8
#define UE_ENGINE_SET_DEBUG_PRIVILEGE 9

#define UE_OPTION_REMOVEALL 1
#define UE_OPTION_DISABLEALL 2
#define UE_OPTION_REMOVEALLDISABLED 3
#define UE_OPTION_REMOVEALLENABLED 4

#define UE_STATIC_DECRYPTOR_XOR 1
#define UE_STATIC_DECRYPTOR_SUB 2
#define UE_STATIC_DECRYPTOR_ADD 3

#define UE_STATIC_DECRYPTOR_FOREWARD 1
#define UE_STATIC_DECRYPTOR_BACKWARD 2

#define UE_STATIC_KEY_SIZE_1 1
#define UE_STATIC_KEY_SIZE_2 2
#define UE_STATIC_KEY_SIZE_4 4
#define UE_STATIC_KEY_SIZE_8 8

#define UE_STATIC_APLIB 1
#define UE_STATIC_APLIB_DEPACK 2
#define UE_STATIC_LZMA 3

#define UE_STATIC_HASH_MD5 1
#define UE_STATIC_HASH_SHA1 2
#define UE_STATIC_HASH_CRC32 3

#define UE_RESOURCE_LANGUAGE_ANY -1

#define UE_PE_OFFSET 0
#define UE_IMAGEBASE 1
#define UE_OEP 2
#define UE_SIZEOFIMAGE 3
#define UE_SIZEOFHEADERS 4
#define UE_SIZEOFOPTIONALHEADER 5
#define UE_SECTIONALIGNMENT 6
#define UE_IMPORTTABLEADDRESS 7
#define UE_IMPORTTABLESIZE 8
#define UE_RESOURCETABLEADDRESS 9
#define UE_RESOURCETABLESIZE 10
#define UE_EXPORTTABLEADDRESS 11
#define UE_EXPORTTABLESIZE 12
#define UE_TLSTABLEADDRESS 13
#define UE_TLSTABLESIZE 14
#define UE_RELOCATIONTABLEADDRESS 15
#define UE_RELOCATIONTABLESIZE 16
#define UE_TIMEDATESTAMP 17
#define UE_SECTIONNUMBER 18
#define UE_CHECKSUM 19
#define UE_SUBSYSTEM 20
#define UE_CHARACTERISTICS 21
#define UE_NUMBEROFRVAANDSIZES 22
#define UE_BASEOFCODE 23
#define UE_BASEOFDATA 24
//leaving some enum space here for future additions
#define UE_SECTIONNAME 40
#define UE_SECTIONVIRTUALOFFSET 41
#define UE_SECTIONVIRTUALSIZE 42
#define UE_SECTIONRAWOFFSET 43
#define UE_SECTIONRAWSIZE 44
#define UE_SECTIONFLAGS 45

#define UE_VANOTFOUND = -2;

#define UE_CH_BREAKPOINT 1
#define UE_CH_SINGLESTEP 2
#define UE_CH_ACCESSVIOLATION 3
#define UE_CH_ILLEGALINSTRUCTION 4
#define UE_CH_NONCONTINUABLEEXCEPTION 5
#define UE_CH_ARRAYBOUNDSEXCEPTION 6
#define UE_CH_FLOATDENORMALOPERAND 7
#define UE_CH_FLOATDEVIDEBYZERO 8
#define UE_CH_INTEGERDEVIDEBYZERO 9
#define UE_CH_INTEGEROVERFLOW 10
#define UE_CH_PRIVILEGEDINSTRUCTION 11
#define UE_CH_PAGEGUARD 12
#define UE_CH_EVERYTHINGELSE 13
#define UE_CH_CREATETHREAD 14
#define UE_CH_EXITTHREAD 15
#define UE_CH_CREATEPROCESS 16
#define UE_CH_EXITPROCESS 17
#define UE_CH_LOADDLL 18
#define UE_CH_UNLOADDLL 19
#define UE_CH_OUTPUTDEBUGSTRING 20
#define UE_CH_AFTEREXCEPTIONPROCESSING 21
#define UE_CH_SYSTEMBREAKPOINT 23
#define UE_CH_UNHANDLEDEXCEPTION 24
#define UE_CH_RIPEVENT 25
#define UE_CH_DEBUGEVENT 26

#define UE_OPTION_HANDLER_RETURN_HANDLECOUNT 1
#define UE_OPTION_HANDLER_RETURN_ACCESS 2
#define UE_OPTION_HANDLER_RETURN_FLAGS 3
#define UE_OPTION_HANDLER_RETURN_TYPENAME 4

#define UE_BREAKPOINT_INT3 1
#define UE_BREAKPOINT_LONG_INT3 2
#define UE_BREAKPOINT_UD2 3

#define UE_BPXREMOVED 0
#define UE_BPXACTIVE 1
#define UE_BPXINACTIVE 2

#define UE_BREAKPOINT 0
#define UE_SINGLESHOOT 1
#define UE_HARDWARE 2
#define UE_MEMORY 3
#define UE_MEMORY_READ 4
#define UE_MEMORY_WRITE 5
#define UE_MEMORY_EXECUTE 6
#define UE_BREAKPOINT_TYPE_INT3 0x10000000
#define UE_BREAKPOINT_TYPE_LONG_INT3 0x20000000
#define UE_BREAKPOINT_TYPE_UD2 0x30000000

#define UE_HARDWARE_EXECUTE 4
#define UE_HARDWARE_WRITE 5
#define UE_HARDWARE_READWRITE 6

#define UE_HARDWARE_SIZE_1 7
#define UE_HARDWARE_SIZE_2 8
#define UE_HARDWARE_SIZE_4 9
#define UE_HARDWARE_SIZE_8 10

#define UE_ON_LIB_LOAD 1
#define UE_ON_LIB_UNLOAD 2
#define UE_ON_LIB_ALL 3

#define UE_APISTART 0
#define UE_APIEND 1

#define UE_PLATFORM_x86 1
#define UE_PLATFORM_x64 2
#define UE_PLATFORM_ALL 3

#define UE_FUNCTION_STDCALL 1
#define UE_FUNCTION_CCALL 2
#define UE_FUNCTION_FASTCALL 3
#define UE_FUNCTION_STDCALL_RET 4
#define UE_FUNCTION_CCALL_RET 5
#define UE_FUNCTION_FASTCALL_RET 6
#define UE_FUNCTION_STDCALL_CALL 7
#define UE_FUNCTION_CCALL_CALL 8
#define UE_FUNCTION_FASTCALL_CALL 9
#define UE_PARAMETER_BYTE 0
#define UE_PARAMETER_WORD 1
#define UE_PARAMETER_DWORD 2
#define UE_PARAMETER_QWORD 3
#define UE_PARAMETER_PTR_BYTE 4
#define UE_PARAMETER_PTR_WORD 5
#define UE_PARAMETER_PTR_DWORD 6
#define UE_PARAMETER_PTR_QWORD 7
#define UE_PARAMETER_STRING 8
#define UE_PARAMETER_UNICODE 9

#define UE_EAX 1
#define UE_EBX 2
#define UE_ECX 3
#define UE_EDX 4
#define UE_EDI 5
#define UE_ESI 6
#define UE_EBP 7
#define UE_ESP 8
#define UE_EIP 9
#define UE_EFLAGS 10
#define UE_DR0 11
#define UE_DR1 12
#define UE_DR2 13
#define UE_DR3 14
#define UE_DR6 15
#define UE_DR7 16
#define UE_RAX 17
#define UE_RBX 18
#define UE_RCX 19
#define UE_RDX 20
#define UE_RDI 21
#define UE_RSI 22
#define UE_RBP 23
#define UE_RSP 24
#define UE_RIP 25
#define UE_RFLAGS 26
#define UE_R8 27
#define UE_R9 28
#define UE_R10 29
#define UE_R11 30
#define UE_R12 31
#define UE_R13 32
#define UE_R14 33
#define UE_R15 34
#define UE_CIP 35
#define UE_CSP 36
#ifdef _WIN64
#define UE_CFLAGS UE_RFLAGS
#else
#define UE_CFLAGS UE_EFLAGS
#endif
#define UE_SEG_GS 37
#define UE_SEG_FS 38
#define UE_SEG_ES 39
#define UE_SEG_DS 40
#define UE_SEG_CS 41
#define UE_SEG_SS 42

typedef struct
{
    DWORD PE32Offset;
    DWORD ImageBase;
    DWORD OriginalEntryPoint;
    DWORD BaseOfCode;
    DWORD BaseOfData;
    DWORD NtSizeOfImage;
    DWORD NtSizeOfHeaders;
    WORD SizeOfOptionalHeaders;
    DWORD FileAlignment;
    DWORD SectionAligment;
    DWORD ImportTableAddress;
    DWORD ImportTableSize;
    DWORD ResourceTableAddress;
    DWORD ResourceTableSize;
    DWORD ExportTableAddress;
    DWORD ExportTableSize;
    DWORD TLSTableAddress;
    DWORD TLSTableSize;
    DWORD RelocationTableAddress;
    DWORD RelocationTableSize;
    DWORD TimeDateStamp;
    WORD SectionNumber;
    DWORD CheckSum;
    WORD SubSystem;
    WORD Characteristics;
    DWORD NumberOfRvaAndSizes;
} PE32Struct, *PPE32Struct;

typedef struct
{
    DWORD PE64Offset;
    DWORD64 ImageBase;
    DWORD OriginalEntryPoint;
    DWORD BaseOfCode;
    DWORD BaseOfData;
    DWORD NtSizeOfImage;
    DWORD NtSizeOfHeaders;
    WORD SizeOfOptionalHeaders;
    DWORD FileAlignment;
    DWORD SectionAligment;
    DWORD ImportTableAddress;
    DWORD ImportTableSize;
    DWORD ResourceTableAddress;
    DWORD ResourceTableSize;
    DWORD ExportTableAddress;
    DWORD ExportTableSize;
    DWORD TLSTableAddress;
    DWORD TLSTableSize;
    DWORD RelocationTableAddress;
    DWORD RelocationTableSize;
    DWORD TimeDateStamp;
    WORD SectionNumber;
    DWORD CheckSum;
    WORD SubSystem;
    WORD Characteristics;
    DWORD NumberOfRvaAndSizes;
} PE64Struct, *PPE64Struct;

#if defined(_WIN64)
typedef PE64Struct PEStruct;
#else
typedef PE32Struct PEStruct;
#endif

typedef struct
{
    bool NewDll;
    int NumberOfImports;
    ULONG_PTR ImageBase;
    ULONG_PTR BaseImportThunk;
    ULONG_PTR ImportThunk;
    char* APIName;
    char* DLLName;
} ImportEnumData, *PImportEnumData;

typedef struct
{
    HANDLE hThread;
    DWORD dwThreadId;
    void* ThreadStartAddress;
    void* ThreadLocalBase;
    void* TebAddress;
    ULONG WaitTime;
    LONG Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    ULONG ThreadState;
    ULONG WaitReason;
} THREAD_ITEM_DATA, *PTHREAD_ITEM_DATA;

typedef struct
{
    HANDLE hFile;
    void* BaseOfDll;
    HANDLE hFileMapping;
    void* hFileMappingView;
    char szLibraryPath[MAX_PATH];
    char szLibraryName[MAX_PATH];
} LIBRARY_ITEM_DATA, *PLIBRARY_ITEM_DATA;

typedef struct
{
    HANDLE hFile;
    void* BaseOfDll;
    HANDLE hFileMapping;
    void* hFileMappingView;
    wchar_t szLibraryPath[MAX_PATH];
    wchar_t szLibraryName[MAX_PATH];
} LIBRARY_ITEM_DATAW, *PLIBRARY_ITEM_DATAW;

typedef struct
{
    HANDLE hProcess;
    DWORD dwProcessId;
    HANDLE hThread;
    DWORD dwThreadId;
    HANDLE hFile;
    void* BaseOfImage;
    void* ThreadStartAddress;
    void* ThreadLocalBase;
} PROCESS_ITEM_DATA, *PPROCESS_ITEM_DATA;

typedef struct
{
    ULONG ProcessId;
    HANDLE hHandle;
} HandlerArray, *PHandlerArray;

typedef struct
{
    char PluginName[64];
    DWORD PluginMajorVersion;
    DWORD PluginMinorVersion;
    HMODULE PluginBaseAddress;
    void* TitanDebuggingCallBack;
    void* TitanRegisterPlugin;
    void* TitanReleasePlugin;
    void* TitanResetPlugin;
    bool PluginDisabled;
} PluginInformation, *PPluginInformation;

#define TEE_MAXIMUM_HOOK_SIZE 14
#define TEE_MAXIMUM_HOOK_RELOCS 7
#if defined(_WIN64)
#define TEE_MAXIMUM_HOOK_INSERT_SIZE 14
#else
#define TEE_MAXIMUM_HOOK_INSERT_SIZE 5
#endif

typedef struct HOOK_ENTRY
{
    bool IATHook;
    BYTE HookType;
    DWORD HookSize;
    void* HookAddress;
    void* RedirectionAddress;
    BYTE HookBytes[TEE_MAXIMUM_HOOK_SIZE];
    BYTE OriginalBytes[TEE_MAXIMUM_HOOK_SIZE];
    void* IATHookModuleBase;
    DWORD IATHookNameHash;
    bool HookIsEnabled;
    bool HookIsRemote;
    void* PatchedEntry;
    DWORD RelocationInfo[TEE_MAXIMUM_HOOK_RELOCS];
    int RelocationCount;
} HOOK_ENTRY, *PHOOK_ENTRY;

#define UE_DEPTH_SURFACE 0
#define UE_DEPTH_DEEP 1

#define UE_UNPACKER_CONDITION_SEARCH_FROM_EP 1

#define UE_UNPACKER_CONDITION_LOADLIBRARY 1
#define UE_UNPACKER_CONDITION_GETPROCADDRESS 2
#define UE_UNPACKER_CONDITION_ENTRYPOINTBREAK 3
#define UE_UNPACKER_CONDITION_RELOCSNAPSHOT1 4
#define UE_UNPACKER_CONDITION_RELOCSNAPSHOT2 5

#define UE_FIELD_OK 0
#define UE_FIELD_BROKEN_NON_FIXABLE 1
#define UE_FIELD_BROKEN_NON_CRITICAL 2
#define UE_FIELD_BROKEN_FIXABLE_FOR_STATIC_USE 3
#define UE_FIELD_BROKEN_BUT_CAN_BE_EMULATED 4
#define UE_FIELD_FIXABLE_NON_CRITICAL 5
#define UE_FIELD_FIXABLE_CRITICAL 6
#define UE_FIELD_NOT_PRESET 7
#define UE_FIELD_NOT_PRESET_WARNING 8

#define UE_RESULT_FILE_OK 10
#define UE_RESULT_FILE_INVALID_BUT_FIXABLE 11
#define UE_RESULT_FILE_INVALID_AND_NON_FIXABLE 12
#define UE_RESULT_FILE_INVALID_FORMAT 13

typedef struct
{
    BYTE OveralEvaluation;
    bool EvaluationTerminatedByException;
    bool FileIs64Bit;
    bool FileIsDLL;
    bool FileIsConsole;
    bool MissingDependencies;
    bool MissingDeclaredAPIs;
    BYTE SignatureMZ;
    BYTE SignaturePE;
    BYTE EntryPoint;
    BYTE ImageBase;
    BYTE SizeOfImage;
    BYTE FileAlignment;
    BYTE SectionAlignment;
    BYTE ExportTable;
    BYTE RelocationTable;
    BYTE ImportTable;
    BYTE ImportTableSection;
    BYTE ImportTableData;
    BYTE IATTable;
    BYTE TLSTable;
    BYTE LoadConfigTable;
    BYTE BoundImportTable;
    BYTE COMHeaderTable;
    BYTE ResourceTable;
    BYTE ResourceData;
    BYTE SectionTable;
} FILE_STATUS_INFO, *PFILE_STATUS_INFO;

typedef struct
{
    BYTE OveralEvaluation;
    bool FixingTerminatedByException;
    bool FileFixPerformed;
    bool StrippedRelocation;
    bool DontFixRelocations;
    DWORD OriginalRelocationTableAddress;
    DWORD OriginalRelocationTableSize;
    bool StrippedExports;
    bool DontFixExports;
    DWORD OriginalExportTableAddress;
    DWORD OriginalExportTableSize;
    bool StrippedResources;
    bool DontFixResources;
    DWORD OriginalResourceTableAddress;
    DWORD OriginalResourceTableSize;
    bool StrippedTLS;
    bool DontFixTLS;
    DWORD OriginalTLSTableAddress;
    DWORD OriginalTLSTableSize;
    bool StrippedLoadConfig;
    bool DontFixLoadConfig;
    DWORD OriginalLoadConfigTableAddress;
    DWORD OriginalLoadConfigTableSize;
    bool StrippedBoundImports;
    bool DontFixBoundImports;
    DWORD OriginalBoundImportTableAddress;
    DWORD OriginalBoundImportTableSize;
    bool StrippedIAT;
    bool DontFixIAT;
    DWORD OriginalImportAddressTableAddress;
    DWORD OriginalImportAddressTableSize;
    bool StrippedCOM;
    bool DontFixCOM;
    DWORD OriginalCOMTableAddress;
    DWORD OriginalCOMTableSize;
} FILE_FIX_INFO, *PFILE_FIX_INFO;

#ifdef __cplusplus
extern "C"
{
#endif

// Global.Function.Declaration:
// TitanEngine.Dumper.functions:
__declspec(dllexport) bool TITCALL DumpProcess(HANDLE hProcess, LPVOID ImageBase, char* szDumpFileName, ULONG_PTR EntryPoint);
__declspec(dllexport) bool TITCALL DumpProcessW(HANDLE hProcess, LPVOID ImageBase, wchar_t* szDumpFileName, ULONG_PTR EntryPoint);
__declspec(dllexport) bool TITCALL DumpProcessEx(DWORD ProcessId, LPVOID ImageBase, char* szDumpFileName, ULONG_PTR EntryPoint);
__declspec(dllexport) bool TITCALL DumpProcessExW(DWORD ProcessId, LPVOID ImageBase, wchar_t* szDumpFileName, ULONG_PTR EntryPoint);
__declspec(dllexport) bool TITCALL DumpMemory(HANDLE hProcess, LPVOID MemoryStart, ULONG_PTR MemorySize, char* szDumpFileName);
__declspec(dllexport) bool TITCALL DumpMemoryW(HANDLE hProcess, LPVOID MemoryStart, ULONG_PTR MemorySize, wchar_t* szDumpFileName);
__declspec(dllexport) bool TITCALL DumpMemoryEx(DWORD ProcessId, LPVOID MemoryStart, ULONG_PTR MemorySize, char* szDumpFileName);
__declspec(dllexport) bool TITCALL DumpMemoryExW(DWORD ProcessId, LPVOID MemoryStart, ULONG_PTR MemorySize, wchar_t* szDumpFileName);
__declspec(dllexport) bool TITCALL DumpRegions(HANDLE hProcess, char* szDumpFolder, bool DumpAboveImageBaseOnly);
__declspec(dllexport) bool TITCALL DumpRegionsW(HANDLE hProcess, wchar_t* szDumpFolder, bool DumpAboveImageBaseOnly);
__declspec(dllexport) bool TITCALL DumpRegionsEx(DWORD ProcessId, char* szDumpFolder, bool DumpAboveImageBaseOnly);
__declspec(dllexport) bool TITCALL DumpRegionsExW(DWORD ProcessId, wchar_t* szDumpFolder, bool DumpAboveImageBaseOnly);
__declspec(dllexport) bool TITCALL DumpModule(HANDLE hProcess, LPVOID ModuleBase, char* szDumpFileName);
__declspec(dllexport) bool TITCALL DumpModuleW(HANDLE hProcess, LPVOID ModuleBase, wchar_t* szDumpFileName);
__declspec(dllexport) bool TITCALL DumpModuleEx(DWORD ProcessId, LPVOID ModuleBase, char* szDumpFileName);
__declspec(dllexport) bool TITCALL DumpModuleExW(DWORD ProcessId, LPVOID ModuleBase, wchar_t* szDumpFileName);
__declspec(dllexport) bool TITCALL PastePEHeader(HANDLE hProcess, LPVOID ImageBase, char* szDebuggedFileName);
__declspec(dllexport) bool TITCALL PastePEHeaderW(HANDLE hProcess, LPVOID ImageBase, wchar_t* szDebuggedFileName);
__declspec(dllexport) bool TITCALL ExtractSection(char* szFileName, char* szDumpFileName, DWORD SectionNumber);
__declspec(dllexport) bool TITCALL ExtractSectionW(wchar_t* szFileName, wchar_t* szDumpFileName, DWORD SectionNumber);
__declspec(dllexport) bool TITCALL ResortFileSections(char* szFileName);
__declspec(dllexport) bool TITCALL ResortFileSectionsW(wchar_t* szFileName);
__declspec(dllexport) bool TITCALL FindOverlay(char* szFileName, LPDWORD OverlayStart, LPDWORD OverlaySize);
__declspec(dllexport) bool TITCALL FindOverlayW(wchar_t* szFileName, LPDWORD OverlayStart, LPDWORD OverlaySize);
__declspec(dllexport) bool TITCALL ExtractOverlay(char* szFileName, char* szExtactedFileName);
__declspec(dllexport) bool TITCALL ExtractOverlayW(wchar_t* szFileName, wchar_t* szExtactedFileName);
__declspec(dllexport) bool TITCALL AddOverlay(char* szFileName, char* szOverlayFileName);
__declspec(dllexport) bool TITCALL AddOverlayW(wchar_t* szFileName, wchar_t* szOverlayFileName);
__declspec(dllexport) bool TITCALL CopyOverlay(char* szInFileName, char* szOutFileName);
__declspec(dllexport) bool TITCALL CopyOverlayW(wchar_t* szInFileName, wchar_t* szOutFileName);
__declspec(dllexport) bool TITCALL RemoveOverlay(char* szFileName);
__declspec(dllexport) bool TITCALL RemoveOverlayW(wchar_t* szFileName);
__declspec(dllexport) bool TITCALL MakeAllSectionsRWE(char* szFileName);
__declspec(dllexport) bool TITCALL MakeAllSectionsRWEW(wchar_t* szFileName);
__declspec(dllexport) long TITCALL AddNewSectionEx(char* szFileName, char* szSectionName, DWORD SectionSize, DWORD SectionAttributes, LPVOID SectionContent, DWORD ContentSize);
__declspec(dllexport) long TITCALL AddNewSectionExW(wchar_t* szFileName, char* szSectionName, DWORD SectionSize, DWORD SectionAttributes, LPVOID SectionContent, DWORD ContentSize);
__declspec(dllexport) long TITCALL AddNewSection(char* szFileName, char* szSectionName, DWORD SectionSize);
__declspec(dllexport) long TITCALL AddNewSectionW(wchar_t* szFileName, char* szSectionName, DWORD SectionSize);
__declspec(dllexport) bool TITCALL ResizeLastSection(char* szFileName, DWORD NumberOfExpandBytes, bool AlignResizeData);
__declspec(dllexport) bool TITCALL ResizeLastSectionW(wchar_t* szFileName, DWORD NumberOfExpandBytes, bool AlignResizeData);
__declspec(dllexport) void TITCALL SetSharedOverlay(char* szFileName);
__declspec(dllexport) void TITCALL SetSharedOverlayW(wchar_t* szFileName);
__declspec(dllexport) char* TITCALL GetSharedOverlay();
__declspec(dllexport) wchar_t* TITCALL GetSharedOverlayW();
__declspec(dllexport) bool TITCALL DeleteLastSection(char* szFileName);
__declspec(dllexport) bool TITCALL DeleteLastSectionW(wchar_t* szFileName);
__declspec(dllexport) bool TITCALL DeleteLastSectionEx(char* szFileName, DWORD NumberOfSections);
__declspec(dllexport) bool TITCALL DeleteLastSectionExW(wchar_t* szFileName, DWORD NumberOfSections);
__declspec(dllexport) ULONG_PTR TITCALL GetPE32DataFromMappedFile(ULONG_PTR FileMapVA, DWORD WhichSection, DWORD WhichData);
__declspec(dllexport) ULONG_PTR TITCALL GetPE32Data(char* szFileName, DWORD WhichSection, DWORD WhichData);
__declspec(dllexport) ULONG_PTR TITCALL GetPE32DataW(wchar_t* szFileName, DWORD WhichSection, DWORD WhichData);
__declspec(dllexport) bool TITCALL GetPE32DataFromMappedFileEx(ULONG_PTR FileMapVA, LPVOID DataStorage);
__declspec(dllexport) bool TITCALL GetPE32DataEx(char* szFileName, LPVOID DataStorage);
__declspec(dllexport) bool TITCALL GetPE32DataExW(wchar_t* szFileName, LPVOID DataStorage);
__declspec(dllexport) bool TITCALL SetPE32DataForMappedFile(ULONG_PTR FileMapVA, DWORD WhichSection, DWORD WhichData, ULONG_PTR NewDataValue);
__declspec(dllexport) bool TITCALL SetPE32Data(char* szFileName, DWORD WhichSection, DWORD WhichData, ULONG_PTR NewDataValue);
__declspec(dllexport) bool TITCALL SetPE32DataW(wchar_t* szFileName, DWORD WhichSection, DWORD WhichData, ULONG_PTR NewDataValue);
__declspec(dllexport) bool TITCALL SetPE32DataForMappedFileEx(ULONG_PTR FileMapVA, LPVOID DataStorage);
__declspec(dllexport) bool TITCALL SetPE32DataEx(char* szFileName, LPVOID DataStorage);
__declspec(dllexport) bool TITCALL SetPE32DataExW(wchar_t* szFileName, LPVOID DataStorage);
__declspec(dllexport) long TITCALL GetPE32SectionNumberFromVA(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert);
__declspec(dllexport) ULONG_PTR TITCALL ConvertVAtoFileOffset(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert, bool ReturnType);
__declspec(dllexport) ULONG_PTR TITCALL ConvertVAtoFileOffsetEx(ULONG_PTR FileMapVA, DWORD FileSize, ULONG_PTR ImageBase, ULONG_PTR AddressToConvert, bool AddressIsRVA, bool ReturnType);
__declspec(dllexport) ULONG_PTR TITCALL ConvertFileOffsetToVA(ULONG_PTR FileMapVA, ULONG_PTR AddressToConvert, bool ReturnType);
__declspec(dllexport) ULONG_PTR TITCALL ConvertFileOffsetToVAEx(ULONG_PTR FileMapVA, DWORD FileSize, ULONG_PTR ImageBase, ULONG_PTR AddressToConvert, bool ReturnType);
__declspec(dllexport) bool TITCALL MemoryReadSafe(HANDLE hProcess, LPVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead);
__declspec(dllexport) bool TITCALL MemoryWriteSafe(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten);
// TitanEngine.Realigner.functions:
__declspec(dllexport) bool TITCALL FixHeaderCheckSum(char* szFileName);
__declspec(dllexport) bool TITCALL FixHeaderCheckSumW(wchar_t* szFileName);
__declspec(dllexport) long TITCALL RealignPE(ULONG_PTR FileMapVA, DWORD FileSize, DWORD RealingMode);
__declspec(dllexport) long TITCALL RealignPEEx(char* szFileName, DWORD RealingFileSize, DWORD ForcedFileAlignment);
__declspec(dllexport) long TITCALL RealignPEExW(wchar_t* szFileName, DWORD RealingFileSize, DWORD ForcedFileAlignment);
__declspec(dllexport) bool TITCALL WipeSection(char* szFileName, int WipeSectionNumber, bool RemovePhysically);
__declspec(dllexport) bool TITCALL WipeSectionW(wchar_t* szFileName, int WipeSectionNumber, bool RemovePhysically);
__declspec(dllexport) bool TITCALL IsPE32FileValidEx(char* szFileName, DWORD CheckDepth, LPVOID FileStatusInfo);
__declspec(dllexport) bool TITCALL IsPE32FileValidExW(wchar_t* szFileName, DWORD CheckDepth, LPVOID FileStatusInfo);
__declspec(dllexport) bool TITCALL FixBrokenPE32FileEx(char* szFileName, LPVOID FileStatusInfo, LPVOID FileFixInfo);
__declspec(dllexport) bool TITCALL FixBrokenPE32FileExW(wchar_t* szFileName, LPVOID FileStatusInfo, LPVOID FileFixInfo);
__declspec(dllexport) bool TITCALL IsFileDLL(char* szFileName, ULONG_PTR FileMapVA);
__declspec(dllexport) bool TITCALL IsFileDLLW(wchar_t* szFileName, ULONG_PTR FileMapVA);
// TitanEngine.Hider.functions:
__declspec(dllexport) void* TITCALL GetPEBLocation(HANDLE hProcess);
__declspec(dllexport) void* TITCALL GetPEBLocation64(HANDLE hProcess);
__declspec(dllexport) void* TITCALL GetTEBLocation(HANDLE hThread);
__declspec(dllexport) void* TITCALL GetTEBLocation64(HANDLE hThread);
__declspec(dllexport) bool TITCALL HideDebugger(HANDLE hProcess, DWORD PatchAPILevel);
__declspec(dllexport) bool TITCALL UnHideDebugger(HANDLE hProcess, DWORD PatchAPILevel);
// TitanEngine.Relocater.functions:
__declspec(dllexport) void TITCALL RelocaterCleanup();
__declspec(dllexport) void TITCALL RelocaterInit(DWORD MemorySize, ULONG_PTR OldImageBase, ULONG_PTR NewImageBase);
__declspec(dllexport) void TITCALL RelocaterAddNewRelocation(HANDLE hProcess, ULONG_PTR RelocateAddress, DWORD RelocateState);
__declspec(dllexport) long TITCALL RelocaterEstimatedSize();
__declspec(dllexport) bool TITCALL RelocaterExportRelocation(ULONG_PTR StorePlace, DWORD StorePlaceRVA, ULONG_PTR FileMapVA);
__declspec(dllexport) bool TITCALL RelocaterExportRelocationEx(char* szFileName, char* szSectionName);
__declspec(dllexport) bool TITCALL RelocaterExportRelocationExW(wchar_t* szFileName, char* szSectionName);
__declspec(dllexport) bool TITCALL RelocaterGrabRelocationTable(HANDLE hProcess, ULONG_PTR MemoryStart, DWORD MemorySize);
__declspec(dllexport) bool TITCALL RelocaterGrabRelocationTableEx(HANDLE hProcess, ULONG_PTR MemoryStart, ULONG_PTR MemorySize, DWORD NtSizeOfImage);
__declspec(dllexport) bool TITCALL RelocaterMakeSnapshot(HANDLE hProcess, char* szSaveFileName, LPVOID MemoryStart, ULONG_PTR MemorySize);
__declspec(dllexport) bool TITCALL RelocaterMakeSnapshotW(HANDLE hProcess, wchar_t* szSaveFileName, LPVOID MemoryStart, ULONG_PTR MemorySize);
__declspec(dllexport) bool TITCALL RelocaterCompareTwoSnapshots(HANDLE hProcess, ULONG_PTR LoadedImageBase, ULONG_PTR NtSizeOfImage, char* szDumpFile1, char* szDumpFile2, ULONG_PTR MemStart);
__declspec(dllexport) bool TITCALL RelocaterCompareTwoSnapshotsW(HANDLE hProcess, ULONG_PTR LoadedImageBase, ULONG_PTR NtSizeOfImage, wchar_t* szDumpFile1, wchar_t* szDumpFile2, ULONG_PTR MemStart);
__declspec(dllexport) bool TITCALL RelocaterChangeFileBase(char* szFileName, ULONG_PTR NewImageBase);
__declspec(dllexport) bool TITCALL RelocaterChangeFileBaseW(wchar_t* szFileName, ULONG_PTR NewImageBase);
__declspec(dllexport) bool TITCALL RelocaterRelocateMemoryBlock(ULONG_PTR FileMapVA, ULONG_PTR MemoryLocation, void* RelocateMemory, DWORD RelocateMemorySize, ULONG_PTR CurrentLoadedBase, ULONG_PTR RelocateBase);
__declspec(dllexport) bool TITCALL RelocaterWipeRelocationTable(char* szFileName);
__declspec(dllexport) bool TITCALL RelocaterWipeRelocationTableW(wchar_t* szFileName);
// TitanEngine.Resourcer.functions:
__declspec(dllexport) ULONG_PTR TITCALL ResourcerLoadFileForResourceUse(char* szFileName);
__declspec(dllexport) ULONG_PTR TITCALL ResourcerLoadFileForResourceUseW(wchar_t* szFileName);
__declspec(dllexport) bool TITCALL ResourcerFreeLoadedFile(LPVOID LoadedFileBase);
__declspec(dllexport) bool TITCALL ResourcerExtractResourceFromFileEx(ULONG_PTR FileMapVA, char* szResourceType, char* szResourceName, char* szExtractedFileName);
__declspec(dllexport) bool TITCALL ResourcerExtractResourceFromFile(char* szFileName, char* szResourceType, char* szResourceName, char* szExtractedFileName);
__declspec(dllexport) bool TITCALL ResourcerExtractResourceFromFileW(wchar_t* szFileName, char* szResourceType, char* szResourceName, char* szExtractedFileName);
__declspec(dllexport) bool TITCALL ResourcerFindResource(char* szFileName, char* szResourceType, DWORD ResourceType, char* szResourceName, DWORD ResourceName, DWORD ResourceLanguage, PULONG_PTR pResourceData, LPDWORD pResourceSize);
__declspec(dllexport) bool TITCALL ResourcerFindResourceW(wchar_t* szFileName, wchar_t* szResourceType, DWORD ResourceType, wchar_t* szResourceName, DWORD ResourceName, DWORD ResourceLanguage, PULONG_PTR pResourceData, LPDWORD pResourceSize);
__declspec(dllexport) bool TITCALL ResourcerFindResourceEx(ULONG_PTR FileMapVA, DWORD FileSize, wchar_t* szResourceType, DWORD ResourceType, wchar_t* szResourceName, DWORD ResourceName, DWORD ResourceLanguage, PULONG_PTR pResourceData, LPDWORD pResourceSize);
__declspec(dllexport) void TITCALL ResourcerEnumerateResource(char* szFileName, void* CallBack);
__declspec(dllexport) void TITCALL ResourcerEnumerateResourceW(wchar_t* szFileName, void* CallBack);
__declspec(dllexport) void TITCALL ResourcerEnumerateResourceEx(ULONG_PTR FileMapVA, DWORD FileSize, void* CallBack);
// TitanEngine.Threader.functions:
__declspec(dllexport) bool TITCALL ThreaderImportRunningThreadData(DWORD ProcessId);
__declspec(dllexport) void* TITCALL ThreaderGetThreadInfo(HANDLE hThread, DWORD ThreadId);
__declspec(dllexport) void TITCALL ThreaderEnumThreadInfo(void* EnumCallBack);
__declspec(dllexport) bool TITCALL ThreaderPauseThread(HANDLE hThread);
__declspec(dllexport) bool TITCALL ThreaderResumeThread(HANDLE hThread);
__declspec(dllexport) bool TITCALL ThreaderTerminateThread(HANDLE hThread, DWORD ThreadExitCode);
__declspec(dllexport) bool TITCALL ThreaderPauseAllThreads(bool LeaveMainRunning);
__declspec(dllexport) bool TITCALL ThreaderResumeAllThreads(bool LeaveMainPaused);
__declspec(dllexport) bool TITCALL ThreaderPauseProcess();
__declspec(dllexport) bool TITCALL ThreaderResumeProcess();
__declspec(dllexport) ULONG_PTR TITCALL ThreaderCreateRemoteThread(ULONG_PTR ThreadStartAddress, bool AutoCloseTheHandle, LPVOID ThreadPassParameter, LPDWORD ThreadId);
__declspec(dllexport) bool TITCALL ThreaderInjectAndExecuteCode(LPVOID InjectCode, DWORD StartDelta, DWORD InjectSize);
__declspec(dllexport) ULONG_PTR TITCALL ThreaderCreateRemoteThreadEx(HANDLE hProcess, ULONG_PTR ThreadStartAddress, bool AutoCloseTheHandle, LPVOID ThreadPassParameter, LPDWORD ThreadId);
__declspec(dllexport) bool TITCALL ThreaderInjectAndExecuteCodeEx(HANDLE hProcess, LPVOID InjectCode, DWORD StartDelta, DWORD InjectSize);
__declspec(dllexport) void TITCALL ThreaderSetCallBackForNextExitThreadEvent(LPVOID exitThreadCallBack);
__declspec(dllexport) bool TITCALL ThreaderIsThreadStillRunning(HANDLE hThread);
__declspec(dllexport) bool TITCALL ThreaderIsThreadActive(HANDLE hThread);
__declspec(dllexport) bool TITCALL ThreaderIsAnyThreadActive();
__declspec(dllexport) bool TITCALL ThreaderExecuteOnlyInjectedThreads();
__declspec(dllexport) ULONG_PTR TITCALL ThreaderGetOpenHandleForThread(DWORD ThreadId);
__declspec(dllexport) bool TITCALL ThreaderIsExceptionInMainThread();
// TitanEngine.Debugger.functions:
__declspec(dllexport) void* TITCALL StaticDisassembleEx(ULONG_PTR DisassmStart, LPVOID DisassmAddress);
__declspec(dllexport) void* TITCALL StaticDisassemble(LPVOID DisassmAddress);
__declspec(dllexport) void* TITCALL DisassembleEx(HANDLE hProcess, LPVOID DisassmAddress, bool ReturnInstructionType);
__declspec(dllexport) void* TITCALL Disassemble(LPVOID DisassmAddress);
__declspec(dllexport) long TITCALL StaticLengthDisassemble(LPVOID DisassmAddress);
__declspec(dllexport) long TITCALL LengthDisassembleEx(HANDLE hProcess, LPVOID DisassmAddress);
__declspec(dllexport) long TITCALL LengthDisassemble(LPVOID DisassmAddress);
__declspec(dllexport) void* TITCALL InitDebug(char* szFileName, char* szCommandLine, char* szCurrentFolder);
__declspec(dllexport) void* TITCALL InitDebugW(wchar_t* szFileName, wchar_t* szCommandLine, wchar_t* szCurrentFolder);
__declspec(dllexport) void* TITCALL InitDebugEx(char* szFileName, char* szCommandLine, char* szCurrentFolder, LPVOID EntryCallBack);
__declspec(dllexport) void* TITCALL InitDebugExW(wchar_t* szFileName, wchar_t* szCommandLine, wchar_t* szCurrentFolder, LPVOID EntryCallBack);
__declspec(dllexport) void* TITCALL InitDLLDebug(char* szFileName, bool ReserveModuleBase, char* szCommandLine, char* szCurrentFolder, LPVOID EntryCallBack);
__declspec(dllexport) void* TITCALL InitDLLDebugW(wchar_t* szFileName, bool ReserveModuleBase, wchar_t* szCommandLine, wchar_t* szCurrentFolder, LPVOID EntryCallBack);
__declspec(dllexport) bool TITCALL StopDebug();
__declspec(dllexport) void TITCALL SetBPXOptions(long DefaultBreakPointType);
__declspec(dllexport) bool TITCALL IsBPXEnabled(ULONG_PTR bpxAddress);
__declspec(dllexport) bool TITCALL EnableBPX(ULONG_PTR bpxAddress);
__declspec(dllexport) bool TITCALL DisableBPX(ULONG_PTR bpxAddress);
__declspec(dllexport) bool TITCALL SetBPX(ULONG_PTR bpxAddress, DWORD bpxType, LPVOID bpxCallBack);
__declspec(dllexport) bool TITCALL DeleteBPX(ULONG_PTR bpxAddress);
__declspec(dllexport) bool TITCALL SafeDeleteBPX(ULONG_PTR bpxAddress);
__declspec(dllexport) bool TITCALL SetAPIBreakPoint(const char* szDLLName, const char* szAPIName, DWORD bpxType, DWORD bpxPlace, LPVOID bpxCallBack);
__declspec(dllexport) bool TITCALL DeleteAPIBreakPoint(const char* szDLLName, const char* szAPIName, DWORD bpxPlace);
__declspec(dllexport) bool TITCALL SafeDeleteAPIBreakPoint(const char* szDLLName, const char* szAPIName, DWORD bpxPlace);
__declspec(dllexport) bool TITCALL SetMemoryBPX(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory, LPVOID bpxCallBack);
__declspec(dllexport) bool TITCALL SetMemoryBPXEx(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory, DWORD BreakPointType, bool RestoreOnHit, LPVOID bpxCallBack);
__declspec(dllexport) bool TITCALL RemoveMemoryBPX(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory);
__declspec(dllexport) bool TITCALL GetContextFPUDataEx(HANDLE hActiveThread, void* FPUSaveArea);
__declspec(dllexport) ULONG_PTR TITCALL GetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister);
__declspec(dllexport) ULONG_PTR TITCALL GetContextData(DWORD IndexOfRegister);
__declspec(dllexport) bool TITCALL SetContextFPUDataEx(HANDLE hActiveThread, void* FPUSaveArea);
__declspec(dllexport) bool TITCALL SetContextDataEx(HANDLE hActiveThread, DWORD IndexOfRegister, ULONG_PTR NewRegisterValue);
__declspec(dllexport) bool TITCALL SetContextData(DWORD IndexOfRegister, ULONG_PTR NewRegisterValue);
__declspec(dllexport) void TITCALL ClearExceptionNumber();
__declspec(dllexport) long TITCALL CurrentExceptionNumber();
__declspec(dllexport) bool TITCALL MatchPatternEx(HANDLE hProcess, void* MemoryToCheck, int SizeOfMemoryToCheck, void* PatternToMatch, int SizeOfPatternToMatch, PBYTE WildCard);
__declspec(dllexport) bool TITCALL MatchPattern(void* MemoryToCheck, int SizeOfMemoryToCheck, void* PatternToMatch, int SizeOfPatternToMatch, PBYTE WildCard);
__declspec(dllexport) ULONG_PTR TITCALL FindEx(HANDLE hProcess, LPVOID MemoryStart, DWORD MemorySize, LPVOID SearchPattern, DWORD PatternSize, LPBYTE WildCard);
extern "C" __declspec(dllexport) ULONG_PTR TITCALL Find(LPVOID MemoryStart, DWORD MemorySize, LPVOID SearchPattern, DWORD PatternSize, LPBYTE WildCard);
__declspec(dllexport) bool TITCALL FillEx(HANDLE hProcess, LPVOID MemoryStart, DWORD MemorySize, PBYTE FillByte);
__declspec(dllexport) bool TITCALL Fill(LPVOID MemoryStart, DWORD MemorySize, PBYTE FillByte);
__declspec(dllexport) bool TITCALL PatchEx(HANDLE hProcess, LPVOID MemoryStart, DWORD MemorySize, LPVOID ReplacePattern, DWORD ReplaceSize, bool AppendNOP, bool PrependNOP);
__declspec(dllexport) bool TITCALL Patch(LPVOID MemoryStart, DWORD MemorySize, LPVOID ReplacePattern, DWORD ReplaceSize, bool AppendNOP, bool PrependNOP);
__declspec(dllexport) bool TITCALL ReplaceEx(HANDLE hProcess, LPVOID MemoryStart, DWORD MemorySize, LPVOID SearchPattern, DWORD PatternSize, DWORD NumberOfRepetitions, LPVOID ReplacePattern, DWORD ReplaceSize, PBYTE WildCard);
__declspec(dllexport) bool TITCALL Replace(LPVOID MemoryStart, DWORD MemorySize, LPVOID SearchPattern, DWORD PatternSize, DWORD NumberOfRepetitions, LPVOID ReplacePattern, DWORD ReplaceSize, PBYTE WildCard);
__declspec(dllexport) void* TITCALL GetDebugData();
__declspec(dllexport) void* TITCALL GetTerminationData();
__declspec(dllexport) long TITCALL GetExitCode();
__declspec(dllexport) ULONG_PTR TITCALL GetDebuggedDLLBaseAddress();
__declspec(dllexport) ULONG_PTR TITCALL GetDebuggedFileBaseAddress();
__declspec(dllexport) bool TITCALL GetRemoteString(HANDLE hProcess, LPVOID StringAddress, LPVOID StringStorage, int MaximumStringSize);
__declspec(dllexport) ULONG_PTR TITCALL GetFunctionParameter(HANDLE hProcess, DWORD FunctionType, DWORD ParameterNumber, DWORD ParameterType);
__declspec(dllexport) ULONG_PTR TITCALL GetJumpDestinationEx(HANDLE hProcess, ULONG_PTR InstructionAddress, bool JustJumps);
__declspec(dllexport) ULONG_PTR TITCALL GetJumpDestination(HANDLE hProcess, ULONG_PTR InstructionAddress);
__declspec(dllexport) bool TITCALL IsJumpGoingToExecuteEx(HANDLE hProcess, HANDLE hThread, ULONG_PTR InstructionAddress, ULONG_PTR RegFlags);
__declspec(dllexport) bool TITCALL IsJumpGoingToExecute();
__declspec(dllexport) void TITCALL SetCustomHandler(DWORD ExceptionId, LPVOID CallBack);
__declspec(dllexport) void TITCALL ForceClose();
__declspec(dllexport) void TITCALL StepInto(LPVOID traceCallBack);
__declspec(dllexport) void TITCALL StepOver(LPVOID traceCallBack);
__declspec(dllexport) void TITCALL StepOut(LPVOID StepOut, bool StepFinal);
__declspec(dllexport) void TITCALL SingleStep(DWORD StepCount, LPVOID StepCallBack);
__declspec(dllexport) bool TITCALL GetUnusedHardwareBreakPointRegister(LPDWORD RegisterIndex);
__declspec(dllexport) bool TITCALL SetHardwareBreakPointEx(HANDLE hActiveThread, ULONG_PTR bpxAddress, DWORD IndexOfRegister, DWORD bpxType, DWORD bpxSize, LPVOID bpxCallBack, LPDWORD IndexOfSelectedRegister);
__declspec(dllexport) bool TITCALL SetHardwareBreakPoint(ULONG_PTR bpxAddress, DWORD IndexOfRegister, DWORD bpxType, DWORD bpxSize, LPVOID bpxCallBack);
__declspec(dllexport) bool TITCALL DeleteHardwareBreakPoint(DWORD IndexOfRegister);
__declspec(dllexport) bool TITCALL RemoveAllBreakPoints(DWORD RemoveOption);
__declspec(dllexport) PROCESS_INFORMATION* TITCALL TitanGetProcessInformation();
__declspec(dllexport) STARTUPINFOW* TITCALL TitanGetStartupInformation();
__declspec(dllexport) void TITCALL DebugLoop();
__declspec(dllexport) void TITCALL SetDebugLoopTimeOut(DWORD TimeOut);
__declspec(dllexport) void TITCALL SetNextDbgContinueStatus(DWORD SetDbgCode);
__declspec(dllexport) bool TITCALL AttachDebugger(DWORD ProcessId, bool KillOnExit, LPVOID DebugInfo, LPVOID CallBack);
__declspec(dllexport) bool TITCALL DetachDebugger(DWORD ProcessId);
__declspec(dllexport) bool TITCALL DetachDebuggerEx(DWORD ProcessId);
__declspec(dllexport) void TITCALL DebugLoopEx(DWORD TimeOut);
__declspec(dllexport) void TITCALL AutoDebugEx(char* szFileName, bool ReserveModuleBase, char* szCommandLine, char* szCurrentFolder, DWORD TimeOut, LPVOID EntryCallBack);
__declspec(dllexport) void TITCALL AutoDebugExW(wchar_t* szFileName, bool ReserveModuleBase, wchar_t* szCommandLine, wchar_t* szCurrentFolder, DWORD TimeOut, LPVOID EntryCallBack);
__declspec(dllexport) bool TITCALL IsFileBeingDebugged();
__declspec(dllexport) void TITCALL SetErrorModel(bool DisplayErrorMessages);
// TitanEngine.FindOEP.functions:
__declspec(dllexport) void TITCALL FindOEPInit();
__declspec(dllexport) bool TITCALL FindOEPGenerically(char* szFileName, LPVOID TraceInitCallBack, LPVOID CallBack);
__declspec(dllexport) bool TITCALL FindOEPGenericallyW(wchar_t* szFileName, LPVOID TraceInitCallBack, LPVOID CallBack);
// TitanEngine.Importer.functions:
__declspec(dllexport) void TITCALL ImporterAddNewDll(char* szDLLName, ULONG_PTR FirstThunk);
__declspec(dllexport) void TITCALL ImporterAddNewAPI(char* szAPIName, ULONG_PTR ThunkValue);
__declspec(dllexport) void TITCALL ImporterAddNewOrdinalAPI(ULONG_PTR OrdinalNumber, ULONG_PTR ThunkValue);
__declspec(dllexport) long TITCALL ImporterGetAddedDllCount();
__declspec(dllexport) long TITCALL ImporterGetAddedAPICount();
__declspec(dllexport) bool TITCALL ImporterExportIAT(ULONG_PTR StorePlace, ULONG_PTR FileMapVA, HANDLE hFileMap);
__declspec(dllexport) long TITCALL ImporterEstimatedSize();
__declspec(dllexport) bool TITCALL ImporterExportIATEx(char* szDumpFileName, char* szExportFileName, char* szSectionName);
__declspec(dllexport) bool TITCALL ImporterExportIATExW(wchar_t* szDumpFileName, wchar_t* szExportFileName, wchar_t* szSectionName = L".RL!TEv2");
__declspec(dllexport) ULONG_PTR TITCALL ImporterFindAPIWriteLocation(char* szAPIName);
__declspec(dllexport) ULONG_PTR TITCALL ImporterFindOrdinalAPIWriteLocation(ULONG_PTR OrdinalNumber);
__declspec(dllexport) ULONG_PTR TITCALL ImporterFindAPIByWriteLocation(ULONG_PTR APIWriteLocation);
__declspec(dllexport) ULONG_PTR TITCALL ImporterFindDLLByWriteLocation(ULONG_PTR APIWriteLocation);
__declspec(dllexport) void* TITCALL ImporterGetDLLName(ULONG_PTR APIAddress);
__declspec(dllexport) void* TITCALL ImporterGetDLLNameW(ULONG_PTR APIAddress);
__declspec(dllexport) void* TITCALL ImporterGetAPIName(ULONG_PTR APIAddress);
__declspec(dllexport) ULONG_PTR TITCALL ImporterGetAPIOrdinalNumber(ULONG_PTR APIAddress);
__declspec(dllexport) void* TITCALL ImporterGetAPINameEx(ULONG_PTR APIAddress, ULONG_PTR DLLBasesList);
__declspec(dllexport) ULONG_PTR TITCALL ImporterGetRemoteAPIAddress(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) ULONG_PTR TITCALL ImporterGetRemoteAPIAddressEx(char* szDLLName, char* szAPIName);
__declspec(dllexport) ULONG_PTR TITCALL ImporterGetLocalAPIAddress(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) void* TITCALL ImporterGetDLLNameFromDebugee(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) void* TITCALL ImporterGetDLLNameFromDebugeeW(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) void* TITCALL ImporterGetAPINameFromDebugee(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) ULONG_PTR TITCALL ImporterGetAPIOrdinalNumberFromDebugee(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) long TITCALL ImporterGetDLLIndexEx(ULONG_PTR APIAddress, ULONG_PTR DLLBasesList);
__declspec(dllexport) long TITCALL ImporterGetDLLIndex(HANDLE hProcess, ULONG_PTR APIAddress, ULONG_PTR DLLBasesList);
__declspec(dllexport) ULONG_PTR TITCALL ImporterGetRemoteDLLBase(HANDLE hProcess, HMODULE LocalModuleBase);
__declspec(dllexport) ULONG_PTR TITCALL ImporterGetRemoteDLLBaseEx(HANDLE hProcess, char* szModuleName);
__declspec(dllexport) void* TITCALL ImporterGetRemoteDLLBaseExW(HANDLE hProcess, wchar_t* szModuleName);
__declspec(dllexport) bool TITCALL ImporterIsForwardedAPI(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) void* TITCALL ImporterGetForwardedAPIName(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) void* TITCALL ImporterGetForwardedDLLName(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) long TITCALL ImporterGetForwardedDLLIndex(HANDLE hProcess, ULONG_PTR APIAddress, ULONG_PTR DLLBasesList);
__declspec(dllexport) ULONG_PTR TITCALL ImporterGetForwardedAPIOrdinalNumber(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) ULONG_PTR TITCALL ImporterGetNearestAPIAddress(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) void* TITCALL ImporterGetNearestAPIName(HANDLE hProcess, ULONG_PTR APIAddress);
__declspec(dllexport) bool TITCALL ImporterCopyOriginalIAT(char* szOriginalFile, char* szDumpFile);
__declspec(dllexport) bool TITCALL ImporterCopyOriginalIATW(wchar_t* szOriginalFile, wchar_t* szDumpFile);
__declspec(dllexport) bool TITCALL ImporterLoadImportTable(char* szFileName);
__declspec(dllexport) bool TITCALL ImporterLoadImportTableW(wchar_t* szFileName);
__declspec(dllexport) bool TITCALL ImporterMoveOriginalIAT(char* szOriginalFile, char* szDumpFile, char* szSectionName);
__declspec(dllexport) bool TITCALL ImporterMoveOriginalIATW(wchar_t* szOriginalFile, wchar_t* szDumpFile, char* szSectionName);
__declspec(dllexport) void TITCALL ImporterAutoSearchIAT(DWORD ProcessId, char* szFileName, ULONG_PTR SearchStart, LPVOID pIATStart, LPVOID pIATSize);
__declspec(dllexport) void TITCALL ImporterAutoSearchIATW(DWORD ProcessIds, wchar_t* szFileName, ULONG_PTR SearchStart, LPVOID pIATStart, LPVOID pIATSize);
__declspec(dllexport) void TITCALL ImporterAutoSearchIATEx(DWORD ProcessId, ULONG_PTR ImageBase, ULONG_PTR SearchStart, LPVOID pIATStart, LPVOID pIATSize);
__declspec(dllexport) void TITCALL ImporterEnumAddedData(LPVOID EnumCallBack);
__declspec(dllexport) long TITCALL ImporterAutoFixIATEx(DWORD ProcessId, char* szDumpedFile, char* szSectionName, bool DumpRunningProcess, bool RealignFile, ULONG_PTR EntryPointAddress, ULONG_PTR ImageBase, ULONG_PTR SearchStart, bool TryAutoFix, bool FixEliminations, LPVOID UnknownPointerFixCallback);
__declspec(dllexport) long TITCALL ImporterAutoFixIATExW(DWORD ProcessId, wchar_t* szDumpedFile, wchar_t* szSectionName, bool DumpRunningProcess, bool RealignFile, ULONG_PTR EntryPointAddress, ULONG_PTR ImageBase, ULONG_PTR SearchStart,  bool TryAutoFix, bool FixEliminations, LPVOID UnknownPointerFixCallback);
__declspec(dllexport) long TITCALL ImporterAutoFixIAT(DWORD ProcessId, char* szDumpedFile, ULONG_PTR SearchStart);
__declspec(dllexport) long TITCALL ImporterAutoFixIATW(DWORD ProcessId, wchar_t* szDumpedFile, ULONG_PTR SearchStart);
__declspec(dllexport) bool TITCALL ImporterDeleteAPI(DWORD_PTR apiAddr);
// Global.Engine.Hook.functions:
__declspec(dllexport) bool TITCALL HooksSafeTransitionEx(LPVOID HookAddressArray, int NumberOfHooks, bool TransitionStart);
__declspec(dllexport) bool TITCALL HooksSafeTransition(LPVOID HookAddress, bool TransitionStart);
__declspec(dllexport) bool TITCALL HooksIsAddressRedirected(LPVOID HookAddress);
__declspec(dllexport) void* TITCALL HooksGetTrampolineAddress(LPVOID HookAddress);
__declspec(dllexport) void* TITCALL HooksGetHookEntryDetails(LPVOID HookAddress);
__declspec(dllexport) bool TITCALL HooksInsertNewRedirection(LPVOID HookAddress, LPVOID RedirectTo, int HookType);
__declspec(dllexport) bool TITCALL HooksInsertNewIATRedirectionEx(ULONG_PTR FileMapVA, ULONG_PTR LoadedModuleBase, char* szHookFunction, LPVOID RedirectTo);
__declspec(dllexport) bool TITCALL HooksInsertNewIATRedirection(char* szModuleName, char* szHookFunction, LPVOID RedirectTo);
__declspec(dllexport) bool TITCALL HooksRemoveRedirection(LPVOID HookAddress, bool RemoveAll);
__declspec(dllexport) bool TITCALL HooksRemoveRedirectionsForModule(HMODULE ModuleBase);
__declspec(dllexport) bool TITCALL HooksRemoveIATRedirection(char* szModuleName, char* szHookFunction, bool RemoveAll);
__declspec(dllexport) bool TITCALL HooksDisableRedirection(LPVOID HookAddress, bool DisableAll);
__declspec(dllexport) bool TITCALL HooksDisableRedirectionsForModule(HMODULE ModuleBase);
__declspec(dllexport) bool TITCALL HooksDisableIATRedirection(char* szModuleName, char* szHookFunction, bool DisableAll);
__declspec(dllexport) bool TITCALL HooksEnableRedirection(LPVOID HookAddress, bool EnableAll);
__declspec(dllexport) bool TITCALL HooksEnableRedirectionsForModule(HMODULE ModuleBase);
__declspec(dllexport) bool TITCALL HooksEnableIATRedirection(char* szModuleName, char* szHookFunction, bool EnableAll);
__declspec(dllexport) void TITCALL HooksScanModuleMemory(HMODULE ModuleBase, LPVOID CallBack);
__declspec(dllexport) void TITCALL HooksScanEntireProcessMemory(LPVOID CallBack);
__declspec(dllexport) void TITCALL HooksScanEntireProcessMemoryEx();
// TitanEngine.Tracer.functions:
__declspec(dllexport) void TITCALL TracerInit();
__declspec(dllexport) ULONG_PTR TITCALL TracerLevel1(HANDLE hProcess, ULONG_PTR AddressToTrace);
__declspec(dllexport) ULONG_PTR TITCALL HashTracerLevel1(HANDLE hProcess, ULONG_PTR AddressToTrace, DWORD InputNumberOfInstructions);
__declspec(dllexport) long TITCALL TracerDetectRedirection(HANDLE hProcess, ULONG_PTR AddressToTrace);
__declspec(dllexport) ULONG_PTR TITCALL TracerFixKnownRedirection(HANDLE hProcess, ULONG_PTR AddressToTrace, DWORD RedirectionId);
__declspec(dllexport) ULONG_PTR TITCALL TracerFixRedirectionViaModule(HMODULE hModuleHandle, HANDLE hProcess, ULONG_PTR AddressToTrace, DWORD IdParameter);
__declspec(dllexport) long TITCALL TracerFixRedirectionViaImpRecPlugin(HANDLE hProcess, char* szPluginName, ULONG_PTR AddressToTrace);
// TitanEngine.Exporter.functions:
__declspec(dllexport) void TITCALL ExporterCleanup();
__declspec(dllexport) void TITCALL ExporterSetImageBase(ULONG_PTR ImageBase);
__declspec(dllexport) void TITCALL ExporterInit(DWORD MemorySize, ULONG_PTR ImageBase, DWORD ExportOrdinalBase, char* szExportModuleName);
__declspec(dllexport) bool TITCALL ExporterAddNewExport(char* szExportName, DWORD ExportRelativeAddress);
__declspec(dllexport) bool TITCALL ExporterAddNewOrdinalExport(DWORD OrdinalNumber, DWORD ExportRelativeAddress);
__declspec(dllexport) long TITCALL ExporterGetAddedExportCount();
__declspec(dllexport) long TITCALL ExporterEstimatedSize();
__declspec(dllexport) bool TITCALL ExporterBuildExportTable(ULONG_PTR StorePlace, ULONG_PTR FileMapVA);
__declspec(dllexport) bool TITCALL ExporterBuildExportTableEx(char* szExportFileName, char* szSectionName);
__declspec(dllexport) bool TITCALL ExporterBuildExportTableExW(wchar_t* szExportFileName, char* szSectionName);
__declspec(dllexport) bool TITCALL ExporterLoadExportTable(char* szFileName);
__declspec(dllexport) bool TITCALL ExporterLoadExportTableW(wchar_t* szFileName);
// TitanEngine.Librarian.functions:
__declspec(dllexport) bool TITCALL LibrarianSetBreakPoint(char* szLibraryName, DWORD bpxType, bool SingleShoot, LPVOID bpxCallBack);
__declspec(dllexport) bool TITCALL LibrarianRemoveBreakPoint(char* szLibraryName, DWORD bpxType);
__declspec(dllexport) void* TITCALL LibrarianGetLibraryInfo(char* szLibraryName);
__declspec(dllexport) void* TITCALL LibrarianGetLibraryInfoW(wchar_t* szLibraryName);
__declspec(dllexport) void* TITCALL LibrarianGetLibraryInfoEx(void* BaseOfDll);
__declspec(dllexport) void* TITCALL LibrarianGetLibraryInfoExW(void* BaseOfDll);
__declspec(dllexport) void TITCALL LibrarianEnumLibraryInfo(void* EnumCallBack);
__declspec(dllexport) void TITCALL LibrarianEnumLibraryInfoW(void* EnumCallBack);
// TitanEngine.Process.functions:
__declspec(dllexport) long TITCALL GetActiveProcessId(char* szImageName);
__declspec(dllexport) long TITCALL GetActiveProcessIdW(wchar_t* szImageName);
__declspec(dllexport) void TITCALL EnumProcessesWithLibrary(char* szLibraryName, void* EnumFunction);
__declspec(dllexport) HANDLE TITCALL TitanOpenProcess(DWORD dwDesiredAccess, bool bInheritHandle, DWORD dwProcessId);
// TitanEngine.TLSFixer.functions:
__declspec(dllexport) bool TITCALL TLSBreakOnCallBack(LPVOID ArrayOfCallBacks, DWORD NumberOfCallBacks, LPVOID bpxCallBack);
__declspec(dllexport) bool TITCALL TLSGrabCallBackData(char* szFileName, LPVOID ArrayOfCallBacks, LPDWORD NumberOfCallBacks);
__declspec(dllexport) bool TITCALL TLSGrabCallBackDataW(wchar_t* szFileName, LPVOID ArrayOfCallBacks, LPDWORD NumberOfCallBacks);
__declspec(dllexport) bool TITCALL TLSBreakOnCallBackEx(char* szFileName, LPVOID bpxCallBack);
__declspec(dllexport) bool TITCALL TLSBreakOnCallBackExW(wchar_t* szFileName, LPVOID bpxCallBack);
__declspec(dllexport) bool TITCALL TLSRemoveCallback(char* szFileName);
__declspec(dllexport) bool TITCALL TLSRemoveCallbackW(wchar_t* szFileName);
__declspec(dllexport) bool TITCALL TLSRemoveTable(char* szFileName);
__declspec(dllexport) bool TITCALL TLSRemoveTableW(wchar_t* szFileName);
__declspec(dllexport) bool TITCALL TLSBackupData(char* szFileName);
__declspec(dllexport) bool TITCALL TLSBackupDataW(wchar_t* szFileName);
__declspec(dllexport) bool TITCALL TLSRestoreData();
__declspec(dllexport) bool TITCALL TLSBuildNewTable(ULONG_PTR FileMapVA, ULONG_PTR StorePlace, ULONG_PTR StorePlaceRVA, LPVOID ArrayOfCallBacks, DWORD NumberOfCallBacks);
__declspec(dllexport) bool TITCALL TLSBuildNewTableEx(char* szFileName, char* szSectionName, LPVOID ArrayOfCallBacks, DWORD NumberOfCallBacks);
__declspec(dllexport) bool TITCALL TLSBuildNewTableExW(wchar_t* szFileName, char* szSectionName, LPVOID ArrayOfCallBacks, DWORD NumberOfCallBacks);
// TitanEngine.TranslateName.functions:
__declspec(dllexport) void* TITCALL TranslateNativeName(char* szNativeName);
__declspec(dllexport) void* TITCALL TranslateNativeNameW(wchar_t* szNativeName);
// TitanEngine.Handler.functions:
__declspec(dllexport) long TITCALL HandlerGetActiveHandleCount(DWORD ProcessId);
__declspec(dllexport) bool TITCALL HandlerIsHandleOpen(DWORD ProcessId, HANDLE hHandle);
__declspec(dllexport) void* TITCALL HandlerGetHandleName(HANDLE hProcess, DWORD ProcessId, HANDLE hHandle, bool TranslateName);
__declspec(dllexport) void* TITCALL HandlerGetHandleNameW(HANDLE hProcess, DWORD ProcessId, HANDLE hHandle, bool TranslateName);
__declspec(dllexport) long TITCALL HandlerEnumerateOpenHandles(DWORD ProcessId, LPVOID HandleBuffer, DWORD MaxHandleCount);
__declspec(dllexport) ULONG_PTR TITCALL HandlerGetHandleDetails(HANDLE hProcess, DWORD ProcessId, HANDLE hHandle, DWORD InformationReturn);
__declspec(dllexport) bool TITCALL HandlerCloseRemoteHandle(HANDLE hProcess, HANDLE hHandle);
__declspec(dllexport) long TITCALL HandlerEnumerateLockHandles(char* szFileOrFolderName, bool NameIsFolder, bool NameIsTranslated, LPVOID HandleDataBuffer, DWORD MaxHandleCount);
__declspec(dllexport) long TITCALL HandlerEnumerateLockHandlesW(wchar_t* szFileOrFolderName, bool NameIsFolder, bool NameIsTranslated, LPVOID HandleDataBuffer, DWORD MaxHandleCount);
__declspec(dllexport) bool TITCALL HandlerCloseAllLockHandles(char* szFileOrFolderName, bool NameIsFolder, bool NameIsTranslated);
__declspec(dllexport) bool TITCALL HandlerCloseAllLockHandlesW(wchar_t* szFileOrFolderName, bool NameIsFolder, bool NameIsTranslated);
__declspec(dllexport) bool TITCALL HandlerIsFileLocked(char* szFileOrFolderName, bool NameIsFolder, bool NameIsTranslated);
__declspec(dllexport) bool TITCALL HandlerIsFileLockedW(wchar_t* szFileOrFolderName, bool NameIsFolder, bool NameIsTranslated);
// TitanEngine.Handler[Mutex].functions:
__declspec(dllexport) long TITCALL HandlerEnumerateOpenMutexes(HANDLE hProcess, DWORD ProcessId, LPVOID HandleBuffer, DWORD MaxHandleCount);
__declspec(dllexport) ULONG_PTR TITCALL HandlerGetOpenMutexHandle(HANDLE hProcess, DWORD ProcessId, char* szMutexString);
__declspec(dllexport) ULONG_PTR TITCALL HandlerGetOpenMutexHandleW(HANDLE hProcess, DWORD ProcessId, wchar_t* szMutexString);
__declspec(dllexport) long TITCALL HandlerGetProcessIdWhichCreatedMutex(char* szMutexString);
__declspec(dllexport) long TITCALL HandlerGetProcessIdWhichCreatedMutexW(wchar_t* szMutexString);
// TitanEngine.Injector.functions:
__declspec(dllexport) bool TITCALL RemoteLoadLibrary(HANDLE hProcess, char* szLibraryFile, bool WaitForThreadExit);
__declspec(dllexport) bool TITCALL RemoteLoadLibraryW(HANDLE hProcess, wchar_t* szLibraryFile, bool WaitForThreadExit);
__declspec(dllexport) bool TITCALL RemoteFreeLibrary(HANDLE hProcess, HMODULE hModule, char* szLibraryFile, bool WaitForThreadExit);
__declspec(dllexport) bool TITCALL RemoteFreeLibraryW(HANDLE hProcess, HMODULE hModule, wchar_t* szLibraryFile, bool WaitForThreadExit);
__declspec(dllexport) bool TITCALL RemoteExitProcess(HANDLE hProcess, DWORD ExitCode);
// TitanEngine.StaticUnpacker.functions:
__declspec(dllexport) bool TITCALL StaticFileLoad(char* szFileName, DWORD DesiredAccess, bool SimulateLoad, LPHANDLE FileHandle, LPDWORD LoadedSize, LPHANDLE FileMap, PULONG_PTR FileMapVA);
__declspec(dllexport) bool TITCALL StaticFileLoadW(wchar_t* szFileName, DWORD DesiredAccess, bool SimulateLoad, LPHANDLE FileHandle, LPDWORD LoadedSize, LPHANDLE FileMap, PULONG_PTR FileMapVA);
__declspec(dllexport) bool TITCALL StaticFileUnload(char* szFileName, bool CommitChanges, HANDLE FileHandle, DWORD LoadedSize, HANDLE FileMap, ULONG_PTR FileMapVA);
__declspec(dllexport) bool TITCALL StaticFileUnloadW(wchar_t* szFileName, bool CommitChanges, HANDLE FileHandle, DWORD LoadedSize, HANDLE FileMap, ULONG_PTR FileMapVA);
__declspec(dllexport) bool TITCALL StaticFileOpen(char* szFileName, DWORD DesiredAccess, LPHANDLE FileHandle, LPDWORD FileSizeLow, LPDWORD FileSizeHigh);
__declspec(dllexport) bool TITCALL StaticFileOpenW(wchar_t* szFileName, DWORD DesiredAccess, LPHANDLE FileHandle, LPDWORD FileSizeLow, LPDWORD FileSizeHigh);
__declspec(dllexport) bool TITCALL StaticFileGetContent(HANDLE FileHandle, DWORD FilePositionLow, LPDWORD FilePositionHigh, void* Buffer, DWORD Size);
__declspec(dllexport) void TITCALL StaticFileClose(HANDLE FileHandle);
__declspec(dllexport) void TITCALL StaticMemoryDecrypt(LPVOID MemoryStart, DWORD MemorySize, DWORD DecryptionType, DWORD DecryptionKeySize, ULONG_PTR DecryptionKey);
__declspec(dllexport) void TITCALL StaticMemoryDecryptEx(LPVOID MemoryStart, DWORD MemorySize, DWORD DecryptionKeySize, void* DecryptionCallBack);
__declspec(dllexport) void TITCALL StaticMemoryDecryptSpecial(LPVOID MemoryStart, DWORD MemorySize, DWORD DecryptionKeySize, DWORD SpecDecryptionType, void* DecryptionCallBack);
__declspec(dllexport) void TITCALL StaticSectionDecrypt(ULONG_PTR FileMapVA, DWORD SectionNumber, bool SimulateLoad, DWORD DecryptionType, DWORD DecryptionKeySize, ULONG_PTR DecryptionKey);
__declspec(dllexport) bool TITCALL StaticMemoryDecompress(void* Source, DWORD SourceSize, void* Destination, DWORD DestinationSize, int Algorithm);
__declspec(dllexport) bool TITCALL StaticRawMemoryCopy(HANDLE hFile, ULONG_PTR FileMapVA, ULONG_PTR VitualAddressToCopy, DWORD Size, bool AddressIsRVA, char* szDumpFileName);
__declspec(dllexport) bool TITCALL StaticRawMemoryCopyW(HANDLE hFile, ULONG_PTR FileMapVA, ULONG_PTR VitualAddressToCopy, DWORD Size, bool AddressIsRVA, wchar_t* szDumpFileName);
__declspec(dllexport) bool TITCALL StaticRawMemoryCopyEx(HANDLE hFile, DWORD RawAddressToCopy, DWORD Size, char* szDumpFileName);
__declspec(dllexport) bool TITCALL StaticRawMemoryCopyExW(HANDLE hFile, DWORD RawAddressToCopy, DWORD Size, wchar_t* szDumpFileName);
__declspec(dllexport) bool TITCALL StaticRawMemoryCopyEx64(HANDLE hFile, DWORD64 RawAddressToCopy, DWORD64 Size, char* szDumpFileName);
__declspec(dllexport) bool TITCALL StaticRawMemoryCopyEx64W(HANDLE hFile, DWORD64 RawAddressToCopy, DWORD64 Size, wchar_t* szDumpFileName);
__declspec(dllexport) bool TITCALL StaticHashMemory(void* MemoryToHash, DWORD SizeOfMemory, void* HashDigest, bool OutputString, int Algorithm);
__declspec(dllexport) bool TITCALL StaticHashFileW(wchar_t* szFileName, char* HashDigest, bool OutputString, int Algorithm);
__declspec(dllexport) bool TITCALL StaticHashFile(char* szFileName, char* HashDigest, bool OutputString, int Algorithm);
// TitanEngine.Engine.functions:
__declspec(dllexport) void TITCALL EngineUnpackerInitialize(char* szFileName, char* szUnpackedFileName, bool DoLogData, bool DoRealignFile, bool DoMoveOverlay, void* EntryCallBack);
__declspec(dllexport) void TITCALL EngineUnpackerInitializeW(wchar_t* szFileName, wchar_t* szUnpackedFileName, bool DoLogData, bool DoRealignFile, bool DoMoveOverlay, void* EntryCallBack);
__declspec(dllexport) bool TITCALL EngineUnpackerSetBreakCondition(void* SearchStart, DWORD SearchSize, void* SearchPattern, DWORD PatternSize, DWORD PatternDelta, ULONG_PTR BreakType, bool SingleBreak, DWORD Parameter1, DWORD Parameter2);
__declspec(dllexport) void TITCALL EngineUnpackerSetEntryPointAddress(ULONG_PTR UnpackedEntryPointAddress);
__declspec(dllexport) void TITCALL EngineUnpackerFinalizeUnpacking();
// TitanEngine.Engine.functions:
__declspec(dllexport) void TITCALL SetEngineVariable(DWORD VariableId, bool VariableSet);
__declspec(dllexport) bool TITCALL EngineCreateMissingDependencies(char* szFileName, char* szOutputFolder, bool LogCreatedFiles);
__declspec(dllexport) bool TITCALL EngineCreateMissingDependenciesW(wchar_t* szFileName, wchar_t* szOutputFolder, bool LogCreatedFiles);
__declspec(dllexport) bool TITCALL EngineFakeMissingDependencies(HANDLE hProcess);
__declspec(dllexport) bool TITCALL EngineDeleteCreatedDependencies();
__declspec(dllexport) bool TITCALL EngineCreateUnpackerWindow(char* WindowUnpackerTitle, char* WindowUnpackerLongTitle, char* WindowUnpackerName, char* WindowUnpackerAuthor, void* StartUnpackingCallBack);
__declspec(dllexport) void TITCALL EngineAddUnpackerWindowLogMessage(char* szLogMessage);
__declspec(dllexport) bool TITCALL EngineCheckStructAlignment(DWORD StructureType, ULONG_PTR StructureSize);
// Global.Engine.Extension.Functions:
__declspec(dllexport) bool TITCALL ExtensionManagerIsPluginLoaded(char* szPluginName);
__declspec(dllexport) bool TITCALL ExtensionManagerIsPluginEnabled(char* szPluginName);
__declspec(dllexport) bool TITCALL ExtensionManagerDisableAllPlugins();
__declspec(dllexport) bool TITCALL ExtensionManagerDisablePlugin(char* szPluginName);
__declspec(dllexport) bool TITCALL ExtensionManagerEnableAllPlugins();
__declspec(dllexport) bool TITCALL ExtensionManagerEnablePlugin(char* szPluginName);
__declspec(dllexport) bool TITCALL ExtensionManagerUnloadAllPlugins();
__declspec(dllexport) bool TITCALL ExtensionManagerUnloadPlugin(char* szPluginName);
__declspec(dllexport) void* TITCALL ExtensionManagerGetPluginInfo(char* szPluginName);

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif /*TITANENGINE*/
