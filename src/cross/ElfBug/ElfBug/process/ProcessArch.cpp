#include <ElfBug/process/ProcessArch.h>
#include <elf.h>
#include <fcntl.h>
#include <sys/personality.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace ElfBug
{
    Arch DetectArchFromElfPath(const char* path)
    {
        if(!path)
            return Arch::Unknown;

        const int fd = open(path, O_RDONLY | O_CLOEXEC);
        if(fd == -1)
            return Arch::Unknown;

        unsigned char hdr[20] = {};
        ssize_t n;
        do
        {
            n = read(fd, hdr, sizeof(hdr));
        }
        while(n == -1 && errno == EINTR);
        close(fd);

        if(n < static_cast<ssize_t>(sizeof(hdr)))
            return Arch::Unknown;

        if(hdr[EI_MAG0] != ELFMAG0 ||
                hdr[EI_MAG1] != ELFMAG1 ||
                hdr[EI_MAG2] != ELFMAG2 ||
                hdr[EI_MAG3] != ELFMAG3)
            return Arch::Unknown;

        uint16_t e_machine;
        std::memcpy(&e_machine, hdr + 18, sizeof(e_machine));

        switch(e_machine)
        {
        case EM_X86_64:
            return Arch::X86_64;
        case EM_386:
            return Arch::I386;
        default:
            return Arch::Unknown;
        }
    }

    Arch DetectArchFromProcExe(const pid_t pid)
    {
        char path[64];
        std::snprintf(path, sizeof(path), "/proc/%d/exe", pid);
        return DetectArchFromElfPath(path);
    }

    ImageId ReadImageIdFromProcExe(const pid_t pid)
    {
        char path[64];
        std::snprintf(path, sizeof(path), "/proc/%d/exe", pid);

        struct stat info = {};
        if(stat(path, &info) == -1)
            return {};
        return {info.st_dev, info.st_ino};
    }

    bool AddressesRandomized(const pid_t pid)
    {
        char path[64];
        std::snprintf(path, sizeof(path), "/proc/%d/personality", pid);
        if(FILE* global = std::fopen("/proc/sys/kernel/randomize_va_space", "re"))
        {
            int level = -1;
            const bool read = std::fscanf(global, "%d", &level) == 1;
            std::fclose(global);
            if(read && level == 0)
                return false;
        }

        FILE* file = std::fopen(path, "re");
        if(!file)
            return true;
        unsigned long persona = 0;
        const bool read = std::fscanf(file, "%lx", &persona) == 1;
        std::fclose(file);
        return !read || (persona & ADDR_NO_RANDOMIZE) == 0;
    }
}
