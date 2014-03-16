#include <windows.h>

//Thanks to: https://github.com/zer0fl4g/Nanomite

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;

typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;

typedef struct _PEB
{
    BYTE InheritedAddressSpace;
    BYTE ReadImageFileExecOptions;
    BYTE BeingDebugged;
    BYTE SpareBool;
    DWORD Mutant;
    DWORD ImageBaseAddress;
    DWORD LoaderData;
    DWORD ProcessParameters;
    DWORD SubSystemData;
    DWORD ProcessHeap;
    DWORD FastPebLock;
    DWORD FastPebLockRoutine;
    DWORD FastPebUnlockRoutine;
    DWORD EnviromentUpdateCount;
    DWORD KernelCallbackTable;
    DWORD UserSharedInfoPtr;
    DWORD ThunksOrOptions;
    DWORD FreeList;
    DWORD TlsExpansionCounter;
    DWORD TlsBitmap;
    DWORD TlsBitmapBits[2];
    DWORD ReadOnlySharedMemoryBase;
    DWORD ReadOnlySharedMemoryHeap;
    DWORD ReadOnlyStaticServerData;
    DWORD AnsiCodePageData;
    DWORD OemCodePageData;
    DWORD UnicodeCaseTableData;
    DWORD NumberOfProcessors;
    DWORD NtGlobalFlag;
    DWORD Reserved;
    LARGE_INTEGER CriticalSectionTimeout;
    DWORD HeapSegmentReserve;
    DWORD HeapSegmentCommit;
    DWORD HeapDeCommitTotalFreeThreshold;
    DWORD HeapDeCommitFreeBlockThreshold;
    DWORD NumberOfHeaps;
    DWORD MaximumNumberOfHeaps;
    DWORD ProcessHeaps;
    DWORD GdiSharedHandleTable;
    DWORD ProcessStarterHelper;
    DWORD GdiDCAttributeList;
    DWORD LoaderLock;
    DWORD OSMajorVersion;
    DWORD OSMinorVersion;
    WORD OSBuildNumber;
    WORD OSCSDVersion;
    DWORD OSPlatformId;
    DWORD ImageSubsystem;
    DWORD ImageSubsystemMajorVersion;
    DWORD ImageSubsystemMinorVersion;
    DWORD ImageProcessAffinityMask;
    DWORD GdiHandleBuffer[34];
    DWORD PostProcessInitRoutine;
    DWORD TlsExpansionBitmap;
    DWORD TlsExpansionBitmapBits[32];
    DWORD SessionId;
    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    DWORD pShimData;
    DWORD AppCompatInfo;
    UNICODE_STRING CSDVersion;
    DWORD ActivationContextData;
    DWORD ProcessAssemblyStorageMap;
    DWORD SystemDefaultActivationContextData;
    DWORD SystemAssemblyStorageMap;
    DWORD MinimumStackCommit;
    DWORD FlsCallback;
    DWORD FlsListHead_Flink;
    DWORD FlsListHead_Blink;
    DWORD FlsBitmap;
    DWORD FlsBitmapBits[4];
    DWORD FlsHighIndex;
} PEB, *PPEB;

typedef struct _TEB
{
    NT_TIB                  Tib;
    PVOID                   EnvironmentPointer;
    CLIENT_ID               Cid;
    PVOID                   ActiveRpcInfo;
    PVOID                   ThreadLocalStoragePointer;
    PPEB                    Peb;
    ULONG                   LastErrorValue;
    ULONG                   CountOfOwnedCriticalSections;
    PVOID                   CsrClientThread;
    PVOID                   Win32ThreadInfo;
    ULONG                   Win32ClientInfo[0x1F];
    PVOID                   WOW32Reserved;
    ULONG                   CurrentLocale;
    ULONG                   FpSoftwareStatusRegister;
    PVOID                   SystemReserved1[0x36];
    PVOID                   Spare1;
    ULONG                   ExceptionCode;
    ULONG                   SpareBytes1[0x28];
    PVOID                   SystemReserved2[0xA];
    ULONG                   GdiRgn;
    ULONG                   GdiPen;
    ULONG                   GdiBrush;
    CLIENT_ID               RealClientId;
    PVOID                   GdiCachedProcessHandle;
    ULONG                   GdiClientPID;
    ULONG                   GdiClientTID;
    PVOID                   GdiThreadLocaleInfo;
    PVOID                   UserReserved[5];
    PVOID                   GlDispatchTable[0x118];
    ULONG                   GlReserved1[0x1A];
    PVOID                   GlReserved2;
    PVOID                   GlSectionInfo;
    PVOID                   GlSection;
    PVOID                   GlTable;
    PVOID                   GlCurrentRC;
    PVOID                   GlContext;
    NTSTATUS                LastStatusValue;
    UNICODE_STRING          StaticUnicodeString;
    WCHAR                   StaticUnicodeBuffer[0x105];
    PVOID                   DeallocationStack;
    PVOID                   TlsSlots[0x40];
    LIST_ENTRY              TlsLinks;
    PVOID                   Vdm;
    PVOID                   ReservedForNtRpc;
    PVOID                   DbgSsReserved[0x2];
    ULONG                   HardErrorDisabled;
    PVOID                   Instrumentation[0x10];
    PVOID                   WinSockData;
    ULONG                   GdiBatchCount;
    ULONG                   Spare2;
    ULONG                   Spare3;
    ULONG                   Spare4;
    PVOID                   ReservedForOle;
    ULONG                   WaitingOnLoaderLock;
    PVOID                   StackCommit;
    PVOID                   StackCommitMax;
    PVOID                   StackReserved;
} TEB, *PTEB;
