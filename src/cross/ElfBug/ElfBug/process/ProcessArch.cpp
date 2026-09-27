#include <ElfBug/process/ProcessArch.h>
#include <ElfBug/process/ProcFs.h>
#include <elf.h>
#include <fcntl.h>
#include <sys/personality.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstddef>
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

        constexpr size_t machineOffset = offsetof(Elf64_Ehdr, e_machine);
        unsigned char header[machineOffset + sizeof(Elf64_Half)] = {};
        ssize_t n;
        do
        {
            n = read(fd, header, sizeof(header));
        }
        while(n == -1 && errno == EINTR);
        close(fd);

        if(n < static_cast<ssize_t>(sizeof(header)) || std::memcmp(header, ELFMAG, SELFMAG) != 0)
            return Arch::Unknown;

        Elf64_Half machine;
        std::memcpy(&machine, header + machineOffset, sizeof(machine));

        switch(machine)
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
        return DetectArchFromElfPath(procfs::Path(pid, "exe").c_str());
    }

    ImageId ReadImageIdFromProcExe(const pid_t pid)
    {
        struct stat info = {};
        if(stat(procfs::Path(pid, "exe").c_str(), &info) == -1)
            return {};
        return {info.st_dev, info.st_ino};
    }

    bool AddressesRandomized(const pid_t pid)
    {
        if(procfs::ParseNumber<int>(procfs::ReadLine("/proc/sys/kernel/randomize_va_space")) == 0)
            return false;

        const unsigned long persona = procfs::ParseNumber<unsigned long>(procfs::ReadLine(procfs::Path(pid, "personality")), 16).value_or(0);
        return (persona & ADDR_NO_RANDOMIZE) == 0;
    }
}
