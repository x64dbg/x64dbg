// One mapping of each kind the memory map labels, published through the mm_* globals.
#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include "TargetUtil.h"

#ifndef PR_SET_VMA
#define PR_SET_VMA 0x53564d41
#define PR_SET_VMA_ANON_NAME 0
#endif

#define MM_LONG_SECTION(n) \
    __attribute__((section(".mm_section_with_a_long_name_" #n), used)) const char mm_long_##n[16] = #n;

MM_LONG_SECTION(01)
MM_LONG_SECTION(02)
MM_LONG_SECTION(03)
MM_LONG_SECTION(04)
MM_LONG_SECTION(05)
MM_LONG_SECTION(06)
MM_LONG_SECTION(07)
MM_LONG_SECTION(08)

extern "C"
{
    volatile int mm_data = 42;
    unsigned char mm_bss[kMemMapBssSize];
    volatile uintptr_t mm_heap = 0;
    volatile uintptr_t mm_reserved = 0;
    volatile uintptr_t mm_shared = 0;
    volatile uintptr_t mm_named = 0;
    volatile int mm_named_ok = 0;
    volatile uintptr_t mm_file = 0;
    char mm_file_path[PATH_MAX] = {};
    volatile uintptr_t mm_main_stack = 0;
    volatile uintptr_t mm_worker_stack = 0;
    volatile uintptr_t mm_worker_tcb = 0;
    volatile int mm_worker_tid = 0;
    // 1 loads libmm_gap.so (2 done, 3 failed); 4 unloads it (5 done, 6 failed).
    volatile int mm_load_gap = 0;
    volatile uintptr_t mm_self_image = 0;
    volatile uintptr_t mm_self_exec = 0;
    // 1 loads libmm_data.so (2 done, 3 failed).
    volatile int mm_load_data = 0;
    // 1 loads another libmm_data.so into a new namespace with dlmopen (2 done, 3 failed).
    volatile int mm_isolate_data = 0;
    volatile uintptr_t mm_phantom = 0;
    // 1 unmaps a page in the middle of mm_bss (2 done, 3 failed).
    volatile int mm_punch_bss = 0;
    // With MM_EXTEND_BSS=1, anonymous memory is mapped right after the .bss tail at startup (2 done, 3 failed).
    volatile int mm_extend_bss = 0;
    volatile uintptr_t mm_extension = 0;
    // 1 maps kMemMapManyRegions pages at mm_many, each its own region (2 done, 3 failed).
    volatile int mm_split_many = 0;
    volatile uintptr_t mm_many = 0;
    // 1 maps an anonymous page at mm_gap_fill (2 done, 3 failed).
    volatile int mm_fill_gap = 0;
    volatile uintptr_t mm_gap_fill = 0;
    extern char _end[];

    void mm_code()
    {
    }
}

namespace
{
    std::string ownDirectory()
    {
        char path[PATH_MAX] = {};
        const ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
        const std::string exe(path, n > 0 ? static_cast<size_t>(n) : 0);
        return exe.substr(0, exe.find_last_of('/') + 1);
    }

    // Maps this executable whole from offset 0, the way a symbolizer reads an ELF.
    void mapOwnImage()
    {
        const int fd = open("/proc/self/exe", O_RDONLY | O_CLOEXEC);
        if(fd == -1)
            return;
        struct stat info = {};
        if(fstat(fd, &info) == 0 && info.st_size > 0)
        {
            const auto size = static_cast<size_t>(info.st_size);
            void* image = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
            void* exec = mmap(nullptr, size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0);
            if(image != MAP_FAILED)
                mm_self_image = reinterpret_cast<uintptr_t>(image);
            if(exec != MAP_FAILED)
                mm_self_exec = reinterpret_cast<uintptr_t>(exec);
        }
        close(fd);
    }

    // This executable's header page at both ends of anonymous memory: it looks loaded, but no data segment is there.
    void mapPhantomImage()
    {
        const int fd = open("/proc/self/exe", O_RDONLY | O_CLOEXEC);
        if(fd == -1)
            return;
        void* span = mmap(nullptr, kMemMapPhantomSpan + 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if(span != MAP_FAILED)
        {
            auto* base = static_cast<char*>(span);
            if(mmap(base, 4096, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0) != MAP_FAILED &&
                    mmap(base + kMemMapPhantomSpan, 4096, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0) != MAP_FAILED)
                mm_phantom = reinterpret_cast<uintptr_t>(base);
        }
        close(fd);
    }

    void* worker(void*)
    {
        volatile int probe = 0;
        mm_worker_stack = reinterpret_cast<uintptr_t>(&probe);
        mm_worker_tcb = reinterpret_cast<uintptr_t>(pthread_self());
        mm_worker_tid = static_cast<int>(syscall(SYS_gettid));
        for(;;)
            nap(1000000);
    }

    // 256 bytes ending in a two-byte character, so a 255-byte cut would split it.
    void mapLongPathFile()
    {
        char dir[] = "/tmp/mm_XXXXXX";
        if(!mkdtemp(dir))
            return;
        const std::string path = std::string(dir) + "/" + std::string(239, 'p') + "\xc3\xa9";
        const int fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
        if(fd != -1)
        {
            if(ftruncate(fd, 4096) == 0)
            {
                void* mapping = mmap(nullptr, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
                if(mapping != MAP_FAILED)
                {
                    mm_file = reinterpret_cast<uintptr_t>(mapping);
                    std::memcpy(mm_file_path, path.c_str(), path.size() + 1);
                }
            }
            close(fd);
            unlink(path.c_str());
        }
        rmdir(dir);
    }

    // A preinit function runs before any library initializer can malloc, so with ASLR off the heap isn't yet right after the .bss tail.
    void extendBss(int, char**, char** envp)
    {
        for(char** entry = envp; *entry; ++entry)
        {
            if(std::strcmp(*entry, "MM_EXTEND_BSS=1") != 0)
                continue;
            void* wanted = reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(_end) + 0xfff) & ~uintptr_t{0xfff});
            const bool placed = mmap(wanted, 4 * 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) == wanted;
            mm_extension = placed ? reinterpret_cast<uintptr_t>(wanted) : 0;
            mm_extend_bss = placed ? 2 : 3;
        }
    }

    __attribute__((section(".preinit_array"), used)) void (*const kPreinit)(int, char**, char**) = extendBss;
}

int main()
{
    volatile int probe = 0;
    mm_main_stack = reinterpret_cast<uintptr_t>(&probe);
    mm_heap = reinterpret_cast<uintptr_t>(malloc(64));
    mm_reserved = reinterpret_cast<uintptr_t>(mmap(nullptr, 4 * 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    mm_shared = reinterpret_cast<uintptr_t>(mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    void* named = mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    mm_named = reinterpret_cast<uintptr_t>(named);
    mm_named_ok = prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, named, 4096, "elfbug") == 0;
    mapLongPathFile();
    mapOwnImage();
    mapPhantomImage();

    pthread_t thread;
    pthread_create(&thread, nullptr, worker, nullptr);

    const std::string dir = ownDirectory();
    const std::string gapLibrary = dir + "libmm_gap.so";
    const std::string dataLibrary = dir + "libmm_data.so";
    void* gap = nullptr;
    for(;;)
    {
        if(mm_load_gap == 1)
        {
            gap = dlopen(gapLibrary.c_str(), RTLD_NOW);
            mm_load_gap = gap ? 2 : 3;
        }
        else if(mm_load_gap == 4)
        {
            mm_load_gap = gap && dlclose(gap) == 0 ? 5 : 6;
            gap = nullptr;
        }
        if(mm_load_data == 1)
            mm_load_data = dlopen(dataLibrary.c_str(), RTLD_NOW) ? 2 : 3;
        if(mm_isolate_data == 1)
            mm_isolate_data = dlmopen(LM_ID_NEWLM, dataLibrary.c_str(), RTLD_NOW) ? 2 : 3;
        if(mm_punch_bss == 1)
        {
            const auto middle = reinterpret_cast<uintptr_t>(mm_bss + sizeof(mm_bss) / 2) & ~uintptr_t{0xfff};
            mm_punch_bss = munmap(reinterpret_cast<void*>(middle), 4096) == 0 ? 2 : 3;
        }
        if(mm_split_many == 1)
        {
            auto* span = static_cast<char*>(mmap(nullptr, kMemMapManyRegions * 4096, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
            bool split = span != MAP_FAILED;
            for(size_t i = 1; split && i < kMemMapManyRegions; i += 2)
                split = mprotect(span + i * 4096, 4096, PROT_READ | PROT_WRITE) == 0;
            mm_many = split ? reinterpret_cast<uintptr_t>(span) : 0;
            mm_split_many = split ? 2 : 3;
        }
        if(mm_fill_gap == 1)
        {
            void* wanted = reinterpret_cast<void*>(mm_gap_fill);
            mm_fill_gap = mmap(wanted, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == wanted ? 2 : 3;
        }
        nap(1000000);
    }
}
