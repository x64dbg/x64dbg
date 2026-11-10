#pragma once

#include <sys/types.h>
#include <ElfBug/types/ElfBug.h>
#include <ElfBug/types/ImageId.h>

namespace ElfBug
{
    Arch DetectArchFromElfPath(const char* path);
    Arch DetectArchFromProcExe(pid_t pid);

    ImageId ReadImageIdFromProcExe(pid_t pid);
    bool AddressesRandomized(pid_t pid);
}
