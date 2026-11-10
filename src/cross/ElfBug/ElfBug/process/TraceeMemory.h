#pragma once

#include <sys/types.h>
#include <ElfBug/types/ElfBug.h>

namespace ElfBug
{
    // Returns the bytes read, or -1.
    ssize_t ReadTraceeVm(pid_t pid, ptr address, void* buffer, ptr size);

    // Falls back to /proc/<pid>/mem when process_vm_readv can't read the first page, such as a PROT_NONE one.
    ssize_t ReadTraceeMemory(pid_t pid, ptr address, void* buffer, ptr size);
}
