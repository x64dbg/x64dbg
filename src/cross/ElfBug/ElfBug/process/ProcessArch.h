#pragma once

#include <sys/types.h>
#include <ElfBug/types/ElfBug.h>

namespace ElfBug
{
    Arch DetectArchFromElfPath(const char* path);
    Arch DetectArchFromProcExe(pid_t pid);

    struct ImageId
    {
        dev_t dev = 0;
        ino_t ino = 0;

        [[nodiscard]] bool Known() const { return ino != 0; }
        bool operator==(const ImageId & other) const { return dev == other.dev && ino == other.ino; }
        bool operator!=(const ImageId & other) const { return !(*this == other); }
    };

    ImageId ReadImageIdFromProcExe(pid_t pid);
    bool AddressesRandomized(pid_t pid);
}
