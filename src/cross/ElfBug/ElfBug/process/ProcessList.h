#pragma once

#include <sys/types.h>
#include <string>
#include <vector>
#include <ElfBug/types/ElfBug.h>

namespace ElfBug
{
    struct ProcessListEntry
    {
        pid_t pid = 0;
        Arch arch = Arch::Unknown;
        bool traced = false;
        std::string name;
        std::string path;
        std::string commandLine;
    };

    // Excludes kernel threads and other users' processes: their exe link isn't readable.
    std::vector<ProcessListEntry> EnumProcesses();

    // Debugger currently tracing pid, 0 when none or unreadable.
    pid_t TracerPid(pid_t pid);

    // Parent of pid, 0 when unreadable.
    pid_t ParentPid(pid_t pid);

    // Thread group pid belongs to, 0 when unreadable.
    pid_t ThreadGroupId(pid_t pid);

    // Thread ids under /proc/<pid>/task. False means the process is gone.
    bool ReadTaskList(pid_t pid, std::vector<pid_t> & tids);
}
