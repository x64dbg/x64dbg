// Axel '0vercl0k' Souchet - January 22 2022
#pragma once
#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <cstdlib>

#if defined(__i386__) || defined(_M_IX86)
#define ARCH_X86
#elif defined(__amd64__) || defined(_M_X64)
#define ARCH_X64
#elif defined(__arm__) || defined(_M_ARM)
#define ARCH_ARM
#elif defined(__aarch64__) || defined(_M_ARM64)
#define ARCH_AARCH64
#elif defined(__EMSCRIPTEN__)
#else
#error Platform not supported.
#endif

#if defined(_WIN32)

#define WINDOWS
#include <Windows.h>

#if defined(ARCH_X86)
#define WINDOWS_X86
#elif defined(ARCH_X64)
#define WINDOWS_X64
#elif defined(ARCH_ARM)
#define WINDOWS_ARM
#elif defined(ARCH_AARCH64)
#define WINDOWS_AARCH64
#endif // ARCH_XXX

#elif defined(linux) || defined(__linux) || defined(__FreeBSD__) ||            \
defined(__FreeBSD_kernel__) || defined(__MACH__) || defined(__EMSCRIPTEN__)

#define LINUX
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(ARCH_X86)
#define LINUX_X86
#elif defined(ARCH_X64)
#define LINUX_X64
#elif defined(ARCH_ARM)
#define LINUX_ARM
#elif defined(ARCH_AARCH64)
#define LINUX_AARCH64
#endif // ARCH_XXX

#else

#error Platform not supported.

#endif // _WIN32

namespace udmpparser
{
#pragma pack(push)
#pragma pack(1)

    enum class ProcessorArch_t : uint16_t
    {
        X86 = 0,
        ARM = 5,
        IA64 = 6,
        AMD64 = 9,
        Unknown = 0xffff
    };

    constexpr uint32_t kWOW64_SIZE_OF_80387_REGISTERS = 80;

    struct FloatingSaveArea32_t
    {
        uint32_t ControlWord;
        uint32_t StatusWord;
        uint32_t TagWord;
        uint32_t ErrorOffset;
        uint32_t ErrorSelector;
        uint32_t DataOffset;
        uint32_t DataSelector;
        uint8_t RegisterArea[kWOW64_SIZE_OF_80387_REGISTERS];
        uint32_t Cr0NpxState;
    };

    static_assert(sizeof(FloatingSaveArea32_t) == 0x70);

    constexpr uint32_t kWOW64_MAXIMUM_SUPPORTED_EXTENSION = 512;

    struct Context32_t
    {
        uint32_t ContextFlags;
        uint32_t Dr0;
        uint32_t Dr1;
        uint32_t Dr2;
        uint32_t Dr3;
        uint32_t Dr6;
        uint32_t Dr7;
        FloatingSaveArea32_t FloatSave;
        uint32_t SegGs;
        uint32_t SegFs;
        uint32_t SegEs;
        uint32_t SegDs;
        uint32_t Edi;
        uint32_t Esi;
        uint32_t Ebx;
        uint32_t Edx;
        uint32_t Ecx;
        uint32_t Eax;
        uint32_t Ebp;
        uint32_t Eip;
        uint32_t SegCs;
        uint32_t EFlags;
        uint32_t Esp;
        uint32_t SegSs;
        uint8_t ExtendedRegisters[kWOW64_MAXIMUM_SUPPORTED_EXTENSION];
    };

    static_assert(sizeof(Context32_t) == 0x2cc);

    struct uint128_t
    {
        uint64_t Low;
        uint64_t High;
    };

    static_assert(sizeof(uint128_t) == 0x10);

    struct Context64_t
    {
        uint64_t P1Home;
        uint64_t P2Home;
        uint64_t P3Home;
        uint64_t P4Home;
        uint64_t P5Home;
        uint64_t P6Home;
        uint32_t ContextFlags;
        uint32_t MxCsr;
        uint16_t SegCs;
        uint16_t SegDs;
        uint16_t SegEs;
        uint16_t SegFs;
        uint16_t SegGs;
        uint16_t SegSs;
        uint32_t EFlags;
        uint64_t Dr0;
        uint64_t Dr1;
        uint64_t Dr2;
        uint64_t Dr3;
        uint64_t Dr6;
        uint64_t Dr7;
        uint64_t Rax;
        uint64_t Rcx;
        uint64_t Rdx;
        uint64_t Rbx;
        uint64_t Rsp;
        uint64_t Rbp;
        uint64_t Rsi;
        uint64_t Rdi;
        uint64_t R8;
        uint64_t R9;
        uint64_t R10;
        uint64_t R11;
        uint64_t R12;
        uint64_t R13;
        uint64_t R14;
        uint64_t R15;
        uint64_t Rip;
        uint16_t ControlWord;
        uint16_t StatusWord;
        uint8_t TagWord;
        uint8_t Reserved1;
        uint16_t ErrorOpcode;
        uint32_t ErrorOffset;
        uint16_t ErrorSelector;
        uint16_t Reserved2;
        uint32_t DataOffset;
        uint16_t DataSelector;
        uint16_t Reserved3;
        uint32_t MxCsr2;
        uint32_t MxCsr_Mask;
        uint128_t FloatRegisters[8];
        uint128_t Xmm0;
        uint128_t Xmm1;
        uint128_t Xmm2;
        uint128_t Xmm3;
        uint128_t Xmm4;
        uint128_t Xmm5;
        uint128_t Xmm6;
        uint128_t Xmm7;
        uint128_t Xmm8;
        uint128_t Xmm9;
        uint128_t Xmm10;
        uint128_t Xmm11;
        uint128_t Xmm12;
        uint128_t Xmm13;
        uint128_t Xmm14;
        uint128_t Xmm15;
        uint8_t Padding[0x60];
        uint128_t VectorRegister[26];
        uint64_t VectorControl;
        uint64_t DebugControl;
        uint64_t LastBranchToRip;
        uint64_t LastBranchFromRip;
        uint64_t LastExceptionToRip;
        uint64_t LastExceptionFromRip;
    };

    static_assert(offsetof(Context64_t, Xmm0) == 0x1a0);
    static_assert(offsetof(Context64_t, VectorRegister) == 0x300);
    static_assert(sizeof(Context64_t) == 0x4d0);

    namespace dmp
    {

        struct Header_t
        {
            static const uint32_t ExpectedSignature = 0x50'4d'44'4d; // 'PMDM';
            static const uint32_t ValidFlagsMask = 0x00'1f'ff'ff;
            uint32_t Signature;
            uint16_t Version;
            uint16_t ImplementationVersion;
            uint32_t NumberOfStreams;
            uint32_t StreamDirectoryRva;
            uint32_t CheckSum;
            uint32_t Reserved;
            uint32_t TimeDateStamp;
            uint32_t Flags;

            bool LooksGood() const
            {
                if(Signature != ExpectedSignature)
                {
                    printf("The signature (%" PRIx32
                           ") does not match the expected signature.\n",
                           Signature);
                    return false;
                }

                if((Flags & ValidFlagsMask) != Flags)
                {
                    printf("The flags have unknown bits set.\n");
                    return false;
                }

                if(NumberOfStreams == 0)
                {
                    printf("There is no streams.\n");
                    return false;
                }

                return true;
            }
        };

        static_assert(sizeof(Header_t) == 0x20);

        struct LocationDescriptor32_t
        {
            uint32_t DataSize = 0;
            uint32_t Rva = 0;
        };

        static_assert(sizeof(LocationDescriptor32_t) == 0x8);

        struct LocationDescriptor64_t
        {
            uint64_t DataSize = 0;
            uint64_t Rva = 0;
        };

        static_assert(sizeof(LocationDescriptor64_t) == 0x10);

        enum class StreamType_t : uint32_t
        {
            Unused = 0,
            ThreadList = 3,
            ModuleList = 4,
            Exception = 6,
            SystemInfo = 7,
            Memory64List = 9,
            MemoryInfoList = 16,
        };

        struct Directory_t
        {
            StreamType_t StreamType = StreamType_t::Unused;
            LocationDescriptor32_t Location;
        };

        static_assert(sizeof(Directory_t) == 0xC);

        struct Memory64ListStreamHdr_t
        {
            uint64_t NumberOfMemoryRanges = 0;
            uint64_t BaseRva = 0;
        };

        static_assert(sizeof(Memory64ListStreamHdr_t) == 0x10);

        struct MemoryDescriptor64_t
        {
            uint64_t StartOfMemoryRange = 0;
            uint64_t DataSize = 0;
        };

        static_assert(sizeof(MemoryDescriptor64_t) == 0x10);

        struct FixedFileInfo_t
        {
            uint32_t Signature = 0;
            uint32_t StrucVersion = 0;
            uint32_t FileVersionMS = 0;
            uint32_t FileVersionLS = 0;
            uint32_t ProductVersionMS = 0;
            uint32_t ProductVersionLS = 0;
            uint32_t FileFlagsMask = 0;
            uint32_t FileFlags = 0;
            uint32_t FileOS = 0;
            uint32_t FileType = 0;
            uint32_t FileSubtype = 0;
            uint32_t FileDateMS = 0;
            uint32_t FileDateLS = 0;
        };

        static_assert(sizeof(FixedFileInfo_t) == 0x34);

        struct ModuleEntry_t
        {
            uint64_t BaseOfImage = 0;
            uint32_t SizeOfImage = 0;
            uint32_t CheckSum = 0;
            uint32_t TimeDateStamp = 0;
            uint32_t ModuleNameRva = 0;
            FixedFileInfo_t VersionInfo;
            LocationDescriptor32_t CvRecord;
            LocationDescriptor32_t MiscRecord;
            uint64_t Reserved0 = 0;
            uint64_t Reserved1 = 0;
        };

        static_assert(sizeof(ModuleEntry_t) == 0x6c);

        struct MemoryInfoListStream_t
        {
            uint32_t SizeOfHeader = 0;
            uint32_t SizeOfEntry = 0;
            uint64_t NumberOfEntries = 0;
        };

        static_assert(sizeof(MemoryInfoListStream_t) == 0x10);

        struct MemoryInfo_t
        {
            uint64_t BaseAddress = 0;
            uint64_t AllocationBase = 0;
            uint32_t AllocationProtect = 0;
            uint32_t __alignment1 = 0;
            uint64_t RegionSize = 0;
            uint32_t State = 0;
            uint32_t Protect = 0;
            uint32_t Type = 0;
            uint32_t __alignment2 = 0;
        };

        static_assert(sizeof(MemoryInfo_t) == 0x30);

        struct MemoryDescriptor_t
        {
            uint64_t StartOfMemoryRange = 0;
            LocationDescriptor32_t Memory;
        };

        static_assert(sizeof(MemoryDescriptor_t) == 0x10);

        struct ThreadEntry_t
        {
            uint32_t ThreadId = 0;
            uint32_t SuspendCount = 0;
            uint32_t PriorityClass = 0;
            uint32_t Priority = 0;
            uint64_t Teb = 0;
            MemoryDescriptor_t Stack;
            LocationDescriptor32_t ThreadContext;
        };

        static_assert(sizeof(ThreadEntry_t) == 0x30);

        struct SystemInfoStream_t
        {
            ProcessorArch_t ProcessorArchitecture = ProcessorArch_t::Unknown;
            uint16_t ProcessorLevel = 0;
            uint16_t ProcessorRevision = 0;
            uint8_t NumberOfProcessors = 0;
            uint8_t ProductType = 0;
            uint32_t MajorVersion = 0;
            uint32_t MinorVersion = 0;
            uint32_t BuildNumber = 0;
            uint32_t PlatformId = 0;
            uint32_t CSDVersionRva = 0;
            uint16_t SuiteMask = 0;
            uint16_t Reserved2 = 0;
        };

        static_assert(sizeof(SystemInfoStream_t) == 32);

        constexpr uint32_t kEXCEPTION_MAXIMUM_PARAMETERS = 15;

        struct ExceptionRecord_t
        {
            uint32_t ExceptionCode;
            uint32_t ExceptionFlags;
            uint64_t ExceptionRecord;
            uint64_t ExceptionAddress;
            uint32_t NumberParameters;
            uint32_t __unusedAlignment;
            uint64_t ExceptionInformation[kEXCEPTION_MAXIMUM_PARAMETERS];
        };

        static_assert(sizeof(ExceptionRecord_t) == 0x98);

        struct ExceptionStream_t
        {
            uint32_t ThreadId = 0;
            uint32_t __alignment = 0;
            ExceptionRecord_t ExceptionRecord;
            LocationDescriptor32_t ThreadContext;
        };

        static_assert(sizeof(ExceptionStream_t) == 0xa8);

    } // namespace dmp
#pragma pack(pop)

#if defined(WINDOWS)
    class FileMap_t
    {
        //
        // Handle to the input file.
        //

        HANDLE File_ = nullptr;

        //
        // Handle to the file mapping.
        //

        HANDLE FileMap_ = nullptr;

        //
        // Base address of the file view.
        //

        PVOID ViewBase_ = nullptr;

        //
        // The end of the view - points right *after* the last byte.
        //

        PVOID ViewEnd_ = nullptr;

    public:
        ~FileMap_t()
        {
            //
            // Unmap the view of the mapping..
            //

            if(ViewBase_ != nullptr)
            {
                UnmapViewOfFile(ViewBase_);
                ViewBase_ = nullptr;
            }

            //
            // Close the handle to the file mapping..
            //

            if(FileMap_ != nullptr)
            {
                CloseHandle(FileMap_);
                FileMap_ = nullptr;
            }

            //
            // And finally the file itself.
            //

            if(File_ != nullptr)
            {
                CloseHandle(File_);
                File_ = nullptr;
            }
        }

        FileMap_t() = default;
        FileMap_t(const FileMap_t &) = delete;
        FileMap_t & operator=(const FileMap_t &) = delete;

        constexpr uint8_t* ViewBase() const { return (uint8_t*)ViewBase_; }

        bool MapFile(const char* PathFile)
        {
            bool Success = true;
            HANDLE File = nullptr;
            HANDLE FileMap = nullptr;
            PVOID ViewBase = nullptr;
            DWORD High = 0, Low = 0;
            DWORD64 FileSize = 0;

            //
            // Open the dump file in read-only.
            //

            File = CreateFileA(PathFile, GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, 0, nullptr);

            if(File == nullptr)
            {

                //
                // If we fail to open the file, let the user know.
                //

                const DWORD GLE = GetLastError();
                printf("CreateFile failed with GLE=%lu.\n", GLE);

                if(GLE == ERROR_FILE_NOT_FOUND)
                {
                    printf("  The file %s was not found.\n", PathFile);
                }

                Success = false;
                goto clean;
            }

            //
            // Create the ro file mapping.
            //

            FileMap = CreateFileMappingA(File, nullptr, PAGE_READONLY, 0, 0, nullptr);

            if(FileMap == nullptr)
            {

                //
                // If we fail to create a file mapping, let
                // the user know.
                //

                const DWORD GLE = GetLastError();
                printf("CreateFileMapping failed with GLE=%lu.\n", GLE);
                Success = false;
                goto clean;
            }

            //
            // Map a view of the file in memory.
            //

            ViewBase = MapViewOfFile(FileMap, FILE_MAP_READ, 0, 0, 0);

            if(ViewBase == nullptr)
            {

                //
                // If we fail to map the view, let the user know.
                //

                const DWORD GLE = GetLastError();
                printf("MapViewOfFile failed with GLE=%lu.\n", GLE);
                Success = false;
                goto clean;
            }

            Low = GetFileSize(File, &High);
            FileSize = (DWORD64(High) << 32) | DWORD64(Low);
            ViewEnd_ = (uint8_t*)ViewBase + FileSize;

            //
            // Everything went well, so grab a copy of the handles for
            // our class and null-out the temporary variables.
            //

            File_ = File;
            File = nullptr;

            FileMap_ = FileMap;
            FileMap = nullptr;

            ViewBase_ = ViewBase;
            ViewBase = nullptr;

clean:

            //
            // Close the handle to the file mapping.
            //

            if(FileMap != nullptr)
            {
                CloseHandle(FileMap);
                FileMap = nullptr;
            }

            //
            // And finally the file itself.
            //

            if(File != nullptr)
            {
                CloseHandle(File);
                File = nullptr;
            }

            return Success;
        }

        bool InBounds(const void* Ptr, const size_t Size = 1)
        {
            const void* EndPtr = (uint8_t*)Ptr + Size;
            return Ptr >= ViewBase_ && EndPtr <= ViewEnd_;
        }
    };

#elif defined(LINUX)

    class FileMap_t
    {
        void* ViewBase_ = nullptr;
        off_t ViewSize_ = 0;
        int Fd_ = -1;

    public:
        ~FileMap_t()
        {
            if(ViewBase_)
            {
                munmap(ViewBase_, ViewSize_);
                ViewBase_ = nullptr;
                ViewSize_ = 0;
            }

            if(Fd_ != -1)
            {
                close(Fd_);
                Fd_ = -1;
            }
        }

        FileMap_t() = default;
        FileMap_t(const FileMap_t &) = delete;
        FileMap_t & operator=(const FileMap_t &) = delete;

        constexpr uint8_t* ViewBase() const { return (uint8_t*)ViewBase_; }

        bool MapFile(const char* PathFile)
        {
            Fd_ = open(PathFile, O_RDONLY);
            if(Fd_ < 0)
            {
                perror("Could not open dump file");
                return false;
            }

            struct stat Stat;
            if(fstat(Fd_, &Stat) < 0)
            {
                perror("Could not stat dump file");
                return false;
            }

            ViewSize_ = Stat.st_size;
            ViewBase_ = mmap(nullptr, ViewSize_, PROT_READ, MAP_SHARED, Fd_, 0);
            if(ViewBase_ == MAP_FAILED)
            {
                perror("Could not mmap");
                return false;
            }

            return true;
        }

        bool InBounds(const void* Ptr, const size_t Size = 1)
        {
            const void* ViewEnd = (uint8_t*)ViewBase_ + ViewSize_;
            const void* PtrEnd = (uint8_t*)Ptr + Size;
            return Ptr >= ViewBase_ && PtrEnd <= ViewEnd;
        }
    };

#endif

    enum class Arch_t { X86, X64 };

    struct MemBlock_t
    {
        uint64_t BaseAddress = 0;
        uint64_t AllocationBase = 0;
        uint32_t AllocationProtect = 0;
        uint64_t RegionSize = 0;
        uint32_t State = 0;
        uint32_t Protect = 0;
        uint32_t Type = 0;
        const uint8_t* Data = nullptr;
        uint64_t DataSize = 0;

        MemBlock_t(const dmp::MemoryInfo_t & Info_)
            : BaseAddress(Info_.BaseAddress), AllocationBase(Info_.AllocationBase),
              AllocationProtect(Info_.AllocationProtect),
              RegionSize(Info_.RegionSize), State(Info_.State),
              Protect(Info_.Protect), Type(Info_.Type) {};
    };

    struct Module_t
    {
        uint64_t BaseOfImage = 0;
        uint32_t SizeOfImage = 0;
        uint32_t CheckSum = 0;
        uint32_t TimeDateStamp = 0;
        std::string ModuleName;
        dmp::FixedFileInfo_t VersionInfo;
        const void* CvRecord = nullptr;
        uint32_t CvRecordSize = 0;
        const void* MiscRecord = nullptr;
        uint32_t MiscRecordSize = 0;

        Module_t(const dmp::ModuleEntry_t & M, const std::string & Name,
                 const void* CvRecord_, const void* MiscRecord_)
            : BaseOfImage(M.BaseOfImage), SizeOfImage(M.SizeOfImage),
              CheckSum(M.CheckSum), TimeDateStamp(M.TimeDateStamp), ModuleName(Name),
              VersionInfo(M.VersionInfo), CvRecord(CvRecord_),
              CvRecordSize(M.CvRecord.DataSize), MiscRecord(MiscRecord_),
              MiscRecordSize(M.MiscRecord.DataSize) {}
    };

    class UnknownContext_t {};

    struct Thread_t
    {
        uint32_t ThreadId = 0;
        uint32_t SuspendCount = 0;
        uint32_t PriorityClass = 0;
        uint32_t Priority = 0;
        uint64_t Teb = 0;
        std::variant<UnknownContext_t, Context32_t, Context64_t> Context;
        Thread_t(const dmp::ThreadEntry_t & T, const void* Context_,
                 const std::optional<ProcessorArch_t> & Arch_)
            : ThreadId(T.ThreadId), SuspendCount(T.SuspendCount),
              PriorityClass(T.PriorityClass), Priority(T.Priority), Teb(T.Teb)
        {
            if(!Arch_)
            {
                return;
            }

            if(*Arch_ == ProcessorArch_t::X86)
            {
                Context = *(Context32_t*)Context_;
            }
            else if(*Arch_ == ProcessorArch_t::AMD64)
            {
                Context = *(Context64_t*)Context_;
            }
        }
    };

    class MemoryView_t
    {
        const void* Begin_ = nullptr;
        const void* End_ = nullptr;

    public:
        MemoryView_t() = default;
        MemoryView_t(const void* Begin, const void* End)
            : Begin_(Begin), End_(End) { }

        constexpr uint8_t* ViewBase() const { return (uint8_t*)Begin_; }

        bool InBounds(const void* Ptr, const size_t Size = 1)
        {
            const void* PtrEnd = (uint8_t*)Ptr + Size;
            return Ptr >= Begin_ && PtrEnd <= End_;
        }
    };

    class UserDumpParser
    {
    private:
        //
        // The mapped file.
        //

        MemoryView_t FileMap_;

        //
        // The memory map; base address -> mem.
        //

        std::map<uint64_t, MemBlock_t> Mem_;

        //
        // The architecture of the dumped process.
        //

        std::optional<ProcessorArch_t> Arch_;

        //
        // The list of loaded modules; base address -> module.
        //

        std::map<uint64_t, Module_t> Modules_;

        //
        // The thread id of the foreground thread.
        //

        std::optional<uint32_t> ForegroundThreadId_;

        //
        // The list of threads; thread id -> thread.
        //

        std::unordered_map<uint32_t, Thread_t> Threads_;

    public:
        //
        // Parse the file.
        //

        bool Parse(const void* Begin, const void* End)
        {

            //
            // Map a view of the file.
            //
            FileMap_ = MemoryView_t(Begin, End);

            //
            // Verify that the mapped file is big enough to pull the headers.
            //

            const auto Hdr = (dmp::Header_t*)FileMap_.ViewBase();
            if(!FileMap_.InBounds(Hdr, sizeof(*Hdr)))
            {
                printf("The header are not in bounds.\n");
                return false;
            }

            //
            // Verify that the header looks sane.
            //

            if(!Hdr->LooksGood())
            {
                printf("The header looks wrong.\n");
                return false;
            }

            //
            // Get a pointer to the base of the stream directory.
            //

            const auto StreamDirectory =
                (dmp::Directory_t*)(FileMap_.ViewBase() + Hdr->StreamDirectoryRva);

            //
            // Verify that it is in bounds.
            //

            const auto StreamDirectorySize =
                Hdr->NumberOfStreams * sizeof(*StreamDirectory);
            if(!FileMap_.InBounds(StreamDirectory, StreamDirectorySize))
            {
                printf("The stream directories are out of the bounds of the file.\n");
                return false;
            }

            //
            // Iterate through the directories.
            //

            std::unordered_map<dmp::StreamType_t, dmp::Directory_t*> Directories;
            for(uint32_t StreamIdx = 0; StreamIdx < Hdr->NumberOfStreams;
                    StreamIdx++)
            {
                const auto CurrentStreamDirectory = &StreamDirectory[StreamIdx];
                const auto Rva = CurrentStreamDirectory->Location.Rva;
                const auto DataSize = CurrentStreamDirectory->Location.DataSize;

                //
                // Verify that the stream content is in bounds.
                //

                const auto StreamStart = FileMap_.ViewBase() + Rva;
                if(!FileMap_.InBounds(StreamStart, DataSize))
                {
                    printf("The stream number %" PRIu32 " is out-of-bounds\n", StreamIdx);
                    return false;
                }

                //
                // Skip unused streams because there are several of them.
                //

                if(CurrentStreamDirectory->StreamType == dmp::StreamType_t::Unused)
                {
                    continue;
                }

                //
                // Keep track of the stream. If we've already seen a stream, fail as I
                // don't think it's expected.
                //

                const auto &[_, Inserted] = Directories.try_emplace(
                                                CurrentStreamDirectory->StreamType, CurrentStreamDirectory);

                if(!Inserted)
                {
                    printf("There are more than one stream of type %" PRIu32 "\n",
                           uint32_t(CurrentStreamDirectory->StreamType));
                    return false;
                }
            }

            //
            // We parse stream in a specific order; it's not strictly necessary but it
            // makes some code a bit simpler to write.
            //

            const dmp::StreamType_t Order[] =
            {
                dmp::StreamType_t::SystemInfo,     dmp::StreamType_t::Exception,
                dmp::StreamType_t::MemoryInfoList, dmp::StreamType_t::Memory64List,
                dmp::StreamType_t::ThreadList,     dmp::StreamType_t::ModuleList
            };

            for(const auto & Type : Order)
            {

                //
                // Check if we have encountered this stream. If not, let's go to the next.
                //

                const auto & Directory = Directories.find(Type);
                if(Directory == Directories.end())
                {
                    continue;
                }

                //
                // Parse the stream. If we don't have a result, it's unexpected.
                //

                const auto & Result = ParseStream(Directory->second);
                if(!Result.has_value())
                {
                    printf("Seems like there is a missing case for %" PRIu32
                           " in ParseStream?\n",
                           uint32_t(Type));
                    return false;
                }

                //
                // If the parsing failed, it is unexpected as well.
                //

                if(!*Result)
                {
                    printf("Failed to parse stream %" PRIu32 ".\n", uint32_t(Type));
                    return false;
                }
            }

            //
            // We are done with this guy.
            //

            Directories.clear();

            //
            // If there's no information regarding the foreground thread, then we're
            // done.
            //

            if(!ForegroundThreadId_)
            {
                return true;
            }

            //
            // If we have a foreground thread id, let's make sure it exists in the list
            // of threads. If it doesn't let's fail.
            //

            const bool ForegroundThreadExists =
                Threads_.find(*ForegroundThreadId_) != Threads_.end();
            if(!ForegroundThreadExists)
            {
                printf("The Exception stream referenced a thread id that does not "
                       "exist in the thread list.\n");
                return false;
            }

            return true;
        }

        const std::map<uint64_t, MemBlock_t> GetMem() const { return Mem_; }

        const MemBlock_t* GetMemBlock(const void* Address) const
        {
            return GetMemBlock(uint64_t(Address));
        }

        const MemBlock_t* GetMemBlock(const uint64_t Address) const
        {
            const auto & Res =
                std::find_if(Mem_.begin(), Mem_.end(), [&](const auto & It)
            {
                return Address >= It.first &&
                       Address < (It.first + It.second.RegionSize);
            });

            if(Res != Mem_.end())
            {
                return &Res->second;
            }

            return nullptr;
        }

        const Module_t* GetModule(const void* Address) const
        {
            return GetModule(uint64_t(Address));
        }

        const Module_t* GetModule(const uint64_t Address) const
        {

            //
            // Look for a module that includes this address.
            //

            const auto & Res =
                std::find_if(Modules_.begin(), Modules_.end(), [&](const auto & It)
            {
                return Address >= It.first &&
                       Address < (It.first + It.second.SizeOfImage);
            });

            //
            // If we have a match, return it!
            //

            if(Res != Modules_.end())
            {
                return &Res->second;
            }

            return nullptr;
        }

        const std::map<uint64_t, Module_t> & GetModules() const { return Modules_; }

        const std::unordered_map<uint32_t, Thread_t> & GetThreads() const
        {
            return Threads_;
        }

        std::optional<uint32_t> GetForegroundThreadId() const
        {
            return ForegroundThreadId_;
        }

    private:
        std::optional<bool> ParseStream(const dmp::Directory_t* StreamDirectory)
        {

            //
            // Parse a stream if we know how to.
            //

            switch(StreamDirectory->StreamType)
            {
            case dmp::StreamType_t::Unused:
            {
                return true;
            }

            case dmp::StreamType_t::SystemInfo:
            {
                return ParseSystemInfoStream(StreamDirectory);
            }

            case dmp::StreamType_t::MemoryInfoList:
            {
                return ParseMemoryInfoListStream(StreamDirectory);
            }

            case dmp::StreamType_t::Memory64List:
            {
                return ParseMemory64ListStream(StreamDirectory);
            }

            case dmp::StreamType_t::ModuleList:
            {
                return ParseModuleListStream(StreamDirectory);
            }

            case dmp::StreamType_t::ThreadList:
            {
                return ParseThreadListStream(StreamDirectory);
            }

            case dmp::StreamType_t::Exception:
            {
                return ParseExceptionStream(StreamDirectory);
            }
            }

            return std::nullopt;
        }

        bool ParseExceptionStream(const dmp::Directory_t* StreamDirectory)
        {

            //
            // Verify that the stream is big enough.
            //

            const auto Exception =
                (dmp::ExceptionStream_t*)(FileMap_.ViewBase() +
                                          StreamDirectory->Location.Rva);

            if(StreamDirectory->Location.DataSize < sizeof(*Exception))
            {
                printf("The size of the Exception stream is not right.\n");
                return false;
            }

            //
            // We only care about the foreground thread id in this stream for now.
            //

            ForegroundThreadId_ = Exception->ThreadId;
            return true;
        }

        bool ParseSystemInfoStream(const dmp::Directory_t* StreamDirectory)
        {

            //
            // Verify that the stream is big enough.
            //

            const auto SystemInfos =
                (dmp::SystemInfoStream_t*)(FileMap_.ViewBase() +
                                           StreamDirectory->Location.Rva);
            if(StreamDirectory->Location.DataSize < sizeof(*SystemInfos))
            {
                printf("The size of the SystemInfo stream is not right.\n");
                return false;
            }

            //
            // We only care about the processor architecture in the SystemInfo stream
            // for now.
            //

            Arch_ = SystemInfos->ProcessorArchitecture;
            return true;
        }

        bool ParseThreadListStream(const dmp::Directory_t* StreamDirectory)
        {

            //
            // Verify that the stream is big enough to grab the number of threads.
            //

            const auto NumberOfThreadsPtr =
                (uint32_t*)(FileMap_.ViewBase() + StreamDirectory->Location.Rva);
            if(StreamDirectory->Location.DataSize < sizeof(*NumberOfThreadsPtr))
            {
                printf("The size of the ThreadList stream is not right.\n");
                return false;
            }

            //
            // Verify that the stream is big enough to fit the array of threads.
            //

            const auto NumberOfThreads = *NumberOfThreadsPtr;
            const auto Threads = (dmp::ThreadEntry_t*)(NumberOfThreadsPtr + 1);
            const auto StreamSize = NumberOfThreads * sizeof(*Threads);
            if(StreamDirectory->Location.DataSize < StreamSize)
            {
                printf("The Thread stream is not right.\n");
                return false;
            }

            //
            // Iterate through the array of threads.
            //

            for(uint32_t ThreadIdx = 0; ThreadIdx < NumberOfThreads; ThreadIdx++)
            {

                //
                // Verify that the thread context is in bounds.
                //

                const auto CurrentThread = &Threads[ThreadIdx];
                const auto ThreadContext =
                    (void*)(FileMap_.ViewBase() + CurrentThread->ThreadContext.Rva);
                const auto ThreadContextDataSize = CurrentThread->ThreadContext.DataSize;
                if(!FileMap_.InBounds(ThreadContext, ThreadContextDataSize))
                {
                    printf("The thread context number %" PRIu32 " is out of bounds.\n",
                           ThreadIdx);
                    return false;
                }

                //
                // Add the thread.
                //

                Threads_.try_emplace(CurrentThread->ThreadId, *CurrentThread,
                                     ThreadContext, Arch_);
            }

            return true;
        }

        bool ParseMemoryInfoListStream(const dmp::Directory_t* StreamDirectory)
        {

            //
            // Verify that the stream is big enough.
            //

            const auto MemoryInfoList =
                (dmp::MemoryInfoListStream_t*)(FileMap_.ViewBase() +
                                               StreamDirectory->Location.Rva);
            if(StreamDirectory->Location.DataSize < sizeof(*MemoryInfoList))
            {
                printf("The size of the MemoryInfoList stream is not right.\n");
                return false;
            }

            //
            // Verify that the size of the header is big enough.
            //

            if(MemoryInfoList->SizeOfHeader < sizeof(*MemoryInfoList))
            {
                printf("The size of the MemoryInfoList header is not right.\n");
                return false;
            }

            //
            // Verify that the size of each entry is big enough.
            //

            if(MemoryInfoList->SizeOfEntry < sizeof(dmp::MemoryInfo_t))
            {
                printf("The size of the MemoryInfo entries are not right.\n");
                return false;
            }

            //
            // Skip the header to get a pointer to the array of memory entries.
            //

            const auto MemoryInfo = (dmp::MemoryInfo_t*)((uint8_t*)MemoryInfoList +
                                    MemoryInfoList->SizeOfHeader);

            //
            // Verify that the stream is big enough for the array of memory entries.
            //

            const uint64_t StreamSize =
                MemoryInfoList->SizeOfHeader +
                (MemoryInfoList->SizeOfEntry * MemoryInfoList->NumberOfEntries);
            if(StreamSize != StreamDirectory->Location.DataSize)
            {
                printf("The MemoryInfoList stream size is not right.\n");
                return false;
            }

            //
            // Iterate through the array.
            //

            for(size_t MemoryInfoIdx = 0;
                    MemoryInfoIdx < MemoryInfoList->NumberOfEntries; MemoryInfoIdx++)
            {
                const auto CurrentMemoryInfo =
                    (dmp::MemoryInfo_t*)((uint8_t*)MemoryInfo +
                                         (MemoryInfoList->SizeOfEntry * MemoryInfoIdx));

                //
                // Insert the entry in the map.
                //

                const uint64_t BaseAddress = CurrentMemoryInfo->BaseAddress;
                const auto &[_, Inserted] =
                    Mem_.try_emplace(BaseAddress, *CurrentMemoryInfo);

                if(!Inserted)
                {
                    printf("The region %" PRIx64 " is already in the memory map.\n",
                           BaseAddress);
                    return false;
                }
            }

            return true;
        }

        bool ParseModuleListStream(const dmp::Directory_t* StreamDirectory)
        {

            //
            // Verify that the stream is big enough to read the number of modules.
            //

            const auto NumberOfModulesPtr =
                (uint32_t*)(FileMap_.ViewBase() + StreamDirectory->Location.Rva);
            if(StreamDirectory->Location.DataSize < sizeof(*NumberOfModulesPtr))
            {
                printf("The ModuleList stream is too small.\n");
                return false;
            }

            //
            // Verify that the stream is big enough for the array of modules.
            //

            const auto NumberOfModules = *NumberOfModulesPtr;
            const auto Module = (dmp::ModuleEntry_t*)(NumberOfModulesPtr + 1);
            const size_t StreamSize =
                sizeof(*NumberOfModulesPtr) + (NumberOfModules * sizeof(*Module));
            if(StreamSize != StreamDirectory->Location.DataSize)
            {
                printf("The ModuleList stream size is not right.\n");
                return false;
            }

            //
            // Iterate through the array.
            //

            for(uint32_t ModuleIdx = 0; ModuleIdx < NumberOfModules; ModuleIdx++)
            {
                const auto CurrentModule = &Module[ModuleIdx];
                const auto NameLengthPtr =
                    (uint32_t*)(FileMap_.ViewBase() + CurrentModule->ModuleNameRva);
                const size_t NameRecordLength = sizeof(*NameLengthPtr) + *NameLengthPtr;

                //
                // Verify that the record is in bounds.
                //

                if(!FileMap_.InBounds(NameLengthPtr, NameRecordLength))
                {
                    printf("The module name record is not in bounds.\n");
                    return false;
                }

                //
                // Verify that the length is well-formed.
                //

                const uint32_t NameLength = *NameLengthPtr;
                const bool WellFormed = (NameLength % 2) == 0;
                if(!WellFormed)
                {
                    printf("The MINIDUMP_STRING for the module index %" PRIu32
                           " is not well formed.\n",
                           ModuleIdx);
                    return false;
                }

                //
                // Read the module name.
                //

                const auto ModuleNamePtr = (char*)(NameLengthPtr + 1);
                const uint32_t StringLen = NameLength / 2;
                std::string ModuleName(StringLen, 0);
                for(size_t CharIdx = 0; CharIdx < NameLength; CharIdx += 2)
                {
                    if(!isprint(ModuleNamePtr[CharIdx]))
                    {
                        printf("The MINIDUMP_STRING for the module index %" PRIu32
                               " has a non printable ascii character.\n",
                               ModuleIdx);
                        return false;
                    }

                    ModuleName[CharIdx / 2] = ModuleNamePtr[CharIdx];
                }

                //
                // Verify that the Cv record is in bounds.
                //

                const auto CvRecord = FileMap_.ViewBase() + CurrentModule->CvRecord.Rva;
                const auto CvRecordSize = CurrentModule->CvRecord.DataSize;
                if(!FileMap_.InBounds(CvRecord, CvRecordSize))
                {
                    printf("The Cv record of the module index %" PRIu32
                           " is not in bounds.\n",
                           ModuleIdx);
                    return false;
                }

                //
                // Verify that the Misc record is in bounds.
                //

                const auto MiscRecord =
                    FileMap_.ViewBase() + CurrentModule->MiscRecord.Rva;
                const auto MiscRecordSize = CurrentModule->MiscRecord.DataSize;
                if(!FileMap_.InBounds(MiscRecord, MiscRecordSize))
                {
                    printf("The Misc record of the module index %" PRIu32
                           " is not in bounds.\n",
                           ModuleIdx);
                    return false;
                }

                //
                // Add the module.
                //

                Modules_.try_emplace(CurrentModule->BaseOfImage, *CurrentModule,
                                     ModuleName, (void*)CvRecord, (void*)MiscRecord);
            }

            return true;
        }

        bool ParseMemory64ListStream(const dmp::Directory_t* StreamDirectory)
        {

            //
            // Verify that the Memory64List stream is big enough.
            //

            const auto Memory64List =
                (dmp::Memory64ListStreamHdr_t*)(FileMap_.ViewBase() +
                                                StreamDirectory->Location.Rva);
            if(StreamDirectory->Location.DataSize < sizeof(*Memory64List))
            {
                printf("The Memory64List stream is too small.\n");
                return false;
            }

            //
            // Verify that the Memory64List stream is big enough for the array.
            //

            const auto Descriptor = (dmp::MemoryDescriptor64_t*)(Memory64List + 1);
            const auto NumberOfMemoryRanges = Memory64List->NumberOfMemoryRanges;
            const uint64_t StreamSize =
                sizeof(*Memory64List) + (NumberOfMemoryRanges * sizeof(*Descriptor));
            if(StreamDirectory->Location.DataSize < StreamSize)
            {
                printf("The Memory64List stream is not right.\n");
                return false;
            }

            //
            // Get a pointer to the base RVA where all the pages' content are stored
            // at.
            //

            auto CurrentData = FileMap_.ViewBase() + Memory64List->BaseRva;

            //
            // Iterate through the array.
            //

            for(uint32_t RangeIdx = 0; RangeIdx < NumberOfMemoryRanges; RangeIdx++)
            {

                //
                // Verify that the range's content is in bounds.
                //

                const auto CurrentDescriptor = &Descriptor[RangeIdx];
                const auto StartOfMemoryRange = CurrentDescriptor->StartOfMemoryRange;
                const size_t DataSize = size_t(CurrentDescriptor->DataSize);
                if(!FileMap_.InBounds(CurrentData, DataSize))
                {
                    printf("The Memory64List memory content number %" PRIu32
                           " has its data out-of-bounds.\n",
                           RangeIdx);
                    return false;
                }

                //
                // If we don't find an existing entry, there is something funky going on.
                //

                const auto & It = Mem_.find(StartOfMemoryRange);
                if(It == Mem_.end())
                {
                    printf("The memory region starting at %" PRIx64
                           " does not exist in the map.\n",
                           StartOfMemoryRange);
                    return false;
                }

                //
                // Update the entry.
                //

                It->second.Data = CurrentData;
                It->second.DataSize = DataSize;
                CurrentData += DataSize;
            }

            return true;
        }
    };
} // namespace udmpparser
