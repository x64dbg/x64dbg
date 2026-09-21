#pragma once

#include <sys/types.h>
#include <ElfBug/types/ElfBug.h>

namespace ElfBug
{
    Arch DetectArchFromElfPath(const char* path);
    Arch DetectArchFromProcExe(pid_t pid);
}
